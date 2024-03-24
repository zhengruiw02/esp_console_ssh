
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

typedef struct ssh_para_t{
    char host[40];
    int  port;
    char user[40];
    char password[40];
    int  hostLen;
    int  userLen;
    int  passwordLen;
} ssh_para_t;

#define SSH_HELPER_SHELL_CMD_LEN (100)
typedef struct{
    ssh_para_t  *ssh_para;
    char        shell_command[SSH_HELPER_SHELL_CMD_LEN];
}ssh_connect_task_para_t;

void ssh_connect_task_set_para(ssh_connect_task_para_t *task_para);

/**
 * @brief Run the ssh connect.
 *
 * @return
 *          - esp_err_t
 */

void ssh_connect_task(void *arg);

#ifdef __cplusplus
}
#endif