// A simulated subset of MicroPython's `machine` module for the WebAssembly
// port. It mimics the RP2350 peripherals used by the badgeware firmware
// (Pin, PWM, ADC, I2C, RTC) and bridges their state to/from the owning web
// worker via Emscripten's EM_ASM, in the same crude style as
// picovector-old/micropython/input.cpp.
//
// The JavaScript side sees a `WorkerGlobalScope.worker.machine` object:
//
//   worker.machine = {
//     gpio:       { <gpio>: 0|1 },           // last value driven by Pin.value(x)
//     gpio_in:    { <gpio>: 0|1 },            // input states the JS host may set
//     pwm:        { <gpio>: {freq, duty} },   // duty is a u16 (0..65535)
//     caselights: [cl0, cl1, cl2, cl3],       // normalised 0..1, CL0..CL3 (GPIO0..3)
//     adc:        { <channel>: u16 },          // ADC inputs the JS host may set
//   }
//
// Anything the firmware writes (PWM duty for the caselights in particular)
// lands here for the host to visualise. Anything the firmware reads (button
// GPIOs, ADC channels) is sourced from here, falling back to sensible
// defaults so the unmodified badgeware code behaves.

#ifdef __EMSCRIPTEN__
#include <emscripten/em_asm.h>
#include <emscripten.h>  // emscripten_sleep / emscripten_get_now
#endif

#include "../picovector/micropython/mp_helpers.hpp"

// The EM_ASM blocks below each begin by ensuring WorkerGlobalScope.worker.machine
// exists. That snippet is inlined rather than factored into a macro because
// emscripten captures the EM_ASM body verbatim and does not expand object-like
// macros within it. It is also kept free of top-level commas so the C
// preprocessor doesn't mistake them for EM_ASM argument separators.

extern "C" {
  #include <string.h>
  #include "py/runtime.h"
  #include "py/objstr.h"

  // Defined in main.c: non-zero while inside a JSPI-suspendable entry point.
  extern size_t external_call_depth_get(void);

  // ----------------------------------------------------- cooperative yield
  // The worker runs MicroPython on a single thread, so inbound messages (button
  // states posted from the host) are only delivered, and outbound postMessages
  // only flushed, when the WASM stack yields to the JS event loop. Scripts that
  // never call badge.update() (e.g. a bare `while True:`) would otherwise never
  // see input change. mp_simulator_yield() is driven from the VM hook (see
  // mpconfigport.h) and from mp_hal_delay_ms(), so it runs for all Python code
  // without the script having to cooperate.
  //
  // Yields are throttled to roughly one frame so compute-bound code isn't
  // crippled, and only happen while it is safe to suspend the stack (i.e. inside
  // mp_js_do_exec and friends). emscripten_sleep(0) gives the event loop a turn,
  // during which queued onmessage handlers run before we resume.
  #ifndef SIMULATOR_YIELD_INTERVAL_MS
  #define SIMULATOR_YIELD_INTERVAL_MS (16.0)
  #endif

  static double simulator_last_yield_ms = -1.0;

  // Record that a yield has just happened (e.g. a display flip already gave the
  // event loop a turn) so the throttle doesn't fire a redundant extra yield.
  void mp_simulator_mark_yield(void) {
#ifdef __EMSCRIPTEN__
    simulator_last_yield_ms = emscripten_get_now();
#endif
  }

  void mp_simulator_yield(void) {
#ifdef __EMSCRIPTEN__
    if (external_call_depth_get() == 0) {
      return;  // not inside a suspendable call; unsafe to emscripten_sleep()
    }
    double now = emscripten_get_now();
    if (simulator_last_yield_ms < 0.0) {
      simulator_last_yield_ms = now;  // prime the throttle without yielding
      return;
    }
    if (now - simulator_last_yield_ms >= SIMULATOR_YIELD_INTERVAL_MS) {
      // While the host has paused execution (e.g. the simulator was scrolled
      // out of view), block here and keep giving the event loop turns so we
      // make no progress yet still receive the resume message. Gated on
      // `running` so import and program setup (which happen before the run loop)
      // are never paused. This halts even a self-looping script, since it is
      // reached from the VM hook.
      while (EM_ASM_INT({
        var w = (typeof WorkerGlobalScope !== 'undefined') ? WorkerGlobalScope.worker : null;
        return (w && w.running && w.paused) ? 1 : 0;
      })) {
        emscripten_sleep(50);
      }
      simulator_last_yield_ms = emscripten_get_now();
      emscripten_sleep(0);
    }
#endif
  }

  // ------------------------------------------------------------------ types
  extern const mp_obj_type_t type_machine_pin;
  extern const mp_obj_type_t type_machine_pwm;
  extern const mp_obj_type_t type_machine_adc;
  extern const mp_obj_type_t type_machine_i2c;
  extern const mp_obj_type_t type_machine_rtc;
  extern const mp_obj_type_t type_machine_board;

  typedef struct { mp_obj_base_t base; int16_t id; uint8_t mode; uint8_t pull; } pin_obj_t;
  typedef struct { mp_obj_base_t base; int16_t id; uint32_t freq; uint16_t duty; } pwm_obj_t;
  typedef struct { mp_obj_base_t base; int16_t channel; int16_t gpio; } adc_obj_t;
  typedef struct { mp_obj_base_t base; uint8_t id; } i2c_obj_t;
  typedef struct { mp_obj_base_t base; } rtc_obj_t;
  typedef struct { mp_obj_base_t base; } board_obj_t;

  // --------------------------------------------------------------- board map
  // Names and GPIO numbers mirror tufty2350/board/pins.csv. CHARGE_STAT lives
  // on an IO expander (EXT_GPIO2) on hardware, so it gets a synthetic id.
  typedef struct { const char *name; int16_t id; } board_pin_t;
  static const board_pin_t board_pins[] = {
    {"CL0", 0}, {"CL1", 1}, {"CL2", 2}, {"CL3", 3},
    {"I2C_SDA", 4}, {"I2C_SCL", 5},
    {"BUTTON_DOWN", 6}, {"BUTTON_A", 7}, {"BUTTON_B", 9},
    {"BUTTON_C", 10}, {"BUTTON_UP", 11},
    {"VBUS_DETECT", 12}, {"RTC_ALARM", 13}, {"BUTTON_RESET", 14},
    {"BUTTON_INT", 15}, {"BUTTON_HOME", 22},
    {"LCD_BACKLIGHT", 26},
    {"VBAT_SENSE", 40}, {"POWER_EN", 41}, {"SENSE_1V1", 42}, {"LIGHT_SENSE", 43},
    {"CHARGE_STAT", 202},
  };

  static int board_pin_id(const char *name) {
    for(size_t i = 0; i < MP_ARRAY_SIZE(board_pins); i++) {
      if(strcmp(board_pins[i].name, name) == 0) return board_pins[i].id;
    }
    return -1;
  }

  // Map a button GPIO to its bit in the input bitmask used by picovector_io.
  static uint8_t button_bit_for_gpio(int id) {
    switch(id) {
      case 22: return 0b100000; // HOME
      case 7:  return 0b010000; // A
      case 9:  return 0b001000; // B
      case 10: return 0b000100; // C
      case 11: return 0b000010; // UP
      case 6:  return 0b000001; // DOWN
      default: return 0;
    }
  }

  // ----------------------------------------------------------- JS bridge I/O
  static int js_gpio_get(int id, int bit) {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
      var s=(typeof WorkerGlobalScope!=='undefined'&&WorkerGlobalScope.worker)?WorkerGlobalScope.worker:null; if(s&&!s.machine){var m={};m.gpio={};m.gpio_in={};m.pwm={};m.caselights=new Array(4).fill(0);m.adc={};s.machine=m;}
      if(!s) return $1 ? 1 : 0;
      if(s.machine.gpio_in[$0] !== undefined) return s.machine.gpio_in[$0] | 0;
      if($1 && (s.input & $1)) return 0;   // buttons are active-low when pressed
      if($1) return 1;                     // ...and pulled high when released
      return s.machine.gpio[$0] | 0;       // otherwise report the last driven value
    }, id, bit);
#else
    (void)id; return bit ? 1 : 0;
#endif
  }

  // Public accessor so other simulator modules (e.g. input.cpp) can sample a
  // GPIO with the same input semantics the firmware sees, including active-low
  // button handling for the known button pins.
  int simulator_gpio_get(int gpio) {
    return js_gpio_get(gpio, button_bit_for_gpio(gpio));
  }

  static void js_gpio_set(int id, int value) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
      var s=(typeof WorkerGlobalScope!=='undefined'&&WorkerGlobalScope.worker)?WorkerGlobalScope.worker:null; if(s&&!s.machine){var m={};m.gpio={};m.gpio_in={};m.pwm={};m.caselights=new Array(4).fill(0);m.adc={};s.machine=m;}
      if(!s) return;
      s.machine.gpio[$0] = $1 ? 1 : 0;
    }, id, value);
#else
    (void)id; (void)value;
#endif
  }

  static void js_pwm_set(int id, uint32_t freq, uint32_t duty) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
      var s=(typeof WorkerGlobalScope!=='undefined'&&WorkerGlobalScope.worker)?WorkerGlobalScope.worker:null; if(s&&!s.machine){var m={};m.gpio={};m.gpio_in={};m.pwm={};m.caselights=new Array(4).fill(0);m.adc={};s.machine=m;}
      if(!s) return;
      var p = s.machine.pwm[$0];
      if(!p) { p = {}; s.machine.pwm[$0] = p; }
      p.freq = $1;
      p.duty = $2;
      // CL0..CL3 (GPIO0..3) drive the caselights; push changes to the host so
      // they propagate without waiting for a display flip. update_caselights()
      // de-dupes and only posts when a value actually changed.
      if($0 >= 0 && $0 < 4) {
        s.machine.caselights[$0] = $2 / 65535;
        if(s.update_caselights) s.update_caselights();
      }
    }, id, (int)freq, (int)duty);
#else
    (void)id; (void)freq; (void)duty;
#endif
  }

  static int js_adc_get(int channel) {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({
      var s=(typeof WorkerGlobalScope!=='undefined'&&WorkerGlobalScope.worker)?WorkerGlobalScope.worker:null; if(s&&!s.machine){var m={};m.gpio={};m.gpio_in={};m.pwm={};m.caselights=new Array(4).fill(0);m.adc={};s.machine=m;}
      if(s && s.machine.adc[$0] !== undefined) return s.machine.adc[$0] | 0;
      if($0 === 0) return 39700;   // VBAT_SENSE  (~4.0V through /2 divider)
      if($0 === 2) return 21845;   // SENSE_1V1   (~1.1V reference)
      if($0 === 3) return 40000;   // LIGHT_SENSE (a comfortable mid level)
      return 0;
    }, channel);
#else
    if(channel == 0) return 39700;
    if(channel == 2) return 21845;
    if(channel == 3) return 40000;
    return 0;
#endif
  }

  // -------------------------------------------------------------- helpers
  static mp_obj_t machine_make_pin(int id) {
    pin_obj_t *self = mp_obj_malloc(pin_obj_t, &type_machine_pin);
    self->id = id;
    self->mode = 0;
    self->pull = 0;
    return MP_OBJ_FROM_PTR(self);
  }

  // Resolve an int gpio, a board-name string, or an existing Pin to a gpio id.
  static int pin_id_from_obj(mp_obj_t obj) {
    if(mp_obj_is_type(obj, &type_machine_pin)) {
      return ((pin_obj_t *)MP_OBJ_TO_PTR(obj))->id;
    }
    if(mp_obj_is_str(obj)) {
      int id = board_pin_id(mp_obj_str_get_str(obj));
      if(id < 0) mp_raise_ValueError(MP_ERROR_TEXT("unknown pin name"));
      return id;
    }
    if(mp_obj_is_int(obj)) {
      return mp_obj_get_int(obj);
    }
    mp_raise_TypeError(MP_ERROR_TEXT("expected a Pin, gpio number or pin name"));
  }

  // =========================================================== Pin.board
  static board_obj_t board_obj = {{ &type_machine_board }};

  MPY_BIND_ATTR(board, {
    (void)self_in;
    if(dest[0] == MP_OBJ_NULL) { // load
      int id = board_pin_id(qstr_str(attr));
      if(id >= 0) {
        dest[0] = machine_make_pin(id);
        return;
      }
    }
    dest[1] = MP_OBJ_SENTINEL; // not found / read-only
  })

  MP_DEFINE_CONST_OBJ_TYPE(
    type_machine_board,
    MP_QSTR_board,
    MP_TYPE_FLAG_NONE,
    attr, (const void *)board_attr
  );

  // ================================================================= Pin
  MPY_BIND_NEW(pin, {
    (void)n_kw;
    if(n_args < 1) mp_raise_TypeError(MP_ERROR_TEXT("Pin requires an id"));
    pin_obj_t *self = mp_obj_malloc(pin_obj_t, &type_machine_pin);
    self->id = pin_id_from_obj(args[0]);
    self->mode = (n_args > 1 && args[1] != mp_const_none) ? (uint8_t)mp_obj_get_int(args[1]) : 0;
    self->pull = (n_args > 2 && args[2] != mp_const_none) ? (uint8_t)mp_obj_get_int(args[2]) : 0;
    return MP_OBJ_FROM_PTR(self);
  })

  // value([x]) -> read when called with no argument, write otherwise.
  MPY_BIND_VAR(1, pin_value, {
    pin_obj_t *self = (pin_obj_t *)MP_OBJ_TO_PTR(args[0]);
    if(n_args == 1) {
      return mp_obj_new_int(js_gpio_get(self->id, button_bit_for_gpio(self->id)));
    }
    js_gpio_set(self->id, mp_obj_is_true(args[1]) ? 1 : 0);
    return mp_const_none;
  })

  MPY_BIND_CLASSMETHOD_ARGS0(pin_on, {
    pin_obj_t *self = (pin_obj_t *)MP_OBJ_TO_PTR(self_in);
    js_gpio_set(self->id, 1);
    return mp_const_none;
  })

  MPY_BIND_CLASSMETHOD_ARGS0(pin_off, {
    pin_obj_t *self = (pin_obj_t *)MP_OBJ_TO_PTR(self_in);
    js_gpio_set(self->id, 0);
    return mp_const_none;
  })

  MPY_BIND_CLASSMETHOD_ARGS0(pin_toggle, {
    pin_obj_t *self = (pin_obj_t *)MP_OBJ_TO_PTR(self_in);
    int v = js_gpio_get(self->id, button_bit_for_gpio(self->id));
    js_gpio_set(self->id, v ? 0 : 1);
    return mp_const_none;
  })

  // init(...) is accepted but does nothing in the simulator.
  MPY_BIND_VAR(1, pin_init, {
    (void)n_args; (void)args;
    return mp_const_none;
  })

  // irq(trigger=..., handler=...) is accepted but never fires in the simulator.
  static mp_obj_t pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    (void)n_args; (void)pos_args; (void)kw_args;
    return mp_const_none;
  }
  static MP_DEFINE_CONST_FUN_OBJ_KW(pin_irq_obj, 1, pin_irq);

  static void pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    pin_obj_t *self = (pin_obj_t *)MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Pin(%d)", self->id);
  }

  static const mp_rom_map_elem_t pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_value),  MP_ROM_PTR(&mpy_binding_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_on),     MP_ROM_PTR(&mpy_binding_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off),    MP_ROM_PTR(&mpy_binding_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&mpy_binding_pin_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_init),   MP_ROM_PTR(&mpy_binding_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq),    MP_ROM_PTR(&pin_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_board),  MP_ROM_PTR(&board_obj) },
    { MP_ROM_QSTR(MP_QSTR_IN),          MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_OUT),         MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN),  MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP),     MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN),   MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING),  MP_ROM_INT(8) },
  };
  static MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
    type_machine_pin,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, (const void *)pin_new,
    print, (const void *)pin_print,
    locals_dict, &pin_locals_dict
  );

  // ================================================================= PWM
  MPY_BIND_NEW(pwm, {
    (void)n_kw;
    if(n_args < 1) mp_raise_TypeError(MP_ERROR_TEXT("PWM requires a pin"));
    pwm_obj_t *self = mp_obj_malloc(pwm_obj_t, &type_machine_pwm);
    self->id = pin_id_from_obj(args[0]);
    self->freq = 1000;
    self->duty = 0;
    js_pwm_set(self->id, self->freq, self->duty);
    return MP_OBJ_FROM_PTR(self);
  })

  MPY_BIND_VAR(1, pwm_freq, {
    pwm_obj_t *self = (pwm_obj_t *)MP_OBJ_TO_PTR(args[0]);
    if(n_args == 1) return mp_obj_new_int(self->freq);
    self->freq = mp_obj_get_int(args[1]);
    js_pwm_set(self->id, self->freq, self->duty);
    return mp_const_none;
  })

  MPY_BIND_VAR(1, pwm_duty_u16, {
    pwm_obj_t *self = (pwm_obj_t *)MP_OBJ_TO_PTR(args[0]);
    if(n_args == 1) return mp_obj_new_int(self->duty);
    int duty = mp_obj_get_int(args[1]);
    if(duty < 0) duty = 0;
    if(duty > 65535) duty = 65535;
    self->duty = (uint16_t)duty;
    js_pwm_set(self->id, self->freq, self->duty);
    return mp_const_none;
  })

  // duty_ns(...) approximated against the current frequency.
  MPY_BIND_VAR(1, pwm_duty_ns, {
    pwm_obj_t *self = (pwm_obj_t *)MP_OBJ_TO_PTR(args[0]);
    if(n_args == 1) {
      uint32_t period_ns = self->freq ? (1000000000u / self->freq) : 0;
      return mp_obj_new_int(period_ns ? (self->duty * period_ns / 65535) : 0);
    }
    uint32_t period_ns = self->freq ? (1000000000u / self->freq) : 0;
    uint32_t ns = mp_obj_get_int(args[1]);
    uint32_t duty = period_ns ? (ns * 65535 / period_ns) : 0;
    if(duty > 65535) duty = 65535;
    self->duty = (uint16_t)duty;
    js_pwm_set(self->id, self->freq, self->duty);
    return mp_const_none;
  })

  MPY_BIND_CLASSMETHOD_ARGS0(pwm_deinit, {
    pwm_obj_t *self = (pwm_obj_t *)MP_OBJ_TO_PTR(self_in);
    self->duty = 0;
    js_pwm_set(self->id, self->freq, 0);
    return mp_const_none;
  })

  MPY_BIND_VAR(1, pwm_init, {
    (void)n_args; (void)args;
    return mp_const_none;
  })

  static void pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    pwm_obj_t *self = (pwm_obj_t *)MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "PWM(Pin(%d), freq=%u, duty_u16=%u)", self->id, self->freq, self->duty);
  }

  static const mp_rom_map_elem_t pwm_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_freq),     MP_ROM_PTR(&mpy_binding_pwm_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty_u16), MP_ROM_PTR(&mpy_binding_pwm_duty_u16_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty_ns),  MP_ROM_PTR(&mpy_binding_pwm_duty_ns_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),   MP_ROM_PTR(&mpy_binding_pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init),     MP_ROM_PTR(&mpy_binding_pwm_init_obj) },
  };
  static MP_DEFINE_CONST_DICT(pwm_locals_dict, pwm_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
    type_machine_pwm,
    MP_QSTR_PWM,
    MP_TYPE_FLAG_NONE,
    make_new, (const void *)pwm_new,
    print, (const void *)pwm_print,
    locals_dict, &pwm_locals_dict
  );

  // ================================================================= ADC
  // RP2350B maps GPIO40..47 to ADC channels 0..7.
  static int16_t adc_channel_for_gpio(int gpio) {
    if(gpio >= 40 && gpio <= 47) return (int16_t)(gpio - 40);
    return -1;
  }

  MPY_BIND_NEW(adc, {
    (void)n_kw;
    if(n_args < 1) mp_raise_TypeError(MP_ERROR_TEXT("ADC requires a source"));
    adc_obj_t *self = mp_obj_malloc(adc_obj_t, &type_machine_adc);
    if(mp_obj_is_int(args[0])) {
      int v = mp_obj_get_int(args[0]);
      if(v >= 0 && v < 8) {        // a bare ADC channel number
        self->channel = (int16_t)v;
        self->gpio = (int16_t)(40 + v);
      } else {                     // a gpio number
        self->gpio = (int16_t)v;
        self->channel = adc_channel_for_gpio(v);
      }
    } else {
      self->gpio = (int16_t)pin_id_from_obj(args[0]);
      self->channel = adc_channel_for_gpio(self->gpio);
    }
    return MP_OBJ_FROM_PTR(self);
  })

  MPY_BIND_CLASSMETHOD_ARGS0(adc_read_u16, {
    adc_obj_t *self = (adc_obj_t *)MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(js_adc_get(self->channel));
  })

  MPY_BIND_CLASSMETHOD_ARGS0(adc_read_uv, {
    adc_obj_t *self = (adc_obj_t *)MP_OBJ_TO_PTR(self_in);
    uint64_t uv = (uint64_t)js_adc_get(self->channel) * 3300000u / 65535u;
    return mp_obj_new_int((mp_int_t)uv);
  })

  static void adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    adc_obj_t *self = (adc_obj_t *)MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "ADC(channel=%d, gpio=%d)", self->channel, self->gpio);
  }

  static const mp_rom_map_elem_t adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_u16), MP_ROM_PTR(&mpy_binding_adc_read_u16_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_uv),  MP_ROM_PTR(&mpy_binding_adc_read_uv_obj) },
  };
  static MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
    type_machine_adc,
    MP_QSTR_ADC,
    MP_TYPE_FLAG_NONE,
    make_new, (const void *)adc_new,
    print, (const void *)adc_print,
    locals_dict, &adc_locals_dict
  );

  // ================================================================= I2C
  // A stub: the simulator has no peripherals on the bus. Drivers like
  // pcf85063a.py are themselves shimmed, so reads simply return zeros.
  MPY_BIND_NEW(i2c, {
    (void)n_args; (void)n_kw; (void)args;
    i2c_obj_t *self = mp_obj_malloc(i2c_obj_t, &type_machine_i2c);
    self->id = 0;
    return MP_OBJ_FROM_PTR(self);
  })

  MPY_BIND_CLASSMETHOD_ARGS0(i2c_scan, {
    (void)self_in;
    return mp_obj_new_list(0, NULL);
  })

  MPY_BIND_VAR(1, i2c_readfrom_mem, {
    (void)args;
    size_t n = (n_args >= 4) ? (size_t)mp_obj_get_int(args[3]) : 0;
    vstr_t vstr;
    vstr_init_len(&vstr, n);
    memset(vstr.buf, 0, n);
    return mp_obj_new_bytes_from_vstr(&vstr);
  })

  MPY_BIND_VAR(1, i2c_writeto_mem, {
    (void)n_args; (void)args;
    return mp_const_none;
  })

  static const mp_rom_map_elem_t i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scan),         MP_ROM_PTR(&mpy_binding_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem), MP_ROM_PTR(&mpy_binding_i2c_readfrom_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem),  MP_ROM_PTR(&mpy_binding_i2c_writeto_mem_obj) },
  };
  static MP_DEFINE_CONST_DICT(i2c_locals_dict, i2c_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
    type_machine_i2c,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, (const void *)i2c_new,
    locals_dict, &i2c_locals_dict
  );

  // ================================================================= RTC
  MPY_BIND_NEW(rtc, {
    (void)n_args; (void)n_kw; (void)args;
    rtc_obj_t *self = mp_obj_malloc(rtc_obj_t, &type_machine_rtc);
    return MP_OBJ_FROM_PTR(self);
  })

  // datetime([(year, month, day, weekday, hours, minutes, seconds, subseconds)])
  MPY_BIND_VAR(1, rtc_datetime, {
    if(n_args > 1) {
      (void)args; // setting the host clock isn't supported; accept and ignore
      return mp_const_none;
    }
    int year = 2026;
    int month = 1;
    int day = 19;
    int weekday = 0; // Mon=0..Sun=6
    int hours = 12;
    int minutes = 0;
    int seconds = 0;
#ifdef __EMSCRIPTEN__
    year    = EM_ASM_INT({ return new Date().getFullYear(); });
    month   = EM_ASM_INT({ return new Date().getMonth() + 1; });
    day     = EM_ASM_INT({ return new Date().getDate(); });
    weekday = EM_ASM_INT({ return (new Date().getDay() + 6) % 7; });
    hours   = EM_ASM_INT({ return new Date().getHours(); });
    minutes = EM_ASM_INT({ return new Date().getMinutes(); });
    seconds = EM_ASM_INT({ return new Date().getSeconds(); });
#endif
    mp_obj_t tuple[8];
    tuple[0] = mp_obj_new_int(year);
    tuple[1] = mp_obj_new_int(month);
    tuple[2] = mp_obj_new_int(day);
    tuple[3] = mp_obj_new_int(weekday);
    tuple[4] = mp_obj_new_int(hours);
    tuple[5] = mp_obj_new_int(minutes);
    tuple[6] = mp_obj_new_int(seconds);
    tuple[7] = mp_obj_new_int(0);
    return mp_obj_new_tuple(8, tuple);
  })

  MPY_BIND_VAR(1, rtc_init, {
    (void)n_args; (void)args;
    return mp_const_none;
  })

  static const mp_rom_map_elem_t rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&mpy_binding_rtc_datetime_obj) },
    { MP_ROM_QSTR(MP_QSTR_init),     MP_ROM_PTR(&mpy_binding_rtc_init_obj) },
  };
  static MP_DEFINE_CONST_DICT(rtc_locals_dict, rtc_locals_dict_table);

  MP_DEFINE_CONST_OBJ_TYPE(
    type_machine_rtc,
    MP_QSTR_RTC,
    MP_TYPE_FLAG_NONE,
    make_new, (const void *)rtc_new,
    locals_dict, &rtc_locals_dict
  );

  // ====================================================== module functions
  MPY_BIND_ARGS0(machine_reset, {
#ifdef __EMSCRIPTEN__
    EM_ASM({
      if(typeof WorkerGlobalScope !== 'undefined' && WorkerGlobalScope.worker) {
        WorkerGlobalScope.worker.postMessage({reset: true});
      }
    });
#endif
    return mp_const_none;
  })

  MPY_BIND_ARGS0(machine_soft_reset, {
    return mp_const_none;
  })

  static const uint8_t machine_uid[8] = {0xBA, 0xD9, 0xE0, 0x05, 0x1E, 0x00, 0x00, 0x01};
  MPY_BIND_ARGS0(machine_unique_id, {
    return mp_obj_new_bytes(machine_uid, sizeof(machine_uid));
  })

  MPY_BIND_VAR(0, machine_freq, {
    (void)n_args; (void)args;
    return mp_obj_new_int(200000000); // 200 MHz, matching the tufty2350 board
  })

  MPY_BIND_VAR(0, machine_idle, {
    (void)n_args; (void)args;
    return mp_const_none;
  })

  MPY_BIND_VAR(0, machine_lightsleep, {
    (void)n_args; (void)args;
    return mp_const_none;
  })

  MPY_BIND_ARGS0(machine_disable_irq, {
    return mp_obj_new_int(0);
  })

  MPY_BIND_ARGS1(machine_enable_irq, state, {
    (void)state;
    return mp_const_none;
  })

  static const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_machine) },
    { MP_ROM_QSTR(MP_QSTR_Pin),         MP_ROM_PTR(&type_machine_pin) },
    { MP_ROM_QSTR(MP_QSTR_PWM),         MP_ROM_PTR(&type_machine_pwm) },
    { MP_ROM_QSTR(MP_QSTR_ADC),         MP_ROM_PTR(&type_machine_adc) },
    { MP_ROM_QSTR(MP_QSTR_I2C),         MP_ROM_PTR(&type_machine_i2c) },
    { MP_ROM_QSTR(MP_QSTR_SoftI2C),     MP_ROM_PTR(&type_machine_i2c) },
    { MP_ROM_QSTR(MP_QSTR_RTC),         MP_ROM_PTR(&type_machine_rtc) },
    { MP_ROM_QSTR(MP_QSTR_reset),       MP_ROM_PTR(&mpy_binding_machine_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset),  MP_ROM_PTR(&mpy_binding_machine_soft_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_unique_id),   MP_ROM_PTR(&mpy_binding_machine_unique_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq),        MP_ROM_PTR(&mpy_binding_machine_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_idle),        MP_ROM_PTR(&mpy_binding_machine_idle_obj) },
    { MP_ROM_QSTR(MP_QSTR_lightsleep),  MP_ROM_PTR(&mpy_binding_machine_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_irq), MP_ROM_PTR(&mpy_binding_machine_disable_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_irq),  MP_ROM_PTR(&mpy_binding_machine_enable_irq_obj) },
  };
  static MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

  // Declared extern (matching genhdr/moduledefs.h) so the const definition keeps
  // external linkage and isn't discarded as an unreferenced internal symbol.
  extern const mp_obj_module_t machine_module;
  const mp_obj_module_t machine_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&machine_module_globals,
  };

  MP_REGISTER_MODULE(MP_QSTR_machine, machine_module);
}
