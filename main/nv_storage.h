#pragma once

#include <string>
#include <vector>
#include <cstddef>

#include "nvs.h"

/* An NVS storage wrapper.
   Used by many components to store non-volatile data
*/
class NVStorage {
public:
    NVStorage();
    ~NVStorage();

    void erase();

    bool has_key(const std::string &key);

    std::string get_string(const std::string &key);
    void set_string(const std::string &key, const std::string &value);

    int32_t get_int(const std::string &key);
    void set_int(const std::string &key, int32_t value);

    std::vector<uint8_t> get_binary(const std::string &key);
    void set_binary(const std::string &key, const void *data, size_t size);

    void clear_entry(const std::string &key);

private:
    nvs_handle_t m_nvs_handle;
};
