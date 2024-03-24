/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "sdkconfig.h"
#include "esp_console.h"
#include "esp_log.h"
#include "console_ssh.h"
#include "libssh2_helper.h"

static const char *TAG = "console_ssh";

/**
 * Static registration of this plugin is achieved by defining the plugin description
 * structure and placing it into .console_cmd_desc section.
 * The name of the section and its placement is determined by linker.lf file in 'plugins' component.
 */
static const console_cmd_plugin_desc_t __attribute__((section(".console_cmd_desc"), used)) PLUGIN = {
    .name = "console_cmd_ssh",
    .plugin_regd_fn = &console_cmd_ssh_register
};

typedef struct ssh_op_t {
    char *name;
    esp_err_t (*operation)(struct ssh_op_t *self, int argc, char *argv[]);
    int arg_cnt;
    int start_index;
    char *help;
} ssh_op_t;

static esp_err_t ssh_help_op(ssh_op_t *self, int argc, char *argv[]);
static esp_err_t ssh_connect_op(ssh_op_t *self, int argc, char *argv[]);

static ssh_op_t cmd_list[] = {
    {.name = "help", .operation = ssh_help_op, .arg_cnt = 2, .start_index = 1, .help = "ssh help: Prints the help text for all wifi commands"},
    {.name = "connect",  .operation = ssh_connect_op,  .arg_cnt = 5, .start_index = 1, .help = "ssh connect <host> <user> <password>: SSH connect to given host."},
    {.name = "connect",  .operation = ssh_connect_op,  .arg_cnt = 6, .start_index = 1, .help = "ssh connect <host> <port> <username> <password>: SSH connect to given host."},

};

static esp_err_t ssh_help_op(ssh_op_t *self, int argc, char *argv[])
{
    int cmd_count = sizeof(cmd_list) / sizeof(cmd_list[0]);

    for (int i = 0; i < cmd_count; i++) {
        if ((cmd_list[i].help != NULL) && (strlen(cmd_list[i].help) != 0)) {
            printf(" %s\n", cmd_list[i].help);
        }
    }

    return ESP_OK;
}

#define SSH_CON_PARA_IDX_OFFSET_HOST (1)
#define SSH_CON_PARA_IDX_OFFSET_PORT (2)
#define SSH_CON_PARA_IDX_OFFSET_USER (2)
#define SSH_CON_PARA_IDX_OFFSET_PASSWORD (3)
static esp_err_t ssh_connect_op(ssh_op_t *self, int argc, char *argv[])
{
    ssh_para_t ssh_para = { 0 };
    uint8_t ssh_para_port_exist = 0;

    if (argc == 6) { ssh_para_port_exist = 1;}

    // ESP_LOGI(TAG, "para host = %s", argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_HOST]);
    // if (ssh_para_port_exist == 1) {
    //     ESP_LOGI(TAG, "para port = %s", argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_PORT]);
    // }
    // ESP_LOGI(TAG, "para user = %s", argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_USER + ssh_para_port_exist]);
    // ESP_LOGI(TAG, "para pw = %s", argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_PASSWORD + ssh_para_port_exist]);

    // parse ssh connect parameters
    //// host
    ssh_para.hostLen = strlen(argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_HOST]);
    // ssh_para.host = malloc(ssh_para.hostLen + 1);
    strlcpy((char *) ssh_para.host, argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_HOST], ssh_para.hostLen + 1);
    //// port
    if (ssh_para_port_exist == 0) { 
        ssh_para.port = 22;
    } else {
        ssh_para.port = atoi(argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_PORT]);
    }
    //// user
    ssh_para.userLen = strlen(argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_USER]);
    // ssh_para.user = malloc(ssh_para.userLen + 1);
    strlcpy((char *) ssh_para.user, argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_USER], ssh_para.userLen + 1);
    //// password
    ssh_para.passwordLen = strlen(argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_PASSWORD]);
    // ssh_para.password = malloc(ssh_para.passwordLen + 1);
    strlcpy((char *) ssh_para.password, argv[self->start_index + SSH_CON_PARA_IDX_OFFSET_PASSWORD], ssh_para.passwordLen + 1);

    ESP_LOGI(TAG, "host = %s", ssh_para.host);
    ESP_LOGI(TAG, "port = %d", ssh_para.port);
    ESP_LOGI(TAG, "username = %s", ssh_para.user);
    ESP_LOGI(TAG, "password = %s\n", ssh_para.password);
    // ESP_LOGI(TAG, "hostLen = %d", ssh_para.hostLen);
    // ESP_LOGI(TAG, "usernameLen = %d", ssh_para.userLen);
    // ESP_LOGI(TAG, "passwordLen = %d", ssh_para.passwordLen);

    // Execute ssh command
    // 
    
    ssh_connect_task_para_t ssh_connect_task_para = {0};
    ssh_connect_task_para.ssh_para = &ssh_para;
    // ssh_connect_task_para.shell_command = "uname -a";
    memcpy(ssh_connect_task_para.shell_command, "uname -a", 8);

    ssh_connect_task_set_para(&ssh_connect_task_para);

    xTaskCreate(&ssh_connect_task, "SSH", 1024*8, (void *)&ssh_connect_task_para , 2, NULL);

    ESP_LOGI(TAG, "ssh_connect_op end.");

    return ESP_OK;
}


/* handle 'wifi' command */
static esp_err_t do_ssh_cmd(int argc, char **argv)
{
    int cmd_count = sizeof(cmd_list) / sizeof(cmd_list[0]);
    ssh_op_t cmd;

    for (int i = 0; i < cmd_count; i++) {
        cmd = cmd_list[i];

        if (argc < cmd.start_index + 1) {
            continue;
        }

        if (!strcmp(cmd.name, argv[cmd.start_index])) {
            /* Get interface for eligible commands */
            if (cmd.arg_cnt == argc) {
                if (cmd.operation != NULL) {
                    if (cmd.operation(&cmd, argc, argv) != ESP_OK) {
                        ESP_LOGE(TAG, "Usage:\n%s", cmd.help);
                        return 0;
                    }
                }
                return ESP_OK;
            }
        }
    }

    ESP_LOGE(TAG, "Command not available");

    return ESP_OK;
}

/**
 * @brief Registers the ping command.
 *
 * @return
 *          - esp_err_t
 */
esp_err_t console_cmd_ssh_register(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "console_cmd_ssh_register");

    const esp_console_cmd_t ping_cmd = {
        .command = "ssh",
        .help = "secure shell connect to target",
        .hint = NULL,
        .func = &do_ssh_cmd,
        // .argtable = &ssh_args
    };

    ret = esp_console_cmd_register(&ping_cmd);
    if (ret) {
        ESP_LOGE(TAG, "Unable to register ssh");
    }

    return ret;
}
