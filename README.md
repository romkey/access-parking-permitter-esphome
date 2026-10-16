# Access Parking Permitter

[![CI](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/ci.yaml/badge.svg)](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/ci.yaml)
[![Lint](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/lint.yaml/badge.svg)](https://github.com/romkey/access-parking-permitter-esphome/actions/workflows/lint.yaml)
[![ESPHome](https://img.shields.io/badge/ESPHome-packages-18BCF2?logo=esphome&logoColor=white)](https://esphome.io/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

ESPHome firmware for the PDX Hackerspace parking permit kiosk. A member
swipes their RFID fob on a Wiegand reader; if the fob is on the access list
the touchscreen greets them and offers an emailed permit link or a printed
blank permit, issued through the Member Zone API. The access list is
synced to the device over the ESPHome native API and stored in flash, so
the kiosk recognizes members without a network round trip.

> **AI disclosure:** This project was written by AI under human oversight,
> following [these rules](https://github.com/romkey/esphome-ai-coding-rules/blob/main/RULES.md).

## What's here

| Path | What it is |
|------|------------|
| `components/access_list/` | External component: RFID list stored in NVS, versioned by SHA-256, replaced atomically |
| `packages/access-list.yaml` | The component plus the `acl_*` API actions an uploader calls |
| `packages/parking-kiosk.yaml` | Wiegand reader, fob lookup, reader beeps and the Member Zone permit API |
| `packages/parking-kiosk-display.yaml` | Touchscreen UI (ILI9488 + XPT2046, LVGL) |
| `packages/status-led.yaml` | Status LED that blinks on warnings and errors |
| `examples/parking-permitter.yaml` | The kiosk on the ESP32 Access Control Base Board |
| `examples/parking-permitter-hosyond-4.yaml` | The kiosk on a Hosyond 4" ESP32 display (E32R40T) |
| `examples/esp32-access-list.yaml`, `esp32-c3-access-list.yaml` | The access list alone on ESP32 and ESP32-C3 |
| `docs/configuration.md` | Full configuration reference, including the sync protocol |

## Technical details

### Kiosk flow

- Idle: "Swipe RFID fob for parking permit".
- Known fob: the reader beeps once, and the screen says "Hi, <name>!" with
  **Email me a link**, **Blank permit** and **Cancel** buttons. With no
  touch for 30 seconds, it goes back to idle.
- Unknown fob: three beeps, then an "Unknown fob" message.
- To try the kiosk without a fob or reader, type `facility,card` (e.g.
  `99,999`) into the **Present Fob** text entity in Home Assistant or the
  web UI. The kiosk handles it exactly like a swipe.
- **Email me a link** asks Member Zone to email the member a link to
  create and print their permit. **Blank permit** issues an active
  two-week permit, prints it blank on Member Zone's printer, and emails a
  link to record what is parked and where. Both POST to
  `<member_zone_url>/api/v1/parking/permit_requests` with the
  `member_zone_token` secret as a bearer token. The screen shows the
  result, such as "Check your email", the permit's expiry date, a printer
  failure, or a short message for the error. It returns to idle after 10
  seconds or when OK is tapped.

In Member Zone, an admin creates a service account for the kiosk with a
role that grants `parking.issue_for_members`, then creates an API token
for it under **Settings → API tokens** with that scope. The token
(`mz_…`) is shown once: put it in `member_zone_token`, and the site's base
URL (e.g. `https://members.example.org`) in `member_zone_url`. Tokens
expire, 365 days by default; an admin can extend one without changing the
secret. The **Member Zone Status** diagnostic sensor shows whether the
kiosk's token works.

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

### Hosyond 4" ESP32 display

`examples/parking-permitter-hosyond-4.yaml` runs the same kiosk on the
Hosyond 4" ESP32 display, sold by lcdwiki/QDtech as the
[E32R40T](https://www.lcdwiki.com/4.0inch_ESP32-32E_Display): an ESP32-32E
(4 MB flash, no PSRAM) with a 320x480 **ST7796** and an **XPT2046**
resistive touch controller. Pins come from section 4.2 of the
[specification](https://www.lcdwiki.com/res/E32R40T/E32R40T_E32N40T_Specification_V1.0.pdf):

| Function | GPIO |
|---|---|
| LCD CS / DC / backlight | 15 / 2 / 27 (reset is tied to EN) |
| SPI SCK / MOSI / MISO, shared by LCD and touch | 14 / 13 / 12 |
| Touch CS / IRQ | 33 / 36 |
| Status LED (red channel of the RGB LED, active-low) | 22 |
| Wiegand D0 / D1 (input header) | 35 / 39 |
| Wiegand LED / beep (I2C header) | 25 / 32 |

The board has no Wiegand interface, and the reader wiring hasn't been
worked out: the reader pins are just free header pins. IO35 and IO39 are
input only with no pull-ups, and most readers run at 5 V, so the reader
needs level shifting. Change the `parking_kiosk_*` reader substitutions to
match however it gets connected. Until then, use the **Present Fob** text
entity to present fobs.

The example removes the display package's separate touch bus and LCD reset
pin with `!remove`, and shrinks the LVGL buffer to 10% because the board has
no PSRAM. The touch calibration defaults may need adjusting for this panel.

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
