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
    cLoader(loadLoader(algorithmPath, targetRam)),
    cRamInfo(createRamInfo(targetRam))
{
    pSwd = std::make_unique<GpioSwd>();
    if(not pSwd->setStateBySw(Swd::TargetState::eHalt))
    {
        return;
    }
    
    ESP_LOGI(TAG, "SWD init success");
    for(const auto& loader: cLoader.loader)
    {
        ESP_LOGI(TAG, "Loader download addr %lX size %X", loader.startAddr, loader.data.size());
        const uint32_t cLength = loader.data.size() / sizeof(uint32_t);
        const uint32_t* cWrite = reinterpret_cast<const uint32_t*>(loader.data.data());
        
        // todo if length is > 1024 
        // create problem
    
        // load Flahs loader
        if(not pSwd->writeMemoryBlcok32(loader.startAddr, cWrite, cLength))
        {
            ESP_LOGE(TAG, "Loader download fails %lX size %X", loader.startAddr, loader.data.size());
            return;
        }

        std::vector<uint32_t> verify;
        verify.resize(cLength);
        if(not pSwd->readMemoryBlcok32(loader.startAddr, verify.data(), cLength))
        {
            ESP_LOGE(TAG, "Loader read fails %lX size %X", loader.startAddr, loader.data.size());
            return;
        }
        for(int i  = 0; i < cLength; i++)
        {
            if(verify.at(i) != cWrite[i])
            {
                ESP_LOGE(TAG, "Loader verify fails Addr %d read %lX write %lX", i, verify.at(i), cWrite[i]);
            }
        }
    }
    ESP_LOGI(TAG, "Loader download done");
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
        ELFIO::dump::section_headers( std::cout, elfIo );
        ELFIO::dump::segment_headers( std::cout, elfIo );
        //ELFIO::dump::symbol_tables( std::cout, elfIo );
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
                            lut.lut[i] = value;
                            ESP_LOGI(TAG, "name %s addr %llX", name.c_str(), value);
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

    ESP_LOGI(TAG, "Loader Size %lX", loaderSize);
    const uint32_t workRamEndAdr = targetRam.ramStartAddr + loaderSize;
    const uint32_t stackEndAdr = workRamEndAdr + cStackSize;
    const uint32_t programMemSize = targetRam.ramSize - (cStackSize + loaderSize);

    WorkRamInfo info{
        .targetSramInfo = targetRam,
        .stackInfo = RamInfo{.ramStartAddr = workRamEndAdr, .ramSize = cStackSize},
        .programMemInfo = RamInfo{.ramStartAddr = stackEndAdr, .ramSize = programMemSize}
    };
    ESP_LOGI(TAG, "Target Ram start %lX size %lX", info.targetSramInfo.ramStartAddr, info.targetSramInfo.ramSize);
    ESP_LOGI(TAG, "Stack start %lX size %lX", info.stackInfo.ramStartAddr, info.stackInfo.ramSize);
    ESP_LOGI(TAG, "ProgramMem start %lX size %lX", info.programMemInfo.ramStartAddr, info.programMemInfo.ramSize);
    return info;
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
    pSwd->writeMemoryBlcok32(cRamInfo.programMemInfo.ramStartAddr, reinterpret_cast<const uint32_t*>(buf), sz / sizeof(uint32_t));
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eProgramPage], adr, sz, cRamInfo.programMemInfo.ramStartAddr, 0,  Swd::FlashAlgoRetType::cBool);
    return 0;
}

unsigned long FlashAlgo::verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    pSwd->writeMemoryBlcok32(cRamInfo.programMemInfo.ramStartAddr, reinterpret_cast<const uint32_t*>(buf), sz / sizeof(uint32_t));
    pSwd->sysCallExec(cLoader.sysCallInfo, cLoader.lut[FuncEntry::eVerify], adr, sz, cRamInfo.programMemInfo.ramStartAddr, 0,  Swd::FlashAlgoRetType::cBool);
    return 0;
}
