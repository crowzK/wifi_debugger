#include <stdint.h>
#include <thread>

class ServerSocket
{
public: 
    ServerSocket(int port);
    virtual ~ServerSocket();

protected:
    const int cServerSocket;
    const int cPort;
    std::thread mServerThread;

    void acceptThread();

    virtual bool serverMain(int acceptSocekt) = 0;
};
