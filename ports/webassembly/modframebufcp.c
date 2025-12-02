#include "py/objmodule.h"
#include "py/runtime.h"
#include <emscripten.h>

const int WIDTH = 160;
const int HEIGHT = 120;

// Update the web framebuffer if there is one
static mp_obj_t mp_module_framebufcp_flip(mp_obj_t buf_in) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer(buf_in, &bufinfo, MP_BUFFER_READ);
    /*
    uint32_t data[WIDTH * HEIGHT];

    uint16_t *src = (uint16_t *)bufinfo.buf;
    uint8_t *dst = (uint8_t *)data;

    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            // 0bRRRRRGGGGGGBBBBB
            *dst++ = (*src >> 8) & 0b11111000;
            *dst++ = (*src >> 3) & 0b11111100;
            *dst++ = (*src << 3) & 0b11111000;
            *dst++ = 0xff;
            src++;
        }
    }
 */
    EM_ASM({
      let data = Module.HEAPU8.slice($0, $0 + 160 * 120 * 4);
      worker.canvas_image.data.set(buf);
      worker.canvas_context.putImageData(worker.canvas_image, 0, 0);
    }, bufinfo.buf);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(mp_module_framebufcp_flip_obj, mp_module_framebufcp_flip);

static const mp_rom_map_elem_t mp_module_framebufcp_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_simulator) },
    { MP_ROM_QSTR(MP_QSTR_flip), MP_ROM_PTR(&mp_module_framebufcp_flip_obj) },
    { MP_ROM_QSTR(MP_QSTR_WIDTH), MP_ROM_INT(WIDTH) },
    { MP_ROM_QSTR(MP_QSTR_HEIGHT), MP_ROM_INT(HEIGHT) },
};
static MP_DEFINE_CONST_DICT(mp_module_framebufcp_globals, mp_module_framebufcp_globals_table);

const mp_obj_module_t mp_module_framebufcp = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_framebufcp_globals,
};

MP_REGISTER_MODULE(MP_QSTR_simulator, mp_module_framebufcp);