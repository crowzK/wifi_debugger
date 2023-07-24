/*
Copyright (C) Yudoc Kim <craven@crowz.kr>
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <utility>
#include <chrono>
#include "log_file.hpp"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include <sstream>
#include "status.hpp"
#include "sntp.h"

using namespace std::chrono_literals;
static const char *TAG = "logFile";

LogFile& LogFile::create()
{
    static LogFile lf;
    return lf;
}

LogFile::LogFile() :
    Client(DebugMsgRx::create(), INT32_MAX),
    Task(__func__),
    mFsManager(FsManager::create()),
    cMountPoint(mFsManager.getMountPoint()),
    mMsgQueue(1024),
    pFile(nullptr)
{

}

LogFile::~LogFile()
{
    if(pFile)
    {
        fclose(pFile);
    }
}

void LogFile::init()
{
    if(mFsManager.isMount())
    {
        start();
    }
}

const std::string LogFile::getFilePath()
{
    std::lock_guard<std::recursive_timed_mutex> lock(mMutex);
    return mFilePath;
}

FILE* LogFile::createFile()
{
    std::lock_guard<std::recursive_timed_mutex> lock(mMutex);

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    std::time_t t = tv_now.tv_sec;
    tm local = *localtime(&t);
    local.tm_year += 1900;
    local.tm_mon += 1;

    std::ostringstream path;
    path << cMountPoint << "/log";
    struct stat _stat = {};
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }

    path << "/" << local.tm_year;
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }

    path << "/" << local.tm_mon;
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }
    path << "/" << local.tm_mday;
    if(stat(path.str().c_str(), &_stat))
    {
        if(mkdir(path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            ESP_LOGE(TAG, "Cannot create dir(%s)", path.str().c_str());
            return nullptr;
        }
    }
    path << "/" << local.tm_year << "-"  << local.tm_mon << "-" << local.tm_mday << "T" << local.tm_hour << "_" << local.tm_min << "_" << local.tm_sec << ".log";
    mFilePath = path.str();
    ESP_LOGI(TAG, "File Open(%s)",mFilePath.c_str());
    return fopen(mFilePath.c_str(), "w");
}

bool LogFile::writeStr(const MsgProxy::Msg& msg)
{
    mMsgQueue.push(msg, 0ms);
    return true;
}

void LogFile::task()
{
    while(mRun and (sntp_getreachability(0) == 0))
    {
        vTaskDelay(100);
    }

    if(mRun)
    {
        std::lock_guard<std::recursive_timed_mutex> lock(mMutex);
        mFsManager.mount();
        pFile = createFile();
    }

    auto start = std::chrono::steady_clock::now();
    while(mRun)
    {
        MsgProxy::Msg msg;
        if(mMsgQueue.pop(msg, 100ms))
        {
            std::lock_guard<std::recursive_timed_mutex> lock(mMutex);
            const auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::hours>(now - start);

            if(duration.count() >= cNewFileCreateDurationHours)
            {
                if(pFile != nullptr)
                {
                    fclose(pFile);
                }
                pFile = createFile();
                start = now;
            }

            if(pFile == nullptr)
            {
                continue;
            }
            
            fwrite(msg.str.data(), 1, msg.str.size(), pFile);

            if(mMsgQueue.isEmpty())
            {
                fflush(pFile);
                fsync(fileno(pFile));
            }
        }
    }
}