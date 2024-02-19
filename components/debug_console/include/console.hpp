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

#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <list>
#include <vector>
#include <mutex>
#include <string>
#include "task.hpp"

class UartByPass;

class Cmd
{
public:
    const std::string cCmd;

    Cmd(std::string&& cmd);
    virtual ~Cmd();
    virtual std::string help() = 0;
    virtual bool excute(const std::vector<std::string>& args) = 0;
};

class Help : protected Cmd
{
public:
    Help();
    ~Help() = default;
    std::string help();
    bool excute(const std::vector<std::string>& args);
};

class Console : protected Task
{
public:
    static Console& create();
    void help();

protected:
    std::recursive_mutex mMutex;
    friend class Cmd;
    std::list<Cmd*> mCmdList;
    std::unique_ptr<Help> mpHelp;
    std::unique_ptr<UartByPass> mpUartConsole;

    Console();
    ~Console() = default;
    
    bool init();
    void add(Cmd& cmd);
    void remove(Cmd& cmd);
    std::vector<std::string> split(const std::string& cmd);
    void task() override;
};

#endif // CONSOLE_HPP
