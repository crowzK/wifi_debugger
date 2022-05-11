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

#ifndef FILE_SYSTEM_MANAGER_HPP
#define FILE_SYSTEM_MANAGER_HPP

#include <memory>
#include <mutex>

class SdCard;

class FsManager
{
public:
    static FsManager& create();

    bool mount();
    bool umount();
    const char* getMountPoint()
    {
        return cMountPoint;
    }

    bool isMount()
    {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        return mpSdcard != nullptr;        
    }

protected:
    static const char* cMountPoint;
    std::recursive_mutex mMutex;
    std::unique_ptr<SdCard> mpSdcard;

    FsManager();
    ~FsManager();
};

#endif // FILE_SYSTEM_MANAGER_HPP
