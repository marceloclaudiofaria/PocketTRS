#include "fabgl.h"
#include "trs.h"
#include "trs-keyboard.h"
#include "trs_screen.h"
#include "i2s.h"
#include "cassette.h"
#include "io.h"
#include "ui.h"
#include "settings.h"
#include "config.h"

#include "led.h"
#include "wifi.h"
#include "ota.h"
#include "storage.h"
#include "event.h"
#include "freertos/task.h"

#include "trs-io.h"
#include "ntp_sync.h"


fabgl::VGA2Controller DisplayController;
fabgl::Canvas         Canvas(&DisplayController);
fabgl::PS2Controller  PS2Controller;

#include "splash"

static void show_splash()
{
  ScreenBuffer* screenBuffer = new ScreenBuffer(MODE_TEXT_64x16);
  memcpy(screenBuffer->getBuffer(), splash, 1024);
  trs_screen.push(screenBuffer);
  trs_screen.refresh();
}

void setup() {
#if 1
  printf("Heap size before VGA init: %d\n", esp_get_free_heap_size());
  printf("DRAM size before VGA init: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

  DisplayController.begin(VGA_RED, VGA_GREEN, VGA_BLUE, VGA_HSYNC, VGA_VSYNC);
  DisplayController.setResolution(VGA_512x192_60Hz);
  DisplayController.enableBackgroundPrimitiveExecution(false);
  DisplayController.enableBackgroundPrimitiveTimeout(false);
  Canvas.setBrushColor(Color::Black);
  Canvas.setGlyphOptions(GlyphOptions().FillBackground(true));
  Canvas.setPenColor(Color::White);

  show_splash();

  init_events();
  init_trs_io();
  init_storage();
  init_i2s();
  init_io();
  init_settings();
  init_wifi();
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  settingsCalibration.setScreenOffset();
  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);

  z80_reset(0);

#if 1
  printf("Heap size after VGA init: %d\n", esp_get_free_heap_size());
  printf("DRAM size after VGA init: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif
}

void loop() {
  static fabgl::VirtualKey lastvk = fabgl::VK_NONE;
  auto keyboard = PS2Controller.keyboard();

  z80_run();
  if (keyboard == nullptr || !keyboard->isKeyboardAvailable()) {
    return;
  }
  if (keyboard->virtualKeyAvailable()) {
    bool down;
    auto vk = keyboard->getNextVirtualKey(&down);
    //printf("VirtualKey = %s\n", keyboard->virtualKeyToString(vk));
    if (down && vk == fabgl::VK_F3 && trs_screen.isTextMode()) {
      configure_pocket_trs();
    } else if (down && vk == fabgl::VK_F9) {
      z80_reset();
    } else {
      process_key(vk, down);
    }
  }
}

extern "C" void app_main()
{
  setup();
  while (true) loop();
}
