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

#define SWD_REG_AP        (1)
#define SWD_REG_DP        (0)
#define SWD_REG_R         (1<<1)
#define SWD_REG_W         (0<<1)
#define SWD_REG_ADR(a)    (a & 0x0c)

static const char* TAG = "Swd";

bool Swd::cleareErrors()
{
    auto res = writeDp(DP_ABORT, STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR);
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
    }
    return res == Response::Ok;
}

bool Swd::readDp(uint8_t addr, uint32_t& dp)
{
    uint8_t req = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(addr);
    auto res = read(req, dp);
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
    }
    return res == Response::Ok;
}

bool Swd::writeDp(uint8_t addr, uint32_t dp)
{
    uint8_t req = SWD_REG_DP | SWD_REG_W | SWD_REG_ADR(addr);
    auto res = write(req, dp);
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
    }
    return res == Response::Ok;
}

bool Swd::readAp(uint8_t addr, uint32_t& ap)
{
    uint8_t req = SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(addr);
    read(req, ap);
    auto res = read(req, ap);
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
    }
    return res == Response::Ok;
}

bool Swd::readApMultiple(uint8_t addr, std::vector<uint32_t>&out)
{
    uint8_t req = SWD_REG_AP | SWD_REG_R | AP_DRW;
    uint32_t ap;
    Response res;
    res = read(req, ap);
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
        return false;
    }
    for(int i = 0; i < out.size() - 1; i++)
    {
        res = read(req, ap);
        if(res != Response::Ok)
        {
            ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
            return false;
        }
        out[i] = ap;
    }
    req = SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF);
    res = read(req, ap);
    out[out.size() - 1] = ap;
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
    }
    return res == Response::Ok;
}

bool Swd::writeAp(uint8_t addr, uint32_t ap)
{
    uint8_t req = SWD_REG_AP | SWD_REG_W | SWD_REG_ADR(addr);
    auto res = write(req, ap);
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
    }
    return res == Response::Ok;
}

bool Swd::writeApMultiple(uint8_t addr, std::vector<uint32_t>&in)
{
    uint8_t req = SWD_REG_AP | SWD_REG_W | (3 << 2);
    Response res;
    for(int i = 1; i < in.size() - 1; i++)
    {
        res = write(req, in[i]);
        if(res != Response::Ok)
        {
            ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
            return false;
        }
    }
    uint32_t dummy;
    req =  SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF);
    res = read(req, dummy);
    if(res != Response::Ok)
    {
        ESP_LOGW(TAG, "%s Res %d", __func__, (int)res);
    }
    return res == Response::Ok;
}
