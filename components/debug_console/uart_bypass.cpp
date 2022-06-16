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

#include "uart_bypass.hpp"

UartByPass::UartByPass() :
    Client(DebugMsgRx::create(), INT32_MAX - 1),
    Cmd("uart")
{

}

bool UartByPass::write(const std::vector<uint8_t>& msg)
{
    printf("%.*s", msg.size(), (char*)msg.data());
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
        char c;
        if(read(stdin_fileno, &c, 1))
        {
            if(c == 2)
            {
                printf("escape cmd enter\n");
                break;
            }
            if(c == '\n')
            {
                c = '\r';
            }
            std::vector<uint8_t> tx;
            tx.push_back(c);
            DebugMsgTx::create().write(tx);
        }
    }
    return true;
}