#include "access_list.h"

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <cstring>

#include <nvs.h>

#include "esphome/components/sha256/sha256.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace access_list {

static const char *const TAG = "access_list";
static const char *const NVS_NAMESPACE = "access_list";
static const char *const NVS_KEY = "records";
// Drop an abandoned upload after this long without a begin/add call.
static const uint32_t UPLOAD_TIMEOUT_MS = 60000;

// Compare the tag at the start of a "tag\tname\n" line with a tag string.
static int compare_tag(const char *line, const char *tag, size_t tag_len) {
  const char *end = strchr(line, '\t');
  size_t line_len = end - line;
  int c = memcmp(line, tag, std::min(line_len, tag_len));
  if (c != 0)
    return c;
  return line_len < tag_len ? -1 : (line_len > tag_len ? 1 : 0);
}

void AccessList::setup() {
  nvs_handle_t handle;
  if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
    ESP_LOGI(TAG, "No stored access list");
    this->load_(std::string());
    return;
  }

  size_t len = 0;
  std::string records;
  esp_err_t err = nvs_get_blob(handle, NVS_KEY, nullptr, &len);
  if (err == ESP_OK && len > 0) {
    records.resize(len);
    err = nvs_get_blob(handle, NVS_KEY, &records[0], &len);
  }
  nvs_close(handle);

  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGE(TAG, "Failed to read access list: %s", esp_err_to_name(err));
    records.clear();
  }
  this->load_(std::move(records));
}

void AccessList::loop() {
  if (this->in_progress_ && millis() - this->last_activity_ > UPLOAD_TIMEOUT_MS) {
    ESP_LOGW(TAG, "Upload timed out after %" PRId32 " of %" PRId32 " records", this->pending_received_,
             this->pending_expected_);
    this->abort_();
  }
}

void AccessList::dump_config() {
  ESP_LOGCONFIG(TAG, "Access list:\n  Records: %u\n  Version: %s", (unsigned) this->size(), this->version_.c_str());
}

bool AccessList::begin(const std::string &version, int32_t count) {
  this->abort_();
  if (count < 0)
    return this->fail_("count must not be negative");
  this->in_progress_ = true;
  this->pending_version_ = version;
  this->pending_expected_ = count;
  this->pending_.reserve(count * 40);
  this->last_activity_ = millis();
  this->last_error_.clear();
  ESP_LOGI(TAG, "Receiving %" PRId32 " records", count);
  return true;
}

bool AccessList::add(const std::string &records) {
  if (!this->in_progress_)
    return this->fail_("no upload in progress");
  if (!records.empty() && records.back() != '\n')
    return this->fail_("records must end with a newline");

  for (size_t pos = 0; pos < records.size();) {
    size_t end = records.find('\n', pos);
    size_t tab = records.find('\t', pos);
    if (tab == std::string::npos || tab >= end)
      return this->fail_("record is missing a tab");
    if (tab == pos)
      return this->fail_("empty tag");
    if (records.find('\t', tab + 1) < end)
      return this->fail_("record has more than one tab");
    if (this->pending_received_ >= this->pending_expected_)
      return this->fail_("more records than announced");
    this->pending_received_++;
    pos = end + 1;
  }

  this->pending_ += records;
  this->last_activity_ = millis();
  this->last_error_.clear();
  return true;
}

bool AccessList::commit() {
  if (!this->in_progress_)
    return this->fail_("no upload in progress");
  if (this->pending_received_ != this->pending_expected_) {
    char buf[64];
    snprintf(buf, sizeof(buf), "received %" PRId32 " of %" PRId32 " records", this->pending_received_,
             this->pending_expected_);
    return this->fail_(buf);
  }
  if (hash_(this->pending_) != this->pending_version_)
    return this->fail_("checksum mismatch");

  nvs_handle_t handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (err == ESP_OK) {
    // An NVS blob write is atomic: a failure here leaves the old list intact.
    err = nvs_set_blob(handle, NVS_KEY, this->pending_.data(), this->pending_.size());
    if (err == ESP_OK)
      err = nvs_commit(handle);
    nvs_close(handle);
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to store access list: %s", esp_err_to_name(err));
    return this->fail_("failed to write to flash");
  }

  std::string records = std::move(this->pending_);
  this->abort_();
  this->load_(std::move(records));
  this->last_error_.clear();
  return true;
}

bool AccessList::lookup(const std::string &tag, std::string *name) const {
  const char *base = this->records_.c_str();
  auto it = std::lower_bound(this->index_.begin(), this->index_.end(), tag, [&](uint32_t offset, const std::string &t) {
    return compare_tag(base + offset, t.data(), t.size()) < 0;
  });
  if (it == this->index_.end() || compare_tag(base + *it, tag.data(), tag.size()) != 0)
    return false;
  if (name != nullptr) {
    const char *start = strchr(base + *it, '\t') + 1;
    *name = std::string(start, strchr(start, '\n') - start);
  }
  return true;
}

bool AccessList::lookup_wiegand26(const std::string &value, std::string *name) const {
  std::string tag = wiegand26_tag(value);
  return !tag.empty() && this->lookup(tag, name);
}

std::string AccessList::wiegand26_tag(const std::string &value) {
  unsigned long long v = strtoull(value.c_str(), nullptr, 10);
  if (v > 0xffffff)
    return "";
  char tag[16];
  snprintf(tag, sizeof(tag), "%u,%u", (unsigned) (v >> 16), (unsigned) (v & 0xffff));
  return tag;
}

bool AccessList::fail_(const char *message) {
  ESP_LOGW(TAG, "Upload failed: %s", message);
  this->last_error_ = message;
  this->abort_();
  return false;
}

void AccessList::abort_() {
  this->in_progress_ = false;
  this->pending_.clear();
  this->pending_.shrink_to_fit();
  this->pending_version_.clear();
  this->pending_expected_ = 0;
  this->pending_received_ = 0;
}

void AccessList::load_(std::string &&records) {
  // Drop anything after the last newline so every indexed line is complete.
  size_t last_newline = records.rfind('\n');
  records.resize(last_newline == std::string::npos ? 0 : last_newline + 1);

  this->records_ = std::move(records);
  this->index_.clear();
  const char *base = this->records_.c_str();
  for (size_t pos = 0; pos < this->records_.size();) {
    size_t end = this->records_.find('\n', pos);
    const char *tab = static_cast<const char *>(memchr(base + pos, '\t', end - pos));
    if (tab != nullptr && tab != base + pos)
      this->index_.push_back(pos);
    pos = end + 1;
  }
  std::sort(this->index_.begin(), this->index_.end(), [base](uint32_t a, uint32_t b) {
    const char *tab = strchr(base + b, '\t');
    return compare_tag(base + a, base + b, tab - (base + b)) < 0;
  });
  this->version_ = hash_(this->records_);
  ESP_LOGI(TAG, "Loaded %u records", (unsigned) this->index_.size());
}

std::string AccessList::hash_(const std::string &records) {
  sha256::SHA256 sha;
  sha.init();
  sha.add(records);
  sha.calculate();
  char hex[65];
  sha.get_hex(hex);
  return std::string(hex);
}

}  // namespace access_list
}  // namespace esphome
