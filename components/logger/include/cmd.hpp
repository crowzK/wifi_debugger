#ifndef _CMD_HPP_
#define _CMD_HPP_

#include <stdint.h>
#include <string>

class Cmd
{
public:
    enum class Type
    {
        eInvalid,
        eClientToSever = 17,
        eServerToClient = 18
    };
    enum class SubCmd
    {
        eUartSetting = 1,
        eInvalid,
    };
    Cmd(uint8_t* buffer, uint32_t size);
    ~Cmd() = default;

    Type getCmdType() const { return cType; };
    SubCmd getSubCmd() const { return cSubCmd; }

protected:
    Type cType;
    SubCmd cSubCmd;
    std::string mCmd;
};

class UartSetting : public Cmd
{
public: 
    uint8_t getBaudrate() const;
    uint8_t getUartPort() const;
};

#endif 
