// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "Raspberry Pi Pico"

#if MICROPY_PY_NETWORK_CYW43
// we have to reduce the flash storage if cyw43 is enabled or else the firmware gets overwritten
#define MICROPY_HW_FLASH_STORAGE_BYTES          (848 * 1024)
#include "enable_cyw43.h"
#define CYW43_PIN_WL_DYNAMIC 1
#define CYW43_PIO_CLOCK_DIV_DYNAMIC 1
#else
#define MICROPY_HW_FLASH_STORAGE_BYTES          (1408 * 1024)
#endif
