# Instructions for AI assistants

Follow https://github.com/romkey/esphome-ai-coding-rules/blob/main/RULES.md.
With Claude Code, the `esphome-rules` plugin loads them for you.

Read docs/overview.md first. `main` is protected: work on a branch and
open a pull request; CI and Lint must pass before it can merge.

<!-- Project-specific notes go below. -->

- This is an application: the parking permit kiosk in
  `examples/parking-permitter.yaml`. The other two examples only prove
  `packages/access-list.yaml` and `components/access_list` build on ESP32
  and ESP32-C3.
- The `acl_*` API actions, their variable names and the record format are
  a protocol shared with the off-device uploader. Changing them is a
  breaking change; document it in CHANGELOG.md.
- `parking-kiosk.yaml` talks to its UI only through the
  `parking_kiosk_ui_*` scripts; keep LVGL out of it.
