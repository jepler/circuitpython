#define MICROPY_HW_BOARD_NAME "Pimoroni Pico Plus 2"
#define MICROPY_HW_MCU_NAME "rp2350b"

#define MICROPY_HW_LED_STATUS (&pin_GPIO25)

#define CIRCUITPY_BOARD_I2C         (1)
#define CIRCUITPY_BOARD_I2C_PIN     {{.scl = &pin_GPIO5, .sda = &pin_GPIO4}}

#define CIRCUITPY_PSRAM_CHIP_SELECT (&pin_GPIO47)
