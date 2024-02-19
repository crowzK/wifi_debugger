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
#include "target_flash.hpp"

#if 0
#define DEFAULT_PROGRAM_PAGE_MIN_SIZE   (256u)

program_target_t * get_flash_algo(uint32_t addr)
{
    region_info_t * flash_region = targetCfg.flash_regions;

    for (; flash_region->start != 0 || flash_region->end != 0; ++flash_region) {
        if (addr >= flash_region->start && addr <= flash_region->end) {
            flash_start = flash_region->start; //save the flash start
            if (flash_region->flash_algo) {
                return flash_region->flash_algo;
            }else{
                return NULL;
            }
        }
    }

    //could not find a flash algo for the region; use default
    if (default_region) {
        flash_start = default_region->start;
        return default_region->flash_algo;
    } else {
        return NULL;
    }
}

bool flash_func_start(flash_func_t func)
{
    program_target_t * flash = current_flash_algo;

    if (last_flash_func != func)
    {
        // Finish the currently active function.
        if (FLASH_FUNC_NOP != last_flash_func &&
            ((flash->algo_flags & kAlgoSingleInitType) == 0 || FLASH_FUNC_NOP == func ) &&
            0 == swd_flash_syscall_exec(&flash->sys_call_s, flash->uninit, last_flash_func, 0, 0, 0, FLASHALGO_RETURN_BOOL)) {
            return ERROR_UNINIT;
        }

        // Start a new function.
        if (FLASH_FUNC_NOP != func &&
            ((flash->algo_flags & kAlgoSingleInitType) == 0 || FLASH_FUNC_NOP == last_flash_func ) &&
            0 == swd_flash_syscall_exec(&flash->sys_call_s, flash->init, flash_start, 0, func, 0, FLASHALGO_RETURN_BOOL)) {
            return ERROR_INIT;
        }

        last_flash_func = func;
    }

    return ERROR_SUCCESS;
}

bool Flash::set(uint32_t addr)
{
    program_target_t * new_flash_algo = get_flash_algo(addr);
    if (new_flash_algo == NULL) {
        return ERROR_ALGO_MISSING;
    }
    if(current_flash_algo != new_flash_algo){
        //run uninit to last func
        bool status = flash_func_start(FLASH_FUNC_NOP);
        if (status != ERROR_SUCCESS) {
            return status;
        }
        // Download flash programming algorithm to target
        if (0 == swd_write_memory(new_flash_algo->algo_start, (uint8_t *)new_flash_algo->algo_blob, new_flash_algo->algo_size)) {
            return ERROR_ALGO_DL;
        }

        current_flash_algo = new_flash_algo;

    }
    return ERROR_SUCCESS;
}

Flash::Flash()
{

}

Flash::~Flash()
{

}

bool Flash::init()
{
    last_flash_func = FLASH_FUNC_NOP;

    current_flash_algo = NULL;

    if (0 == target_set_state(RESET_PROGRAM)) {
        return false;
    }

    //get default region
    region_info_t * flash_region = targetCfg.flash_regions;
    
    for (; flash_region->start != 0 || flash_region->end != 0; ++flash_region) {
        if (flash_region->flags & kRegionIsDefault) {
            default_region = flash_region;
            break;
        }
    }

    state = STATE_OPEN;
    return true;
}

bool Flash::deinit(void)
{
    bool status = flash_func_start(FLASH_FUNC_NOP);
    if(not status) 
    {
        return status;
    }
    if (config_get_auto_rst()) {
        // Resume the target if configured to do so
        target_set_state(RESET_RUN);
    } else {
        // Leave the target halted until a reset occurs
        target_set_state(RESET_PROGRAM);
    }
    // Check to see if anything needs to be done after programming.
    // This is usually a no-op for most targets.
    target_set_state(POST_FLASH_RESET);

    state = STATE_CLOSED;
    swd_off();
    return true;
}

bool Flash::programPage(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    bool status = true;
    program_target_t * flash = current_flash_algo;

    if (!flash) 
    {
        return false;
    }

    // check if security bits were set
    if (g_target_family && g_target_family->security_bits_set){
        if (1 == g_target_family->security_bits_set(addr, (uint8_t *)buf, size)) {
            return ERROR_SECURITY_BITS;
        }
    }

    status = flash_func_start(FLASH_FUNC_PROGRAM);

    if (status != ERROR_SUCCESS) {
        return status;
    }

    while (size > 0) {
        uint32_t write_size = MIN(size, flash->program_buffer_size);

        // Write page to buffer
        if (!swd_write_memory(flash->program_buffer, (uint8_t *)buf, write_size)) {
            return ERROR_ALGO_DATA_SEQ;
        }

        // Run flash programming
        if (!swd_flash_syscall_exec(&flash->sys_call_s,
                                    flash->program_page,
                                    addr,
                                    write_size,
                                    flash->program_buffer,
                                    0,
                                    FLASHALGO_RETURN_BOOL)) {
            return ERROR_WRITE;
        }

        if (config_get_automation_allowed()) {
            // Verify data flashed if in automation mode
            if (flash->verify != 0) {
                status = flash_func_start(FLASH_FUNC_VERIFY);
                if (status != ERROR_SUCCESS) {
                    return status;
                }
                flash_algo_return_t return_type;
                if ((flash->algo_flags & kAlgoVerifyReturnsAddress) != 0) {
                    return_type = FLASHALGO_RETURN_POINTER;
                } else {
                    return_type = FLASHALGO_RETURN_BOOL;
                }
                if (!swd_flash_syscall_exec(&flash->sys_call_s,
                                    flash->verify,
                                    addr,
                                    write_size,
                                    flash->program_buffer,
                                    0,
                                    return_type)) {
                    return ERROR_WRITE_VERIFY;
                }
            } else {
                while (write_size > 0) {
                    uint8_t rb_buf[16];
                    uint32_t verify_size = MIN(write_size, sizeof(rb_buf));
                    if (!swd_read_memory(addr, rb_buf, verify_size)) {
                        return ERROR_ALGO_DATA_SEQ;
                    }
                    if (memcmp(buf, rb_buf, verify_size) != 0) {
                        return ERROR_WRITE_VERIFY;
                    }
                    addr += verify_size;
                    buf += verify_size;
                    size -= verify_size;
                    write_size -= verify_size;
                }
                continue;
            }
        }
        addr += write_size;
        buf += write_size;
        size -= write_size;

    }

    return ERROR_SUCCESS;
}

bool Flash::eraseSector(uint32_t addr)
{
    if (g_board_info.target_cfg) {
        bool status = ERROR_SUCCESS;
        program_target_t * flash = current_flash_algo;

        if (!flash) {
            return ERROR_INTERNAL;
        }

        // Check to make sure the address is on a sector boundary
        if ((addr % target_flash_erase_sector_size(addr)) != 0) {
            return ERROR_ERASE_SECTOR;
        }

        status = flash_func_start(FLASH_FUNC_ERASE);

        if (status != ERROR_SUCCESS) {
            return status;
        }

        if (0 == swd_flash_syscall_exec(&flash->sys_call_s, flash->erase_sector, addr, 0, 0, 0, FLASHALGO_RETURN_BOOL)) {
            return ERROR_ERASE_SECTOR;
        }

        return ERROR_SUCCESS;
    } else {
        return ERROR_FAILURE;
    }
}

bool Flash::eraseChip(void)
{
    if (g_board_info.target_cfg){
        bool status = ERROR_SUCCESS;
        region_info_t * flash_region = targetCfg.flash_regions;

        for (; flash_region->start != 0 || flash_region->end != 0; ++flash_region) {
            program_target_t *new_flash_algo = get_flash_algo(flash_region->start);
            if ((new_flash_algo != NULL) && ((new_flash_algo->algo_flags & kAlgoSkipChipErase) != 0)) {
                // skip flash region
                continue;
            }
            status = target_flash_set(flash_region->start);
            if (status != ERROR_SUCCESS) {
                return status;
            }
            status = flash_func_start(FLASH_FUNC_ERASE);
            if (status != ERROR_SUCCESS) {
                return status;
            }
            if (0 == swd_flash_syscall_exec(&current_flash_algo->sys_call_s, current_flash_algo->erase_chip, 0, 0, 0, 0, FLASHALGO_RETURN_BOOL)) {
                return ERROR_ERASE_ALL;
            }
        }

        // Reset and re-initialize the target after the erase if required
        if (targetCfg.erase_reset) {
            status = target_flash_init();
        }

        return status;
    } else {
        return ERROR_FAILURE;
    }
}

uint32_t Flash::getPorgramPageSize(uint32_t addr)
{
    if (g_board_info.target_cfg){
        uint32_t size = DEFAULT_PROGRAM_PAGE_MIN_SIZE;
        if (size > target_flash_erase_sector_size(addr)) {
            size = target_flash_erase_sector_size(addr);
        }
        return size;
    } else {
        return 0;
    }
}

uint32_t Flash::getEraseSectorSize(uint32_t addr)
{
    if (g_board_info.target_cfg){
        if(targetCfg.sector_info_length > 0) {
            int sector_index = targetCfg.sector_info_length - 1;
            for (; sector_index >= 0; sector_index--) {
                if (addr >= targetCfg.sectors_info[sector_index].start) {
                    return targetCfg.sectors_info[sector_index].size;
                }
            }
        }
        //sector information should be in sector_info
        util_assert(0);
        return 0;
    } else {
        return 0;
    }
}

bool Flash::isFlashBusy(){
    return (state == STATE_OPEN);
}

#endif