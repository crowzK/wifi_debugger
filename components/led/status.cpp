#include "status.hpp"

Status& Status::get()
{
    static Status status;
    return status;
}

Status::Status() :
    mLed(static_cast<gpio_num_t>(CONFIG_STATUS_LED_PIN))
{

}

void Status::report(Error err, bool status)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mStatus.set(err, status);
    mLed.on(mStatus.test(Error::eWifiConnect));
}

bool Status::getError(Error err)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mStatus.test(err);
}
