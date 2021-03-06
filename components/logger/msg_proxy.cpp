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
#include <iostream>
#include <sstream>
#include <regex>
#include "msg_proxy.hpp"
#include "esp_log.h"


//-------------------------------------------------------------------
// Client
//-------------------------------------------------------------------
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

//-------------------------------------------------------------------
// MsgProxy
//-------------------------------------------------------------------
MsgProxy::MsgProxy(const char* cName) :
    Task(cName),
    mQueue(cQueueSize)
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

bool MsgProxy::write(char* msg)
{
    Msg _msg;
    gettimeofday(&_msg.time, NULL);
    _msg.str = std::string(msg);
    return mQueue.push(_msg, std::chrono::milliseconds(100));
}

void MsgProxy::sendLine(const Msg& msg)
{  
    std::list<Client*> erase;
    for(auto it = mClientList.begin(); it != mClientList.end(); ++it)
    {
        if(*it == nullptr)
        {
            continue;
        }
        if((*it)->writeLine(msg) == false)
        {
            erase.push_back(*it);
        }
    }

    for(auto it = erase.begin(); it != erase.end(); ++it)
    {
        delete *it;
    }
}

void MsgProxy::sendStr(const Msg& msg)
{
    std::list<Client*> erase;
    for(auto it = mClientList.begin(); it != mClientList.end(); ++it)
    {
        if(*it == nullptr)
        {
            continue;
        }
        if((*it)->writeStr(msg) == false)
        {
            erase.push_back(*it);
        }
    }

    for(auto it = erase.begin(); it != erase.end(); ++it)
    {
        delete *it;
    }
}

std::string MsgProxy::getHeader(const struct timeval& time)
{
    std::time_t t = time.tv_sec;
    tm local = *localtime(&t);
    int ms = time.tv_usec / 1000;

    char str[20];
    sprintf(str, "[%02dT%02d:%02d:%02d:%03d] ", local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, ms);
    return std::string(str);
}

//-------------------------------------------------------------------
// DebugMsgRx
//-------------------------------------------------------------------

DebugMsgRx& DebugMsgRx::create()
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

void DebugMsgRx::task()
{
    while (mRun) 
    {
        Msg msg;
        if(mQueue.pop(msg, std::chrono::milliseconds(1000)) and msg.str.length())
        {
            sendStr(msg);
            for(int i = 0; i < msg.str.length(); i ++)
            {
                if(mLine.str.length() == 0)
                {
                    mLine = msg;
                    mLine.str = getHeader(mLine.time);
                }
                const char& ch = msg.str[i];
                mLine.str += ch;
                if(ch == cStrEnd)
                {
                    std::lock_guard<std::recursive_mutex> lock(mMutex);
                    sendLine(mLine);
                    mLine.clear();
                }    
            }
        }
    }
}

//-------------------------------------------------------------------
// DebugMsgTx
//-------------------------------------------------------------------

DebugMsgTx& DebugMsgTx::create()
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
        Msg msg;
        if(mQueue.pop(msg, std::chrono::milliseconds(1000)) and msg.str.size())
        {
            std::lock_guard<std::recursive_mutex> lock(mMutex);
            sendStr(msg);
        }
    }
}
