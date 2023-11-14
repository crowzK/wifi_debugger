idf_component_register(SRCS "pyocd_io_console.cpp" "console.cpp" "uart_bypass.cpp" "pyocd_io_console.cpp" "pyocd_io_uart.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES console esp-idf-cpp io blocking_queue logger storage driver pyocd_probe_server esp_tinyusb)
