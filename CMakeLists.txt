set(component_def "-DHAVE_CONFIG_H")

set(component_include
    "."
	libssh2/include
	)

set(component_srcs
    console_ssh.c
    libssh2_helper.c
	libssh2/src/agent.c
	libssh2/src/bcrypt_pbkdf.c
	libssh2/src/blowfish.c
	libssh2/src/channel.c
	libssh2/src/comp.c
	libssh2/src/crypt.c
	libssh2/src/crypto.c
	libssh2/src/global.c
	libssh2/src/hostkey.c
	libssh2/src/keepalive.c
	libssh2/src/kex.c
	libssh2/src/knownhost.c
	libssh2/src/libgcrypt.c
	libssh2/src/mac.c
	libssh2/src/mbedtls.c
	libssh2/src/misc.c
	libssh2/src/openssl.c
	libssh2/src/os400qc3.c
	libssh2/src/packet.c
	libssh2/src/pem.c
	libssh2/src/publickey.c
	libssh2/src/scp.c
	libssh2/src/session.c
	libssh2/src/sftp.c
	libssh2/src/transport.c
	libssh2/src/userauth.c
	libssh2/src/version.c
	libssh2/src/wincng.c
	)

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "${component_include}"
                       PRIV_REQUIRES mbedtls console
                       WHOLE_ARCHIVE)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE ${component_def})
target_compile_options(${COMPONENT_TARGET} PRIVATE -Wno-maybe-uninitialized)
