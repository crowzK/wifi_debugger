idf_component_register(SRCS "log_file.cpp" "uart.cpp" "logger_web.cpp" "msg_proxy.cpp" "cmd.cpp" "file_server.cpp" "setting.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main
                    EMBED_FILES "root.html")
