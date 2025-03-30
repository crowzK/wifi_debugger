set(src_dirs 
    libs/socket
    libs/JSON_Decoder
    pyocd_server/src
    swd/src
    src/
)

set(exclude_srcs 
    pyocd_server/src/SW_DP.c 
)

set(include_dirs 
    .
    pyocd_server/src 
    pyocd_server/include
    libs/socket
    libs/JSON_Decoder
    swd/include
    include
)

idf_component_register(
    SRC_DIRS "${src_dirs}"
    EXCLUDE_SRCS "ocd.cpp" "${exclude_srcs}"
    INCLUDE_DIRS "${include_dirs}" 
    PRIV_REQUIRES driver storage esp-idf-cpp
)

