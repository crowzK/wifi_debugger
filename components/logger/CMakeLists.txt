idf_component_register(SRCS "uart.cpp" "logger_web.cpp" 
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main nvs_flash app_update blocking_queue
                    EMBED_FILES "root.html")
