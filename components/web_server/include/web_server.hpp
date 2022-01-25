#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <list>
#include <mutex>
#include "esp_http_server.h"
#include <esp_event.h>

class UriHandler
{
public:
    const httpd_uri_t cUri;
    UriHandler(const char* uri, httpd_method_t method, esp_err_t (*wsHandler)(httpd_req_t *r) = nullptr);
    ~UriHandler();

    void start(httpd_handle_t serverHandle);
    void stop(httpd_handle_t serverHandle);

protected:
    static esp_err_t handler(httpd_req_t *req);
    virtual esp_err_t userHandler(httpd_req *req) = 0;
};

class WebServer
{
public:
    static WebServer& get();
    void add(UriHandler& handler);
    void remove(UriHandler& handler);

protected:
    std::recursive_mutex mMutex;
    std::list<UriHandler*> mUriHandlers;

    httpd_handle_t serverHandle;

    WebServer();
    ~WebServer();

    static void handler(WebServer* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};


#endif // WEB_SERVER_HPP
