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


static const char *TAG = "sdcard";

SdCard::SdCard(const char* mountPoint) :
    cMountPoint(mountPoint),
    pSdcard(nullptr),
    mInit(false)
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = true,
        .use_one_fat = false,
    };
    ESP_LOGI(TAG, "Initializing SD card");

#ifdef CONFIG_SD_SPI
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
    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
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
#elif defined(CONFIG_SD_SDIO_4BIT) || defined(CONFIG_SD_SDIO_1BIT)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#if defined(CONFIG_SD_SDIO_4BIT)
    slot_config.width = 4;
#else
    slot_config.width = 1;
#endif
    slot_config.clk = (gpio_num_t)cClk;
    slot_config.cmd = (gpio_num_t)cCmd;
    slot_config.d0 = (gpio_num_t)cD0;
    slot_config.d1 =(gpio_num_t)cD1;
#if defined(CONFIG_SD_SDIO_4BIT)
    slot_config.d2 = (gpio_num_t)cD2;
    slot_config.d3 = (gpio_num_t)cD3;
    slot_config.cd = (gpio_num_t)cCd;
#endif
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ret = esp_vfs_fat_sdmmc_mount(cMountPoint, &host, &slot_config, &mount_config, (sdmmc_card_t**)&pSdcard);
#else
    static_assert(0);
#endif

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
    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, (sdmmc_card_t*)pSdcard);
    mInit = true;
}

SdCard::~SdCard()
{
    esp_vfs_fat_sdcard_unmount(cMountPoint, (sdmmc_card_t*)pSdcard);
#ifdef CONFIG_SD_SPI
    spi_bus_free((spi_host_device_t)cSpiPort);
#endif // CONFIG_SD_SPI

#ifdef CONFIG_SD_SDIO

#endif //CONFIG_SD_SDIO
}

bool SdCard::isInit() const
{
    return mInit;
}