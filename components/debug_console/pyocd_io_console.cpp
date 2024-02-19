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

#include "pyocd_io_console.hpp"

//-------------------------------------------------------------------
// PyOcdIoConsole
//-------------------------------------------------------------------
PyOcdIoConsole::PyOcdIoConsole() :
    PyOcdIo(nullptr),
    Cmd("pyocd"),
    mPyOcdParser(*this)
{

}

PyOcdIoConsole::~PyOcdIoConsole()
{

}

uint32_t PyOcdIoConsole::send(const char* message, uint32_t len)
{
    return fwrite(message, 1, len, stdout);
}

std::string PyOcdIoConsole::help()
{
    return std::string("PyOCD Server Using Console");
}

bool PyOcdIoConsole::excute(const std::vector<std::string>& args)
{
    int stdin_fileno = fileno(stdin);
    printf("execute\n");
    while(1)
    {
        const uint32_t len = read(stdin_fileno, mRxBuff, cBufSize - 1);
        mRxBuff[len] = 0;
        if(len == 2)
        {
            if(mRxBuff[0] == 2)
            {
                printf("escape cmd enter\n");
                break;
            }  
        }
        if(len)
        {
            mPyOcdParser.parse(mRxBuff, len);
        }
    }
    return true;
}
