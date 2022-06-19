/*
Copyright (C) Yudoc Kim <craven@crowz.kr>
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include <string.h>
#include <dirent.h> 
#include <fstream>
#include <filesystem>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "ota.hpp"
#include "fs_manager.hpp"

#define HASH_LEN 32 /* SHA-256 digest length */

static const char *TAG = "ota";

#if CONFIG_M5STACK_CORE
const char* Ota::cBinFileName = "m5stack.bin";
#elif CONFIG_TTGO_T1
const char* Ota::cBinFileName = "ttgot1.bin";
#elif CONFIG_WIFI_DEBUGGER_V_0_1
const char* Ota::cBinFileName = "wifiDebuggerV1.bin";
#elif CONFIG_WIFI_DEBUGGER_V_0_2
const char* Ota::cBinFileName = "wifiDebuggerV2.bin";
#endif

const char* Ota::cBinFileDir = "/sdcard/firmware";

void Ota::printSha256 (const uint8_t *image_hash, const char *label) const
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

Ota& Ota::create()
{
    static Ota ota;
    return ota;
}

std::string Ota::getBinFilePath()
{
    return std::string(cBinFileDir) + std::string("/") + std::string(cBinFileName);
}

Ota::Ota()
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    printSha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    printSha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    printSha256(sha_256, "SHA-256 for current firmware: ");

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update_partition = NULL;
    const esp_partition_t *configured = esp_ota_get_boot_partition();

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            // run diagnostic function ...
        }
    }

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                    configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
                running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
                update_partition->subtype, update_partition->address);

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }
}

bool Ota::firmwareBinCheck(uint8_t* binHeader) const
{
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) 
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    esp_app_desc_t new_app_info;
    // check current version with downloading
    memcpy(&new_app_info, &binHeader[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);
    ESP_LOGI(TAG, "New firmware version");

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) 
    {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
    esp_app_desc_t invalid_app_info;
    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) 
    {
        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
    }

    // check current version with last invalid partition
    if (last_invalid_app != NULL)
    {
        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) 
        {
            ESP_LOGW(TAG, "New version is the same as invalid version.");
            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
        }
    }
    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) 
    {
        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        return false;
    }
    return true;
}

std::vector<std::string> Ota::searchBins()
{
    std::vector<std::string> bins;
    std::string path(FsManager::create().getMountPoint());
    path += "/firmware";

    struct dirent *dir;
    DIR *d = opendir(path.c_str());
    if(d == nullptr)
    {
        return bins;
    }

    while((dir = readdir(d)) != NULL) 
    {
        bins.push_back(std::string(dir->d_name));
    }
    closedir(d);
    return bins;
}

void Ota::update(const std::string& filePath)
{
    std::vector<uint8_t> buffer;
    buffer.resize(1024);

    ESP_LOGI(TAG, "Starting OTA example");
    FsManager::create().mount();

    //if(not std::filesystem::exists(filePath))
    //{
    //    ESP_LOGE(TAG, "File(%s) not exist", filePath.c_str());
    //    return;
    //}

    std::ifstream bin(filePath, std::ios_base::binary);
    if(not bin)
    {
        ESP_LOGE(TAG, "File(%s) cannot open", filePath.c_str());
        return;
    }

    const uint32_t cHeaderSize = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);
    bin.read(reinterpret_cast<char*>(buffer.data()), cHeaderSize);
    if(not bin or not firmwareBinCheck(buffer.data()))
    {
        ESP_LOGE(TAG, "File(%s) check fail", filePath.c_str());
        return;
    }
    
    const esp_partition_t *update_partition = NULL;
    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        esp_restart();
        return ;
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    bin.seekg(0, std::ios::beg); // back to the start!
    int binary_file_length = 0;
    while (1) 
    {
        int length = buffer.size();
        bin.read(reinterpret_cast<char*>(buffer.data()), length);
        if(not bin)
        {
            length = bin.gcount();
        }
        if(length == 0)
        {
            break;
        }
        err = esp_ota_write( update_handle, (const void *)buffer.data(), length);
        if (err != ESP_OK)
        {
            esp_ota_abort(update_handle);
            break;
        }
        binary_file_length += length;
        ESP_LOGI(TAG, "Written image length %d", binary_file_length);
        if(length < buffer.size())
        {
            break;
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);

    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    remove(filePath.c_str());
    esp_restart();
    return ;
}

