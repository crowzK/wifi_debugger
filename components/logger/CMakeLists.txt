idf_component_register(SRCS "sdcard.cpp" "uart.cpp" "logger_web.cpp" "task.cpp" "debug_msg_handler.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main nvs_flash app_update blocking_queue
                    EMBED_FILES "root.html")
