#include <stdint.h>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include "socket.hpp"
#include "ArduinoJson.h"

class Request
{
public:
    enum Cmd
    {
        hello,
        readprop,
        open,
        close,
        lock,
        unlock,
        connect,
        disconnect,
        swj_sequence,
        swd_sequence,
        jtag_sequence,
        set_clock,
        reset,
        assert_reset,
        is_reset_asserted,
        flush,
        read_dp,
        write_dp,
        read_ap,
        write_ap,
        read_ap_multiple,
        write_ap_multiple,
        get_memory_interface_for_ap,
        swo_start,
        swo_stop,
        swo_read,
        read_mem,
        write_mem,
        read_block32,
        write_block32,
        read_block8,
        write_block8,
        cmdSize
    };
    static std::string toString(Cmd cmd);
    static Cmd fromString(const std::string& str);
    
protected:
    static const std::string cCmdStr[cmdSize];
};

class PyOcdServer : public ServerSocket
{
public:
    PyOcdServer();
    ~PyOcdServer() = default;

protected:
    StaticJsonDocument<1024> mDoc;

    bool serverMain(int acceptSocekt) override;
    void sendResult(int socket, int id);
};
