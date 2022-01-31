#include "pyocd_probe.hpp"

void PyOcdProbe::connect()
{

}

void PyOcdProbe::disconnect()
{

}

void PyOcdProbe::setClock(uint32_t frequency)
{

}

void PyOcdProbe::reset()
{

}

void PyOcdProbe::assert_reset(bool asserted)
{

}

void PyOcdProbe::is_reset_asserted()
{

}

void PyOcdProbe::flush()
{

}


void PyOcdProbe::get_memory_interface_for_ap(uint32_t ap_address_version, uint32_t ap_address_address)
{

}


void PyOcdProbe::read_dp(uint32_t addr)
{

}

void PyOcdProbe::write_dp(uint32_t addr, uint32_t data)
{

}


void PyOcdProbe::read_ap(uint32_t addr)
{

}

void PyOcdProbe::write_ap(uint32_t addr, uint32_t data)
{

}


void PyOcdProbe::read_ap_multiple(uint32_t addr, uint32_t count)
{

}

void PyOcdProbe::write_ap_multiple(uint32_t addr, std::vector<uint32_t> values)
{

}


void PyOcdProbe::read_mem(uint32_t addr, uint32_t xfer_size)
{

}

void PyOcdProbe::write_mem(uint32_t addr, uint32_t value, uint32_t xfer_size)
{

}


void PyOcdProbe::read_block32(uint32_t addr, uint32_t count)
{

}

void PyOcdProbe::write_block32(uint32_t addr, const std::vector<uint32_t>& data)
{

}


void PyOcdProbe::read_block8()
{

}

void PyOcdProbe::write_block8(uint32_t addr, const std::vector<uint8_t>& data)
{

}
