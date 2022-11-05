idf_component_register(SRCS "fs_manager.cpp" "sdcard.cpp" "setting.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES nvs_flash fatfs)
