#ifndef TELNET_HPP
#define TELNET_HPP

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class TelnetClient
{
public:
    TelnetClient(int socket);
    ~TelnetClient();

    //! \brief It handles the recv data from the telnet client
    //! \note it will send the data to the UART
    void tcpRxTask();

protected:
    const int socket;
    void * telnet;
    std::atomic<bool> run;

    std::thread txHandle;
    std::recursive_mutex mutex;

    //! \brief It handles the recv data from the UART
    //! \note it will send the data to the Telnet client
    void tcpTxTask();

    void sendUart(const char *buffer, size_t size);
    void sendToClient(const char *buffer, size_t size);
    static void eventHandler(void *telnet, void *ev, TelnetClient *tcpClient);
};

class Telnet
{
public:
    Telnet();
    ~Telnet();

protected:
    std::thread threadHandle;
    void thread();
};

void start_telnet();

#endif