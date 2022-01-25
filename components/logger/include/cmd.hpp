#ifndef _CMD_HPP_
#define _CMD_HPP_

#include <stdint.h>
#include <string>
#include <vector>

class Cmd
{
public:
    enum class Type : uint8_t
    {
        eInvalid,
        eClientToSever = 17,
        eServerToClient = 18
    };
    enum class SubCmd : uint8_t
    {
        eUartSetting = 1,
        eInvalid,
    };
    Cmd(Type, SubCmd);
    Cmd(uint8_t* buffer, uint32_t size);
    ~Cmd() = default;

    Type getCmdType() const { return cType; };
    SubCmd getSubCmd() const { return cSubCmd; }
    std::vector<uint8_t> getCmd();

protected:
    Type cType;
    SubCmd cSubCmd;
    std::string mCmd;
};

class UartSetting : public Cmd
{
public:
    UartSetting(int baudrate, int port);
    ~UartSetting() = default;
    uint8_t getBaudrate() const;
    uint8_t getUartPort() const;
};

#endif 
