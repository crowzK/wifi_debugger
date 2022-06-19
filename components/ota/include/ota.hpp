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

#ifndef OTA_HPP
#define OTA_HPP

#include <string>
#include <vector>
#include "bin_upload.hpp"

class Ota
{
public:
    static const char* cBinFileDir;
    static const char* cBinFileName;

    static Ota& create();
    static std::string getBinFilePath();

    //! \brief Firmware update with given file
    void update(const std::string& filePath);

    //! \brief Search firmware binary files from the SD card
    std::vector<std::string> searchBins();


protected:
    BinUploadHandler mUploadHandler;

    Ota();
    ~Ota() = default;

    void printSha256(const uint8_t *image_hash, const char *label) const;
    bool firmwareBinCheck(uint8_t* binHeader) const;
};

#endif // OTA_HPP
