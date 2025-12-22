#include "py/objmodule.h"
#include "py/runtime.h"
#include <emscripten.h>

uint32_t framebuffer[320 * 240];

// Update the web framebuffer if there is one
static mp_obj_t mp_module_simulator_update(mp_obj_t fullres) {
    if(mp_obj_is_true(fullres)) {
        EM_ASM({
            let data = Module.HEAPU8.slice($0, $0 + 320 * 240 * 4);
            WorkerGlobalScope.worker.flip_hires(data);
        }, (uint8_t *)framebuffer);
    } else {
        EM_ASM({
            let data = Module.HEAPU8.slice($0, $0 + 160 * 120 * 4);
            WorkerGlobalScope.worker.flip_lores(data);
        }, (uint8_t *)framebuffer);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_module_simulator_update_obj, mp_module_simulator_update);

static mp_obj_t mp_module_simulator_get_buffer() {
    return mp_obj_new_bytearray_by_ref(sizeof(framebuffer), framebuffer);
}
static MP_DEFINE_CONST_FUN_OBJ_0(mp_module_simulator_get_buffer_obj, mp_module_simulator_get_buffer);

static const mp_rom_map_elem_t mp_module_simulator_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_simulator) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&mp_module_simulator_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_buffer), MP_ROM_PTR(&mp_module_simulator_get_buffer_obj) },
};
static MP_DEFINE_CONST_DICT(mp_module_simulator_globals, mp_module_simulator_globals_table);

const mp_obj_module_t mp_module_simulator = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_simulator_globals,
};

MP_REGISTER_MODULE(MP_QSTR_simulator, mp_module_simulator);