#pragma once

#include <Arduino.h>

#ifdef PIN_USER_BTN

#include <helpers/ui/MomentaryButton.h>

#ifndef BTN_ADVERT_LONG_PRESS_MS
  #define BTN_ADVERT_LONG_PRESS_MS 3000
#endif
#ifndef BTN_ADVERT_MIN_INTERVAL_MS
  #define BTN_ADVERT_MIN_INTERVAL_MS 10000   // airtime guard: ignore spamming
#endif

#define BTN_ADVERT_NONE   0
#define BTN_ADVERT_ZEROHOP 1   // short click
#define BTN_ADVERT_FLOOD   2   // long press

// Headless button handler: no display, no UITask required.
class ButtonAdvert {
  MomentaryButton _btn;
  unsigned long _next_read;
  unsigned long _blocked_until;
  bool _started;

public:
  ButtonAdvert()
    : _btn(PIN_USER_BTN, BTN_ADVERT_LONG_PRESS_MS, /*reverse=*/true, /*pulldownup=*/true),
      _next_read(0), _blocked_until(0), _started(false) { }

  void begin() { _btn.begin(); _started = true; }

  // Call from the main loop. Returns BTN_ADVERT_* .
  uint8_t check() {
    if (!_started) begin();                       // safe if setup() hook was missed
    if (millis() < _next_read) return BTN_ADVERT_NONE;
    _next_read = millis() + 20;                   // 50 Hz poll, plenty for debounce

    uint8_t ev = _btn.check();
    if (ev != BUTTON_EVENT_CLICK && ev != BUTTON_EVENT_LONG_PRESS) return BTN_ADVERT_NONE;
    if (millis() < _blocked_until) return BTN_ADVERT_NONE;

    _blocked_until = millis() + BTN_ADVERT_MIN_INTERVAL_MS;
    return (ev == BUTTON_EVENT_LONG_PRESS) ? BTN_ADVERT_FLOOD : BTN_ADVERT_ZEROHOP;
  }
};

#endif
