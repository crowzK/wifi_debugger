#ifndef UASRT_HPP
#define UASRT_HPP

#include <vector>
#include <thread>
#include <atomic>
#include "../../blocking_queue/include/blocking_queue.hpp"

class UartService
{
public:
    void start(BlockingQueue<std::vector<uint8_t>>&);
    void stop();
    bool isRun() const;
    
protected:
    const char* cName;
    std::atomic<bool> run;
    std::thread threadHandle;
    
    UartService(const char* cName);
    virtual ~UartService();
    virtual void thread(BlockingQueue<std::vector<uint8_t>>& queue) = 0;
};

class UartTx : public UartService
{
public:
    UartTx();
    ~UartTx() = default;
    
protected:
    void thread(BlockingQueue<std::vector<uint8_t>>& queue);
};

class UartRx : public UartService
{
public:
    UartRx();
    ~UartRx() = default;
    
protected:
    void thread(BlockingQueue<std::vector<uint8_t>>& queue);
};

void start_uart_service(BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ);

#endif //UASRT_HPP