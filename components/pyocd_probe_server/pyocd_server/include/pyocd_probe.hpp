#include <stdint.h>
#include <vector>

class PyOcdProbe
{
    PyOcdProbe();
    ~PyOcdProbe();

    void hello();
    void readprop();

// -------------------------------------------
//          Target control functions
// -------------------------------------------
    void connect();
    void disconnect();
    void setClock(uint32_t frequency);
    void reset();
    void assert_reset(bool asserted);
    void is_reset_asserted();
    void flush();

// -------------------------------------------
//           DAP Access functions
// -------------------------------------------
    void get_memory_interface_for_ap(uint32_t ap_address_version, uint32_t ap_address_address);

    void read_dp(uint32_t addr);
    void write_dp(uint32_t addr, uint32_t data);

    void read_ap(uint32_t addr);
    void write_ap(uint32_t addr, uint32_t data);
    
    void read_ap_multiple(uint32_t addr, uint32_t count);
    void write_ap_multiple(uint32_t addr, std::vector<uint32_t> values);

    void read_mem(uint32_t addr, uint32_t xfer_size);
    void write_mem(uint32_t addr, uint32_t value, uint32_t xfer_size);

    void read_block32(uint32_t addr, uint32_t count);
    void write_block32(uint32_t addr, const std::vector<uint32_t>& data);

    void read_block8();
    void write_block8(uint32_t addr, const std::vector<uint8_t>& data);
};
