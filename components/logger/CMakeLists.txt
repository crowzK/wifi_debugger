idf_component_register(SRCS "log_file.cpp" "uart.cpp" "logger_web.cpp" "task.cpp" "debug_msg_handler.cpp" "cmd.cpp" "file_server.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main nvs_flash app_update blocking_queue fs_manager
                    EMBED_FILES "root.html")
