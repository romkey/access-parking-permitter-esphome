# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- `packages/access-list.yaml`: the access list component and its `acl_*`
  sync actions.
- `packages/parking-kiosk.yaml`: Wiegand reader, fob lookup and the Member
  Zone parking permit webhook.
- `packages/parking-kiosk-display.yaml`: the LVGL touchscreen UI.
- `examples/parking-permitter.yaml` for the ESP32 Access Control Base
  Board, plus ESP32 and ESP32-C3 access list examples.
- `examples/parking-permitter-hosyond-4.yaml`: the kiosk on a Hosyond 4"
  ESP32 display (lcdwiki E32R40T, ST7796 + XPT2046 on one SPI bus). The
  Wiegand reader pins are free header pins, not a tested wiring.
- Fallback hotspot, captive portal, safe mode and factory reset buttons,
  password-protected web server, esphome-lib packages.
- Docker, CI, lint, release and Renovate setup per
  [esphome-ai-coding-rules](https://github.com/romkey/esphome-ai-coding-rules).

### Changed

- Restructured from a single device config into packages and examples;
  the component moved to `components/access_list`.
- The `acl_begin` action's `version` variable is now `list_version`
  (ESPHome rejects `version` once the `version` text sensor is loaded).
  The uploader must send `list_version`.
- OTA uses the api encryption key instead of `ota_password`.
- Entity ids are prefixed `parking_kiosk_`; the reader switches and tag
  sensors have entity categories. The board's "LED" switch is now a status
  LED.

### Removed

- The Python sync script, its Dockerfile and docker-compose.yml.
