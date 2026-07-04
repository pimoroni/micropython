# RPI_PICO2 "FATLFS" variant: expose the on-board littlefs storage to a USB host
# as an editable FAT16 drive (extmod/fatlfs), entered via a double-tap reset or
# fatlfs.reboot_msc(). MICROPY_FATLFS=1 is set globally (not just on the firmware
# target) so shared/tinyusb/tusb_config.h sees it and builds in the MSC class;
# mpconfigport.h derives MICROPY_HW_USB_MSC from it. The ports/rp2 CMakeLists
# if(MICROPY_FATLFS) block then compiles and links the fatlfs sources.
set(MICROPY_FATLFS ON)
add_compile_definitions(MICROPY_FATLFS=1)
