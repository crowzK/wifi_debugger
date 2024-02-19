/**
 * @file    target_flash.c
 * @brief   Implementation of target_flash.h
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2009-2019, ARM Limited, All Rights Reserved
 * Copyright 2019, Cypress Semiconductor Corporation
 * or a subsidiary of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include "target.hpp"
#include "debug_cm.h"

Target::Target()
{
}

Target::~Target()
{
}

void Target::initBeforeDebug()
{
}

void Target::preRunConfig()
{
}

void Target::unlock()
{
}

void Target::setSecurityBits(uint32_t addr, uint8_t *data, uint32_t size)
{
}

void Target::setState(target_state_t state)
{
}

void Target::swdSetReset(uint8_t asserted)
{
}

void Target::validateBin(const uint8_t *buf)
{
}

void Target::validateHex(const uint8_t *buf)
{
}

const region_info_t &Target::getDefaultRegion(uint32_t index)
{
}

bool Target::setTargetStateSw(target_state_t state)
{
	uint32_t val;
	int8_t ap_retries = 2;
	/* Calling swd_init prior to enterring RUN state causes operations to fail. */
	if (state != RUN)
	{
		swd_init();
	}

	switch (state)
	{
	case RESET_HOLD:
		reset(1);
		break;

	case RESET_RUN:
		reset(1);
		osDelay(2);
		reset(0);
		osDelay(2);

		if (not initDebug())
		{
			return false;
		}

		// Power down
		// Per ADIv6 spec. Clear first CSYSPWRUPREQ followed by CDBGPWRUPREQ
		if (not swd.readDp(DP_CTRL_STAT, &val))
		{
			return false;
		}

		if (not swd.writeDp(DP_CTRL_STAT, val & ~CSYSPWRUPREQ))
		{
			return false;
		}

		// Wait until ACK is deasserted
		do
		{
			if (not swd.readDp(DP_CTRL_STAT, &val))
			{
				return false;
			}
		} while ((val & (CSYSPWRUPACK)) != 0);

		if (not swd.writeDp(DP_CTRL_STAT, val & ~CDBGPWRUPREQ))
		{
			return false;
		}

		// Wait until ACK is deasserted
		do
		{
			if (not swd.readDp(DP_CTRL_STAT, &val))
			{
				return false;
			}
		} while ((val & (CDBGPWRUPACK)) != 0);

		swd_off();
		break;

	case RESET_PROGRAM:
		if (not initDebug())
		{
			return false;
		}

		// Enable debug and halt the core (DHCSR <- 0xA05F0003)
		while (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_HALT))
		{
			if (--ap_retries <= 0)
			{
				return false;
			}
			// Target is in invalid state?
			reset(1);
			osDelay(2);
			reset(0);
			osDelay(2);
		}

		// Wait until core is halted
		do
		{
			if (not swd.readMemory(DBG_HCSR, 32, &val))
			{
				return false;
			}
		} while ((val & S_HALT) == 0);

		// Enable halt on reset
		if (not swd.writeMemory(DBG_EMCR, 32, VC_CORERESET))
		{
			return false;
		}

		// Perform a soft reset
		if (not swd.readMemory(NVIC_AIRCR, 32, &val))
		{
			return false;
		}

		if (not swd.writeMemory(NVIC_AIRCR, 32, VECTKEY | (val & SCB_AIRCR_PRIGROUP_Msk) | soft_reset))
		{
			return false;
		}

		osDelay(2);

		do
		{
			if (not swd.readMemory(DBG_HCSR, 32, &val))
			{
				return false;
			}
		} while ((val & S_HALT) == 0);

		// Disable halt on reset
		if (not swd.writeMemory(DBG_EMCR, 32, 0))
		{
			return false;
		}

		break;

	case NO_DEBUG:
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY))
		{
			return false;
		}

		break;

	case DEBUG:
		if (not swd.jtagToSwd())
		{
			return false;
		}

		if (not swd.cleareErrors())
		{
			return false;
		}

		// Ensure CTRL/STAT register selected in DPBANKSEL
		if (not swd.writeDp(DP_SELECT, 0))
		{
			return false;
		}

		// Power up
		if (not swd.writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ))
		{
			return false;
		}

		// Enable debug
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN))
		{
			return false;
		}

		break;

	case HALT:
		if (not initDebug())
		{
			return false;
		}

		// Enable debug and halt the core (DHCSR <- 0xA05F0003)
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_HALT))
		{
			return false;
		}

		// Wait until core is halted
		do
		{
			if (not swd.readMemory(DBG_HCSR, 32, &val))
			{
				return false;
			}
		} while ((val & S_HALT) == 0);
		break;

	case RUN:
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY))
		{
			return false;
		}
		swd_off();
		break;

	case POST_FLASH_RESET:
		// This state should be handled in target_reset.c, nothing needs to be done here.
		break;

	default:
		return false;
	}

	return true;
}

bool Target::setTargetStateHw(target_state_t state)
{
	uint32_t val;
	int8_t ap_retries = 2;
	/* Calling swd_init prior to entering RUN state causes operations to fail. */
	if (state != RUN)
	{
		swd_init();
	}

	switch (state)
	{
	case RESET_HOLD:
		reset(1);
		break;

	case RESET_RUN:
		reset(1);
		osDelay(2);
		reset(0);
		osDelay(2);
		swd_off();
		break;

	case RESET_PROGRAM:
		if (not initDebug())
		{
			return false;
		}

		if (reset_connect == CONNECT_UNDER_RESET)
		{
			// Assert reset
			reset(1);
			osDelay(2);
		}

		// Enable debug
		while (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN))
		{
			if (--ap_retries <= 0)
				return 0;
			// Target is in invalid state?
			reset(1);
			osDelay(2);
			reset(0);
			osDelay(2);
		}

		// Enable halt on reset
		if (not swd.writeMemory(DBG_EMCR, 32, VC_CORERESET))
		{
			return false;
		}

		if (reset_connect == CONNECT_NORMAL)
		{
			// Assert reset
			reset(1);
			osDelay(2);
		}

		// Deassert reset
		reset(0);
		osDelay(2);

		do
		{
			if (not swd.readMemory(DBG_HCSR, 32, &val))
			{
				return false;
			}
		} while ((val & S_HALT) == 0);

		// Disable halt on reset
		if (not swd.writeMemory(DBG_EMCR, 32, 0))
		{
			return false;
		}

		break;

	case NO_DEBUG:
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY))
		{
			return false;
		}

		break;

	case DEBUG:
		if (not swd.jtagToSwd())
		{
			return false;
		}

		if (not swd.cleareErrors())
		{
			return false;
		}

		// Ensure CTRL/STAT register selected in DPBANKSEL
		if (not swd.writeDp(DP_SELECT, 0))
		{
			return false;
		}

		// Power up
		if (not swd.writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ))
		{
			return false;
		}

		// Enable debug
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN))
		{
			return false;
		}

		break;

	case HALT:
		if (not initDebug())
		{
			return false;
		}

		// Enable debug and halt the core (DHCSR <- 0xA05F0003)
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_HALT))
		{
			return false;
		}

		// Wait until core is halted
		do
		{
			if (not swd.readMemory(DBG_HCSR, 32, &val))
			{
				return false;
			}
		} while ((val & S_HALT) == 0);
		break;

	case RUN:
		if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY))
		{
			return false;
		}
		swd_off();
		break;

	case POST_FLASH_RESET:
		// This state should be handled in target_reset.c, nothing needs to be done here.
		break;

	default:
		return false;
	}

	return true;
}

bool Target::initDebug()
{
	uint32_t tmp = 0;
	int i = 0;
	int timeout = 100;
	// init dap state with fake values
	dap_state.select = 0xffffffff;
	dap_state.csw = 0xffffffff;

	int8_t retries = 4;
	int8_t do_abort = 0;
	do
	{
		if (do_abort)
		{
			// do an abort on stale target, then reset the device
			swd_write_dp(DP_ABORT, DAPABORT);
			swd_set_target_reset(1);
			osDelay(2);
			swd_set_target_reset(0);
			osDelay(2);
			do_abort = 0;
		}
		swd_init();
		// call a target dependant function
		// this function can do several stuff before really
		// initing the debug
		if (g_target_family && g_target_family->target_before_init_debug)
		{
			g_target_family->target_before_init_debug();
		}

		if (not swd.jtagToSwd())
		{
			do_abort = 1;
			continue;
		}

		if (not swd.cleareErrors())
		{
			do_abort = 1;
			continue;
		}

		if (not swd.writeDp(DP_SELECT, 0))
		{
			do_abort = 1;
			continue;
		}

		// Power up
		if (not swd.writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ))
		{
			do_abort = 1;
			continue;
		}

		for (i = 0; i < timeout; i++)
		{
			if (not swd.readDp(DP_CTRL_STAT, &tmp))
			{
				do_abort = 1;
				break;
			}
			if ((tmp & (CDBGPWRUPACK | CSYSPWRUPACK)) == (CDBGPWRUPACK | CSYSPWRUPACK))
			{
				// Break from loop if powerup is complete
				break;
			}
		}
		if ((i == timeout) || (do_abort == 1))
		{
			// Unable to powerup DP
			do_abort = 1;
			continue;
		}

		if (not swd.writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ | TRNNORMAL | MASKLANE))
		{
			do_abort = 1;
			continue;
		}

		// call a target dependant function:
		// some target can enter in a lock state
		// this function can unlock these targets
		if (g_target_family && g_target_family->target_unlock_sequence)
		{
			g_target_family->target_unlock_sequence();
		}

		if (not swd.writeDp(DP_SELECT, 0))
		{
			do_abort = 1;
			continue;
		}

		return true;

	} while (--retries > 0);

	return false;
}

bool Target::writeGPRs(GPRs &gprs)
{
	uint32_t i, status;

	if (not swd.writeDp(DP_SELECT, 0))
	{
		return false;
	}

	// R0, R1, R2, R3
	for (i = 0; i < 4; i++)
	{
		if (not swd.writeGPR(i, gprs.r[i]))
		{
			return false;
		}
	}

	// R9
	if (not swd.writeGPR(9, gprs.r[9]))
	{
		return false;
	}

	// R13, R14, R15
	for (i = 13; i < 16; i++)
	{
		if (not swd.writeGPR(i, gprs.r[i]))
		{
			return false;
		}
	}

	// xPSR
	if (not swd.writeGPR(16, gprs.xpsr))
	{
		return false;
	}

	if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_MASKINTS | C_HALT))
	{
		return false;
	}

	if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_MASKINTS))
	{
		return false;
	}

	// check status
	if (not swd.readDp(DP_CTRL_STAT, status))
	{
		return false;
	}

	if (status & (STICKYERR | WDATAERR))
	{
		return false;
	}

	return true;
}

bool Target::sysCallExec(const program_syscall_t *sysCallParam, uint32_t entry, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, flash_algo_return_t return_type)
{
	GPRs gprs = {{0}, 0};
	// Call flash algorithm function on target and wait for result.
	gprs.r[0] = arg1;						  // R0: Argument 1
	gprs.r[1] = arg2;						  // R1: Argument 2
	gprs.r[2] = arg3;						  // R2: Argument 3
	gprs.r[3] = arg4;						  // R3: Argument 4
	gprs.r[9] = sysCallParam->static_base;	  // SB: Static Base
	gprs.r[13] = sysCallParam->stack_pointer; // SP: Stack Pointer
	gprs.r[14] = sysCallParam->breakpoint;	  // LR: Exit Point
	gprs.r[15] = entry;						  // PC: Entry Point
	gprs.xpsr = 0x01000000;					  // xPSR: T = 1, ISR = 0

	if (not swd.writeGPRs(gprs))
	{
		return false;
	}

	if (not swd.waitUntilHalted())
	{
		return false;
	}

	if (not swd.readGPR(0, gprs.r[0]))
	{
		return false;
	}

	// remove the C_MASKINTS
	if (not swd.writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_HALT))
	{
		return false;
	}

	if (return_type == FLASHALGO_RETURN_POINTER)
	{
		// Flash verify functions return pointer to byte following the buffer if successful.
		if (gprs.r[0] != (arg1 + arg2))
		{
			return false;
		}
	}
	else
	{
		// Flash functions return 0 if successful.
		if (gprs.r[0] != 0)
		{
			return false;
		}
	}

	return true;
}
