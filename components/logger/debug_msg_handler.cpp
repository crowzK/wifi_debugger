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

#include <algorithm>
#include "debug_msg_handler.hpp"
#include "esp_log.h"


//*******************************************************************
// Client
//*******************************************************************
Client::Client(MsgProxy& debugMsg, int id) :
    cId(id),
    mDebugMsg(debugMsg)
{
    mDebugMsg.addClient(*this);
}

Client::~Client()
{
    mDebugMsg.removeClient(*this);
}

//*******************************************************************
// MsgProxy
//*******************************************************************

MsgProxy::MsgProxy(const char* cName) :
    Task(cName),
    mQueue(10)
{
}

MsgProxy::~MsgProxy()
{
    stop();
}

bool MsgProxy::addClient(Client& client)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto it = std::find_if(mClientList.begin(), mClientList.end(), [&client](Client* pClient){ return &client == pClient; });
    if(it == mClientList.end())
    {
        ESP_LOGI("MsgProxy", "addClient %d", client.cId);
        mClientList.push_back(&client);
    }
    return true;
}

bool MsgProxy::removeClient(Client& client)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto it = std::find_if(mClientList.begin(), mClientList.end(), [&client](Client* pClient){ return &client == pClient; });
    if(it != mClientList.end())
    {
        ESP_LOGI("MsgProxy", "removeClient %d", client.cId);
        mClientList.erase(it);
    }
    return true;
}

bool MsgProxy::isAdded(int id)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto it = std::find_if(mClientList.begin(), mClientList.end(), [&id](Client* pClient){ return id == pClient->cId; });
    return it != mClientList.end();
}

bool MsgProxy::write(std::vector<uint8_t>& msg)
{
    return mQueue.push(msg, std::chrono::milliseconds(100));
}

void MsgProxy::sendMsg(const std::vector<uint8_t>&msg)
{  
    std::list<Client*> erase;
    for(auto it = mClientList.begin(); it != mClientList.end(); ++it)
    {
        if(*it == nullptr)
        {
            continue;
        }
        if((*it)->write(msg) == false)
        {
            erase.push_back(*it);
        }
    }

    for(auto it = erase.begin(); it != erase.end(); ++it)
    {
        delete *it;
    }
}

//*******************************************************************
// DebugMsgRx
//*******************************************************************

DebugMsgRx& DebugMsgRx::get()
{
    static DebugMsgRx rx;
    rx.start();
    return rx;
}

DebugMsgRx::DebugMsgRx() :
    MsgProxy(__func__)
{

}

DebugMsgRx::~DebugMsgRx()
{

}

void DebugMsgRx::getTime(std::vector<uint8_t>& msg)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);

    std::time_t t = tv_now.tv_sec;
    tm local = *localtime(&t);
    int ms = tv_now.tv_usec / 1000;

    msg.resize(18);
    sprintf((char*)msg.data(), "[%02dT%02d:%02d:%02d:%03d]", local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, ms);
    msg[17] = ' ';
}

void DebugMsgRx::task()
{
	std::vector<uint8_t> strBuffer;
	strBuffer.reserve(512);
    // strBuffer.resize(timeStrLen);

    while (mRun) 
    {
        std::vector<uint8_t> msg;
        if(mQueue.pop(msg, std::chrono::milliseconds(1000)) and msg.size())
        {
            std::lock_guard<std::recursive_mutex> lock(mMutex);

            for(int i = 0; i < msg.size(); ++i)
            {
                uint8_t d = msg[i];
                if(strBuffer.size() == 0)
                {
					getTime(strBuffer);
                }
				if(d == '\r')
				{
					continue;
				}
				if(d == '\n' or strBuffer.size() >= 512)
				{
					strBuffer.push_back('\r');
					strBuffer.push_back('\n');
                    sendMsg(strBuffer);
                    strBuffer.resize(0);
					continue;
				}
				strBuffer.push_back(d);
            }
        }
    }
}

//*******************************************************************
// DebugMsgTx
//*******************************************************************

DebugMsgTx& DebugMsgTx::get()
{
    static DebugMsgTx tx;
    tx.start();
    return tx;
}

DebugMsgTx::DebugMsgTx() :
    MsgProxy(__func__)
{

}

DebugMsgTx::~DebugMsgTx()
{

}

void DebugMsgTx::task()
{
    while (mRun) 
    {
        std::vector<uint8_t> msg;
        if(mQueue.pop(msg, std::chrono::milliseconds(1000)) and msg.size())
        {
            std::lock_guard<std::recursive_mutex> lock(mMutex);
            sendMsg(msg);
        }
    }
}
