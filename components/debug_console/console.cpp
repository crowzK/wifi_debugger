/* Wi-Fi Provisioning Manager Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
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
#include "log_console_out.hpp"

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

void Console::init()
{
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_usb_serial_jtag_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_usb_serial_jtag_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Enable non-blocking mode on stdin and stdout */
    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);

    usb_serial_jtag_driver_config_t usb_serial_jtag_config
    {
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024
    };

    /* Install USB-SERIAL-JTAG driver for interrupt-driven reads and writes */
    usb_serial_jtag_driver_install(&usb_serial_jtag_config);

    /* Tell vfs to use usb-serial-jtag driver */
    esp_vfs_usb_serial_jtag_use_driver();
}

void Console::help()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    printf("\n\n---------------------------------\n");
    printf("Console cmd help\n");
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

    while (std::getline(iss, buffer, ' ')) 
    {
        result.push_back(buffer);
    }

    return result;
}

void Console::task()
{
    int stdin_fileno = fileno(stdin);
    mpHelp = std::make_unique<Help>();
    mpUartConsole = std::make_unique<LogConsoleOut>();
    help();
    std::string cmd;
    while(mRun)
    {
        char c;
        if(read(stdin_fileno, &c, 1))
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
    }
};