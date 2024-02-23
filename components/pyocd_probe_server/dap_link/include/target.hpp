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
#include "swd.hpp"

class Target
{
public:
	//! @brief Creates a family ID from a vendor ID and family index within that vendor.
	//! copied from target_family.h at DAPLink
	#define CREATE_FAMILY_ID(vendor, family) ((vendor) << 8 | (family))

	//! @brief States into which the target can be placed.
	//!
	//! These enums are passed to target_set_state() and indicate the desired state into which
	//! the target should be reset.
	//! copied from target_family.h at DAPLink
	enum target_state_t 
	{
		RESET_HOLD,              //!< Hold target in reset
		RESET_PROGRAM,           //!< Reset target and setup for flash programming
		RESET_RUN,               //!< Reset target and run normally
		NO_DEBUG,                //!< Disable debug on running target
		DEBUG,                   //!< Enable debug on running target
		HALT,                    //!< Halt the target without resetting it
		RUN,                     //!< Resume the target without resetting it
		POST_FLASH_RESET,        //!< Reset target after flash programming
		POWER_ON,                //!< Poweron the target
		SHUTDOWN,                //!< Poweroff the target
	};

	//! @brief Options for reset.
	enum reset_type_t 
	{
		kHardwareReset = 1,
		kSoftwareReset,
	};

	//! @brief Unique IDs for vendors.
	//!
	//! The vendor IDs are the same as those used for the _DeviceVendorEnum_ defined for the PDSC file
	//! format from CMSIS-Packs. See the [DeviceVendorEnum
	//! documentation](https://arm-software.github.io/CMSIS_5/Pack/html/pdsc_family_pg.html#DeviceVendorEnum)
	//! for the list of ID values.
	//! copied from target_family.h at DAPLink
	enum _vendor_ids {
		kStub_VendorID = 0,
		kNXP_VendorID = 11,
		kTI_VendorID = 16,
		kMaxim_VendorID = 23,
		kNordic_VendorID = 54,
		kToshiba_VendorID = 92,
		kRenesas_VendorID = 117,
		kRealtek_VendorID = 124,
		kAmbiq_VendorID = 120,
	};

	//! @brief Unique IDs for device families supported by DAPLink.
	//!
	//! The values of these enums are created with the CREATE_FAMILY_ID() macro. Vendor IDs come from
	//! the #_vendor_ids enumeration. The family index for each ID is simply an integer that is unique
	//! within the family.
	//!
	//! There are several "stub" families defined with a stub vendor. These families are meant to be
	//! used for devices that do not require any customized behaviour in order to be successfully
	//! controlled by DAPLink. The individual stub families provide some options for what reset type
	//! should be used, either hardware or software.
	//!
	//! To add a new family, first determine if you can simply use one of the stub families. For many
	//! devices, the stub families are sufficient and using them reduces complexity.
	//!
	//! If you do need a new family ID, first check if the vendor is present in #_vendor_ids. If not,
	//! add the vendor ID to that enum (see its documentation for the source of vendor ID values).
	//! Then pick a unique family index by adding 1 to the highest existing family index within that
	//! vendor. For a family with a new vendor, the family index should be 1.
	//! copied from target_family.h at DAPLink
	enum family_id_t {
		kStub_HWReset_FamilyID = CREATE_FAMILY_ID(kStub_VendorID, 1),
		kStub_SWVectReset_FamilyID = CREATE_FAMILY_ID(kStub_VendorID, 2),
		kStub_SWSysReset_FamilyID = CREATE_FAMILY_ID(kStub_VendorID, 3),
		kNXP_KinetisK_FamilyID = CREATE_FAMILY_ID(kNXP_VendorID, 1),
		kNXP_KinetisL_FamilyID = CREATE_FAMILY_ID(kNXP_VendorID, 2),
		kNXP_Mimxrt_FamilyID = CREATE_FAMILY_ID(kNXP_VendorID, 3),
		kNXP_RapidIot_FamilyID = CREATE_FAMILY_ID(kNXP_VendorID, 4),
		kNXP_KinetisK32W_FamilyID = CREATE_FAMILY_ID(kNXP_VendorID, 5),
		kNXP_LPC55xx_FamilyID = CREATE_FAMILY_ID(kNXP_VendorID, 6),
		kNordic_Nrf51_FamilyID = CREATE_FAMILY_ID(kNordic_VendorID, 1),
		kNordic_Nrf52_FamilyID = CREATE_FAMILY_ID(kNordic_VendorID, 2),
		kRealtek_Rtl8195am_FamilyID = CREATE_FAMILY_ID(kRealtek_VendorID, 1),
		kTI_Cc3220sf_FamilyID = CREATE_FAMILY_ID(kTI_VendorID, 1),
		kToshiba_Tz_FamilyID = CREATE_FAMILY_ID(kToshiba_VendorID, 1),
		kRenesas_FamilyID = CREATE_FAMILY_ID(kRenesas_VendorID, 1),
		kAmbiq_ama3b1kk_FamilyID = CREATE_FAMILY_ID(kAmbiq_VendorID, 1),
		kMaxim_MAX3262X_FamilyID = CREATE_FAMILY_ID(kMaxim_VendorID, 1),
		kMaxim_MAX3266X_FamilyID = CREATE_FAMILY_ID(kMaxim_VendorID, 2),
	};

	enum SWD_CONNECT_TYPE{
		CONNECT_NORMAL,
		CONNECT_UNDER_RESET,
	};

	enum flash_algo_return_t{
		FLASHALGO_RETURN_BOOL,
		FLASHALGO_RETURN_POINTER
	};

	Target();
	virtual ~Target() = default;

	virtual void initBeforeDebug();
	virtual void preRunConfig();
	virtual void unlock();
	virtual void setSecurityBits(uint32_t addr, uint8_t *data, uint32_t size);
	virtual void setState(target_state_t state);
	virtual void reset(uint8_t asserted);
	virtual uint32_t getApsel();
	virtual void validateBin(const uint8_t *buf);
	virtual void validateHex(const uint8_t *buf);

	virtual const region_info_t& getDefaultRegion(uint32_t index);

protected:
    //! ARM General Purpose Registors
    struct GPRs
    {
        uint32_t r[16];
        uint32_t xpsr;
    };

	target_cfg_t targetCfg;
	Swd& swd;
	bool initDebug();
	bool setTargetStateSw(target_state_t state);
	bool setTargetStateHw(target_state_t state);
    bool writeGPRs(GPRs& gprs);
    bool sysCallExec(const program_syscall_t *sysCallParam, uint32_t entry, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, flash_algo_return_t return_type);
};
