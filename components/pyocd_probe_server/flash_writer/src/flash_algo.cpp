#include "flash_algo.hpp"
#include "elfio/elfio.hpp"
#include <fstream>
#include "gpio_swd.hpp"

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
    cLoader(loadLoader(algorithmPath)),
    cRamInfo(createRamInfo(targetRam))
{
    pSwd = std::make_unique<GpioSwd>();
    if(not pSwd->initDebug())
    {
        return;
    }
    for(const auto& loader: cLoader.loader)
    {
        // load Flahs loader
        pSwd->writeMemoryBlcok32(loader.startAddr, reinterpret_cast<const uint32_t*>(loader.data.data()), loader.data.size() / sizeof(uint32_t));
    }
}

FlashAlgo::~FlashAlgo()
{
}

FlashAlgo::FlashLoaderInfo FlashAlgo::loadLoader(const std::string &algorithmPath)
{
    FlashLoaderInfo lut{};
    ELFIO::elfio elfIo;
    elfIo.load(algorithmPath);
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
            .startAddr = static_cast<uint32_t>(seg->get_physical_address()),
        };
        std::copy(data, data + size, std::back_inserter(prg.data));
        lut.loader.push_back(std::move(prg));
    }

    return lut;
}

FlashAlgo::WorkRamInfo FlashAlgo::createRamInfo(const RamInfo& targetRam)
{
    const auto& loaderEndSection = cLoader.loader.end();
    const uint32_t workRamEndAdr = loaderEndSection->startAddr + loaderEndSection->data.size();
    const uint32_t stackEndAdr = workRamEndAdr + cStackSize;
    const uint32_t programMemSize = targetRam.ramSize - (cStackSize + targetRam.ramSize);

    return WorkRamInfo {
        .targetSramInfo = targetRam,
        .stackInfo = RamInfo{.ramStartAddr = workRamEndAdr, .ramSize = cStackSize},
        .programMemInfo = RamInfo{.ramStartAddr = stackEndAdr, .ramSize = programMemSize}
    };
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
