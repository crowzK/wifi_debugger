
/*
Copyright (C) Yudoc Kim <craven@crowz.kr>
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <esp_log.h>
#include "spi_swd.hpp"

static const char * TAG = "SWDP";
static spi_device_handle_t txspi;
static spi_device_handle_t txspi_parity;
static spi_device_handle_t rxspi;

#define SPI_CLK 1000000

spi_device_interface_config_t readCfg = {
	.command_bits = 0,
	.mode = 1,          //SPI mode 3
	.clock_speed_hz = SPI_CLK,
	.spics_io_num = -1,
	.flags = SPI_DEVICE_3WIRE | SPI_DEVICE_POSITIVE_CS | SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_BIT_LSBFIRST,
	.queue_size = 1,
};

spi_device_interface_config_t writeCfg = {
	.command_bits = 0,
	.mode = 0,          //SPI mode 3                                                                                                                                                                        
	.clock_speed_hz = SPI_CLK,
	.spics_io_num = -1,
	.flags = SPI_DEVICE_3WIRE | SPI_DEVICE_POSITIVE_CS | SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_BIT_LSBFIRST,
	.queue_size = 1,
};

spi_device_interface_config_t writeCfgParity = {
	.command_bits = 2,
	.address_bits = 32,
	.mode = 0,          //SPI mode 3
	.clock_speed_hz = SPI_CLK,
	.spics_io_num = -1,
	.flags = SPI_DEVICE_3WIRE | SPI_DEVICE_POSITIVE_CS | SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_BIT_LSBFIRST,
	.queue_size = 1,
};

static inline uint32_t swdptap_seq_in(int ticks)
{
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_RXDATA,
        .cmd = 0,
        .length = (size_t)ticks,
		.rxlength = (size_t)ticks,
    };
    spi_device_transmit(rxspi, &t);

	uint32_t data = *((uint32_t*)t.rx_data);
	return data;
}

static inline bool swdptap_seq_in_parity(uint32_t *ret, int ticks)
{
	uint64_t buffer = 0;
	spi_transaction_t t = {
        .cmd = 0,
        .length = (size_t)(ticks + 3),
		.rxlength = (size_t)(ticks + 3),
		.rx_buffer = &buffer,
    };
    spi_device_transmit(rxspi, &t);
	int parity = (buffer >> 32) & 1;
	*ret = (uint32_t)(buffer);	
	//ESP_LOGI(TAG, "%s %d %X", __func__, ticks, *ret);
	//uint8_t* data = (uint8_t*)&buffer;
	//ESP_LOGI(TAG, "%s %d %02X%02X%02X%02X%02X%02X%02X%02X  %llX", __func__, ticks, 
	//	data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], buffer);
	//ESP_LOGI(TAG, "%s %d %d %d %X", __func__, ticks, parity,  __builtin_popcount(*ret), (uint32_t)(buffer >> 32));
	parity = parity + __builtin_popcount(*ret);
	return parity & 1;
}

static inline void swdptap_seq_out(uint32_t MS, int ticks)
{
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .cmd = 0,
        .length = (size_t)ticks,
		.rx_data = {}
    };
	*(uint32_t*)t.tx_data = MS;
	//ESP_LOGI(TAG, "%s %d %X", __func__, ticks, MS);
    spi_device_transmit(txspi, &t);
}

static inline void swdptap_seq_out_parity(uint32_t MS, int ticks)
{
	//ESP_LOGI(TAG, "%s", __func__);
	int parity = __builtin_popcount(MS);
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .cmd = 0,
		.addr = MS,
        .length = 1,
		.tx_data = {},
    };
	t.tx_data[0] = parity & 1;
	//ESP_LOGI(TAG, "%s %d %X", __func__, ticks, MS);
    spi_device_transmit(txspi_parity, &t);
}

SpiSwd::SpiSwd()
{
    esp_err_t ret;
    ESP_LOGI(TAG, "Initializing bus SPI%d...", VSPI_HOST+1);
    spi_bus_config_t buscfg={
        .mosi_io_num = cPinSwDio,
        .miso_io_num = -1,
        .sclk_io_num = cPinSwClk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    //Initialize the SPI bus
    ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);
    spi_bus_add_device(VSPI_HOST, &writeCfg, &txspi);
    spi_bus_add_device(VSPI_HOST, &readCfg, &rxspi);
    spi_bus_add_device(VSPI_HOST, &writeCfgParity, &txspi_parity);
    gpio_set_pull_mode(cPinSwDio, GPIO_PULLUP_ONLY);
}

uint32_t SpiSwd::sequence(uint64_t data, uint8_t bitLength)
{
    uint32_t d = data & 0xffffffff;
    swdptap_seq_out(d, std::min(bitLength, (uint8_t)32));
    if(bitLength > 32)
    {
        bitLength -= 32;
        uint32_t d = (data >> 8) & 0xffffffff;
        swdptap_seq_out(d, bitLength);
    }
    return 0;
}

SpiSwd::Response SpiSwd::write(Cmd cmd, uint32_t data)
{
    Response ack;
    do
    {
        swdptap_seq_out(getCmd(cmd), 8);
        ack = static_cast<Response>(swdptap_seq_in(3));
        swdptap_seq_out_parity(data, 32);
    } while(ack == Response::Wait);
    return ack;
}

SpiSwd::Response SpiSwd::read(Cmd cmd, uint32_t& data)
{
    Response ack;
    do
    {
        swdptap_seq_out(getCmd(cmd), 8);
        ack = static_cast<Response>(swdptap_seq_in(3));
        swdptap_seq_in_parity(&data, 32);
    } while(ack == Response::Wait);
    return ack;
}
