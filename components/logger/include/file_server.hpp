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

#ifndef FILE_SERVER_HPP
#define FILE_SERVER_HPP

#include "logger_web.hpp"
#include "esp_vfs.h"

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

class FileServerHandler : public UriHandler
{
public:
    static FileServerHandler& create();

protected:
    static const char* cBasePath;
    std::unique_ptr<std::array<char, SCRATCH_BUFSIZE>> mBuffer;
    
    FileServerHandler();
    ~FileServerHandler() = default;

    esp_err_t userHandler(httpd_req *req) override;
    esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);
    const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);
    esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath);
};

#endif //FILE_SERVER_HPP
