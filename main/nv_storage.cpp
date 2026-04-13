#include "nv_storage.h"

#include <cassert>
#include <cstring>
#include <cstddef>

#include "nvs_flash.h"
#include "esp_log.h"

static constexpr auto s_default_namespace = "nvm";
static constexpr size_t s_max_value_length = 64;

static constexpr auto TAG = "NVStorage";

NVStorage::NVStorage() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition erased and re-initialized");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(nvs_open(s_default_namespace, NVS_READWRITE, &m_nvs_handle));
}

NVStorage::~NVStorage() {
    nvs_flash_deinit();
}

void NVStorage::erase() {
    ESP_LOGI(TAG, "Erasing NVS storage");
    ESP_ERROR_CHECK(nvs_erase_all(m_nvs_handle));
    ESP_ERROR_CHECK(nvs_commit(m_nvs_handle));
    ESP_LOGI(TAG, "NVS storage erased");
}

bool NVStorage::has_key(const std::string &key) {
    return nvs_find_key(m_nvs_handle, key.c_str(), nullptr) == ESP_OK;
}

std::string NVStorage::get_string(const std::string &key) {
    size_t length = s_max_value_length;
    char value[s_max_value_length];

    ESP_LOGV(TAG, "Reading value for key: %s", key.c_str());

    auto err = nvs_get_str(m_nvs_handle, key.c_str(), value, &length);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Key not found: %s", key.c_str());
        return {};
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading key %s: %s", key.c_str(), esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }

    ESP_LOGV(TAG, "Read value for key: %s, value: %s", key.c_str(), value);

    // Returned length includes the null terminator, so we need one char less
    return {value, length - 1};
}

void NVStorage::set_string(const std::string &key, const std::string &value) {
    assert(value.length() < s_max_value_length);

    ESP_LOGV(TAG, "Writing value for key: %s, value: %s", key.c_str(), value.c_str());

    ESP_ERROR_CHECK(nvs_set_str(m_nvs_handle, key.c_str(), value.c_str()));
    ESP_ERROR_CHECK(nvs_commit(m_nvs_handle));

    ESP_LOGV(TAG, "Value for key: %s has been written", key.c_str());
}

int32_t NVStorage::get_int(const std::string &key) {
    int32_t value = 0;
    ESP_LOGV(TAG, "Reading integer value for key: %s", key.c_str());

    auto err = nvs_get_i32(m_nvs_handle, key.c_str(), &value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Key not found: %s", key.c_str());
        return 0;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading key %s: %s", key.c_str(), esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }

    ESP_LOGV(TAG, "Read integer value for key: %s, value: %ld", key.c_str(), value);
    return value;
}

void NVStorage::set_int(const std::string &key, int32_t value) {
    ESP_LOGV(TAG, "Writing integer value for key: %s, value: %ld", key.c_str(), value);

    ESP_ERROR_CHECK(nvs_set_i32(m_nvs_handle, key.c_str(), value));
    ESP_ERROR_CHECK(nvs_commit(m_nvs_handle));

    ESP_LOGV(TAG, "Integer value for key: %s has been written", key.c_str());
}


std::vector<uint8_t> NVStorage::get_binary(const std::string &key) {
    // Get value size first
    size_t size = 0;
    auto err = nvs_get_blob(m_nvs_handle, key.c_str(), nullptr, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Key not found: %s", key.c_str());
        return {};
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading key %s: %s", key.c_str(), esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }

    auto value = std::vector<uint8_t>(size);

    ESP_LOGV(TAG, "Reading binary data for key: %s", key.c_str());

    err = nvs_get_blob(m_nvs_handle, key.c_str(), value.data(), &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Key not found: %s", key.c_str());
        return {};
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading key %s: %s", key.c_str(), esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }

    value.resize(size);
    ESP_LOGV(TAG, "Read binary data for key: %s, size: %zu", key.c_str(), size);

    return value;
}

void NVStorage::set_binary(const std::string &key, const void *data, size_t size) {
    assert(size <= s_max_value_length);

    ESP_LOGV(TAG, "Writing binary data for key: %s, size: %zu", key.c_str(), size);

    ESP_ERROR_CHECK(nvs_set_blob(m_nvs_handle, key.c_str(), data, size));
    ESP_ERROR_CHECK(nvs_commit(m_nvs_handle));

    ESP_LOGV(TAG, "Binary data for key: %s has been written", key.c_str());
}

void NVStorage::clear_entry(const std::string &key) {
    ESP_LOGV(TAG, "Clearing entry for key: %s", key.c_str());

    ESP_ERROR_CHECK(nvs_erase_key(m_nvs_handle, key.c_str()));
    ESP_ERROR_CHECK(nvs_commit(m_nvs_handle));

    ESP_LOGV(TAG, "Entry for key: %s has been cleared", key.c_str());
}
