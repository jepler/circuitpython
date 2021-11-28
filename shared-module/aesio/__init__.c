#include <string.h>

#include "py/runtime.h"

#include "shared-bindings/aesio/__init__.h"
#include "shared-module/aesio/__init__.h"

void common_hal_aesio_aes_construct(aesio_aes_obj_t *self, const uint8_t *key,
    uint32_t key_length, const uint8_t *iv_counter,
    int mode) {
    self->mode = mode;
    common_hal_aesio_aes_rekey(self, key, key_length, iv_counter);
}

void common_hal_aesio_aes_rekey(aesio_aes_obj_t *self, const uint8_t *key,
    uint32_t key_length, const uint8_t *iv_counter) {
    memset(&self->ctx, 0, sizeof(self->ctx));
    if (iv_counter != NULL) {
        AES_init_ctx_iv(&self->ctx, key, key_length, iv_counter);
    } else {
        AES_init_ctx(&self->ctx, key, key_length);
    }
}

void common_hal_aesio_aes_set_mode(aesio_aes_obj_t *self, int mode) {
    self->mode = mode;
}

void common_hal_aesio_aes_encrypt(aesio_aes_obj_t *self, uint8_t *buffer,
    size_t length) {
    switch (self->mode) {
        case AES_MODE_ECB:
            AES_ECB_encrypt(&self->ctx, buffer);
            break;
        case AES_MODE_CBC:
            AES_CBC_encrypt_buffer(&self->ctx, buffer, length);
            break;
        case AES_MODE_CTR:
            AES_CTR_xcrypt_buffer(&self->ctx, buffer, length);
            break;
    }
}

void common_hal_aesio_aes_decrypt(aesio_aes_obj_t *self, uint8_t *buffer,
    size_t length) {
    switch (self->mode) {
        case AES_MODE_ECB:
            AES_ECB_decrypt(&self->ctx, buffer);
            break;
        case AES_MODE_CBC:
            AES_CBC_decrypt_buffer(&self->ctx, buffer, length);
            break;
        case AES_MODE_CTR:
            AES_CTR_xcrypt_buffer(&self->ctx, buffer, length);
            break;
    }
}
