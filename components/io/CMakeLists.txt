idf_component_register(SRCS "led.cpp" "status.cpp" "button.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main esp-idf-cpp)
