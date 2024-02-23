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
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <memory>
#include "JSON_Decoder.h"
#include "swd.hpp"
#include "pyocd_io_socket.hpp"

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
    enum ArgumentType
    {
        eNone,
        eInt,
        eString,
        eArray,
    };
    static ArgumentType getArgType(Cmd cmd);
    static std::string toString(Cmd cmd);
    static Cmd fromString(const std::string& str);
    
protected:
    static const std::string cCmdStr[cmdSize];

};

class PyOcdParser : public JsonListener
{
public:
    PyOcdParser(PyOcdIo& io);
    ~PyOcdParser() = default;

    void parse(char* msg, int len);
protected:
    enum class Key
    {
        eId,
        eRequest,
        eArguments,
        eInvalid
    };
    PyOcdIo& mPyOcdIo;
    int mId;
    Request::Cmd mRequest;
    Key mKey;

    uint32_t mIntArgument;
    std::string mStrArgument;
    std::vector<uint32_t> mArrayArgument;

    std::unique_ptr<Swd> pSwd;
    JSON_Decoder mPaser;

    void startDocument() override;
    void endDocument() override;
    void startObject() override;
    void endObject() override;
    void startArray() override;
    void endArray() override;
    void key(const char *key) override;
    void value(const char *value) override;
    void whitespace(char c) override;
    void error( const char *message ) override;
    
    void sendOkay();
    void sendString(const char * str);
    void sendInt(uint32_t val);
    void sendArray(const std::vector<uint32_t>&);
    void sendError(const char * error);
};

class PyOcdServer
{
public:
    static PyOcdServer& create();

protected:
    PyOcdIoSocket mPyOcdServerSocket;
    PyOcdParser mPyOcdParser;

    PyOcdServer();
    ~PyOcdServer() = default;
};
