idf_component_register(SRCS "console.cpp" "log_console_out.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES console esp-idf-cpp io blocking_queue logger)