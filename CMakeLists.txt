idf_component_register(SRCS "console_ssh.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES libssh2 console
                    WHOLE_ARCHIVE)
