#pragma once

#include <fstream>
#include <string>

/* Read-only FATFS partition wrapper.
   Uses esp_vfs_fat_spiflash_mount_ro — no wear levelling needed since we only read.
   The partition image is pre-built by fatfs_create_spiflash_image() at build time.
*/
class FlashStorage {
public:
    FlashStorage(const std::string& mount_path, const std::string& partition_label);
    ~FlashStorage();

    // Open a file from this partition in binary read mode.
    // Returns a closed stream on failure — caller must check is_open().
    std::fstream open_file(const std::string& filename);

private:
    std::string m_mount_path;
    std::string m_partition_label;
};
