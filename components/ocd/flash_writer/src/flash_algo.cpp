#include "flash_algo.hpp"
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include <fstream>
#include "gpio_swd.hpp"
#include "esp_log.h"


static const char *TAG = "FlashAlgo";

std::array<const char* const, 8> FlashAlgo::cFuncStr
{{
    "Init",
    "UnInit",
    "EraseChip",
    "EraseSector",
    "ProgramPage",
    "Verify",
    "ProgramPages",
    "GetAesKey",
}};

FlashAlgo::FlashAlgo(const std::string &algorithmPath, const RamInfo& targetRam) :
    cLoader(loadLoader(algorithmPath, targetRam))
{
    pSwd = std::make_unique<GpioSwd>();
    if(not pSwd->setStateBySw(Swd::TargetState::eResetProgram))
    {
        return;
    }
    
    ESP_LOGI(TAG, "SWD init success");
    for(const auto& loader: cLoader.loader)
    {
        ESP_LOGI(TAG, "Loader download addr 0x%lX size 0x%X", loader.startAddr, loader.data.size());
        writeSwd(loader.startAddr, loader.data.data(), loader.data.size());
    }
    ESP_LOGI(TAG, "Loader download done");
    pSwd->setStateBySw(Swd::TargetState::eResetProgram);
    ESP_LOGI(TAG, "ResetProgram done");
}

FlashAlgo::~FlashAlgo()
{
}

FlashAlgo::FlashLoaderInfo FlashAlgo::loadLoader(const std::string &algorithmPath, const RamInfo& targetRam)
{
    FlashLoaderInfo lut{};
    ELFIO::elfio elfIo;
    ESP_LOGI(TAG, "load %s", algorithmPath.c_str());
    if(elfIo.load(algorithmPath))
    { 
        ESP_LOGI(TAG, "loadLoader");

        ELFIO::dump::header( std::cout, elfIo );
        //ELFIO::dump::section_headers( std::cout, elfIo );
        ELFIO::dump::segment_headers( std::cout, elfIo );
        ELFIO::dump::symbol_tables( std::cout, elfIo );
        //ELFIO::dump::notes( std::cout, elfIo );
        //ELFIO::dump::modinfo( std::cout, elfIo );
        //ELFIO::dump::dynamic_tags( std::cout, elfIo );
        //ELFIO::dump::section_datas( std::cout, elfIo );
        //ELFIO::dump::segment_datas( std::cout, elfIo );

        for (const auto &sec : elfIo.sections)
        {
            if (ELFIO::SHT_SYMTAB == sec->get_type() || ELFIO::SHT_DYNSYM == sec->get_type())
            {
                ELFIO::const_symbol_section_accessor symbols(elfIo, sec.get());

                ELFIO::Elf_Xword sym_no = symbols.get_symbols_num();
                if (sym_no == 0)
                {
                    continue;
                }

                for (ELFIO::Elf_Xword i = 0; i < sym_no; ++i)
                {
                    std::string name;
                    ELFIO::Elf64_Addr value = 0;
                    ELFIO::Elf_Xword size = 0;
                    unsigned char bind = 0;
                    unsigned char type = 0;
                    ELFIO::Elf_Half section = 0;
                    unsigned char other = 0;
                    symbols.get_symbol(i, name, value, size, bind, type, section, other);
                    for(int i = 0 ; i < FlashAlgo::FuncEntry::eSize; i ++)
                    {
                        if(std::string(cFuncStr[i]) == name)
                        {
                            lut.lut[i] = value + targetRam.ramStartAddr;
                            ESP_LOGI(TAG, "name %s addr 0x%lX", name.c_str(), lut.lut[i]);
                        }
                    }
                }
            }
        }

        for (const auto &seg : elfIo.segments)
        {
            const char* data = seg->get_data();
            const uint32_t size = seg->get_file_size();
            Program prg {
                .startAddr = static_cast<uint32_t>(seg->get_physical_address() + targetRam.ramStartAddr),
            };
            if(size == 0)
            {
                continue;
            }
            std::copy(data, data + size, std::back_inserter(prg.data));
            lut.loader.push_back(std::move(prg));
            ESP_LOGI(TAG, "Loader data start %lX size %lX", prg.startAddr, size);
        }
    }
    lut.workRamInfo = createRamInfo(targetRam);
    lut.sysCallInfo.breakPoint = lut.workRamInfo.targetSramInfo.ramStartAddr + 1;
    lut.sysCallInfo.staticBase = lut.workRamInfo.programMemInfo.ramStartAddr;
    lut.sysCallInfo.stackPointer = lut.workRamInfo.stackInfo.ramStartAddr + lut.workRamInfo.stackInfo.ramSize - 4;
    return lut;
}

FlashAlgo::WorkRamInfo FlashAlgo::createRamInfo(const RamInfo& targetRam)
{
    if( cLoader.loader.size() == 0)
    {
        return WorkRamInfo{};
    }
    uint32_t loaderSize = 0;
    for(const auto& loader: cLoader.loader)
    {
        constexpr uint32_t cAlign = 32 - 1;
        loaderSize += loader.data.size();

        // 4byte align
        loaderSize += cAlign;
        loaderSize &= ~cAlign;
    }

    ESP_LOGI(TAG, "Loader Size 0x%lX", loaderSize);
    const uint32_t workRamEndAdr = targetRam.ramStartAddr + loaderSize;
    const uint32_t stackEndAdr = workRamEndAdr + cStackSize;
    const uint32_t programMemSize = targetRam.ramSize - (cStackSize + loaderSize);

    WorkRamInfo info{
        .targetSramInfo = targetRam,
        .stackInfo = RamInfo{.ramStartAddr = workRamEndAdr, .ramSize = cStackSize},
        .programMemInfo = RamInfo{.ramStartAddr = stackEndAdr, .ramSize = programMemSize}
    };
    ESP_LOGI(TAG, "Target Ram start 0x%lX size 0x%lX", info.targetSramInfo.ramStartAddr, info.targetSramInfo.ramSize);
    ESP_LOGI(TAG, "Stack start %lX size %lX", info.stackInfo.ramStartAddr, info.stackInfo.ramSize);
    ESP_LOGI(TAG, "ProgramMem start 0x%lX size 0x%lX", info.programMemInfo.ramStartAddr, info.programMemInfo.ramSize);
    return info;
}

uint32_t FlashAlgo::writeSwd(uint32_t addr, const uint8_t* buffer, uint32_t bufferSize)
{
    ESP_LOGI(TAG, "%s %lX size %lX", __func__, addr, bufferSize);

    constexpr uint32_t cMaxPageSize = 1024;
    std::vector<uint32_t> verify;
    verify.resize(cMaxPageSize / sizeof(uint32_t));

    // load Flash loader
    uint32_t index = 0;
    uint32_t leng = bufferSize;
    do
    {
        const uint32_t chunkLen8 = std::min(leng, cMaxPageSize);
        const uint32_t chunkLen = chunkLen8 / sizeof(uint32_t);
        const uint32_t* cpWrite = reinterpret_cast<const uint32_t*>(&buffer[index]);

        ESP_LOGI(TAG, "%s addr 0x%lX leng 0x%lX", __func__, addr, chunkLen8);
        if(not pSwd->writeMemoryBlcok32(addr, cpWrite, chunkLen))
        {
            ESP_LOGE(TAG, "%s fails addr 0x%lX size 0x%lX", __func__, addr, index);
            return index;
        }

        if(not pSwd->readMemoryBlcok32(addr, verify.data(), chunkLen))
        {
            ESP_LOGE(TAG, "%s read fails addr 0x%lX size 0x%lX", __func__, addr, index);
            return index;
        }
        for(int i  = 0; i < chunkLen; i++)
        {
            if(verify.at(i) != cpWrite[i])
            {
                ESP_LOGE(TAG, "%s verify fails Addr 0x%lu read 0x%lX write 0x%lX", __func__, addr + i, verify.at(i), cpWrite[i]);
            }
        }

        leng -= chunkLen8;
        addr += chunkLen8;
        index += chunkLen8;
    } while(leng > 0);
    return index;
}

int FlashAlgo::blankCheck(unsigned long adr, unsigned long sz, unsigned char pat)
{
    // pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::blankCheck], adr, sz, pat, 0, Swd::FlashAlgoRetType::cBool);
    return 0;
}

int FlashAlgo::eraseChip(void)
{
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eEraseChip], 0, 0, 0, 0, Swd::FlashAlgoRetType::cBool);
    return 0;
}

int FlashAlgo::eraseSector(unsigned long adr)
{
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eEraseSector], adr, 0, 0, 0, Swd::FlashAlgoRetType::cBool);
    return 0;
}

int FlashAlgo::init(unsigned long adr, unsigned long clk, unsigned long fnc)
{
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eInit], adr, clk, 0, 0, Swd::FlashAlgoRetType::cBool);
    return 0;
}

int FlashAlgo::unInit(unsigned long fnc)
{
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eUnInit], fnc, 0, 0, 0,  Swd::FlashAlgoRetType::cBool);
    return 0;
}

int FlashAlgo::programPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{   
    pSwd->writeMemoryBlcok32(cLoader.workRamInfo.programMemInfo.ramStartAddr, reinterpret_cast<const uint32_t*>(buf), sz / sizeof(uint32_t));
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eProgramPage], adr, sz, cLoader.workRamInfo.programMemInfo.ramStartAddr, 0,  Swd::FlashAlgoRetType::cBool);
    return 0;
}

unsigned long FlashAlgo::verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    pSwd->writeMemoryBlcok32(cLoader.workRamInfo.programMemInfo.ramStartAddr, reinterpret_cast<const uint32_t*>(buf), sz / sizeof(uint32_t));
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eVerify], adr, sz, cLoader.workRamInfo.programMemInfo.ramStartAddr, 0,  Swd::FlashAlgoRetType::cBool);
    return 0;
}
