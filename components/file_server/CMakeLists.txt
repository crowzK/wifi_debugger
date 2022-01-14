idf_component_register(SRCS "file_server.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main 
                    EMBED_FILES "favicon.ico" "upload_script.html")
