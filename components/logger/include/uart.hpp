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

#ifndef UASRT_HPP
#define UASRT_HPP

#include <vector>
#include <thread>
#include <atomic>
#include "msg_proxy.hpp"
#include "task.hpp"
#include "console.hpp"

//! For sending message through UART
class UartTx : public Client
{
public:
    UartTx(int uartPortNum);
    ~UartTx() = default;
    
protected:
    const int cUartNum;
    bool writeStr(const MsgProxy::Msg& msg) override;
};

//! For receinv message from the UART
class UartRx : public Task
{
public:
    UartRx(int uartPortNum);
    ~UartRx() = default;
    
protected:
    const int cUartNum;
    void task() override;
};

class SettingCmd : protected Cmd
{
public:
    SettingCmd();
    ~SettingCmd() = default;

private:
    bool excute(const std::vector<std::string>& args) override;
    std::string help() override;
};


//! For send/recv message through UART
class UartService
{
public:
    struct Config
    {
        int baudRate;
        int uartNum;
    };

    static UartService& create();
    void init(const Config& cfg);
    const Config& getCfg() const { return mConfig; };

protected:
#if (CONFIG_M5STACK_CORE | CONFIG_TTGO_T1)
    static constexpr int cTxPin = 17;
    static constexpr int cRxPin = 16;
#elif CONFIG_WIFI_DEBUGGER_V_0_1
    static constexpr int cTxPin = 10;
    static constexpr int cRxPin = 1;
#elif CONFIG_WIFI_DEBUGGER_V_0_2
    static constexpr int cTxPin = 21;
    static constexpr int cRxPin = 20;
#elif CONFIG_WIFI_DEBUGGER_V_0_4
    static constexpr int cTxPin = 47;
    static constexpr int cRxPin = 48;
#elif CONFIG_WIFI_DEBUGGER_V_0_6
    static constexpr int cTxPin = 6;
    static constexpr int cRxPin = 7;
#endif

    Config mConfig;
    std::unique_ptr<UartRx> pUartRx;
    std::unique_ptr<UartTx> pUartTx;
    SettingCmd mOption;

    UartService();
};


#endif //UASRT_HPP
