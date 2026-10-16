# Configuration

## `components/access_list`

RFID access list stored in NVS as `tag<TAB>name<LF>` lines. ESP32 only.

| Option | Default | Meaning |
|--------|---------|---------|
| `id` | (generated) | Id for lambdas |
| `records` | (none) | Sensor: number of records loaded |
| `last_updated` | (none) | Text sensor, `timestamp` device class: when the list was last committed (ISO 8601 UTC) |

The update time is read from the system clock at `acl_commit` and stored in
NVS with the list. It stays unknown if no `time:` source had set the clock
by then, and for a list stored before this was added, until the next upload.

C++ API, for lambdas:

| Method | Meaning |
|--------|---------|
| `lookup(tag, &name)` | `true` and the name if `tag` (e.g. `"99,999"`) is on the list |
| `lookup_wiegand26(value, &name)` | Same, for the wiegand `on_tag` value (`facility << 16 \| card`) |
| `AccessList::wiegand26_tag(value)` | Converts an `on_tag` value to `"facility,card"`; `""` if wider than 24 bits |
| `version()`, `size()` | SHA-256 of the stored list, number of records |
| `updated()` | `time_t` of the last commit, `0` if unknown |
| `begin()`, `add()`, `commit()`, `last_error()` | The upload steps the API actions call |

```yaml
external_components:
  - source: github://romkey/access-parking-permitter-esphome@v0.1.0
    components: [access_list]
```

## `packages/access-list.yaml`

Adds `access_list` and the API actions that sync it.

**Requires:** the `access_list` external component; encrypted `api:`.

| Substitution | Default | Meaning |
|--------------|---------|---------|
| `access_list_id` | `acl` | Component id |
| `access_list_action` | `acl` | Prefix of the action names |
| `access_list_records_name` | `Access List Records` | Record count sensor name |
| `access_list_last_updated_name` | `Access List Updated` | Last updated sensor name |

```yaml
packages:
  access_list: github://romkey/access-parking-permitter-esphome/packages/access-list.yaml@v0.1.0
```

### Syncing protocol

| Action       | Arguments                             | Purpose                                 |
|--------------|---------------------------------------|-----------------------------------------|
| `acl_info`   |                                       | Returns `{"version", "count"}`          |
| `acl_begin`  | `list_version: string`, `count: int`  | Starts an upload                        |
| `acl_add`    | `records: string`                     | Adds a batch of `rfid<TAB>name<LF>` lines |
| `acl_commit` |                                       | Verifies and stores the upload          |

`version` is the SHA-256 (lowercase hex) of the list serialized as sorted
`rfid<TAB>name<LF>` lines; an uploader can skip the sync when it matches.
RFIDs are `facility,card` with no leading zeros (`99,999`). Names must not
contain tabs or newlines. Send `acl_add` in batches (the original uploader
used 50 records per call).

The device keeps records in RAM until `acl_commit`, which checks the record
count and hash and then writes the list to NVS in one atomic blob write. If
an upload fails, or stalls for 60 seconds, the old list stays in place.
Each action replies with success or an error message; stop on the first
failure.

## `packages/parking-kiosk.yaml`

Wiegand reader, fob lookup, reader feedback and the Member Zone parking
permit API.

**Requires:** `packages/access-list.yaml`; a UI that provides the scripts
`parking_kiosk_ui_idle`, `parking_kiosk_ui_member` (shows the global
`parking_kiosk_name`) and `parking_kiosk_ui_message` (`title`, `body`
strings), such as `packages/parking-kiosk-display.yaml`; secrets
`member_zone_url` (base URL, no trailing slash) and `member_zone_token`
(an `mz_…` API token with the `parking.issue_for_members` scope). The UI calls
`parking_kiosk_request_permit` (`permit_mode`: `link` or `blank`),
`parking_kiosk_go_idle` and, on touch, `parking_kiosk_activity`.

| Substitution | Default | Meaning |
|--------------|---------|---------|
| `parking_kiosk_d0_pin` | (required) | Wiegand D0 |
| `parking_kiosk_d1_pin` | (required) | Wiegand D1 |
| `parking_kiosk_reader_led_pin` | (required) | Reader LED wire |
| `parking_kiosk_reader_beep_pin` | (required) | Reader beeper wire |
| `parking_kiosk_reader_inverted` | `true` | LED and beeper wires are active-low |
| `parking_kiosk_access_list_id` | `acl` | `access_list` id |
| `parking_kiosk_choice_timeout` | `30s` | Back to idle with no touch on the member screen |
| `parking_kiosk_message_timeout` | `10s` | Back to idle after a message |
| `parking_kiosk_member_zone_url` | `!secret member_zone_url` | Member Zone base URL |
| `parking_kiosk_member_zone_token` | `!secret member_zone_token` | API token |
| `parking_kiosk_printer_id` | `""` | Printer for blank permits, from `GET /api/v1/parking/printers`; empty uses Member Zone's default |
| `parking_kiosk_http_timeout` | `10s` | HTTP request timeout |
| `parking_kiosk_member_zone_status_name` | `Member Zone Status` | Token check result |
| `parking_kiosk_test_print_name` | `Test Permit (Print)` | Test button name |
| `parking_kiosk_test_no_print_name` | `Test Permit (No Print)` | Test button name |
| `parking_kiosk_test_entity_category` | `config` | Category of the test buttons |
| `parking_kiosk_reader_led_name` | `Reader LED` | Switch name |
| `parking_kiosk_reader_beep_name` | `Reader Beep` | Switch name |
| `parking_kiosk_reader_entity_category` | `config` | Category of both switches |
| `parking_kiosk_last_tag_name` | `Last Tag` | Raw `on_tag` value |
| `parking_kiosk_last_raw_name` | `Last Tag Raw` | Bits as hex |
| `parking_kiosk_last_h10301_name` | `Last Tag H10301` | `facility,card` |
| `parking_kiosk_tag_entity_category` | `diagnostic` | Category of the tag sensors and Member Zone Status |
| `parking_kiosk_fob_input_name` | `Present Fob` | Text entity name |
| `parking_kiosk_fob_input_entity_category` | `config` | Category of the text entity |

Setting the **Present Fob** text entity to `facility,card` (e.g. `99,999`)
handles that fob exactly as if it had been swiped on the reader: the same
lookup, beeps, screens and permit request. Input that isn't `0-255,0-65535` is
logged and ignored. Anyone who can set the entity can request a permit for
any member, so keep the API and web server protected.

### Member Zone API

Both permit buttons `POST <member_zone_url>/api/v1/parking/permit_requests`
with `Authorization: Bearer <member_zone_token>`:

| Button | Body | Result |
|--------|------|--------|
| Email me a link | `{"rfid": "<facility,card>", "mode": "link"}` | Member Zone emails the member a link to create and print their permit |
| Blank permit | `{"rfid": "<facility,card>", "mode": "blank", "print": true}` (or `"printer_id"`) | An active two-week permit, printed blank, and an email with a link to record what is parked and where |

The screen shows the result: the expiry date, that the permit didn't print
(`print.error`; the permit was still issued), or that Member Zone has no
email address for the member. Each error `code` (`invalid_token`,
`insufficient_scope`, `not_found`, `conflict`, `ineligible_member`,
`rate_limited`, …) gets a short message. Requests are never retried, since
a retry issues a second permit. A request with no response says to wait
for the email before trying again.

The **Test Permit (Print)** and **Test Permit (No Print)** buttons try the
API without the touchscreen. Each requests a `blank` permit, with or
without `print`, for the most recently presented fob, whether it was
swiped or entered in **Present Fob** (read from **Last Tag H10301**). The
result shows on the kiosk screen and in the log. They issue a real
permit and email the member, so use a fob you can spare. With no fob
presented yet they show "No fob yet".

Once the network is up, and every 24 hours, `GET /api/v1/me` checks that
the token is valid and has `parking.issue_for_members`. **Member Zone
Status** shows `OK`, `Token rejected`, `Token lacks
parking.issue_for_members`, `Unreachable` or `Error <status>`; a 401 or
403 on a permit request updates it too.

```yaml
substitutions:
  parking_kiosk_d0_pin: GPIO18
  parking_kiosk_d1_pin: GPIO17
  parking_kiosk_reader_led_pin: GPIO16
  parking_kiosk_reader_beep_pin: GPIO15

packages:
  parking_kiosk: github://romkey/access-parking-permitter-esphome/packages/parking-kiosk.yaml@v0.1.0
```

## `packages/parking-kiosk-display.yaml`

320x480 SPI TFT with XPT2046 touch, drawn with LVGL. Display and touch
are on separate SPI buses.

**Requires:** `packages/parking-kiosk.yaml`.

| Substitution | Default | Meaning |
|--------------|---------|---------|
| `parking_kiosk_display_clk_pin`, `_mosi_pin`, `_miso_pin` | (required) | Display SPI bus |
| `parking_kiosk_display_cs_pin`, `_dc_pin`, `_reset_pin` | (required) | Display control |
| `parking_kiosk_backlight_pin` | (required) | Backlight, PWM |
| `parking_kiosk_touch_clk_pin`, `_mosi_pin`, `_miso_pin` | (required) | Touch SPI bus |
| `parking_kiosk_touch_cs_pin`, `_irq_pin` | (required) | Touch control |
| `parking_kiosk_display_model` | `ILI9488` | `mipi_spi` model; `ST7796` for the MSP3526 |
| `parking_kiosk_display_data_rate` | `40MHz` | Display SPI clock |
| `parking_kiosk_touch_x_min`, `_x_max`, `_y_min`, `_y_max` | `280`, `3860`, `340`, `3860` | Touch calibration |
| `parking_kiosk_backlight_name` | `Backlight` | Light name |
| `parking_kiosk_backlight_entity_category` | `config` | Light category |

See `examples/parking-permitter.yaml` for a complete pin set, and for
extending pins with `!extend` (e.g. `ignore_strapping_warning`).

## `packages/status-led.yaml`

Blinks an LED on warnings and errors.

**Requires:** `status_led_pin`.

| Substitution | Default | Meaning |
|--------------|---------|---------|
| `status_led_pin` | (required) | GPIO the LED is on |
| `status_led_inverted` | `false` | `true` for an active-low LED |

```yaml
substitutions:
  status_led_pin: GPIO8

packages:
  status_led: github://romkey/access-parking-permitter-esphome/packages/status-led.yaml@v0.1.0
```
