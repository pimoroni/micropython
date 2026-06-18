// Badgeware simulator for the Pimoroni Tufty 2350.
#define MICROPY_HW_BOARD_NAME                   "Pimoroni Tufty 2350"
#define MICROPY_HW_MCU_NAME                     "RP2350"

#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_FULL_FEATURES)
#define MICROPY_ENABLE_VM_YIELD               (1)
#define MICROPY_GC_SPLIT_HEAP                   (1)
#define MICROPY_GC_SPLIT_HEAP_AUTO              (1)
#define MICROPY_PY_WEAKREF                      (1)
#define MICROPY_TRACKED_ALLOC                   (1)
