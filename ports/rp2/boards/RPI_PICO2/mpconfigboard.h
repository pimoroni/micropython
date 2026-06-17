// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "Raspberry Pi Pico2"
#define MICROPY_HW_FLASH_STORAGE_BYTES          (PICO_FLASH_SIZE_BYTES - 1024 * 1024)

// Run the bytecode interpreter and its hottest leaf functions from SRAM.
#define MICROPY_HW_VM_IN_RAM                    (1)
