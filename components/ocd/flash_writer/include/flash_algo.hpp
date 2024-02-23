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

#pragma once

#include <stdint.h>
#include <string>
#include "FlashOS.h"

class FlashAlgo
{
public:
	struct TargerRamInfo
	{
		uint32_t ramStartAddr;
		uint32_t ramEndAddr;
		uint32_t programBufferAddr;	// when it calls `programPage`, the debugger write data to this memory
		uint32_t programBufferSize;
	};

	FlashAlgo(const std::string& algorithmPath, TargerRamInfo targetRam);
	~FlashAlgo();

	int blankCheck(unsigned long adr, unsigned long sz, unsigned char pat);
	int eraseChip(void);
	int eraseSector(unsigned long adr);
	int init(unsigned long adr, unsigned long clk, unsigned long fnc);
	int unInit(unsigned long fnc);
	int programPage(unsigned long adr, unsigned long sz, unsigned char *buf);
	unsigned long verify(unsigned long adr, unsigned long sz, unsigned char *buf);

private:
	struct FlashAlgoFuncLUT
	{
		// CMSIS DAP
		const uint32_t  init;
		const uint32_t  uninit;
		const uint32_t  erase_chip;
		const uint32_t  erase_sector;
		const uint32_t  program_page;
		const uint32_t  verify;

		// not for the CMSIS_DAP
		const uint32_t  program_pages;
		const uint32_t  get_aes_key;
	};
	const TargerRamInfo cTargerRamInfo;
	const FlashAlgoFuncLUT cFuncLut;
	FlashAlgoFuncLUT createLut(const std::string& algorithmPath);
};