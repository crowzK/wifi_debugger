#include "swd.hpp"
#include <esp_log.h>

// Debug Port Register Addresses
#define DP_IDCODE                       0x00U   // IDCODE Register (SW Read only)
#define DP_ABORT                        0x00U   // Abort Register (SW Write only)
#define DP_CTRL_STAT                    0x04U   // Control & Status
#define DP_WCR                          0x04U   // Wire Control Register (SW Only)
#define DP_SELECT                       0x08U   // Select Register (JTAG R/W & SW W)
#define DP_RESEND                       0x08U   // Resend (SW Read Only)
#define DP_RDBUFF                       0x0CU   // Read Buffer (Read Only)

// Abort Register definitions
#define DAPABORT       0x00000001  // DAP Abort
#define STKCMPCLR      0x00000002  // Clear STICKYCMP Flag (SW Only)
#define STKERRCLR      0x00000004  // Clear STICKYERR Flag (SW Only)
#define WDERRCLR       0x00000008  // Clear WDATAERR Flag (SW Only)
#define ORUNERRCLR     0x00000010  // Clear STICKYORUN Flag (SW Only)

// Access Port Register Addresses
#define AP_CSW         0x00        // Control and Status Word
#define AP_TAR         0x04        // Transfer Address
#define AP_DRW         0x0C        // Data Read/Write
#define AP_BD0         0x10        // Banked Data 0
#define AP_BD1         0x14        // Banked Data 1
#define AP_BD2         0x18        // Banked Data 2
#define AP_BD3         0x1C        // Banked Data 3
#define AP_ROM         0xF8        // Debug ROM Address
#define AP_IDR         0xFC        // Identification Register

// Debug Select Register definitions
#define CTRLSEL        0x00000001  // CTRLSEL (SW Only)
#define APBANKSEL      0x000000F0  // APBANKSEL Mask
#define APSEL          0xFF000000  // APSEL Mask

// AP Control and Status Word definitions
#define CSW_SIZE       0x00000007  // Access Size: Selection Mask
#define CSW_SIZE8      0x00000000  // Access Size: 8-bit
#define CSW_SIZE16     0x00000001  // Access Size: 16-bit
#define CSW_SIZE32     0x00000002  // Access Size: 32-bit
#define CSW_ADDRINC    0x00000030  // Auto Address Increment Mask
#define CSW_NADDRINC   0x00000000  // No Address Increment
#define CSW_SADDRINC   0x00000010  // Single Address Increment
#define CSW_PADDRINC   0x00000020  // Packed Address Increment
#define CSW_DBGSTAT    0x00000040  // Debug Status
#define CSW_TINPROG    0x00000080  // Transfer in progress
#define CSW_HPROT      0x02000000  // User/Privilege Control
#define CSW_MSTRTYPE   0x20000000  // Master Type Mask
#define CSW_MSTRCORE   0x00000000  // Master Type: Core
#define CSW_MSTRDBG    0x20000000  // Master Type: Debug
#define CSW_RESERVED   0x01000000  // Reserved Value

#define CSW_VALUE (CSW_RESERVED | CSW_MSTRDBG | CSW_HPROT | CSW_DBGSTAT | CSW_SADDRINC)

#define SWD_REG_AP        (1)
#define SWD_REG_DP        (0)
#define SWD_REG_R         (1<<1)
#define SWD_REG_W         (0<<1)
#define SWD_REG_ADR(a)    (a & 0x0c)

static const char* TAG = "Swd";

Swd::Swd() :
    mCsw(UINT32_MAX)
{

}

bool Swd::errorCheck(const char* func, Response res)
{
    if(res != Response::Ok)
    {
        ESP_LOGE(TAG, "%s Res %d", func, (int)res);
        return false;
    }
    return true;
}

bool Swd::cleareErrors()
{
    return writeDp(DP_ABORT, STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR);
}

bool Swd::readDp(uint8_t addr, uint32_t& dp)
{
    auto res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(addr), dp);
    return errorCheck(__func__, res);
}

bool Swd::writeDp(uint8_t addr, uint32_t dp)
{
    auto res = write(SWD_REG_DP | SWD_REG_W | SWD_REG_ADR(addr), dp);
    return errorCheck(__func__, res);
}

bool Swd::readAp(uint8_t addr, uint32_t& ap, bool dpSelect)
{
    if(dpSelect)
    {
        constexpr uint32_t apsel = DP_SELECT & 0xff000000;
        constexpr uint32_t bank_sel = DP_SELECT & APBANKSEL;
        if(not writeDp(DP_SELECT, apsel | bank_sel))
        {
            return false;
        }
    }
    read(SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(addr), ap);
    auto res = read(SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(addr), ap);
    return errorCheck(__func__, res);
}

bool Swd::readApMultiple(uint32_t* pBuffer, uint32_t length)
{
    uint32_t ap;
    Response res;

    res = read(SWD_REG_AP | SWD_REG_R | AP_DRW, ap);
    if(not errorCheck(__func__, res))
    {
        return false;
    }
    for(int i = 0; i < length - 1; i++)
    {
        res = read(SWD_REG_AP | SWD_REG_R | AP_DRW, ap);
        if(not errorCheck(__func__, res))
        {
            return false;
        }
        pBuffer[i] = ap;
    }
    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), ap);
    pBuffer[length - 1] = ap;
    return errorCheck(__func__, res);
}

bool Swd::writeAp(uint8_t addr, uint32_t ap, bool dpSelect)
{
    if(dpSelect)
    {
        constexpr uint32_t apsel = DP_SELECT & 0xff000000;
        constexpr uint32_t bank_sel = DP_SELECT & APBANKSEL;
        if(not writeDp(DP_SELECT, apsel | bank_sel))
        {
            return false;
        }
        if((addr == AP_CSW) and (ap == mCsw))
        {
            return true;
        }
    }

    auto res = write(SWD_REG_AP | SWD_REG_W | SWD_REG_ADR(addr), ap);
    if(not errorCheck(__func__, res))
    {
        return false;
    }

    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), ap);
    return errorCheck(__func__, res);
}

bool Swd::writeApMultiple(const uint32_t* pBuffer, uint32_t length)
{
    Response res;
    for(int i = 0; i < length; i++)
    {
        res = write(SWD_REG_AP | SWD_REG_W | AP_DRW, pBuffer[i]);
        if(not errorCheck(__func__, res))
        {
            return false;
        }
    }
    uint32_t dummy;
    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), dummy);
    return errorCheck(__func__, res);
}

bool Swd::writeMemory(uint32_t addr, uint32_t transferSize, uint32_t data)
{
    uint32_t csw;
    uint32_t tmp;
    switch (transferSize)
    {
    case 8:
        csw = CSW_VALUE | CSW_SIZE8;
        tmp = data << ((addr & 0x03) << 3);
        break;
    case 16:
        csw = CSW_VALUE | CSW_SIZE16;
        tmp = data << ((addr & 0x02) << 3);
        break;
    case 32:
        csw = CSW_VALUE | CSW_SIZE32;
        tmp = data;
        break;
    default:
        return false;
    }
    if(not writeAp(AP_CSW, csw, true))
    {
        return false;
    }

    // TAR
    auto res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if(not errorCheck(__func__, res))
    {
        return false;
    }

    // data
    res = write(SWD_REG_AP | SWD_REG_W | AP_DRW, tmp);
    if(not errorCheck(__func__, res))
    {
        return false;
    }

    // dummy read
    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), tmp);
    return errorCheck(__func__, res);
}

bool Swd::readMemory(uint32_t addr, uint32_t transferSize, uint32_t& data)
{
    uint32_t csw;
    switch (transferSize)
    {
    case 8:
        csw = CSW_VALUE | CSW_SIZE8;
        break;
    case 16:
        csw = CSW_VALUE | CSW_SIZE16;
        break;
    case 32:
        csw = CSW_VALUE | CSW_SIZE32;
        break;
    default:
        return false;
    }

    // CSW
    if(not writeAp(AP_CSW, csw, true))
    {
        return false;
    }

    // TAR
    Response res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if(not errorCheck(__func__, res))
    {
        return false;
    }
    
    uint32_t tmp;
    // dummy read
    res = read(SWD_REG_AP | SWD_REG_R | AP_DRW, tmp);
    if(not errorCheck(__func__, res))
    {
        return false;
    }

    // read data
    res = read(SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), tmp);
    if(not errorCheck(__func__, res))
    {
        return false;
    }

    switch (transferSize)
    {
    case 8:
        data = tmp >> ((addr & 0x03) << 3);
        break;
    case 16:
        data = tmp >> ((addr & 0x02) << 3);
        break;
    case 32:
        data = tmp;
        break;
    default:
        return false;
    }
    return true;
}

bool Swd::writeMemoryBlcok32(uint32_t addr, const uint32_t* pBuffer, uint32_t length)
{
    if(not writeAp(AP_CSW, (CSW_VALUE | CSW_SIZE32), true))
    {
        return false;
    }

    // TAR
    Response res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if(not errorCheck(__func__, res))
    {
        return false;
    }
    
    return writeApMultiple(pBuffer, length);
}

bool Swd::readMemoryBlcok32(uint32_t addr, uint32_t* pBuffer, uint32_t length)
{
    if(not writeAp(AP_CSW, (CSW_VALUE | CSW_SIZE32), true))
    {
        return false;
    }

    // TAR
    Response res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if(not errorCheck(__func__, res))
    {
        return false;
    }

    return readApMultiple(pBuffer, length);
}
