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
#include <array>
#include <vector>
#include <memory>
#include "FlashOS.h"
#include "swd.hpp"

class FlashAlgo
{
public:
	struct RamInfo
	{
		uint32_t ramStartAddr;
		uint32_t ramSize;
	};

	FlashAlgo(const std::string& algorithmPath, const RamInfo& targetRam);
	~FlashAlgo();

	bool blankCheck(unsigned long adr, unsigned long sz, unsigned char pat);
	bool eraseChip(void);
	bool eraseSector(unsigned long adr);
	bool init(unsigned long adr, unsigned long clk, unsigned long fnc);
	bool unInit(unsigned long fnc);
	bool programPage(unsigned long adr, unsigned long sz, unsigned char *buf);
	unsigned long verify(unsigned long adr, unsigned long sz, unsigned char *buf);

private:
	enum FuncEntry
	{
		eInit,
		eUnInit,
		eEraseChip,
		eEraseSector,
		eProgramPage,
		eVerify,

		eProgramPages,
		eSetPublicKey,
		eGetAesKey,
		eSize
	};

	using FlashAlgoFuncLUT = std::array<uint32_t, FuncEntry::eSize>;
	struct Program
	{
		uint32_t startAddr;
		std::vector<uint8_t> data;
	};
	struct WorkRamInfo
	{
		RamInfo targetSramInfo;
		RamInfo stackInfo;
		RamInfo programMemInfo;
	};
	struct LoaderDescription
	{
		uint16_t version;    // Version Number and Architecture
		uint8_t devName[128];    // Device Name and Description
		uint16_t devType;    // Device Type: ONCHIP, EXT8BIT, EXT16BIT, ...
		uint32_t DevAdr;    // Default Device Start Address
		uint32_t szDev;    // Total Size of Device
		uint32_t szPage;    // Programming Page Size
		uint32_t Res;    // Reserved for future Extension
		uint32_t valEmpty;    // Content of Erased Memory

		uint32_t    toProg;    // Time Out of Program Page Function
		uint32_t   toErase;    // Time Out of Erase Sector Function

		struct FlashSectors
		{
			uint32_t szSector;
			uint32_t addrSector;
		};
		FlashSectors sectors[512];
	}__attribute__((packed));
	
	struct FlashLoaderInfo
	{
		FlashAlgoFuncLUT lut;
		WorkRamInfo workRamInfo;
		Swd::ProgramSysCall sysCallInfo;
		std::vector<Program> loader;
		LoaderDescription loaderDesc;
	};

	static std::array<const char* const,FuncEntry::eSize> cFuncStr;
	const FlashLoaderInfo cLoader;
    std::unique_ptr<Swd> pSwd;

	FlashLoaderInfo loadLoader(const std::string& algorithmPath, const RamInfo& targetRam);
	WorkRamInfo createRamInfo(const RamInfo& targetRam);

	uint32_t writeSwd(uint32_t addr, const uint8_t* buffer, uint32_t bufferSize);
};