#include "task.hpp"
#include <esp_log.h>


//*******************************************************************
// Task
//*******************************************************************

Task::Task(const char* taskName) :
    cName(taskName),
    mRun(false)
{
    ESP_LOGI(cName, "Created");
}

Task::~Task()
{
    ESP_LOGI(cName, "Terminated");
    stop();
}

void Task::stop()
{
    if(mRun)
    {
        mRun = false;
        mThreadHandle.join();
    }
}

void Task::start()
{
    if(not mRun)
    {
        mRun = true;
        mThreadHandle = std::thread([this]{ task(); });
    }
}
