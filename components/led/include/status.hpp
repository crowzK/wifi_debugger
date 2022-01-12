#ifndef _STATUS_HPP_
#define _STATUS_HPP_

#include <stdint.h>
#include <bitset>
#include <mutex>
#include "led.hpp"

class Status
{
public:
    enum Error
    {
        eWifiConnect,
        eSdcard,
        eSize
    };
    static Status& get();
    void report(Error err, bool status);
    bool getError(Error err);

protected:
    std::bitset<Error::eSize> mStatus;
    std::recursive_mutex mMutex;
    Led mLed;

    Status();
    ~Status() = default;
};

#endif //_STATUS_HPP_
