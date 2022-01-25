#include "sdcard.hpp"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "driver/sdmmc_host.h"
#include <sstream>
#include "status.hpp"


static const char *TAG = "sdcard";
const char* SdCard::cMountPoint = "/sdcard";

SdCard::SdCard() :
    Client(DebugMsgRx::get(), INT32_MAX),
    pSdcard(nullptr),
    pFile(nullptr),
    mInited(false)
{
    Status::get().report(Status::Error::eSdcard, true);
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = cSpiPort;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = cPinMosi,
        .miso_io_num = cPinMiso,
        .sclk_io_num = cPinClk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, host.slot);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = 
    {
        .host_id = (spi_host_device_t)cSpiPort,
        .gpio_cs = (gpio_num_t)cPinCs,
        .gpio_cd = GPIO_NUM_NC,
        .gpio_wp = GPIO_NUM_NC,
        .gpio_int = GPIO_NUM_NC
    };

    ret = esp_vfs_fat_sdspi_mount(cMountPoint, &host, &slot_config, &mount_config, (sdmmc_card_t**)&pSdcard);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    sdmmc_card_print_info(stdout, (sdmmc_card_t*)pSdcard);
    mInited = true;
    Status::get().report(Status::Error::eSdcard, false);
}

SdCard::~SdCard()
{
    if(pFile)
    {
        fclose(pFile);
    }
    esp_vfs_fat_sdcard_unmount(cMountPoint, (sdmmc_card_t*)pSdcard);
    spi_bus_free((spi_host_device_t)cSpiPort);
}

void SdCard::init()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if(not mInited)
    {
        return;
    }
    pFile = createFile();
}

FILE* SdCard::createFile()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    std::time_t t = tv_now.tv_sec;
    tm local = *localtime(&t);
    local.tm_year += 1900;
    local.tm_mon += 1;

    std::ostringstream path;
    path << cMountPoint << "/log";
    struct stat _stat = {};
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }

    path << "/" << local.tm_year;
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }

    path << "/" << local.tm_mon;
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }
    path << "/" << local.tm_mday;
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }
    path << "/" << local.tm_year << "-"  << local.tm_mon << "-" << local.tm_mday << "T" << local.tm_hour << "_" << local.tm_min << "_" << local.tm_sec << ".log";
    ESP_LOGI(TAG, "File Open(%s)", path.str().c_str());
    return fopen(path.str().c_str(), "w");
}

bool SdCard::write(const char* msg, uint32_t length)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if((not mInited) or (pFile == nullptr))
    {
        return true;
    }
    
    fwrite(msg, 1, length, pFile);
    fflush(pFile);
    fsync(fileno(pFile));
    return true;
}

bool SdCard::write(const std::vector<uint8_t>& msg)
{
    return write((char*)msg.data(), msg.size());
}
