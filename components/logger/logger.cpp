#include "logger.hpp"
#include "uart.hpp"
#include "logger_web.hpp"

static BlockingQueue<std::vector<uint8_t>> uartTx(10);
static BlockingQueue<std::vector<uint8_t>> uartRx(10);

////////////////////////////////////////////////////////////////////
// start_telnet
////////////////////////////////////////////////////////////////////
void start_telnet()
{
	start_uart_service(uartTx, uartRx);
	start_logger_web(uartTx, uartRx);
}