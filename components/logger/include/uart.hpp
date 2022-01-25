#ifndef UASRT_HPP
#define UASRT_HPP

#include <vector>
#include <thread>
#include <atomic>
#include "blocking_queue.hpp"
#include "debug_msg_handler.hpp"
#include "task.hpp"

class UartTx : public Client
{
public:
    UartTx(int uartPortNum);
    ~UartTx() = default;
    
protected:
    const int cUartNum;
    bool write(const std::vector<uint8_t>& msg);
};

class UartRx : public Task
{
public:
    UartRx(int uartPortNum);
    ~UartRx() = default;
    
protected:
    const int cUartNum;
    void task() override;
};

class UartService
{
public:
    struct Config
    {
        int baudRate;
        int uartNum;
    };

    static UartService& get();
    void init(const Config& cfg);
    const Config& getCfg() const { return mConfig; };

protected:
    Config mConfig;
    std::unique_ptr<UartRx> pUartRx;
    std::unique_ptr<UartTx> pUartTx;

    UartService();
};

#endif //UASRT_HPP
