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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <sstream>
#include "esp_vfs_dev.h"
#include "esp_vfs_usb_serial_jtag.h"
#include "driver/usb_serial_jtag.h"
//#include "hal/usb_serial_jtag_ll.h"
#include "uart_bypass.hpp"
#include "pyocd_io_console.hpp"

#include "console.hpp"
//-------------------------------------------------------------------
// Console
//-------------------------------------------------------------------
Cmd::Cmd(std::string&& cmd) :
    cCmd(std::move(cmd))
{
    Console::create().add(*this);
}

Cmd::~Cmd()
{
    Console::create().remove(*this);
}

//-------------------------------------------------------------------
// Console
//-------------------------------------------------------------------
Help::Help() :
    Cmd("help")
{

}

std::string Help::help()
{
    return std::string("print all cmd");
}

bool Help::excute(const std::vector<std::string>& args)
{
    Console::create().help();
    return true;
}

//-------------------------------------------------------------------
// Console
//-------------------------------------------------------------------
Console& Console::create()
{
    static Console console;
    return console;
}

Console::Console() :
    Task(__func__)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    init();
    start();
}

bool Console::init()
{
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_usb_serial_jtag_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_usb_serial_jtag_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);

    usb_serial_jtag_driver_config_t usb_serial_jtag_config
    {
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024
    };

    /* Install USB-SERIAL-JTAG driver for interrupt-driven reads and writes */
    usb_serial_jtag_driver_install(&usb_serial_jtag_config);

    return true;
}

void Console::help()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    printf("\n\n---------------------------------\n");
    printf("Console cmd help\n");
    printf("Use `#` as a delimiter for parameters instead of spaces\n");
    for(auto& pCmd : mCmdList)
    {
        printf("%10s | %s\n", pCmd->cCmd.c_str(), pCmd->help().c_str());
    }
    printf("---------------------------------\n");
}

void Console::add(Cmd& cmd)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    for(const auto& pCmd : mCmdList)
    {
        if(pCmd->cCmd == cmd.cCmd)
        {
            // already registered
            return;
        }
    }
    printf("cmd [%s] add\n", cmd.cCmd.c_str());
    mCmdList.push_back(&cmd);
}

void Console::remove(Cmd& cmd)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mCmdList.remove_if(
        [&cmd](const auto& pCmd)
        {
            return cmd.cCmd == pCmd->cCmd;
        });
    printf("cmd [%s] remove\n", cmd.cCmd.c_str());
}

std::vector<std::string> Console::split(const std::string& cmd)
{
    std::istringstream iss(cmd);
    std::string buffer;

    std::vector<std::string> result;

    while (std::getline(iss, buffer, '#')) 
    {
        printf("split: %s\n", buffer.c_str());
        result.push_back(buffer);
    }

    return result;
}

void Console::task()
{
    // waiting usb-serial cable attached
    while(1)
    {
        char c;
        int ret = usb_serial_jtag_read_bytes(&c, 1, pdMS_TO_TICKS(100));
        if(ret > 0)
        {
            break;
        }
    }

    /* Tell vfs to use usb-serial-jtag driver */
    esp_vfs_usb_serial_jtag_use_driver();

    int stdin_fileno = fileno(stdin);
    mpHelp = std::make_unique<Help>();
    mpUartConsole = std::make_unique<UartByPass>();
    mpPyOcdConsole = std::make_unique<PyOcdIoConsole>();

    help();
    std::string cmd;
    while(mRun)
    {
        char c;
        if(read(stdin_fileno, &c, 1) > 0)
        {
            if(c == '\r')
            {
                ; // ignore
            }
            else if(c == '\n')
            {
                bool execute = false;
                if(cmd.size() > 0)
                {
                    putchar('\n');
                    putchar('\n');
                    auto args = split(cmd);
                    if(args.size() > 0)
                    {
                        std::lock_guard<std::recursive_mutex> lock(mMutex);
                        for(auto& pCmd : mCmdList)
                        {
                            if(pCmd->cCmd == args.at(0))
                            {
                                execute = true;
                                pCmd->excute(args);
                            }
                        }
                    }
                    cmd.clear();
                }
                if(not execute)
                {
                    help();
                }
            }
            else
            {
                cmd += c;
                putchar(c);
                fflush(stdout);
            }
        }
        else
        {
            vTaskDelay(100);
        }
    }
};
