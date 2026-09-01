# Upstream PR pack — generic advert button for headless nodes

Target repo: `meshcore-dev/MeshCore` · fork from **`dev`** · branch `feature/headless-advert-button`

Per CONTRIBUTING.md this is a new feature, so open a `[Feature request]` issue first and get a
maintainer thumbs-up before raising the PR. Style rules from that file: 2-space indent, no tabs,
camelCase functions/variables, PascalCase classes, ALL_CAPS defines, follow `.clang-format`
(note the existing codebase writes a space before the paren: `void begin ()`).

---

## 1. New file: `src/helpers/AdvertButton.h`

Header-only, so no `build_src_filter` changes are needed in any variant.

```cpp
#pragma once

#include <Arduino.h>

/**
 * Advert button for headless nodes.
 *
 * Nodes with a display already have a button-driven advert via UITask's advert page.
 * This gives the same capability to nodes built without DISPLAY_CLASS.
 *
 * Opt in from a variant's build_flags:
 *   -D BUTTON_ADVERT=1
 *   -D PIN_USER_BTN=<gpio>
 *
 * Optional overrides:
 *   -D BUTTON_ADVERT_LONG_PRESS_MILLIS=3000   long-press threshold
 *   -D BUTTON_ADVERT_MIN_INTERVAL_MILLIS=10000  airtime guard between adverts
 *   -D BUTTON_ADVERT_REVERSED=true            true = active low (switch to GND)
 *   -D BUTTON_ADVERT_PULL=true                enable the internal pull resistor
 */
#if defined (BUTTON_ADVERT) && defined (PIN_USER_BTN) && !defined (DISPLAY_CLASS)
  #define WITH_ADVERT_BUTTON
#endif

#ifdef WITH_ADVERT_BUTTON

#include <helpers/ui/MomentaryButton.h>

#ifndef BUTTON_ADVERT_LONG_PRESS_MILLIS
  #define BUTTON_ADVERT_LONG_PRESS_MILLIS 3000
#endif

#ifndef BUTTON_ADVERT_MIN_INTERVAL_MILLIS
  #define BUTTON_ADVERT_MIN_INTERVAL_MILLIS 10000
#endif

#ifndef BUTTON_ADVERT_REVERSED
  #define BUTTON_ADVERT_REVERSED true
#endif

#ifndef BUTTON_ADVERT_PULL
  #define BUTTON_ADVERT_PULL true
#endif

#define ADVERT_BUTTON_NONE      0
#define ADVERT_BUTTON_ZERO_HOP  1   // short click
#define ADVERT_BUTTON_FLOOD     2   // long press

class AdvertButton {
  MomentaryButton _btn;
  unsigned long _next_read;
  unsigned long _blocked_until;

public:
  AdvertButton ()
    : _btn (PIN_USER_BTN, BUTTON_ADVERT_LONG_PRESS_MILLIS, BUTTON_ADVERT_REVERSED, BUTTON_ADVERT_PULL),
      _next_read (0), _blocked_until (0) { }

  void begin () { _btn.begin (); }

  // Poll from the main loop. Returns one of the ADVERT_BUTTON_* codes.
  uint8_t check () {
    if (millis () < _next_read) return ADVERT_BUTTON_NONE;
    _next_read = millis () + 20;   // 50 Hz is ample for debounce

    uint8_t ev = _btn.check ();
    if (ev != BUTTON_EVENT_CLICK && ev != BUTTON_EVENT_LONG_PRESS) return ADVERT_BUTTON_NONE;

    // airtime guard: ignore repeat presses inside the cool-off window
    if (millis () < _blocked_until) return ADVERT_BUTTON_NONE;
    _blocked_until = millis () + BUTTON_ADVERT_MIN_INTERVAL_MILLIS;

    return (ev == BUTTON_EVENT_LONG_PRESS) ? ADVERT_BUTTON_FLOOD : ADVERT_BUTTON_ZERO_HOP;
  }
};

#endif
```

---

## 2. `examples/simple_repeater/main.cpp`

Near the top, beside the existing SenseCAP Solar button block:

```cpp
#include <helpers/AdvertButton.h>

#if defined (WITH_ADVERT_BUTTON) && defined (_SEEED_SENSECAP_SOLAR_H_)
  #error "BUTTON_ADVERT conflicts with the SenseCAP Solar power-off button on PIN_USER_BTN"
#endif

#ifdef WITH_ADVERT_BUTTON
static AdvertButton advert_btn;
#endif
```

In `setup ()`, after `board.begin ();`:

```cpp
#ifdef WITH_ADVERT_BUTTON
  advert_btn.begin ();
#endif
```

In `loop ()`, immediately before `the_mesh.loop ();`:

```cpp
#ifdef WITH_ADVERT_BUTTON
  switch (advert_btn.check ()) {
    case ADVERT_BUTTON_ZERO_HOP:
      MESH_DEBUG_PRINTLN ("Button: sending zero-hop advert");
      the_mesh.sendSelfAdvertisement (1000, false);
      break;
    case ADVERT_BUTTON_FLOOD:
      MESH_DEBUG_PRINTLN ("Button: sending flood advert");
      the_mesh.sendSelfAdvertisement (1000, true);
      break;
  }
#endif
```

`void sendSelfAdvertisement (int delay_millis, bool flood)` is already public on the repeater's
`MyMesh` — it is what the boot advert and the `advert` CLI command use.

---

## 3. `examples/companion_radio/main.cpp`

Top of file, after `#include "MyMesh.h"`:

```cpp
#include <helpers/AdvertButton.h>

#ifdef WITH_ADVERT_BUTTON
static AdvertButton advert_btn;
#endif
```

End of `setup ()`:

```cpp
#ifdef WITH_ADVERT_BUTTON
  advert_btn.begin ();
#endif
```

Top of `loop ()`:

```cpp
void loop () {
#ifdef WITH_ADVERT_BUTTON
  if (advert_btn.check () != ADVERT_BUTTON_NONE) {
    the_mesh.advert ();
  }
#endif
  the_mesh.loop ();
  sensors.loop ();
#ifdef DISPLAY_CLASS
  ui_task.loop ();
#endif
  rtc_clock.tick ();
}
```

`bool advert ()` is already public on the companion `MyMesh` — the same call the UITask advert
page makes on display-equipped boards such as the T-Beam. The companion mesh has no zero-hop /
flood split, so both press types map to the one call.

---

## 4. Docs

Add to the build-flags reference:

| Flag | Purpose | Values |
|---|---|---|
| `BUTTON_ADVERT` | Send an advert from the user button on nodes built without a display | `1` or undefined |
| `BUTTON_ADVERT_LONG_PRESS_MILLIS` | Long-press threshold (flood advert) | default `3000` |
| `BUTTON_ADVERT_MIN_INTERVAL_MILLIS` | Minimum gap between button adverts | default `10000` |
| `BUTTON_ADVERT_REVERSED` | `true` for an active-low button wired to GND | default `true` |
| `BUTTON_ADVERT_PULL` | Enable the MCU's internal pull resistor | default `true` |

Prose to accompany it:

> On boards with a display the user button already reaches the advert page in the on-device menu.
> On a headless build, define `BUTTON_ADVERT=1` together with `PIN_USER_BTN` to get the same
> capability from the button alone: a short press sends a zero-hop advert, holding the button for
> three seconds sends a flood advert. Repeat presses inside a ten-second window are ignored to
> protect airtime. Note that a node with power saving enabled sleeps between packets and will not
> react to the button until it next wakes.

---

## 5. Example variant usage

```ini
    ; headless advert button — momentary switch from GPIO10 to GND
    -D BUTTON_ADVERT=1
    -D PIN_USER_BTN=10
```

---

## 6. PR description (paste into the PR body)

**Title:** Add generic button-triggered advert for headless builds 🤖🤖

(The 🤖🤖 suffix is MeshCore's opt-in marker from CONTRIBUTING.md, indicating the change was drafted with an automated agent; it fast-tracks review. Drop it if you would rather go through the standard queue.)

Closes #<issue number from the feature request>

### What

Adds an opt-in `BUTTON_ADVERT` build flag that lets a node send an advert from its user button
when the firmware is built without `DISPLAY_CLASS`.

- Short press: zero-hop advert (repeater), advert (companion)
- Long press (3 s, configurable): flood advert (repeater)
- Ten-second cool-off between adverts so a stuck or fidgeted button cannot flood the channel

### Why

Boards with a display can already do this — `ui-new/UITask.cpp` calls `the_mesh.advert ()` from the
advert page, which is how the T-Beam v1.2 menu works. That whole path is compiled only under
`DISPLAY_CLASS`, so a headless node (bare ESP32-C3 SuperMini, XIAO, RAK, a screenless repeater in a
box on a hill) has no way to trigger an advert without a serial console or a BLE client. Screenless
builds are common, and "walk up and press the button to re-announce" is a routine field operation.

### How

New header-only helper `src/helpers/AdvertButton.h` wrapping the existing `MomentaryButton`, plus a
small guarded block in the two app main files — deliberately mirroring the existing
`#if defined (PIN_USER_BTN) && defined (_SEEED_SENSECAP_SOLAR_H_)` power-off block in
`simple_repeater/main.cpp` in both placement and style.

### Compatibility

- Opt-in only. Without `-D BUTTON_ADVERT=1` nothing changes for any existing variant, including
  boards that already define `PIN_USER_BTN`.
- Excluded automatically when `DISPLAY_CLASS` is defined, so display builds keep the menu behaviour
  and the button is never handled twice.
- `#error` guard prevents combining it with the SenseCAP Solar power-off button, which claims the
  same pin.
- No new source files enter any `build_src_filter`; the helper is header-only.

### Testing (1.9.2026 TBD)

~~- Bench-tested on an ESP32-C3 SuperMini + SX1278 headless repeater build, button on GPIO 10 to GND~~
~~- Verified short press produces a zero-hop advert and a 3 s hold produces a flood advert, both seen
  by a neighbouring node~~
~~- Verified the cool-off suppresses repeat presses~~
~~- Verified an unchanged build (flag absent) is byte-identical in behaviour~~

### Known limitation

With `powersaving_enabled`, the node sleeps in `board.sleep ()` and will not observe a press until
it wakes. Documented rather than solved; wiring the button to a wake-capable pin would be a
separate change.
