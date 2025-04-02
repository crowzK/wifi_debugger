#include "fs_manager.hpp"
#include "flash_writer.hpp"
#include "flash_algo.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "FlashWriter";
FlashWriter::FlashWriter() :
    Task(__func__)
{
    start();
}

FlashWriter::~FlashWriter()
{

}

void FlashWriter::connectTarget(bool connect)
{
    auto& fsm = FsManager::create();
    if(fsm.mount())
    {
        std::string path = std::string(fsm.getMountPoint()) + std::string("/STM32F4xx_512.FLM");
        
        ESP_LOGI(TAG, "%s crate flash algorithm()", __func__);
        mpFlashAlgorithm = std::make_unique<FlashAlgo>(path, FlashAlgo::RamInfo{.ramStartAddr = 0x20000000, .ramSize = 0x2000});
        bool result = mpFlashAlgorithm->init(0x8000000, 0, 1);
        ESP_LOGI(TAG, "%s init() %s", __func__, result ? "Success" : "Fails");
        result = mpFlashAlgorithm->eraseChip();
        ESP_LOGI(TAG, "%s eraseChip() %s", __func__, result ? "Success" : "Fails");
        ESP_LOGI(TAG, "%s done()", __func__);
    }
}

void FlashWriter::readTargetInfo()
{
    
}

void FlashWriter::targetUpdate(const std::string& binFile, uint32_t startAddr)
{
    
}

void FlashWriter::task()
{
    connectTarget();

    while (mRun)
    {
        vTaskDelay(100);
    }
}