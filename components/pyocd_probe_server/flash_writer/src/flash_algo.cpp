#include "flash_algo.hpp"
#include "elfio/elfio.hpp"


FlashAlgo::FlashAlgo(const std::string& algorithmPath, TargerRamInfo targetRam) :
    cTargerRamInfo(targetRam),
    cFuncLut(createLut(algorithmPath))
{

}

FlashAlgo::~FlashAlgo()
{

}

FlashAlgo::FlashAlgoFuncLUT FlashAlgo::createLut(const std::string& algorithmPath)
{
    FlashAlgoFuncLUT lut{};
    return lut;
}


int FlashAlgo::blankCheck(unsigned long adr, unsigned long sz, unsigned char pat)
{
    return 0;
}

int FlashAlgo::eraseChip(void)
{
    return 0;
}

int FlashAlgo::eraseSector(unsigned long adr)
{
    return 0;
}

int FlashAlgo::init(unsigned long adr, unsigned long clk, unsigned long fnc)
{
    return 0;
}

int FlashAlgo::unInit(unsigned long fnc)
{
    return 0;
}

int FlashAlgo::programPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    return 0;
}

unsigned long FlashAlgo::verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    return 0;
}
