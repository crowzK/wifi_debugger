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

#ifndef FILE_HPP
#define FILE_HPP

#include <memory>
#include <mutex>
#include <vector>

class FsManager;

class File
{
public:
    static bool isFileExists(const std::string& filePath);
    File();
    ~File() = default;

    bool open(const std::string& filePath);
    bool close();
    int write(const std::vector<uint8_t>& data);
    std::vector<uint8_t> read(int count);

protected:
    std::recursive_mutex mMutex;
    const std::string cFilePath;
    FsManager& mFsManager;
};

#endif // FILE_HPP
