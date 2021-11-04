/*
 * Sean Middleditch
 * sean@sourcemud.org
 *
 * The author or authors of this code dedicate any and all copyright interest
 * in this code to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and successors. We
 * intend this dedication to be an overt act of relinquishment in perpetuity of
 * all present and future rights to this code under copyright law. 
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "libtelnet.h"
#include "logger.hpp"
#include "esp_log.h"
#include "uart.hpp"

#define SOCKET int
#define MAX_USERS 64
#define LINEBUFFER_SIZE 256

static BlockingQueue<std::vector<uint8_t>> uartTx(10);
static BlockingQueue<std::vector<uint8_t>> uartRx(10);

static const char *TAG = "logger";

////////////////////////////////////////////////////////////////////
// TelnetClient
////////////////////////////////////////////////////////////////////
TelnetClient::TelnetClient(int socket) :
	socket(socket),
	run(true),
	txHandle(std::thread([this]{ tcpTxTask(); }))
{
	static const telnet_telopt_t telopts[] = 
	{
		{ TELNET_TELOPT_COMPRESS2,	TELNET_WILL, TELNET_DONT },
		{ -1, 0, 0 }
	};

	/* init, welcome */
	telnet = telnet_init(telopts, (telnet_event_handler_t)eventHandler, 0, this);
	telnet_negotiate(static_cast<telnet_t*>(telnet), TELNET_WILL, TELNET_TELOPT_COMPRESS2);
	telnet_negotiate(static_cast<telnet_t*>(telnet), TELNET_WILL, TELNET_TELOPT_ECHO);
	telnet_printf(static_cast<telnet_t*>(telnet), "WIFI Debugger\n");
}

TelnetClient::~TelnetClient()
{
	run = false;
	txHandle.join();
	telnet_free(static_cast<telnet_t*>(telnet));
	shutdown(socket, 0);
	close(socket);
}

void TelnetClient::tcpRxTask()
{
	char buffer[512];
	while(run)
	{
		int rs;
		if ((rs = recv(socket, buffer, sizeof(buffer), 0)) > 0) 
		{
			std::lock_guard<std::recursive_mutex> guard(mutex);
			telnet_recv(static_cast<telnet_t*>(telnet), buffer, rs);
		}
		else
		{
			run = false;
		}
	}
}

void TelnetClient::tcpTxTask()
{
	std::vector<uint8_t> strBuffer;
	strBuffer.reserve(512);
	while(run)
	{
		std::vector<uint8_t> rx;
		if(uartRx.pop(rx, std::chrono::milliseconds(100)) and rx.size())
		{
			for(int i = 0; i < rx.size(); i++)
			{
				uint8_t d = rx[i];
				if(d == '\r')
				{
					continue;
				}
				if(d == '\n')
				{
					strBuffer.push_back('\r');
					strBuffer.push_back('\n');
					struct timeval tv_now;
					gettimeofday(&tv_now, NULL);

					char buffer[40];
					int m = tv_now.tv_sec / 60;
					int s = tv_now.tv_sec % 60;
					int ms = tv_now.tv_usec / 1000;

					sprintf(buffer, "[%02d:%02d:%03d] ", m, s, ms);
					std::lock_guard<std::recursive_mutex> guard(mutex);
					telnet_send(static_cast<telnet_t*>(telnet), buffer, strlen(buffer));
					telnet_send(static_cast<telnet_t*>(telnet), (char*)strBuffer.data(), strBuffer.size());
					strBuffer.clear();
					continue;
				}
				strBuffer.push_back(d);
				if(strBuffer.size() >= 512)
				{
					strBuffer.clear();
				}
			}
		}
	}
}

void TelnetClient::sendUart(const char *buffer, size_t size)
{
    if(size == 0)
    {
        return;
    }
	std::vector<uint8_t> tx;
	if(buffer[0] == '`')
	{
		switch(buffer[1])
		{
		case 'c':
			tx.push_back(3);
			uartTx.push(tx, std::chrono::milliseconds(100));
			return;
		case 'r':
			tx.push_back(18);
			uartTx.push(tx, std::chrono::milliseconds(100));
			return;
		case 'u':
			tx.push_back(21);
			uartTx.push(tx, std::chrono::milliseconds(100));
			return;
		}
	}
	tx.resize(size);
    memcpy(tx.data(), buffer, size);
	uartTx.push(tx, std::chrono::milliseconds(100));
}

void TelnetClient::sendToClient(const char *buffer, size_t size)
{
	int rs;

	/* send data */
	while (size > 0) 
	{
		if ((rs = send(socket, buffer, (int)size, 0)) == -1) 
		{
			if (errno != EINTR && errno != ECONNRESET) 
			{
				ESP_LOGE(TAG, "send() failed: %s\n", strerror(errno));
			} 
			else 
			{
				return;
			}
		} 
		else if (rs == 0) 
		{
			ESP_LOGE(TAG, "send() unexpectedly returned 0\n");
		}

		/* update pointer and size to see if we've got more to send */
		buffer += rs;
		size -= rs;
	}
}

void TelnetClient::eventHandler(void *telnet, void *ev, TelnetClient *tcpClient)
{
	TelnetClient& telnetClient = *tcpClient;
	telnet_event_t& event = *static_cast<telnet_event_t*>(ev);

	switch (event.type) 
	{
	/* data received */
	case TELNET_EV_DATA:
		telnetClient.sendUart(event.data.buffer, event.data.size);
		telnet_negotiate(static_cast<telnet_t*>(telnet), TELNET_WONT, TELNET_TELOPT_ECHO);
		telnet_negotiate(static_cast<telnet_t*>(telnet), TELNET_WILL, TELNET_TELOPT_ECHO);
		break;
	/* data must be sent */
	case TELNET_EV_SEND:
		telnetClient.sendToClient(event.data.buffer, event.data.size);
		break;
	/* enable compress2 if accepted by client */
	case TELNET_EV_DO:
		if (event.neg.telopt == TELNET_TELOPT_COMPRESS2)
		{
			telnet_begin_compress2(static_cast<telnet_t*>(telnet));
		}
		break;
	/* error */
	case TELNET_EV_ERROR:
		close(telnetClient.socket);
		telnet_free(static_cast<telnet_t*>(telnet));
		break;
	default:
		/* ignore */
		break;
	}
}

////////////////////////////////////////////////////////////////////
// Telnet
////////////////////////////////////////////////////////////////////
Telnet::Telnet() :
	threadHandle(std::thread([this]{ thread(); }))
{

}

Telnet::~Telnet()
{

}

void Telnet::thread()
{
    char addr_str[128];
    int ip_protocol = IPPROTO_IP;
    int keepAlive = 1;
    int keepIdle = 5;
    int keepInterval = 5;
    int keepCount = 3;
    struct sockaddr_storage dest_addr;

	struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
	dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
	dest_addr_ip4->sin_family = AF_INET;
	dest_addr_ip4->sin_port = htons(23);

    int listen_sock = socket(AF_INET, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) 
	{
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) 
	{
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", AF_INET);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", 23);

    err = listen(listen_sock, 1);
    if (err != 0) 
	{
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) 
	{

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) 
		{
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

		TelnetClient client(sock);
		client.tcpRxTask();
    }

CLEAN_UP:
    close(listen_sock);
}

////////////////////////////////////////////////////////////////////
// start_telnet
////////////////////////////////////////////////////////////////////
void start_telnet()
{
	static Telnet _telnet;
	start_uart_service(uartTx, uartRx);
}