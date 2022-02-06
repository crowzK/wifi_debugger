#include <stdint.h>

class ServerSocket
{
public: 
    ServerSocket(int port);
    virtual ~ServerSocket();

protected:
    const int cServerSocket;
    const int cPort;

    void acceptThread();

    virtual bool serverMain(int acceptSocekt) = 0;

    static void task(ServerSocket* pSock);
};
