#include "pyocd_server.hpp"
#include "gpio_swd.hpp"

PyOcdServer::PyOcdServer()
{

}

void PyOcdServer::start(bool start)
{

}

void PyOcdServer::connect()
{

}

void PyOcdServer::disconnect()
{

}

void PyOcdServer::setClock(uint32_t frequency)
{

}

void PyOcdServer::reset()
{

}

void PyOcdServer::assert_reset(bool asserted)
{

}

void PyOcdServer::is_reset_asserted()
{

}

void PyOcdServer::flush()
{

}


void PyOcdServer::get_memory_interface_for_ap(uint32_t ap_address_version, uint32_t ap_address_address)
{

}


void PyOcdServer::read_dp(uint32_t addr)
{

}

void PyOcdServer::write_dp(uint32_t addr, uint32_t data)
{

}


void PyOcdServer::read_ap(uint32_t addr)
{

}

void PyOcdServer::write_ap(uint32_t addr, uint32_t data)
{

}


void PyOcdServer::read_ap_multiple(uint32_t addr, uint32_t count)
{

}

void PyOcdServer::write_ap_multiple(uint32_t addr, std::vector<uint32_t> values)
{

}


void PyOcdServer::read_mem(uint32_t addr, uint32_t xfer_size)
{

}

void PyOcdServer::write_mem(uint32_t addr, uint32_t value, uint32_t xfer_size)
{

}


void PyOcdServer::read_block32(uint32_t addr, uint32_t count)
{

}

void PyOcdServer::write_block32(uint32_t addr, const std::vector<uint32_t>& data)
{

}


void PyOcdServer::read_block8()
{

}

void PyOcdServer::write_block8(uint32_t addr, const std::vector<uint8_t>& data)
{

}

