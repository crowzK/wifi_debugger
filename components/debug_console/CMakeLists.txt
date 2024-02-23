if(CONFIG_TINYUSB_CDC_ENABLED)
idf_component_register(SRCS "pyocd_io_console.cpp" "console.cpp" "uart_bypass.cpp" "pyocd_io_console.cpp" "pyocd_io_uart.cpp" "console_s3.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES console esp-idf-cpp io blocking_queue logger storage pyocd_probe_server esp_tinyusb driver)
else()
idf_component_register(SRCS "pyocd_io_console.cpp" "console.cpp" "uart_bypass.cpp" "pyocd_io_console.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES console esp-idf-cpp io blocking_queue logger storage pyocd_probe_server driver)
endif()