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

#include "libtelnet.h"
#include "esp_log.h"
#include "msg_buffer.h"

static QueueHandle_t uartTxQ ;
static QueueHandle_t uartRxQ ;

#define SOCKET int
#define MAX_USERS 64
#define LINEBUFFER_SIZE 256

static const char *TAG = "telnet";

static const telnet_telopt_t telopts[] = {
	{ TELNET_TELOPT_COMPRESS2,	TELNET_WILL, TELNET_DONT },
	{ -1, 0, 0 }
};

struct user_t {
	char *name;
	SOCKET sock;
	telnet_t *telnet;
	char linebuf[256];
	int linepos;
};

static struct user_t users[MAX_USERS];

static void linebuffer_push(char *buffer, size_t size, int *linepos,
		char ch, void (*cb)(const char *line, size_t overflow, void *ud),
		void *ud) {

	/* CRLF -- line terminator */
	if (ch == '\n' && *linepos > 0 && buffer[*linepos - 1] == '\r') {
		/* NUL terminate (replaces \r in buffer), notify app, clear */
		buffer[*linepos - 1] = 0;
		cb(buffer, 0, ud);
		*linepos = 0;

	/* CRNUL -- just a CR */
	} else if (ch == 0 && *linepos > 0 && buffer[*linepos - 1] == '\r') {
		/* do nothing, the CR is already in the buffer */

	/* anything else (including technically invalid CR followed by
	 * anything besides LF or NUL -- just buffer if we have room
	 * \r
	 */
	} else if (*linepos != (int)size) {
		buffer[(*linepos)++] = ch;

	/* buffer overflow */
	} else {
		/* terminate (NOTE: eats a byte), notify app, clear buffer */
		buffer[size - 1] = 0;
		cb(buffer, size - 1, ud);
		*linepos = 0;
	}
}

static void _send_uart(const char *from, const char *msg) 
{
    MsgBuffer msgBuff;
    msgBuff.len = strlen(msg);
    if(msgBuff.len == 0)
    {
        return;
    }

    msgBuff.pMessage = (uint8_t*) malloc(msgBuff.len);
    memcpy((char*)msgBuff.pMessage, msg, msgBuff.len);

    if(pdFALSE == xQueueSendToBack(uartTxQ , &msgBuff, 10))
    {
        free(msgBuff.pMessage);
    }
}

static void _message(const char *from, const char *msg) {
	int i;
	for (i = 0; i != MAX_USERS; ++i) {
		if (users[i].sock != -1) {
			telnet_printf(users[i].telnet, "%s\n", msg);
		}
	}
}

static void _send(SOCKET sock, const char *buffer, size_t size) {
	int rs;

	/* ignore on invalid socket */
	if (sock == -1)
		return;

	/* send data */
	while (size > 0) {
		if ((rs = send(sock, buffer, (int)size, 0)) == -1) {
			if (errno != EINTR && errno != ECONNRESET) {
				ESP_LOGE(TAG, "send() failed: %s\n", strerror(errno));
			} else {
				return;
			}
		} else if (rs == 0) {
			ESP_LOGE(TAG, "send() unexpectedly returned 0\n");
		}

		/* update pointer and size to see if we've got more to send */
		buffer += rs;
		size -= rs;
	}
}

/* process input line */
static void _online(const char *line, size_t overflow, void *ud) {
	struct user_t *user = (struct user_t*)ud;
	int i;

	(void)overflow;

	/* if the user has no name, this is his "login" */
	if (user->name == 0) {
		/* must not be empty, must be at least 32 chars */
		if (strlen(line) == 0 || strlen(line) > 32) {
			telnet_printf(user->telnet, "Invalid name.\nEnter name: ");
			return;
		}

		/* must not already be in use */
		for (i = 0; i != MAX_USERS; ++i) {
			if (users[i].name != 0 && strcmp(users[i].name, line) == 0) {
				telnet_printf(user->telnet, "Name in use.\nEnter name: ");
				return;
			}
		}

		/* keep name */
		user->name = strdup(line);
		telnet_printf(user->telnet, "Welcome, %s!\n", line);
		return;
	}

	/* just a message -- send to all users */
	_send_uart(user->name, line);
}

static void _input(struct user_t *user, const char *buffer,
		size_t size) {
	unsigned int i;
	for (i = 0; user->sock != -1 && i != size; ++i)
		linebuffer_push(user->linebuf, sizeof(user->linebuf), &user->linepos,
				(char)buffer[i], _online, user);
}

static void _event_handler(telnet_t *telnet, telnet_event_t *ev,
		void *user_data) {
	struct user_t *user = (struct user_t*)user_data;

	switch (ev->type) {
	/* data received */
	case TELNET_EV_DATA:
		_input(user, ev->data.buffer, ev->data.size);
		telnet_negotiate(telnet, TELNET_WONT, TELNET_TELOPT_ECHO);
		telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
		break;
	/* data must be sent */
	case TELNET_EV_SEND:
		_send(user->sock, ev->data.buffer, ev->data.size);
		break;
	/* enable compress2 if accepted by client */
	case TELNET_EV_DO:
		if (ev->neg.telopt == TELNET_TELOPT_COMPRESS2)
			telnet_begin_compress2(telnet);
		break;
	/* error */
	case TELNET_EV_ERROR:
		close(user->sock);
		user->sock = -1;
		if (user->name != 0) {
			_message(user->name, "** HAS HAD AN ERROR **");
			free(user->name);
			user->name = 0;
		}
		telnet_free(user->telnet);
		break;
	default:
		/* ignore */
		break;
	}
}

static void rcv_uart(void * param)
{
    MsgBuffer msg = {};
    while(1)
    {
        if(pdTRUE == xQueueReceive(uartRxQ , &msg, 100))
        {
            msg.pMessage[msg.len] = 0;
            _message("uart", (char*)msg.pMessage);
            free(msg.pMessage);
        }
    }
}

static void telnet(void * param) 
{
	char buffer[512];
	short listen_port;
	SOCKET listen_sock;
	SOCKET client_sock;
	int rs;
	int i;
	struct sockaddr_in addr;
	socklen_t addrlen;
	struct pollfd pfd[MAX_USERS + 1];


	/* initialize data structures */
	memset(&pfd, 0, sizeof(pfd));
	memset(users, 0, sizeof(users));
	for (i = 0; i != MAX_USERS; ++i)
		users[i].sock = -1;

	/* parse listening port */
	listen_port = 23;

	/* create listening socket */
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		ESP_LOGE(TAG, "socket() failed: %s\n", strerror(errno));
        return;
	}

	/* reuse address option */
	rs = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&rs, sizeof(rs));

	/* bind to listening addr/port */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(listen_port);
	if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		ESP_LOGE(TAG, "bind() failed: %s\n", strerror(errno));
		close(listen_sock);
        return;
	}

	/* listen for clients */
	if (listen(listen_sock, 5) == -1) {
		ESP_LOGE(TAG, "listen() failed: %s\n", strerror(errno));
		close(listen_sock);
        return;
	}

	ESP_LOGI(TAG, "LISTENING ON PORT %d\n", listen_port);

	/* initialize listening descriptors */
	pfd[MAX_USERS].fd = listen_sock;
	pfd[MAX_USERS].events = POLLIN;

	/* loop for ever */
	for (;;) {
		/* prepare for poll */
		for (i = 0; i != MAX_USERS; ++i) {
			if (users[i].sock != -1) {
				pfd[i].fd = users[i].sock;
				pfd[i].events = POLLIN;
			} else {
				pfd[i].fd = -1;
				pfd[i].events = 0;
			}
		}

		/* poll */
		rs = poll(pfd, MAX_USERS + 1, -1);
		if (rs == -1 && errno != EINTR) {
			ESP_LOGE(TAG, "poll() failed: %s\n", strerror(errno));
			close(listen_sock);
			return;
		}

		/* new connection */
		if (pfd[MAX_USERS].revents & (POLLIN | POLLERR | POLLHUP)) {
			/* acept the sock */
			addrlen = sizeof(addr);
			if ((client_sock = accept(listen_sock, (struct sockaddr *)&addr,
					&addrlen)) == -1) {
				ESP_LOGE(TAG, "accept() failed: %s\n", strerror(errno));
			    return;
			}

			ESP_LOGI(TAG, "Connection received.\n");

			/* find a free user */
			for (i = 0; i != MAX_USERS; ++i)
				if (users[i].sock == -1)
					break;
			if (i == MAX_USERS) {
				ESP_LOGI(TAG, "  rejected (too many users)\n");
				_send(client_sock, "Too many users.\r\n", 14);
				close(client_sock);
			}

			/* init, welcome */
			users[i].sock = client_sock;
			users[i].telnet = telnet_init(telopts, _event_handler, 0,
					&users[i]);
			telnet_negotiate(users[i].telnet, TELNET_WILL,
					TELNET_TELOPT_COMPRESS2);
			telnet_printf(users[i].telnet, "Enter name: ");

			telnet_negotiate(users[i].telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
		}

		/* read from client */
		for (i = 0; i != MAX_USERS; ++i) {
			/* skip users that aren't actually connected */
			if (users[i].sock == -1)
				continue;

			if (pfd[i].revents & (POLLIN | POLLERR | POLLHUP)) {
				if ((rs = recv(users[i].sock, buffer, sizeof(buffer), 0)) > 0) {
					telnet_recv(users[i].telnet, buffer, rs);
				} else if (rs == 0) {
					ESP_LOGI(TAG, "Connection closed.\n");
					close(users[i].sock);
					users[i].sock = -1;
					if (users[i].name != 0) {
						_message(users[i].name, "** HAS DISCONNECTED **");
						free(users[i].name);
						users[i].name = 0;
					}
					telnet_free(users[i].telnet);
				} else if (errno != EINTR) {
					ESP_LOGE(TAG, "recv(client) failed: %s\n",
							strerror(errno));
					exit(1);
				}
			}
		}
	}
}

void start_telnet(QueueHandle_t _txQ, QueueHandle_t _rxQ)
{
    uartTxQ  = _txQ;
    uartRxQ  = _rxQ;
    xTaskCreate(telnet, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
    xTaskCreate(rcv_uart, "rcv_uart", 4096, (void*)AF_INET, 5, NULL);
}
