set(src_dirs 
    libs/socket
    libs/JSON_Decoder
    pyocd_server/src
    swd/src
    flash_writer/src
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
    libs/ELFIO
    swd/include
    dap_link/include
)

idf_component_register(
    SRC_DIRS "${src_dirs}"
    EXCLUDE_SRCS "${exclude_srcs}"
    INCLUDE_DIRS "${include_dirs}" 
    PRIV_REQUIRES driver storage
)

