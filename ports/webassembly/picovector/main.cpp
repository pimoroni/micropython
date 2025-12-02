#include "drivers/st7701/st7701.hpp"

#include "ff.h"

#include "pico/bootrom.h"

#include "hardware/structs/xip_ctrl.h"

#include "image.hpp"
#include "png.hpp"
#include "span.hpp"
#include "psram.hpp"

//#include "picovector.hpp"


#include <algorithm>

//using namespace std;
using namespace pimoroni;
using namespace picovector;
using namespace picovector::brushes;
using namespace picovector::shapes;

FATFS fs;

#define FRAME_WIDTH 240
#define FRAME_HEIGHT 240

static const uint USER_SWITCH = 46;
static const uint BACKLIGHT = 45;
static const uint LCD_CLK = 26;
static const uint LCD_CS = 28;
static const uint LCD_DAT = 27;
static const uint LCD_DC = -1;
static const uint LCD_D0 = 1;

uint16_t back_buffer[FRAME_WIDTH * FRAME_HEIGHT];
ST7701* presto;
uint32_t fb_data[240 * 240];

int time_ms();
int time_us();
void alloc_debug();
void process_uart_rx();
void convert_framebuffer(image img, uint16_t *p);

//typedef void (*render_span_callback)(uint32_t *dst, int32_t w, uint32_t c);


void __not_in_flash_func(span_grid)(image *target, shape *shape, render_span *spans, int count) { 
  uint32_t white = 0xffaabbcc;
  uint32_t black = 0xff112233;

  int xo = sin(time_ms() / 1000.0f) * 10.0f;
  int yo = cos(time_ms() / 1000.0f) * 10.0f;

  render_span *span = spans;
  while(count--) {
    int x = span->x;
    int y = span->y;
    int w = span->w;

    uint32_t *dst = target->ptr(x, y);

    while(w--) {
      bool w = (((x + xo) >> 3) + ((y + yo) >> 3)) & 0b1;
      *dst++ = w ? white : black;
      x++;
    }
    span++;
  }
}

int main() {
    set_sys_clock_khz(250000, true);
    stdio_init_all();
    stdio_usb_init();

    sleep_ms(2000);

    sfe_setup_psram(47);

    gpio_init(LCD_CS);
    gpio_put(LCD_CS, 1);
    gpio_set_dir(LCD_CS, 1);

    gpio_init(USER_SWITCH);
    gpio_set_dir(USER_SWITCH, 0);

    // mount the micro sd card
    FRESULT fr = f_mount(&fs, "", 1);
    assert(fr == FR_OK);

    // setup presto
    presto = new ST7701(FRAME_WIDTH, FRAME_HEIGHT, ROTATE_0, SPIPins{spi1, LCD_CS, LCD_CLK, LCD_DAT, PIN_UNUSED, LCD_DC, BACKLIGHT}, back_buffer);
    presto->init();

    // load our assets
    image *dino_big = png::load_image("/dino4.png");
    image *dino_small = png::load_image("/dino2.png");
    image dw = dino_small->window(rect(260, 0, 214, 157));

    // we need a way to set this up, but if the user wants to use the
    // interpolators in their own code they might modify the configuration..
    // do we have to do this everytime we're about to render something to be
    // sure or do we just say that this interpolator is out of bounds if you're
    // using pico graphics 2?
    interp_config cfg = interp_default_config();
    interp_config_set_blend(&cfg, true);
    interp_set_config(interp0, 0, &cfg);
    cfg = interp_default_config();
    interp_set_config(interp0, 1, &cfg);

    // why do we need to offset the framebuffer by a few bytes? it makes no sense
    // but without it we see a couple of top left corner pixels being 
    // corrupted and the device crashes when you attempt to write to those
    // pixels
        
    image fb(fb_data, 320, 240);

    std::vector<shape> shapes(25);

    //shapes[19] = poly::rounded_rectangle(-0.75f, -0.75f, 0.75f, 0.75f, 0.1f, 0.2f, 0.3f, 0.4f);

    float j = 0;
    while (true) {
      j = j + 1.0f;
      span_pixels_drawn = 0;

      // if we receive "STOP" via the USB serial we'll put ourselves into dfu
      process_uart_rx();

      // stop PSRAM transactions when resetting or it won't wake up properly..
      if(gpio_get(USER_SWITCH) == 0) {sleep_ms(1000);}
      
      // clear the framebuffer
      fb.clear(fb.pen(20, 30, 40));
      
      // start rendering with pico graphics 2 features
      int start = time_us();

      //uint32_t white = fb.pen(255, 255, 255, 200);

      float stroke = sin(j / 50.0f) * 0.1f + 0.15f;

      colour white = colour();
      brightness lighten = brightness();
      lighten.amount = 30;
      white.col = fb.pen(255, 255, 255, 100);
      shapes[0] = circle(0.0f, 0.0f, 1.0f);      
      shapes[0].style = &white;
      shapes[1] = circle(0.0f, 0.0f, 1.0f);      
      shapes[1].style = &lighten;
      shapes[1].stroke(-stroke);

      shapes[2] = regular_polygon(0.0f, 0.0f, 3, 1.0f);
      shapes[2].style = &white;
      shapes[3] = regular_polygon(0.0f, 0.0f, 3, 1.0f);
      shapes[3].style = &white;
      shapes[3].stroke(-stroke);

      shapes[4] = regular_polygon(0.0f, 0.0f, 4, 1.0f);
      shapes[4].style = &white;
      shapes[5] = regular_polygon(0.0f, 0.0f, 4, 1.0f);
      shapes[5].style = &white;
      shapes[5].stroke(-stroke);

      shapes[6] = regular_polygon(0.0f, 0.0f, 5, 1.0f);
      shapes[6].style = &white;
      shapes[7] = regular_polygon(0.0f, 0.0f, 5, 1.0f);
      shapes[7].style = &white;
      shapes[7].stroke(-stroke);

      shapes[8] = regular_polygon(0.0f, 0.0f, 6, 1.0f);
      shapes[8].style = &white;
      shapes[9] = regular_polygon(0.0f, 0.0f, 6, 1.0f);
      shapes[9].style = &white;
      shapes[9].stroke(-stroke);

      shapes[10] = star(0.0f, 0.0f, 9.0f, 1.0f, 0.5f);      
      shapes[10].style = &white;
      shapes[11] = star(0.0f, 0.0f, 9.0f, 1.0f, 0.5f);      
      shapes[11].style = &white;
      shapes[11].stroke(-stroke);

      shapes[12] = line(-0.5f, -0.5f, 0.5f, 0.5f);      
      shapes[12].style = &white;
      shapes[12].stroke(-stroke);

      shapes[13] = squircle(0.0f, 0.0f, 0.75f, 4.0f);
      shapes[13].style = &white;
      shapes[14] = squircle(0.0f, 0.0f, 0.75f, 4.0f);
      shapes[14].style = &white;
      shapes[14].stroke(-stroke);

      shapes[15] = pie(0.0f, 0.0f, 0.0f, 180.0f + (sin(j / 25.0f) * 75.0f), 1.0f);
      shapes[15].style = &white;
      shapes[16] = pie(0.0f, 0.0f, 0.0f, 180.0f + (sin(j / 25.0f) * 75.0f), 1.0f);
      shapes[16].style = &white;
      shapes[16].stroke(-stroke);

      shapes[17] = rectangle(-0.75f, -0.75f, 0.75f, 0.75f);
      shapes[17].style = &white;
      shapes[18] = rectangle(-0.75f, -0.75f, 0.75f, 0.75f);
      shapes[18].style = &white;
      shapes[18].stroke(-stroke);


      for(int i = 0; i < shapes.size(); i++) {
        shape shape = shapes[i];

        int x = (i % 5) * 48 + 24;
        int y = (i / 5) * 48 + 24;

        mat3 transform;
        transform.translate(x, y).rotate(j / 5.0f).scale(20);
        shape.transform = transform;
        shape.draw(fb);
      }

      // draw some nice alpha blended circles
      // for(int i = 0; i < 100; i++) {
      //   rect r(0, 0, 50, 50);
      //   r.offset(rand() % 240, rand() % 240);          
      //   uint32_t c = fb.pen(rand() % 255, rand() % 255, rand() % 255, 100);
      //   fb.circle(r.tl(), 25, c);
      // }


      // float rr = sin(j / 25.0f) * 100.0f + 160.0f;
      // mat3 transform;
      // transform.translate(120, 120).rotate(j).scale(rr);
      // poly s = poly::star(poly_point(0.0f, 0.0f), 9.0f, 1.0f, 0.5f, 0.2f);
      // s.draw(fb, fb.pen(255, 0, 0), transform);

      // draw some nice alpha blended stars doing whizzy things
      // poly star_poly = poly::star(poly_point(0.0f, 0.0f), 9.0f, 1.0f, 0.5f);
      // for(int i = 0; i < 50; i++) {
      //   float scale = i * 1.5f;
      //   float x = sin(i / 5.0f + j / 30.0f) * scale * 1.5f;
      //   float y = cos(i / 7.0f + j / 20.0f) * scale * 1.5f;
      //   float ro = 50.0f * (i / 50.0f) + 5.0f;

      //   mat3 transform;
      //   transform.translate(120 + x, 120 + y).scale(ro);
        
      //   uint32_t c = fb.pen(255 - i, 225 - i, 200 - i, 150 - i);
      
        
      //   star_poly.draw(fb, c, transform);

      //   fb.circle(point(120 + x, 120 + y), 5, fb.pen(255, 255, 255));
      // }


      // draw a zoomy scaling dinosaur
      float fi = sin(j / 50.0f) * 50.0f + 100.0f;
      rect dinorect = rect(120 - fi, 120 - fi, fi * 2, fi * 2);
      dino_small->blit(fb, dinorect);

      

      // write some performance statistics
      int end = time_us();
      float pixels_per_ms = float(span_pixels_drawn) * (1000000.0f / float(end - start)) / 1000000;
      int fps = 1000000 / (end - start);
      printf("%dfps [%d pixels / %.3fmps]\n", fps, span_pixels_drawn, pixels_per_ms);

      // can wait for vsync to avoid tearing
      //presto->wait_for_vsync();
      
      // convert our framebuffer into the presto back buffer format
      convert_framebuffer(fb, back_buffer);

    }
}



int time_ms() {
  absolute_time_t t = get_absolute_time();
  return to_ms_since_boot(t);
}

int time_us() {
  absolute_time_t t = get_absolute_time();
  return to_us_since_boot(t);
}

void process_uart_rx() {
  static std::string uart_buffer;
  int c = getchar_timeout_us(0);
  if (c == PICO_ERROR_TIMEOUT) {return;}
  uart_buffer += c;
  if(uart_buffer == "STOP") {
    printf("resetting into DFU mode");
    sleep_ms(1000); // let monitoring terminal disconnect
    reset_usb_boot(0, 0);
  }
}

void convert_framebuffer(image img, uint16_t *p) {
  for(int i = 0; i < img.b.w * img.b.h; i++) {
    uint8_t *pc = (uint8_t *)&img.p[i];
    uint8_t r = pc[1] >> 3;
    uint8_t g = pc[2] >> 2;
    uint8_t b = pc[3] >> 3;
    uint16_t c565 = __builtin_bswap16((r << 11) | (g << 5) | b);
    *p++ = c565;
  }
}

void alloc_debug() {
  for(int i = 0; i < 1000; i++) {
    uint8_t *p = (uint8_t *)malloc(1024);
    printf("%x allocated %d", p, i);
  }
}