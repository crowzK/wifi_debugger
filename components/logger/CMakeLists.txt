idf_component_register(SRCS "logger.cpp" "uart.cpp" "logger_web.cpp" 
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main nvs_flash app_update
                    EMBED_FILES "root.html")
