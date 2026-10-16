# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- `packages/access-list.yaml`: the access list component and its `acl_*`
  sync actions.
- `packages/parking-kiosk.yaml`: Wiegand reader, fob lookup and Member
  Zone parking permits.
- `packages/parking-kiosk-display.yaml`: the LVGL touchscreen UI.
- `examples/parking-permitter.yaml` for the ESP32 Access Control Base
  Board, plus ESP32 and ESP32-C3 access list examples.
- Access List Records and Access List Updated diagnostic sensors in
  `packages/access-list.yaml` (`records` and `last_updated` options on
  `access_list`). The update time is stored in NVS with the list.
- `examples/parking-permitter-hosyond-4.yaml`: the kiosk on a Hosyond 4"
  ESP32 display (lcdwiki E32R40T, ST7796 + XPT2046 on one SPI bus). The
  Wiegand reader pins are free header pins, not a tested wiring.
- Present Fob text entity in `packages/parking-kiosk.yaml`: enter a fob as
  `facility,card` and the kiosk handles it as if it had been swiped.
- Fallback hotspot, captive portal, safe mode and factory reset buttons,
  password-protected web server, esphome-lib packages.
- Docker, CI, lint, release and Renovate setup per
  [esphome-ai-coding-rules](https://github.com/romkey/esphome-ai-coding-rules).

- Member Zone Status diagnostic sensor: the result of checking the token
  with `GET /api/v1/me` at startup and daily.
- `parking_kiosk_printer_id` substitution to print blank permits on a
  specific Member Zone printer.

### Changed

- **Breaking:** permits are requested from the Member Zone API
  (`POST /api/v1/parking/permit_requests`, bearer token) instead of the
  device webhook. The secrets `permit_webhook_url` and
  `permit_webhook_token` are replaced by `member_zone_url` (base URL) and
  `member_zone_token` (an `mz_…` API token with the
  `parking.issue_for_members` scope). The Blank permit button now prints
  the permit on Member Zone's printer.
- **Breaking:** the `parking_kiosk_webhook_timeout` substitution is now
  `parking_kiosk_http_timeout`.
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
