#ifndef SDCARD_HPP
#define SDCARD_HPP

#include <stdio.h>
#include "debug_msg_handler.hpp"

class SdCard : public Client
{
public:
    SdCard();
    ~SdCard();

    bool write(const char* msg, uint32_t length);
    bool write(const std::vector<uint8_t>& msg) override;

protected:
    static constexpr int cPinMiso = 19;
    static constexpr int cPinMosi = 23;
    static constexpr int cPinClk = 18;
    static constexpr int cPinCs = 4;
    static constexpr int cSpiPort = 2;
    static const char* cMountPoint;
    void* pSdcard;
    FILE* pFile;

    FILE* createFile();
};

#endif
