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

	int blankCheck(unsigned long adr, unsigned long sz, unsigned char pat);
	int eraseChip(void);
	int eraseSector(unsigned long adr);
	int init(unsigned long adr, unsigned long clk, unsigned long fnc);
	int unInit(unsigned long fnc);
	int programPage(unsigned long adr, unsigned long sz, unsigned char *buf);
	unsigned long verify(unsigned long adr, unsigned long sz, unsigned char *buf);

private:
	static constexpr uint32_t cStackSize = 1024;
	enum FuncEntry
	{
		eInit,
		eUnInit,
		eEraseChip,
		eEraseSector,
		eProgramPage,
		eVerify,

		eProgramPages,
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
	struct FlashLoaderInfo
	{
		FlashAlgoFuncLUT lut;
		WorkRamInfo workRamInfo;
		Swd::ProgramSysCall sysCallInfo;
		std::vector<Program> loader;
	};
	static std::array<const char* const,FuncEntry::eSize> cFuncStr;
	const FlashLoaderInfo cLoader;
    std::unique_ptr<Swd> pSwd;
	FlashLoaderInfo loadLoader(const std::string& algorithmPath, const RamInfo& targetRam);
	WorkRamInfo createRamInfo(const RamInfo& targetRam);

	uint32_t writeSwd(uint32_t addr, const uint8_t* buffer, uint32_t bufferSize);
};