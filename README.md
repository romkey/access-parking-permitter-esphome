# Access Parking Permitter

[![CI](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/ci.yaml/badge.svg)](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/ci.yaml)
[![Lint](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/lint.yaml/badge.svg)](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/lint.yaml)
[![ESPHome](https://img.shields.io/badge/ESPHome-packages-18BCF2?logo=esphome&logoColor=white)](https://esphome.io/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

ESPHome firmware for the PDX Hackerspace parking permit kiosk. A member
swipes their RFID fob on a Wiegand reader; if the fob is on the access list
the touchscreen greets them and offers an emailed permit link or a blank
permit, issued through the Member Zone device webhook. The access list is
synced to the device over the ESPHome native API and stored in flash, so
the kiosk recognizes members without a network round trip.

> **AI disclosure:** This project was written by AI under human oversight,
> following [these rules](https://github.com/romkey/esphome-ai-coding-rules/blob/main/RULES.md).

## What's here

| Path | What it is |
|------|------------|
| `components/access_list/` | External component: RFID list stored in NVS, versioned by SHA-256, replaced atomically |
| `packages/access-list.yaml` | The component plus the `acl_*` API actions an uploader calls |
| `packages/parking-kiosk.yaml` | Wiegand reader, fob lookup, reader beeps and the permit webhook |
| `packages/parking-kiosk-display.yaml` | Touchscreen UI (ILI9488 + XPT2046, LVGL) |
| `packages/status-led.yaml` | Status LED that blinks on warnings and errors |
| `examples/parking-permitter.yaml` | The kiosk on the ESP32 Access Control Base Board |
| `examples/esp32-access-list.yaml`, `esp32-c3-access-list.yaml` | The access list alone on ESP32 and ESP32-C3 |
| `docs/configuration.md` | Full configuration reference, including the sync protocol |

## Technical details

### Kiosk flow

- Idle: "Swipe RFID fob for parking permit".
- Known fob: the reader beeps once, and the screen says "Hi, <name>!" with
  **Email me a link**, **Blank permit** and **Cancel** buttons. With no
  touch for 30 seconds, it goes back to idle.
- Unknown fob: three beeps, then an "Unknown fob" message.
- Either permit button POSTs `{"rfid": "<facility,card>", "mode": "link"|"blank"}`
  to the `permit_webhook_url` secret, with `permit_webhook_token` in the
  `X-Device-Token` header. The screen shows the result, such as "Check your
  email" or the blank permit's expiry date to write on it, or the server's
  error. It returns to idle after 10 seconds or when OK is tapped.

Register the kiosk under Member Zone **Settings → Webhook devices** to get
its token. The webhook URL is
`https://<member zone host>/webhooks/devices/parking_permits`.

### Hardware

[ESP32 Access Control Base Board](https://github.com/romkey/esp32-access-control)
0.1.0 (ESP32-S3-WROOM-1 N16R8). Pins come from the board's KiCad PCB:

| Function | GPIO |
|---|---|
| Wiegand 1 D0 / D1 / LED / beep | 18 / 17 / 16 / 15 |
| TFT CS / DC / RST / backlight | 3 / 9 / 46 / 12 |
| TFT SCK / MOSI (SDI) / MISO (SDO) | 11 / 13 / 10 |
| Touch CLK / DIN / DO / CS / IRQ | 45 / 47 / 21 / 48 / 14 |
| Status LED | 8 |

The display header matches the common red 3.5" 320x480 SPI module
(MSP3520), which pairs an **ILI9488** with an **XPT2046** resistive touch
controller. The display and touch use separate SPI buses on this board,
which avoids the ILI9488's habit of not releasing MISO. If the picture is
garbled or the colors are inverted, you may have the ST7796 version of the
module (MSP3526): set `parking_kiosk_display_model: ST7796`. Touch
calibration values are typical defaults. If touches land in the wrong
place, set the logger to `DEBUG`, touch the corners, and copy the raw
values into the `parking_kiosk_touch_*` substitutions.

### Syncing the access list

The device exposes four API actions. An uploader (not part of this repo)
uses them to replace the list:

| Action       | Arguments                             | Purpose                                 |
|--------------|---------------------------------------|-----------------------------------------|
| `acl_info`   |                                       | Returns `{"version", "count"}`          |
| `acl_begin`  | `list_version: string`, `count: int`  | Starts an upload                        |
| `acl_add`    | `records: string`                     | Adds a batch of `rfid<TAB>name<LF>` lines |
| `acl_commit` |                                       | Verifies and stores the upload          |

Details are in [docs/configuration.md](docs/configuration.md#syncing-protocol).

## Using the packages in your own config

```yaml
external_components:
  - source: github://romkey/access-parking-permitter-esphome@v0.1.0
    components: [access_list]

packages:
  access_list: github://romkey/access-parking-permitter-esphome/packages/access-list.yaml@v0.1.0
```

To check a fob in an automation:

```cpp
std::string name;
if (id(acl).lookup("99,999", &name)) { ... }
// wiegand on_tag delivers facility<<16|card as a decimal string:
if (id(acl).lookup_wiegand26(x, &name)) { ... }
```

See [docs/configuration.md](docs/configuration.md) for every option.

## Building

Requires Docker.

```bash
cp examples/secrets.yaml.example examples/secrets.yaml   # then edit it
docker compose run --rm validate
docker compose run --rm build
docker compose run --rm lint
```

Flash the kiosk over the network:

```bash
docker compose run --rm esphome run examples/parking-permitter.yaml
```

Docker Desktop on macOS can't pass USB through: flash a new board the first
time with https://web.esphome.io, and use OTA after that.

The first build downloads the toolchains into a Docker volume; later builds
reuse them and only recompile what changed.

## License

[MIT](LICENSE).
