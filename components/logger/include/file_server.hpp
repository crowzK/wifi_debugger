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
    FileServerHandler(httpd_handle_t server);
    ~FileServerHandler();

protected:
    static const char* cBasePath;
    std::unique_ptr<std::array<char, SCRATCH_BUFSIZE>> mBuffer;
    
    esp_err_t userHandler(httpd_req *req) override;
    esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);
    const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);
    esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath);
};

#endif //FILE_SERVER_HPP
