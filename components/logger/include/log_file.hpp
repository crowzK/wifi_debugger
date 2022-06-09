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

#ifndef LOG_FILE_HPP
#define LOG_FILE_HPP

#include <stdio.h>
#include <mutex>
#include <string>
#include "debug_msg_handler.hpp"
#include "fs_manager.hpp"

//! It is SD card class inherit logger client
class LogFile : public Client
{
public:
    static LogFile& create();
    void init();
    const std::string getFilePath();

protected:
    std::recursive_timed_mutex mMutex;
    FsManager& mFsManager;
    const char* cMountPoint;

    FILE* pFile;
    std::string mFilePath;

    LogFile();
    ~LogFile();

    //! \brief Create Log file
    FILE* createFile();

    //! \brief Write a mesage to the SD card
    //! \param msg message pointer
    //! \param length message length
    bool write(const char* msg, uint32_t length);

    //! \brief Write a mesage to the SD card
    //! \param msg message vector
    bool write(const std::vector<uint8_t>& msg) override;
};

#endif
