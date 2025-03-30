#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "socket.hpp"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"

ServerSocket::ServerSocket(int port) :
    cServerSocket(socket(AF_INET, SOCK_STREAM, IPPROTO_IP)),
    cPort(port)
{
    xTaskCreatePinnedToCore(
                    (TaskFunction_t)task,   /* Function to implement the task */
                    "ServerSocket", /* Name of the task */
                    10000,      /* Stack size in words */
                    this,       /* Task input parameter */
                    22,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    1);  /* Core where the task should run */
}

ServerSocket::~ServerSocket()
{

}

void ServerSocket::acceptThread()
{
    ESP_LOGE("ServerSocket", "start");
    if (cServerSocket < 0)
    {
        ESP_LOGE("ServerSocket", "socket() failed %d", cServerSocket);
        return;
    }
    int opt = 1;
    setsockopt(cServerSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(cServerSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cPort);
    socklen_t sock_len = sizeof(addr);

    if (0 > (bind(cServerSocket, (const struct sockaddr*) &addr, sock_len)))
    {
        ESP_LOGE("ServerSocket", "Socket unable to bind: errno %d", errno);
        ESP_LOGE("ServerSocket", "bind() failed");
        return;
    }

    // Start listening
    if (0 > listen(cServerSocket, 1))
    {
        ESP_LOGE("ServerSocket", "listen() failed");
        return;
    }

    int accepted_socket = 0;
    while (0 < (accepted_socket = accept(cServerSocket, (struct sockaddr*) &addr, &sock_len)))
    {
        if(false == serverMain(accepted_socket))
        {
            return;
        }
    }

    if(accepted_socket)
    {
        close(accepted_socket);
    }
}

void ServerSocket::task(ServerSocket* pSock)
{
    pSock->acceptThread();
}
