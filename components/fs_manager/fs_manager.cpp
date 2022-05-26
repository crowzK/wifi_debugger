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

#include "fs_manager.hpp"
#include "sdcard.hpp"

const char* FsManager::cMountPoint = "/sdcard";

FsManager& FsManager::create()
{
    static FsManager fm;
    return fm;
}

FsManager::FsManager()
{
    mount();
}

FsManager::~FsManager()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    umount();
}

bool FsManager::mount()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    if(not mpSdcard)
    {
        mpSdcard = std::make_unique<SdCard>(cMountPoint);
    }
    return true;
}

bool FsManager::umount()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    // Does not support unmount yet.
    //if(mpSdcard)
    //{
    //    mpSdcard.reset();
    //}
    return true;
}

bool FsManager::isMount()
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mpSdcard and mpSdcard->isInit();        
}
