#include "fs_manager.hpp"
#include "flash_writer.hpp"
#include "flash_algo.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
        mpFlashAlgorithm = std::make_unique<FlashAlgo>(path, FlashAlgo::RamInfo{.ramStartAddr = 0x20000000, .ramSize = 0x4000});
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