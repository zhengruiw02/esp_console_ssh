# esp-idf-console-ssh
ssh client for esp-idf.   
You can use the ssh API to execute remote command.   
This project use [this](https://github.com/libssh2/libssh2) ssh library.   

# Software requirements
ESP-IDF Ver5.1 which you need to config to use mbed TLS v1.3, which can be config in `sdkconfig` by `CONFIG_MBEDTLS_SSL_PROTO_TLS1_3`.


# Installation

```
git clone https://github.com/zhengruiw02/esp_console_ssh.git
cd esp_console_ssh/
git submodule update --init
idf.py set-target {esp32/esp32s2/esp32s3/esp32c3}
idf.py menuconfig
idf.py flash
```

# Console commands

```
ssh help: Prints the help text for all wifi commands.
ssh connect <host> <user> <password>: SSH connect to given host.
ssh connect <host> <port> <username> <password>: SSH connect to given host with specify port.
```