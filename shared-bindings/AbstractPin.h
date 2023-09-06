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
