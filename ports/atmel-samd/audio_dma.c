/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "audio_dma.h"
#include "samd/clocks.h"
#include "samd/events.h"
#include "samd/dma.h"

#include "shared-bindings/audiocore/RawSample.h"
#include "shared-bindings/audiocore/WaveFile.h"

#include "py/mpstate.h"
#include "py/runtime.h"

#if CIRCUITPY_AUDIOIO || CIRCUITPY_AUDIOBUSIO

#define SECOND_SIBLING (uint8_t)-1
#define NO_SIBLING     (uint8_t)-2

static audio_dma_t* audio_dma_state[AUDIO_DMA_CHANNEL_COUNT];

// This cannot be in audio_dma_state because it's volatile.
static volatile bool audio_dma_pending[AUDIO_DMA_CHANNEL_COUNT];

static bool audio_dma_allocated[AUDIO_DMA_CHANNEL_COUNT];

uint8_t audio_dma_allocate_channel(void) {
    uint8_t channel;
    for (channel = 0; channel < AUDIO_DMA_CHANNEL_COUNT; channel++) {
        if (!audio_dma_allocated[channel]) {
            audio_dma_allocated[channel] = true;
            return channel;
        }
    }
    return channel; // i.e., return failure
}

void audio_dma_free_channel(uint8_t channel) {
    assert(channel < AUDIO_DMA_CHANNEL_COUNT);
    assert(audio_dma_allocated[channel]);
    audio_dma_disable_channel(channel);
    audio_dma_allocated[channel] = false;
}

void audio_dma_disable_channel(uint8_t channel) {
    if (channel >= AUDIO_DMA_CHANNEL_COUNT)
        return;
    dma_disable_channel(channel);
}

void audio_dma_enable_channel(uint8_t channel) {
    if (channel >= AUDIO_DMA_CHANNEL_COUNT)
        return;
    dma_enable_channel(channel);
}

void audio_dma_convert_signed(audio_dma_t* dma, uint8_t* buffer, uint32_t buffer_length,
                              uint8_t** output_buffer, uint32_t* output_buffer_length,
                              uint8_t* output_spacing) {
    if (dma->first_buffer_free) {
        *output_buffer = dma->first_buffer;
    } else {
        *output_buffer = dma->second_buffer;
    }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-align"
    if (dma->signed_to_unsigned || dma->unsigned_to_signed) {
        *output_buffer_length = buffer_length / dma->spacing;
        *output_spacing = 1;
        uint32_t out_i = 0;
        if (dma->bytes_per_sample == 1) {
            for (uint32_t i = 0; i < buffer_length; i += dma->spacing) {
                if (dma->signed_to_unsigned) {
                    ((uint8_t*) *output_buffer)[out_i] = ((int8_t*) buffer)[i] + 0x80;
                } else {
                    ((int8_t*) *output_buffer)[out_i] = ((uint8_t*) buffer)[i] - 0x80;
                }
                out_i += 1;
            }
        } else if (dma->bytes_per_sample == 2) {
            for (uint32_t i = 0; i < buffer_length / 2; i += dma->spacing) {
                if (dma->signed_to_unsigned) {
                    ((uint16_t*) *output_buffer)[out_i] = ((int16_t*) buffer)[i] + 0x8000;
                } else {
                    ((int16_t*) *output_buffer)[out_i] = ((uint16_t*) buffer)[i] - 0x8000;
                }
                out_i += 1;
            }
        }
    } else {
        *output_buffer = buffer;
        *output_buffer_length = buffer_length;
        *output_spacing = dma->spacing;
    }
    #pragma GCC diagnostic pop
    dma->first_buffer_free = !dma->first_buffer_free;
}

void audio_dma_load_next_block(audio_dma_t* dma) {
    uint8_t* buffer;
    uint32_t buffer_length;
    audioio_get_buffer_result_t get_buffer_result =
        audiosample_get_buffer(dma->sample, false, dma->audio_channel,
                               &buffer, &buffer_length);

    DmacDescriptor* descriptor = dma->second_descriptor;
    if (dma->first_descriptor_free) {
        descriptor = dma_descriptor(dma->dma_channel);
    }
    dma->first_descriptor_free = !dma->first_descriptor_free;

    if (get_buffer_result == GET_BUFFER_ERROR) {
        audio_dma_stop(dma);
#ifdef SAMD51
        if (dma->sibling_channel != NO_SIBLING ) {
            audio_dma_stop(audio_dma_state[dma->sibling_channel]);
        }
#endif
        return;
    }

    uint8_t* output_buffer;
    uint32_t output_buffer_length;
    uint8_t output_spacing;
    audio_dma_convert_signed(dma, buffer, buffer_length, &output_buffer, &output_buffer_length,
        &output_spacing);

    descriptor->BTCNT.reg = output_buffer_length / dma->beat_size / output_spacing;
    descriptor->SRCADDR.reg = ((uint32_t) output_buffer) + output_buffer_length;
    if (get_buffer_result == GET_BUFFER_DONE) {
        if (dma->loop) {
            audiosample_reset_buffer(dma->sample, dma->single_channel, dma->audio_channel);
        } else {
            descriptor->DESCADDR.reg = 0;
        }
    }
    descriptor->BTCTRL.bit.VALID = true;

#ifdef SAMD51
    if (dma->sibling_channel != NO_SIBLING) {
        dma = audio_dma_state[dma->sibling_channel];
        descriptor = dma->second_descriptor;
        if (dma->first_descriptor_free) {
            descriptor = dma_descriptor(dma->dma_channel);
        }
        dma->first_descriptor_free = !dma->first_descriptor_free;

        audio_dma_convert_signed(dma, buffer + dma->beat_size, buffer_length, &output_buffer, &output_buffer_length,
            &output_spacing);

        descriptor->BTCNT.reg = output_buffer_length / dma->beat_size / output_spacing;
        descriptor->SRCADDR.reg = ((uint32_t) output_buffer) + output_buffer_length;
        if (get_buffer_result == GET_BUFFER_DONE) {
            if (!dma->loop) {
                descriptor->DESCADDR.reg = 0;
            }
        }
        descriptor->BTCTRL.bit.VALID = true;
    }
#endif
}

static void setup_audio_descriptor(DmacDescriptor* descriptor, uint8_t beat_size,
                                   uint8_t spacing, uint32_t output_register_address) {
    uint32_t beat_size_reg = DMAC_BTCTRL_BEATSIZE_BYTE;
    if (beat_size == 2) {
        beat_size_reg = DMAC_BTCTRL_BEATSIZE_HWORD;
    } else if (beat_size == 4) {
        beat_size_reg = DMAC_BTCTRL_BEATSIZE_WORD;
    }
    descriptor->BTCTRL.reg = beat_size_reg |
                             DMAC_BTCTRL_SRCINC |
                             DMAC_BTCTRL_EVOSEL_BLOCK |
                             DMAC_BTCTRL_STEPSIZE(spacing - 1) |
                             DMAC_BTCTRL_STEPSEL_SRC;
    descriptor->DSTADDR.reg = output_register_address;
}

// Playback should be shutdown before calling this.
audio_dma_result audio_dma_setup_playback(audio_dma_t* dma,
                              mp_obj_t sample,
                              bool loop,
                              bool single_channel,
                              uint8_t audio_channel,
                              bool output_signed,
                              uint32_t output_register_address,
                              uint8_t dma_trigger_source) {
    uint8_t dma_channel = audio_dma_allocate_channel();
    if (dma_channel >= AUDIO_DMA_CHANNEL_COUNT) {
        return AUDIO_DMA_DMA_BUSY;
    }

    dma->sample = sample;
    dma->loop = loop;
    dma->single_channel = single_channel;
    dma->audio_channel = audio_channel;
    dma->dma_channel = dma_channel;
#ifdef SAMD51
    dma->sibling_channel = NO_SIBLING;
#endif
    dma->signed_to_unsigned = false;
    dma->unsigned_to_signed = false;
    dma->second_descriptor = NULL;
    dma->spacing = 1;
    dma->first_descriptor_free = true;
    audiosample_reset_buffer(sample, single_channel, audio_channel);

    bool single_buffer;
    bool samples_signed;
    uint32_t max_buffer_length;
    audiosample_get_buffer_structure(sample, single_channel, &single_buffer, &samples_signed,
                                     &max_buffer_length, &dma->spacing);
    uint8_t output_spacing = dma->spacing;
    if (output_signed != samples_signed) {
        output_spacing = 1;
        max_buffer_length /= dma->spacing;
        dma->first_buffer = (uint8_t*) m_malloc(max_buffer_length, false);
        if (dma->first_buffer == NULL) {
            return AUDIO_DMA_MEMORY_ERROR;
        }
        dma->first_buffer_free = true;
        if (!single_buffer) {
            dma->second_buffer = (uint8_t*) m_malloc(max_buffer_length, false);
            if (dma->second_buffer == NULL) {
                return AUDIO_DMA_MEMORY_ERROR;
            }
        }
        dma->signed_to_unsigned = !output_signed && samples_signed;
        dma->unsigned_to_signed = output_signed && !samples_signed;
    }

    dma->event_channel = 0xff;
    if (!single_buffer) {
        dma->second_descriptor = (DmacDescriptor*) m_malloc(sizeof(DmacDescriptor), false);
        if (dma->second_descriptor == NULL) {
            return AUDIO_DMA_MEMORY_ERROR;
        }

        // We're likely double buffering so set up the block interrupts.
        turn_on_event_system();
        dma->event_channel = find_sync_event_channel();

        if (dma->event_channel >= EVSYS_SYNCH_NUM) {
            mp_raise_RuntimeError(translate("All sync event channels in use"));
        }
        init_event_channel_interrupt(dma->event_channel, CORE_GCLK, EVSYS_ID_GEN_DMAC_CH_0 + dma_channel);

        // We keep the audio_dma_t for internal use and the sample as a root pointer because it
        // contains the audiodma structure.
        audio_dma_state[dma->dma_channel] = dma;
        MP_STATE_PORT(playing_audio)[dma->dma_channel] = dma->sample;
    }


    if (audiosample_bits_per_sample(sample) == 16) {
        dma->beat_size = 2;
        dma->bytes_per_sample = 2;
    } else {
        dma->beat_size = 1;
        dma->bytes_per_sample = 1;
        if (single_channel) {
            output_register_address += 1;
        }
    }
    // Transfer both channels at once.
    if (!single_channel && audiosample_channel_count(sample) == 2) {
        dma->beat_size *= 2;
    }

    DmacDescriptor* first_descriptor = dma_descriptor(dma_channel);
    setup_audio_descriptor(first_descriptor, dma->beat_size, output_spacing, output_register_address);
    if (single_buffer) {
        first_descriptor->DESCADDR.reg = 0;
        if (dma->loop) {
            first_descriptor->DESCADDR.reg = (uint32_t) first_descriptor;
        }
    } else {
        first_descriptor->DESCADDR.reg = (uint32_t) dma->second_descriptor;
        setup_audio_descriptor(dma->second_descriptor, dma->beat_size, output_spacing, output_register_address);
        dma->second_descriptor->DESCADDR.reg = (uint32_t) first_descriptor;
    }

    return AUDIO_DMA_OK;
}

#ifdef SAMD51
audio_dma_result audio_dma_link_channels(audio_dma_t* first, audio_dma_t* second) {
    first->sibling_channel = second->dma_channel;
    second->sibling_channel = SECOND_SIBLING;
    return AUDIO_DMA_OK;
}
#endif


audio_dma_result audio_dma_preload(audio_dma_t* dma, uint8_t dma_trigger_source) {
    // Load the first two blocks up front.
    uint8_t channel = dma->dma_channel;
    audio_dma_pending[channel] = true;
    audio_dma_load_next_block(dma);
    if (dma->second_descriptor) {
        audio_dma_load_next_block(dma);
    }

    dma_configure(dma->dma_channel, dma_trigger_source, true);
    audio_dma_enable_channel(dma->dma_channel);

#ifdef SAMD51
    if (dma->sibling_channel != NO_SIBLING) {
        dma = audio_dma_state[dma->sibling_channel];
        dma_configure(dma->dma_channel, dma_trigger_source, true);
        audio_dma_enable_channel(dma->dma_channel);
    }
#endif
    audio_dma_pending[channel] = false;

    return AUDIO_DMA_OK;
}


void audio_dma_stop(audio_dma_t* dma) {
    uint8_t channel = dma->dma_channel;
    if (channel < AUDIO_DMA_CHANNEL_COUNT) {
        audio_dma_disable_channel(channel);
        disable_event_channel(dma->event_channel);
        MP_STATE_PORT(playing_audio)[channel] = NULL;
        audio_dma_state[channel] = NULL;
        audio_dma_free_channel(dma->dma_channel);
    }
    dma->dma_channel = AUDIO_DMA_CHANNEL_COUNT;
}

void audio_dma_pause(audio_dma_t* dma) {
    dma_suspend_channel(dma->dma_channel);
}

void audio_dma_resume(audio_dma_t* dma) {
    dma_resume_channel(dma->dma_channel);
}

bool audio_dma_get_paused(audio_dma_t* dma) {
    if (dma->dma_channel >= AUDIO_DMA_CHANNEL_COUNT) {
        return false;
    }
    uint32_t status = dma_transfer_status(dma->dma_channel);

    return (status & DMAC_CHINTFLAG_SUSP) != 0;
}

void audio_dma_init(audio_dma_t* dma) {
    dma->dma_channel = AUDIO_DMA_CHANNEL_COUNT;
}

void audio_dma_reset(void) {
    for (uint8_t i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        audio_dma_state[i] = NULL;
        audio_dma_pending[i] = false;
        audio_dma_allocated[i] = false;
        audio_dma_disable_channel(i);
        dma_descriptor(i)->BTCTRL.bit.VALID = false;
        MP_STATE_PORT(playing_audio)[i] = NULL;
    }
}

bool audio_dma_get_playing(audio_dma_t* dma) {
    if (dma->dma_channel >= AUDIO_DMA_CHANNEL_COUNT) {
        return false;
    }
    uint32_t status = dma_transfer_status(dma->dma_channel);
    if ((status & DMAC_CHINTFLAG_TCMPL) != 0 || (status & DMAC_CHINTFLAG_TERR) != 0) {
        audio_dma_stop(dma);
    }

    return (status & DMAC_CHINTFLAG_TERR) == 0;
}

// WARN(tannewt): DO NOT print from here. Printing calls background tasks such as this and causes a
// stack overflow.

void audio_dma_background(void) {
    for (uint8_t i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        if (audio_dma_pending[i]) {
            continue;
        }
        audio_dma_t* dma = audio_dma_state[i];
        if (dma == NULL) {
            continue;
        }

#ifdef SAMD51
        if (dma->sibling_channel == SECOND_SIBLING) {
            continue;
        }
#endif

        bool block_done = event_interrupt_active(dma->event_channel);
        if (!block_done) {
            continue;
        }

        // audio_dma_load_next_block() can call Python code, which can call audio_dma_background()
        // recursively at the next background processing time. So disallow recursive calls to here.
        audio_dma_pending[i] = true;
        audio_dma_load_next_block(dma);
        audio_dma_pending[i] = false;
    }
}
#endif
