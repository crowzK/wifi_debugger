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
#include "flash_blob.h"
#include "target_config.h"
#include "target.hpp"

class Flash
{
public:
	enum state_t
	{
		STATE_CLOSED,
		STATE_OPEN,
		STATE_ERROR
	};

	Flash();
	~Flash();

	bool init();
	bool deinit();
	bool programPage(uint32_t addr, const uint8_t* buf, uint32_t size);
	bool eraseSector(uint32_t sector);
	bool eraseChip();
	uint32_t getPorgramPageSize(uint32_t addr);
	uint32_t getEraseSectorSize(uint32_t addr);
	bool isFlashBusy();
	bool setFlashAlgo(uint32_t addr);

protected:
	enum flash_func_t
	{
		FLASH_FUNC_NOP,
		FLASH_FUNC_ERASE,
		FLASH_FUNC_PROGRAM,
		FLASH_FUNC_VERIFY
	} ;

	state_t state = STATE_CLOSED;

	flash_func_t last_flash_func = FLASH_FUNC_NOP;

	//saved flash algo
	program_target_t * current_flash_algo = NULL;

	//saved default region for default flash algo
	region_info_t * default_region = NULL;

	//saved flash start from flash algo
	uint32_t flash_start = 0;

	Target target;

	bool flashFuncStart(flash_func_t func);
	bool flashAlgoDownload(uint32_t addr);
};