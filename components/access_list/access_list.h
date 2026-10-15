#pragma once

#include <ctime>
#include <string>
#include <vector>

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace access_list {

// RFID access list stored in NVS as "tag\tname\n" lines. The list's version is
// the SHA-256 of that text, so the uploader can tell whether a sync is needed.
//
// An upload is begin() -> add() ... -> commit(). Records accumulate in RAM and
// only replace the stored list once commit() verifies the count and hash.
// commit() also stores the time from the system clock, if it has been set.
class AccessList : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool begin(const std::string &version, int32_t count);
  // Add a batch of "tag\tname\n" lines.
  bool add(const std::string &records);
  bool commit();

  // Empty after a successful begin/add/commit, otherwise why it failed.
  const std::string &last_error() const { return this->last_error_; }

  // Look up a tag. Returns true and sets *name (if given) when the tag is on the list.
  bool lookup(const std::string &tag, std::string *name = nullptr) const;
  // Convenience for the wiegand component's on_tag value (facility << 16 | card),
  // which is matched against "facility,card" entries.
  bool lookup_wiegand26(const std::string &value, std::string *name = nullptr) const;
  // Convert an on_tag value to "facility,card". Returns "" for values wider than 24 bits.
  static std::string wiegand26_tag(const std::string &value);

  const std::string &version() const { return this->version_; }
  size_t size() const { return this->index_.size(); }
  // When the stored list was committed, or 0 if unknown (the clock wasn't set,
  // or the list predates this field).
  time_t updated() const { return this->updated_; }

  void set_records_sensor(sensor::Sensor *sensor) { this->records_sensor_ = sensor; }
  void set_last_updated_sensor(text_sensor::TextSensor *sensor) { this->last_updated_sensor_ = sensor; }

 protected:
  bool fail_(const char *message);
  void abort_();
  void load_(std::string &&records);
  static std::string hash_(const std::string &records);
  void publish_();

  // Live list: the raw text plus offsets of each line, sorted by tag.
  std::string records_;
  std::vector<uint32_t> index_;
  std::string version_;
  time_t updated_{0};

  sensor::Sensor *records_sensor_{nullptr};
  text_sensor::TextSensor *last_updated_sensor_{nullptr};

  // Upload in progress.
  bool in_progress_{false};
  std::string pending_;
  std::string pending_version_;
  int32_t pending_expected_{0};
  int32_t pending_received_{0};
  uint32_t last_activity_{0};

  std::string last_error_;
};

}  // namespace access_list
}  // namespace esphome
