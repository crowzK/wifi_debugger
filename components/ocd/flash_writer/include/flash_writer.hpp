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

#include <memory>
#include <string>
#include <mutex>
#include "task.hpp"

class FlashAlgo;

class FlashWriter : protected Task
{
public:
	FlashWriter();
	~FlashWriter();

	void targetUpdate(const std::string& binFile, uint32_t startAddr);

protected:
	std::recursive_mutex mMutex;
	std::unique_ptr<FlashAlgo> mpFlashAlgorithm;

	void connectTarget(bool connect = true);
	void readTargetInfo();
    void task() override;
};