idf_component_register(SRCS "console.cpp" "uart_bypass.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES console esp-idf-cpp io blocking_queue logger)
