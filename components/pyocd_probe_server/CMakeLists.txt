set(src_dirs 
    . 
    pyocd_server/src
    pyocd_server/libs/socket
)

set(exclude_srcs 
    pyocd_server/src/SW_DP.c 
)

set(include_dirs 
    .
    pyocd_server/src 
    pyocd_server/include
    pyocd_server/libs/ArduinoJson/src
    pyocd_server/libs/socket
)

idf_component_register(
    SRC_DIRS "${src_dirs}"
    EXCLUDE_SRCS "${exclude_srcs}"
    INCLUDE_DIRS "${include_dirs}" 
    PRIV_REQUIRES main
)

