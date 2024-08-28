// https://www.sparkfun.com/products/17717

#define MICROPY_HW_BOARD_NAME "SparkFun Pro Micro RP2350"
#define MICROPY_HW_FLASH_STORAGE_BYTES (PICO_FLASH_SIZE_BYTES - 1024 * 1024)

#define MICROPY_HW_USB_VID (0x1B4F)
#define MICROPY_HW_USB_PID (0x0039)

#define MICROPY_HW_UART1_TX (8)
#define MICROPY_HW_UART1_RX (9)
#define MICROPY_HW_UART1_CTS (10)
#define MICROPY_HW_UART1_RTS (11)

#define MICROPY_HW_PSRAM_CS_PIN (19)

#define MICROPY_HW_ENABLE_PSRAM (1)

// NeoPixel data GPIO25, power not toggleable
