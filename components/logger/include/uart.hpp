#ifndef UASRT_HPP
#define UASRT_HPP

#include <vector>
#include <thread>
#include <atomic>
#include "blocking_queue.hpp"

class UartThread
{
public:
    void start(BlockingQueue<std::vector<uint8_t>>&);
    void stop();
    bool isRun() const;
    
protected:
    const char* cName;
    std::atomic<bool> run;
    std::thread threadHandle;
    
    UartThread(const char* cName);
    virtual ~UartThread();
    virtual void thread(BlockingQueue<std::vector<uint8_t>>& queue) = 0;
};

class UartTx : public UartThread
{
public:
    UartTx(int uartPortNum);
    ~UartTx() = default;
    
protected:
    const int cUartNum;
    void thread(BlockingQueue<std::vector<uint8_t>>& queue);
};

class UartRx : public UartThread
{
public:
    UartRx(int uartPortNum);
    ~UartRx() = default;
    
protected:
    const int cUartNum;
    void thread(BlockingQueue<std::vector<uint8_t>>& queue);
};

class UartService
{
public:
    UartService(int uartPortNum);
    ~UartService();

    bool isRun() const;
    
    void start(int baudRate, BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ);
    void stop();

    int getPort() const {return cUartNum;};
    int getBaudRate() const {return mBaudRate;};

protected:
    int mBaudRate;
    const int cUartNum;
    UartTx txTask;
    UartRx rxTask;
};

void start_uart_service(BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ);

#endif //UASRT_HPP
