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

#include <esp_log.h>
#include <errno.h>
#include "pyocd_io_socket.hpp"
#include "lwip/sockets.h"

PyOcdIoSocket::PyOcdIoSocket(RcvCallback&& callback) :
    PyOcdIo(std::move(callback)),
    ServerSocket(5555),
    socket(-1)
{

}

PyOcdIoSocket::~PyOcdIoSocket()
{

}

uint32_t PyOcdIoSocket::send(const char* message, uint32_t len)
{
    if(socket < 0)
    {
        return 0;
    }
    return lwip_send(socket, message, len, 0);
}

bool PyOcdIoSocket::serverMain(int accepted_socket)
{
    socket = accepted_socket;
    char rx_buffer[512];
    while(true)
    {
        const int len = lwip_recv(socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE("PyOcdIoSocket", "Error occurred during receiving: errno %d", (int)errno);
            return false;
        }
        else if (len == 0) 
        {
            ESP_LOGW("PyOcdIoSocket", "Connection closed");
        }
        else 
        {
            mRcvCallback(rx_buffer, len);
        }
    }
    return true;
}