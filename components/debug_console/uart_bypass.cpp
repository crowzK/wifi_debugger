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

#include <unistd.h>
#include <stdint.h>
#include "uart_bypass.hpp"
#include "esp_log.h"


//-------------------------------------------------------------------
// LineEndMap
//-------------------------------------------------------------------
LineEndMap::LineEndMap() :
    Cmd("omap"),
    mLineEndMapStr("lfcr")
{
    setLineEnd(mLineEndMapStr);
}
LineEndMap::~LineEndMap()
{

}

std::string LineEndMap::help()
{
    return std::string("UART output mamping `crlf`: cr -> lf, `lfcr` lf -> cr, `crcrlf` -> cr -> crlf");
}

bool LineEndMap::setLineEnd(const std::string& str)
{
    if(str == std::string("crlf"))
    {
        mLineEndMap = Map::eCrLf;
    }
    else if(str == std::string("lfcr"))
    {
        mLineEndMap = Map::eLfCr;
    }
    else if(str == std::string("crcrlf"))
    {
        mLineEndMap = Map::eCrCrLf;
    }
    else if(str == std::string("lfcrlf"))
    {
        mLineEndMap = Map::eLfCrLf;
    }
    else
    {
        return false;
    }
    mLineEndMapStr = str.c_str();
    printf("%s: %s\n", __func__, mLineEndMapStr.c_str());
    return true;
}

bool LineEndMap::excute(const std::vector<std::string>& args)
{
    if(args.size() == 1)
    {
        printf("CurrentMode: %s\n", mLineEndMapStr.c_str());
    }
    else if(args.size() == 2)
    {
        return setLineEnd(args.at(1));
    }
    else
    {
        return false;
    }
    return true;
}

//-------------------------------------------------------------------
// UartByPass
//-------------------------------------------------------------------
UartByPass::UartByPass() :
    Client(DebugMsgRx::create(), INT32_MAX - 1),
    Cmd("uart"),
    Task(__func__),
    mQueue(20)
{
    start();
}

bool UartByPass::writeStr(const MsgProxy::Msg& msg)
{
    mQueue.push(msg.str, std::chrono::milliseconds(10));
    return true;
}

std::string UartByPass::help()
{
    return std::string("Send message direct to the UART");
}

bool UartByPass::excute(const std::vector<std::string>& args)
{
    int stdin_fileno = fileno(stdin);
    printf("Enter usb-uart mode press ctrl+B if you want to exit\n");
    while(1)
    {
        char str[3] = {};
        if(read(stdin_fileno, str, 1))
        {
            if(str[0] == 2)
            {
                printf("escape cmd enter\n");
                break;
            }
            switch (mLineEndMode.getMap())
            {
            default:
            case LineEndMap::eCrLf:
                if(str[0] == '\r')
                {
                    str[0] = '\n';
                }
                break;
            case LineEndMap::eCrCrLf:
                if(str[0] == '\r')
                {
                    str[0] = '\r';
                    str[1] = '\n';
                }
                break;
            case LineEndMap::eLfCr:
                if(str[0] == '\n')
                {
                    str[0] = '\r';
                }
                break;
            case LineEndMap::eLfCrLf:
                if(str[0] == '\n')
                {
                    str[0] = '\r';
                    str[1] = '\n';
                }
                break;
            }
            DebugMsgTx::create().write(str);
        }
    }
    return true;
}

void UartByPass::task()
{
    while(mRun)
    {
        std::string str;
        if(mQueue.pop(str, std::chrono::milliseconds(1000)) and str.length())
        {
            fwrite(str.c_str(), 1,str.length(), stdout);
        }
        if(mQueue.isEmpty())
        {
            fflush(stdout);
        }
    }
}
