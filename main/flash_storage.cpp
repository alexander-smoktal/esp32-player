#include "flash_storage.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"

static constexpr auto TAG = "FlashStorage";

FlashStorage::FlashStorage(const std::string& mount_path, const std::string& partition_label)
    : m_mount_path(mount_path)
    , m_partition_label(partition_label) {

    esp_vfs_fat_mount_config_t cfg = {
        .format_if_mount_failed    = false,
        .max_files                 = 4,
        .allocation_unit_size      = 0,
        .disk_status_check_enable  = false,
        .use_one_fat               = false,
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount_ro(m_mount_path.c_str(),
                                                   m_partition_label.c_str(),
                                                   &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount '%s' at '%s': %s",
                 m_partition_label.c_str(), m_mount_path.c_str(), esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Mounted '%s' at '%s'", m_partition_label.c_str(), m_mount_path.c_str());
    }
}

FlashStorage::~FlashStorage() {
    esp_vfs_fat_spiflash_unmount_ro(m_mount_path.c_str(), m_partition_label.c_str());
    ESP_LOGI(TAG, "Unmounted '%s'", m_partition_label.c_str());
}

std::fstream FlashStorage::open_file(const std::string& filename) {
    std::string path = m_mount_path + "/" + filename;
    ESP_LOGI(TAG, "Opening '%s'", path.c_str());

    std::fstream fs;
    fs.open(path, std::fstream::in | std::fstream::binary);
    if (!fs.is_open()) {
        ESP_LOGE(TAG, "Failed to open '%s'", path.c_str());
    }
    return fs;
}
