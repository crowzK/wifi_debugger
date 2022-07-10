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
#include <memory>
#include <list>
#include <mutex>
#include <string>
#include "blocking_queue.hpp"
#include "task.hpp"

class Client;

//! debug message proxy
//! It will help broadcast message to the clients
class MsgProxy : public Task
{
public:
    struct Msg
    {
        std::string str;
        bool strStart;
        struct timeval time;
    };

    //! \brief Add client
    bool addClient(Client& client);

    //! \brief Remove client
    bool removeClient(Client& client);

    //! \brief Check whether client is already added or not
    bool isAdded(int id);

    //! \brief Write message for broadcating
    //! \note it pushes message to the Queue and 
    //! sendMsg() will pop messages form the queue and send its clients
    bool write(Msg& msg);

    static std::vector<std::string> split(const std::string& cmd);
    static std::vector<Msg> convToMsg(char* str);

    static std::string getTime(const struct timeval& time);

protected:
    BlockingQueue<Msg> mQueue;
    std::list<Client*> mClientList;
    std::recursive_mutex mMutex;

    MsgProxy(const char* cName);
    virtual ~MsgProxy();

    //! \brief send messages to the clients.
    //! \note child class must call this to send data to the clients.  
    void sendMsg(const Msg& msg);
};

//! It's a interface class to receive messages from the proxy.
class Client
{
public:
    const int cId;
    Client(MsgProxy& debugMsg, int id);
    virtual ~Client();

    //! \brief proxy will call this write method to deliver message 
    virtual bool write(const MsgProxy::Msg& msg) = 0;

protected:
    MsgProxy& mDebugMsg;
};

//! It will handle the messages from the UART
class DebugMsgRx : public MsgProxy
{
public:
    static DebugMsgRx& create();

protected:
    DebugMsgRx();
    ~DebugMsgRx();
    void task() override;
};

//! It will handle the messages to the UART
class DebugMsgTx : public MsgProxy
{
public:
    static DebugMsgTx& create();

protected:
    DebugMsgTx();
    ~DebugMsgTx();
    void task() override;
};

#endif
