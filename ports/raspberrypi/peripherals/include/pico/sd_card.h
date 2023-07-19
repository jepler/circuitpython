/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_SD_CARD_H
#define _PICO_SD_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pico.h"
#include "hardware/pio.h"

#define SD_OK (0)
#define SD_ERR_STUCK (-1)
#define SD_ERR_BAD_RESPONSE (-2)
#define SD_ERR_CRC (-3)
#define SD_ERR_BAD_PARAM (-4)

#define PICO_SD_MAX_BLOCK_COUNT 32

#define SD_SECTOR_SIZE 512

typedef enum {bw_unknown, bw_narrow, bw_wide} pico_pio_bus_width;
typedef struct {
    PIO sd_pio;
    uint8_t clk, cmd, dat0;
    uint8_t allow_four_data_pins, bytes_swap_on_read;
    pico_pio_bus_width bus_width;
    uint clk_program_offset;
    uint32_t capacity;
} pico_pio_sdio;

int sd_init(pico_pio_sdio *self);
void sd_deinit(pico_pio_sdio *self);
int sd_readblocks_sync(pico_pio_sdio *self, uint32_t *buf, uint32_t block, uint block_count);
int sd_readblocks_async(pico_pio_sdio *self, uint32_t *buf, uint32_t block, uint block_count);
int sd_readblocks_scatter_async(pico_pio_sdio *self, uint32_t *control_words, uint32_t block, uint block_count);
void sd_set_byteswap_on_read(pico_pio_sdio *self, bool swap);
bool sd_scatter_read_complete(pico_pio_sdio *self, int *status);
int sd_writeblocks_async(pico_pio_sdio *self, const uint32_t *data, uint32_t sector_num, uint sector_count);
bool sd_write_complete(pico_pio_sdio *self, int *status);
int sd_read_sectors_1bit_crc_async(pico_pio_sdio *self, uint32_t *sector_buf, uint32_t sector, uint sector_count);
int sd_set_wide_bus(pico_pio_sdio *self, bool wide);
int sd_set_clock_divider(pico_pio_sdio *self, int div);

#endif

#ifdef __cplusplus
}
#endif
