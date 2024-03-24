idf_component_register(SRCS "libssh2_helper.c" "console_ssh.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES libssh2 console
                    WHOLE_ARCHIVE)
