
#include "../picovector.hpp"

#ifdef PICO
#include "pico/stdlib.h"
#endif

#include "mp_helpers.hpp"

using namespace picovector;

extern "C" {
  #include "py/stream.h"
  #include "py/reader.h"
  #include "py/runtime.h"
  #include "extmod/vfs.h"
  #include "py/mphal.h"

  extern const mp_obj_type_t type_Input;

#define BUTTON_HOME 0b100000
#define BUTTON_A    0b010000
#define BUTTON_B    0b001000
#define BUTTON_C    0b000100
#define BUTTON_UP   0b000010
#define BUTTON_DOWN 0b000001


  uint8_t picovector_buttons;
  uint8_t picovector_changed_buttons;
  mp_uint_t picovector_ticks;
  mp_uint_t picovector_last_ticks;


  void modinput_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    (void)self_in;

    if(attr == MP_QSTR_ticks && dest[0] == MP_OBJ_NULL) {
      dest[0] = mp_obj_new_int_from_ll(picovector_ticks);
      return;
    }

    if(attr == MP_QSTR_ticks_delta && dest[0] == MP_OBJ_NULL) {
      dest[0] = mp_obj_new_int_from_ll(picovector_ticks - picovector_last_ticks);
      return;
    }

    if((attr == MP_QSTR_held || attr == MP_QSTR_pressed || attr == MP_QSTR_released || attr == MP_QSTR_changed) && dest[0] == MP_OBJ_NULL) {
      mp_obj_t t_items[6];
      uint8_t buttons = 0;

      switch(attr) {
        case MP_QSTR_held:
          buttons = picovector_buttons;
          break;
        case MP_QSTR_pressed:
          buttons = picovector_buttons & picovector_changed_buttons;
          break;
        case MP_QSTR_released:
          buttons = ~picovector_buttons & picovector_changed_buttons;
          break;
        case MP_QSTR_changed:
          buttons = picovector_changed_buttons;
          break;
        default:
          break;
      }
      int n = 0;
      if(buttons & BUTTON_HOME) {
        t_items[n] = MP_ROM_INT(BUTTON_HOME);
        n++;
      }
      if(buttons & BUTTON_A) {
        t_items[n] = MP_ROM_INT(BUTTON_A);
        n++;
      }
      if(buttons & BUTTON_B) {
        t_items[n] = MP_ROM_INT(BUTTON_B);
        n++;
      }
      if(buttons & BUTTON_C) {
        t_items[n] = MP_ROM_INT(BUTTON_C);
        n++;
      }
      if(buttons & BUTTON_UP) {
        t_items[n] = MP_ROM_INT(BUTTON_UP);
        n++;
      }
      if(buttons & BUTTON_DOWN) {
        t_items[n] = MP_ROM_INT(BUTTON_DOWN);
        n++;
      }
      dest[0] = mp_obj_new_tuple(n, t_items);
      return;
    }

    dest[1] = MP_OBJ_SENTINEL;
  }

  // Call io.poll() to set up frame stable input and tick values
  mp_obj_t modinput_poll(mp_obj_t buttons_in) {
    uint8_t buttons = mp_obj_get_int(buttons_in);
    /*buttons |= gpio_get(BW_SWITCH_A)    ? 0 : BUTTON_A;
    buttons |= gpio_get(BW_SWITCH_B)    ? 0 : BUTTON_B;
    buttons |= gpio_get(BW_SWITCH_C)    ? 0 : BUTTON_C;
    buttons |= gpio_get(BW_SWITCH_UP)   ? 0 : BUTTON_UP;
    buttons |= gpio_get(BW_SWITCH_DOWN) ? 0 : BUTTON_DOWN;
    buttons |= gpio_get(BW_SWITCH_HOME) ? 0 : BUTTON_HOME;*/

    picovector_changed_buttons = buttons ^ picovector_buttons;
    picovector_buttons = buttons;
    picovector_last_ticks = picovector_ticks;
    picovector_ticks = mp_hal_ticks_ms();
// TODO: Make input work somehow?
#ifdef PICO
    uint8_t buttons = 0;
    buttons |= gpio_get(BW_SWITCH_A)    ? 0 : BUTTON_A;
    buttons |= gpio_get(BW_SWITCH_B)    ? 0 : BUTTON_B;
    buttons |= gpio_get(BW_SWITCH_C)    ? 0 : BUTTON_C;
    buttons |= gpio_get(BW_SWITCH_UP)   ? 0 : BUTTON_UP;
    buttons |= gpio_get(BW_SWITCH_DOWN) ? 0 : BUTTON_DOWN;
    buttons |= gpio_get(BW_SWITCH_HOME) ? 0 : BUTTON_HOME;

    picovector_changed_buttons = buttons ^ picovector_buttons;
    picovector_buttons = buttons;
    picovector_last_ticks = picovector_ticks;
    picovector_ticks = mp_hal_ticks_ms();
#endif
    return mp_const_none;
  }

}

