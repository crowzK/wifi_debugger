#ifndef SDCARD_HPP
#define SDCARD_HPP

#include <stdio.h>
#include "debug_msg_handler.hpp"
#include "sdkconfig.h"
#include <mutex>

class SdCard : public Client
{
public:
    SdCard();
    ~SdCard();
    void init();
    bool write(const char* msg, uint32_t length);
    bool write(const std::vector<uint8_t>& msg) override;

protected:
    static const char* cMountPoint;
    std::recursive_mutex mMutex;

    void* pSdcard;
    FILE* pFile;
    bool mInited;

    FILE* createFile();
};

#endif
