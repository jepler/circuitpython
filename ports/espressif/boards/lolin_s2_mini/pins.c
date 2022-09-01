#include "shared-bindings/board/__init__.h"

STATIC const mp_rom_map_elem_t board_module_globals_table[] = {
    CIRCUITPYTHON_BOARD_DICT_STANDARD_ITEMS

    // S2 Mini Board bottom, right, top-bottom
    // GPIO0-GPIO14: broken out as a bloc on ESP32-S2FN4R2 SoC
    // mpconfigboard.h: GPIO0: CIRCUITPY_BOOT_BUTTON
    { MP_ROM_QSTR(MP_QSTR_BUTTON), MP_ROM_PTR(&pin_GPIO0) },
    { MP_ROM_QSTR(MP_QSTR_IO0), MP_ROM_PTR(&pin_GPIO0) },  // RTC_GPIO0,GPIO0
    { MP_ROM_QSTR(MP_QSTR_IO1), MP_ROM_PTR(&pin_GPIO1) },  // RTC_GPIO1,GPIO1,TOUCH1,ADC1_CH0

    { MP_ROM_QSTR(MP_QSTR_IO2), MP_ROM_PTR(&pin_GPIO2) },  // RTC_GPIO2,GPIO2,TOUCH2,ADC1_CH1
    { MP_ROM_QSTR(MP_QSTR_IO3), MP_ROM_PTR(&pin_GPIO3) },  // RTC_GPIO3,GPIO3,TOUCH3,ADC1_CH2
    { MP_ROM_QSTR(MP_QSTR_A0),  MP_ROM_PTR(&pin_GPIO3) },  // D1 mini pin A0

    { MP_ROM_QSTR(MP_QSTR_IO4), MP_ROM_PTR(&pin_GPIO4) },  // RTC_GPIO4,GPIO4,TOUCH4,ADC1_CH3
    { MP_ROM_QSTR(MP_QSTR_IO5), MP_ROM_PTR(&pin_GPIO5) },  // RTC_GPIO5,GPIO5,TOUCH5,ADC1_CH4
    { MP_ROM_QSTR(MP_QSTR_D0),  MP_ROM_PTR(&pin_GPIO5) },  // D1 mini pin D0 GPIO16

    { MP_ROM_QSTR(MP_QSTR_IO6), MP_ROM_PTR(&pin_GPIO6) },  // RTC_GPIO6,GPIO6,TOUCH6,ADC1_CH5
    { MP_ROM_QSTR(MP_QSTR_IO7), MP_ROM_PTR(&pin_GPIO7) },  // RTC_GPIO7,GPIO7,TOUCH7,ADC1_CH6
    { MP_ROM_QSTR(MP_QSTR_SCK), MP_ROM_PTR(&pin_GPIO7) },  // def from Wemos MP
    { MP_ROM_QSTR(MP_QSTR_D5),  MP_ROM_PTR(&pin_GPIO7) },  // D1 mini pin D5 GPIO14

    // mpconfigboard.h: GPIO8/GPIO9: SCL/SDA I2C0
    { MP_ROM_QSTR(MP_QSTR_IO8), MP_ROM_PTR(&pin_GPIO8) },  // RTC_GPIO8,GPIO8,TOUCH8,ADC1_CH7
    { MP_ROM_QSTR(MP_QSTR_IO9), MP_ROM_PTR(&pin_GPIO9) },  // RTC_GPIO9,GPIO9,TOUCH9,ADC1_CH8,FSPIHD
    { MP_ROM_QSTR(MP_QSTR_MISO), MP_ROM_PTR(&pin_GPIO9) }, // def from Wemos MP
    { MP_ROM_QSTR(MP_QSTR_D6),  MP_ROM_PTR(&pin_GPIO9) },  // D1 mini pin D6 GPIO12

    { MP_ROM_QSTR(MP_QSTR_IO10), MP_ROM_PTR(&pin_GPIO10) },// RTC_GPIO10,GPIO10,TOUCH10,ADC1_CH9,FSPICS0,FSPIIO4
    { MP_ROM_QSTR(MP_QSTR_IO11), MP_ROM_PTR(&pin_GPIO11) },// RTC_GPIO11,GPIO11,TOUCH11,ADC2_CH0,FSPID,FSPIIO5
    { MP_ROM_QSTR(MP_QSTR_MOSI), MP_ROM_PTR(&pin_GPIO11) },// def from Wemos MP
    { MP_ROM_QSTR(MP_QSTR_D7),   MP_ROM_PTR(&pin_GPIO11) },// D1 mini pin D7 GPIO13

    { MP_ROM_QSTR(MP_QSTR_IO12), MP_ROM_PTR(&pin_GPIO12) },// RTC_GPIO12,GPIO12,TOUCH12,ADC2_CH1,FSPICLK,FSPIIO6
    { MP_ROM_QSTR(MP_QSTR_IO13), MP_ROM_PTR(&pin_GPIO13) },// RTC_GPIO13,GPIO13,TOUCH13,ADC2_CH2,FSPIQ,FSPIIO7
    { MP_ROM_QSTR(MP_QSTR_D8),   MP_ROM_PTR(&pin_GPIO13) },// D1 mini pin D8 GPIO15

    { MP_ROM_QSTR(MP_QSTR_IO14), MP_ROM_PTR(&pin_GPIO14) },// RTC_GPIO14,GPIO14,TOUCH14,ADC2_CH3,FSPIWP,FSPIDQS

    // S2 Mini Board bottom, left, bottom-top
    // mpconfigboard.h: GPIO15: CIRCUITPY_STATUS_LED_POWER
    { MP_ROM_QSTR(MP_QSTR_LED),  MP_ROM_PTR(&pin_GPIO15) },
    { MP_ROM_QSTR(MP_QSTR_IO15), MP_ROM_PTR(&pin_GPIO15) },// XTAL_32K_P: RTC_GPIO15,GPIO15,U0RTS,ADC2_CH4,XTAL_32K_P

    { MP_ROM_QSTR(MP_QSTR_IO16), MP_ROM_PTR(&pin_GPIO16) },// XTAL_32K_N: RTC_GPIO16,GPIO16,U0CTS,ADC2_CH5,XTAL_32K_N
    { MP_ROM_QSTR(MP_QSTR_D4),   MP_ROM_PTR(&pin_GPIO16) },// D1 mini pin D4 GPIO2 LED
    { MP_ROM_QSTR(MP_QSTR_IO17), MP_ROM_PTR(&pin_GPIO17) },// DAC_1: RTC_GPIO17,GPIO17,U1TXD,ADC2_CH6,DAC_1

    { MP_ROM_QSTR(MP_QSTR_IO18), MP_ROM_PTR(&pin_GPIO18) },// DAC_2: RTC_GPIO18,GPIO18,U1RXD,ADC2_CH7,DAC_2,CLK_OUT3
    { MP_ROM_QSTR(MP_QSTR_D3),   MP_ROM_PTR(&pin_GPIO18) },// D1 mini pin D3 GPIO0
    // skip GPIO19-GPIO20: USB_D-/USB_D+
    { MP_ROM_QSTR(MP_QSTR_IO21), MP_ROM_PTR(&pin_GPIO21) },// RTC_GPIO21,GPIO21

    // skip GPIO22-GPIO25: not broken out on ESP32-S2FN4R2 SoC
    // skip GPIO26-GPIO32: SPI Flash & RAM, not broken out on S2 Mini (internal to ESP32-S2FN4R2 SoC?)

    // GPIO33-GPIO40: broken out as a bloc on ESP32-S2FN4R2 SoC, last 2 half of JTAG
    { MP_ROM_QSTR(MP_QSTR_IO33), MP_ROM_PTR(&pin_GPIO33) },// SPIIO4,GPIO33,FSPIHD
    { MP_ROM_QSTR(MP_QSTR_SDA),  MP_ROM_PTR(&pin_GPIO33) },// def from Wemos MP
    { MP_ROM_QSTR(MP_QSTR_D2),   MP_ROM_PTR(&pin_GPIO33) },// D1 mini pin D2 GPIO4
    { MP_ROM_QSTR(MP_QSTR_IO34), MP_ROM_PTR(&pin_GPIO34) },// SPIIO5,GPIO34,FSPICS0

    // mpconfigboard.h: GPIO35/GPIO36/GPIO37: MOSI/MESO/SCK SPI
    { MP_ROM_QSTR(MP_QSTR_IO35), MP_ROM_PTR(&pin_GPIO35) },// SPIIO6,GPIO35,FSPID
    { MP_ROM_QSTR(MP_QSTR_SCL),  MP_ROM_PTR(&pin_GPIO35) },// def from Wemos MP
    { MP_ROM_QSTR(MP_QSTR_D1),   MP_ROM_PTR(&pin_GPIO35) },// D1 mini pin D1 GPIO5
    { MP_ROM_QSTR(MP_QSTR_IO36), MP_ROM_PTR(&pin_GPIO36) },// SPIIO7,GPIO36,FSPICLK

    { MP_ROM_QSTR(MP_QSTR_IO37), MP_ROM_PTR(&pin_GPIO37) },// SPIDQS,GPIO37,FSPIQ
    { MP_ROM_QSTR(MP_QSTR_IO38), MP_ROM_PTR(&pin_GPIO38) },//        GPIO38,FSPIWP

    { MP_ROM_QSTR(MP_QSTR_IO39), MP_ROM_PTR(&pin_GPIO39) },// MTCK,GPIO39,CLK_OUT3
    { MP_ROM_QSTR(MP_QSTR_IO40), MP_ROM_PTR(&pin_GPIO40) },// MTDO,GPIO40,CLK_OUT2

    // S2 Mini - not broken out on board
    /*
    { MP_ROM_QSTR(MP_QSTR_IO41), MP_ROM_PTR(&pin_GPIO41) },// MTDI,GPIO41,CLK_OUT1
    { MP_ROM_QSTR(MP_QSTR_IO42), MP_ROM_PTR(&pin_GPIO42) },// MTMS,GPIO42
    { MP_ROM_QSTR(MP_QSTR_TX),   MP_ROM_PTR(&pin_GPIO43) },  // U0TXD,GPIO43,CLK_OUT1
    { MP_ROM_QSTR(MP_QSTR_IO43), MP_ROM_PTR(&pin_GPIO43) },//
    { MP_ROM_QSTR(MP_QSTR_RX),   MP_ROM_PTR(&pin_GPIO44) },  // U0RXD,GPIO44,CLK_OUT2
    { MP_ROM_QSTR(MP_QSTR_IO44), MP_ROM_PTR(&pin_GPIO44) },
    { MP_ROM_QSTR(MP_QSTR_IO45), MP_ROM_PTR(&pin_GPIO45) },// GPIO45
    { MP_ROM_QSTR(MP_QSTR_IO46), MP_ROM_PTR(&pin_GPIO46) },// GPIO46
    */
    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&board_i2c_obj) }, // board singleton implicit from schematic/shield standard
};
MP_DEFINE_CONST_DICT(board_module_globals, board_module_globals_table);
