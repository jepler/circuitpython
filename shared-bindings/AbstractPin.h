#pragma once

typedef enum _digitalio_direction_t {
    DIRECTION_INPUT,
    DIRECTION_OUTPUT
} digitalio_direction_t;

typedef enum _digitalio_pull_t {
    PULL_NONE,
    PULL_UP,
    PULL_DOWN
} digitalio_pull_t;

typedef enum {
    DRIVE_MODE_PUSH_PULL,
    DRIVE_MODE_OPEN_DRAIN
} digitalio_drive_mode_t;

typedef enum {
    DIGITALINOUT_OK,
    DIGITALINOUT_PIN_BUSY,
    #if CIRCUITPY_DIGITALIO_HAVE_INPUT_ONLY
    DIGITALINOUT_INPUT_ONLY,
    #endif
    #if CIRCUITPY_DIGITALIO_HAVE_INVALID_PULL
    DIGITALINOUT_INVALID_PULL,
    #endif
    #if CIRCUITPY_DIGITALIO_HAVE_INVALID_DRIVE_MODE
    DIGITALINOUT_INVALID_DRIVE_MODE,
    #endif
} digitalinout_result_t;
