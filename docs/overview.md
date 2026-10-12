# Access Parking Permitter overview

**What:** ESPHome parking permit kiosk: Wiegand fob reader, an RFID access
list synced over the native API, and a touchscreen that requests permits
from the Member Zone webhook.
**Kind:** app (no published firmware: the kiosk needs site secrets).
**Hardware:** ESP32 Access Control Base Board 0.1.0 (ESP32-S3-WROOM-1
N16R8), Wiegand reader, 3.5" 320x480 ILI9488 + XPT2046 SPI module. No I2C.

## Files

- `components/access_list/`: C++ component; list in NVS, SHA-256 version,
  atomic replace via begin/add/commit.
- `packages/access-list.yaml`: component + `acl_info/begin/add/commit` API
  actions.
- `packages/parking-kiosk.yaml`: reader, lookup, beeps, scripts, webhook.
- `packages/parking-kiosk-display.yaml`: LVGL UI; implements the
  `parking_kiosk_ui_*` scripts.
- `packages/status-led.yaml`: status LED.
- `examples/parking-permitter.yaml`: the kiosk; all pins.
- `examples/esp32-access-list.yaml`, `esp32-c3-access-list.yaml`: access
  list only, to prove it builds on Xtensa and RISC-V.
- `examples/common/base.yaml`: shared project, network and recovery config.

## Required by packages

- `access-list.yaml`: the `access_list` external component, encrypted `api:`.
- `parking-kiosk.yaml`: `access-list.yaml`; a UI providing
  `parking_kiosk_ui_idle`, `parking_kiosk_ui_member`,
  `parking_kiosk_ui_message(title, body)`; secrets `permit_webhook_url`,
  `permit_webhook_token`; pin substitutions `parking_kiosk_d0_pin`,
  `_d1_pin`, `_reader_led_pin`, `_reader_beep_pin`.
- `parking-kiosk-display.yaml`: `parking-kiosk.yaml`; display, touch and
  backlight pin substitutions.
- `status-led.yaml`: `status_led_pin`.

## Conventions

- Substitutions prefixed with package name; all have defaults unless listed
  above. Kiosk packages have fixed `parking_kiosk_` ids: include once.
- The `acl_*` actions are a protocol shared with the off-device uploader.
- Rules: https://github.com/romkey/esphome-ai-coding-rules/blob/main/RULES.md
