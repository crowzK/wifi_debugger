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

extern "C"
{
    #include <dirent.h>
}

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_system.h"
#include <vector>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "file_server.hpp"
#include "fs_manager.hpp"
#include "bin_upload.hpp"
#include "ota.hpp"

static const char *TAG = "bin_upload";

static void rebootTimerCb(void *arg)
{
    esp_restart();
}
static std::string uriPath = std::string("/binupload/") + std::string(Ota::cBinFileName);

BinUploadHandler::BinUploadHandler() :
    UriHandler(uriPath.c_str(), HTTP_POST),
    cBasePath(FsManager::create().getMountPoint())
{

}

esp_err_t BinUploadHandler::userHandler(httpd_req *req)
{
    auto path = Ota::getBinFilePath();
    const char* filepath = path.c_str();
    printf("file %s\n", filepath);
    FILE *fd = NULL;
    struct stat file_stat;

    if (stat(filepath, &file_stat) == 0) {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }

    fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    /* Retrieve the pointer to scratch buffer for temporary storage */
    std::vector<char> buf;
    buf.resize(SCRATCH_BUFSIZE);
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    while (remaining > 0) {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, buf.data(), MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (received && (received != fwrite(buf.data(), 1, received, fd))) {
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    /* Close file upon upload completion */
    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File uploaded successfully");

    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = &rebootTimerCb,
            /* argument specified here will be passed to timer callback function */
            .arg = (void*) nullptr,
            .name = "one-shot"
    };
    esp_timer_handle_t oneshot_timer;
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 1000000));
    
    return ESP_OK;
}