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

#ifndef DEBUG_MSG_HANDLER_HPP
#define DEBUG_MSG_HANDLER_HPP

#include <stdint.h>
#include <vector>
#include <memory>
#include <list>
#include <mutex>
#include "blocking_queue.hpp"
#include "task.hpp"

class MsgProxy;

class Client
{
public:
    const int cId;
    Client(MsgProxy& debugMsg, int id);
    virtual ~Client();
    virtual bool write(const std::vector<uint8_t>& msg) = 0;

protected:
    MsgProxy& mDebugMsg;
};

class MsgProxy : public Task
{
public:
    bool addClient(Client& client);
    bool removeClient(Client& client);
    bool isAdded(int id);
    bool write(std::vector<uint8_t>& msg);

protected:
    BlockingQueue<std::vector<uint8_t>> mQueue;
    std::list<Client*> mClientList;
    std::recursive_mutex mMutex;

    MsgProxy(const char* cName);
    virtual ~MsgProxy();
    void sendMsg(const std::vector<uint8_t>&msg);
};

//! It will handle the messages from the UART
class DebugMsgRx : public MsgProxy
{
public:
    static DebugMsgRx& get();

protected:
    DebugMsgRx();
    ~DebugMsgRx();
    void getTime(std::vector<uint8_t>& msg);
    void task() override;
};

//! It will handle the messages to the UART
class DebugMsgTx : public MsgProxy
{
public:
    static DebugMsgTx& get();

protected:
    DebugMsgTx();
    ~DebugMsgTx();
    void task() override;
};

#endif
