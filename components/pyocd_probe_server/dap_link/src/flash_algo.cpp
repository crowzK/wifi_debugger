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

#include "flash_algo.hpp"

int FlashAlgo::blankCheck(unsigned long adr, unsigned long sz, unsigned char pat)
{

}

int FlashAlgo::eraseChip(void)
{

}

int FlashAlgo::eraseSector(unsigned long adr)
{

}

int FlashAlgo::init(unsigned long adr, unsigned long clk, unsigned long fnc)
{

}

int FlashAlgo::programPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{

}

int FlashAlgo::unInit(unsigned long fnc)
{

}

unsigned long FlashAlgo::verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{

}


FlashAlgo::FlashAlgo(std::string algorithm)
{

}

FlashAlgo::~FlashAlgo()
{

}
