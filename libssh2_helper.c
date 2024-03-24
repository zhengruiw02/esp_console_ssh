/* ssh Client Example

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "netdb.h" // gethostbyname

#include "libssh2_config.h"
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "libssh2_helper.h"

static const char *TAG = "SSH";

ssh_para_t local_ssh_para = { 0 };
ssh_connect_task_para_t local_ssh_connect_task_para = {0};


static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
	struct timeval timeout;
	int rc;
	fd_set fd;
	fd_set *writefd = NULL;
	fd_set *readfd = NULL;
	int dir;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	FD_ZERO(&fd);

	FD_SET(socket_fd, &fd);

	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(session);

	if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;

	if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;

	rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

	return rc;
}

void ssh_connect_task_set_para(ssh_connect_task_para_t *task_para)
{
    // parse ssh connect parameters
    //// host
    local_ssh_para.hostLen = task_para->ssh_para->hostLen;
	memcpy(local_ssh_para.host, task_para->ssh_para->host, local_ssh_para.hostLen);
    //// port
	local_ssh_para.port = task_para->ssh_para->port;
    //// user	
    local_ssh_para.userLen = task_para->ssh_para->userLen;
	memcpy(local_ssh_para.user, task_para->ssh_para->user, local_ssh_para.userLen);
    //// password
	
    local_ssh_para.passwordLen = task_para->ssh_para->passwordLen;
	memcpy(local_ssh_para.password, task_para->ssh_para->password, local_ssh_para.passwordLen);

	local_ssh_connect_task_para.ssh_para= &local_ssh_para;
	memcpy(local_ssh_connect_task_para.shell_command, task_para->shell_command, SSH_HELPER_SHELL_CMD_LEN);
}

#define BUFSIZE 3200

void ssh_connect_task(void *arg)
{
	ESP_LOGI(TAG, "ssh_connect_task entered.");
	ssh_connect_task_para_t *ssh_connect_task_para = (ssh_connect_task_para_t *)&local_ssh_connect_task_para;
	ESP_LOGI(TAG, "Start task_parameter.command=%s", ssh_connect_task_para->shell_command);
	
	ESP_LOGI(TAG, "CONFIG_SSH_HOST=%s", ssh_connect_task_para->ssh_para->host);
	ESP_LOGI(TAG, "CONFIG_SSH_PORT=%d", ssh_connect_task_para->ssh_para->port);
	ESP_LOGI(TAG, "user=%s", ssh_connect_task_para->ssh_para->user);
	ESP_LOGI(TAG, "password=%s", ssh_connect_task_para->ssh_para->password);

	// SSH Staff
	int sock;
	struct sockaddr_in sin;
	LIBSSH2_SESSION *session;
	LIBSSH2_CHANNEL *channel;

	ESP_LOGI(TAG, "libssh2_version is %s", LIBSSH2_VERSION);
	int rc = libssh2_init(0);
	if(rc) {
		ESP_LOGE(TAG, "libssh2 initialization failed (%d)", rc);
		while(1) { vTaskDelay(1); }
	}

	ESP_LOGI(TAG, "CONFIG_SSH_HOST=%s", ssh_connect_task_para->ssh_para->host);
	ESP_LOGI(TAG, "CONFIG_SSH_PORT=%d", ssh_connect_task_para->ssh_para->port);
	sin.sin_family = AF_INET;
	//sin.sin_port = htons(22);
	sin.sin_port = htons(ssh_connect_task_para->ssh_para->port);
	sin.sin_addr.s_addr = inet_addr(ssh_connect_task_para->ssh_para->host);
	ESP_LOGI(TAG, "sin.sin_addr.s_addr=%lx", sin.sin_addr.s_addr);
	if (sin.sin_addr.s_addr == 0xffffffff) {
		struct hostent *hp;
		hp = gethostbyname(ssh_connect_task_para->ssh_para->host);
		if (hp == NULL) {
			ESP_LOGE(TAG, "gethostbyname fail %s", ssh_connect_task_para->ssh_para->host);
			while(1) { vTaskDelay(1); }
		}
		struct ip4_addr *ip4_addr;
		ip4_addr = (struct ip4_addr *)hp->h_addr;
		sin.sin_addr.s_addr = ip4_addr->addr;
		ESP_LOGI(TAG, "sin.sin_addr.s_addr=%lx", sin.sin_addr.s_addr);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		ESP_LOGE(TAG, "failed to create socket!");
		while(1) { vTaskDelay(1); }
	}

	if(connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
		ESP_LOGE(TAG, "failed to connect!");
		while(1) { vTaskDelay(1); }
	}

	/* Create a session instance */
	session = libssh2_session_init();
	if(!session) {
		ESP_LOGE(TAG, "failed to session init");
		while(1) { vTaskDelay(1); }
	}

	/* ... start it up. This will trade welcome banners, exchange keys,
	 * and setup crypto, compression, and MAC layers
	 */
	rc = libssh2_session_handshake(session, sock);
	if(rc) {
		ESP_LOGE(TAG, "Failure establishing SSH session: %d", rc);
		while(1) { vTaskDelay(1); }
	}

	/* We could authenticate via password */
	if(libssh2_userauth_password(session, ssh_connect_task_para->ssh_para->user, ssh_connect_task_para->ssh_para->password)) {
		ESP_LOGE(TAG, "Authentication by password failed.");
		ESP_LOGE(TAG, "Authentication username : [%s].", ssh_connect_task_para->ssh_para->user);
		while(1) { vTaskDelay(1); }
	}

#if 0
	/* We could authenticate via privatekey */
	char publickey[64];
	char privatekey[64];
	strcpy(publickey, "/spiffs/id_rsa.pub");
	strcpy(privatekey, "/spiffs/id_rsa");
	if(libssh2_userauth_publickey_fromfile(session, CONFIG_SSH_USER, publickey, privatekey, NULL)) {
		ESP_LOGE(TAG, "Authentication by privatekey failed.");
		ESP_LOGE(TAG, "Authentication username : [%s].", CONFIG_SSH_USER);
		while(1) { vTaskDelay(1); }
	}
#endif


	libssh2_trace(session, LIBSSH2_TRACE_SOCKET);

	/* Exec non-blocking on the remove host */
	while((channel = libssh2_channel_open_session(session)) == NULL &&
		libssh2_session_last_error(session, NULL, NULL, 0) ==
		LIBSSH2_ERROR_EAGAIN) {
		waitsocket(sock, session);
	}
	if(channel == NULL) {
		ESP_LOGE(TAG, "libssh2_channel_open_session failed.");
		while(1) { vTaskDelay(1); }
	}

	while((rc = libssh2_channel_exec(channel, ssh_connect_task_para->shell_command)) == LIBSSH2_ERROR_EAGAIN)
	waitsocket(sock, session);
	if(rc != 0) {
		ESP_LOGE(TAG, "libssh2_channel_exec failed: %d", rc);
		while(1) { vTaskDelay(1); }
	}

	int bytecount = 0;
	for(;;) {
		/* loop until we block */
		int rc;
		do {
			char buffer[128];
			rc = libssh2_channel_read(channel, buffer, sizeof(buffer) );
			if(rc > 0) {
				int i;
				bytecount += rc;
				//fprintf(stderr, "We read:\n");
				for(i = 0; i < rc; ++i)
					//fputc(buffer[i], stderr);
					fputc(buffer[i], stdout);
				//fprintf(stderr, "\n");
			}
			else if(rc < 0) {
					/* no need to output this for the EAGAIN case */
					ESP_LOGI(TAG, "libssh2_channel_read returned %d", rc);
					//while(1) { vTaskDelay(1); }
			}
		}
		while(rc > 0);

		/* this is due to blocking that would occur otherwise so we loop on
		 this condition */
		if(rc == LIBSSH2_ERROR_EAGAIN) {
			waitsocket(sock, session);
		}
		else
			break;
	} // end for
	printf("\n");


	int exitcode = 127;
	char *exitsignal = (char *)"none";
	while((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
	waitsocket(sock, session);
	if(rc == 0) {
		exitcode = libssh2_channel_get_exit_status(channel);
		libssh2_channel_get_exit_signal(channel, &exitsignal,
										NULL, NULL, NULL, NULL, NULL);
	} else {
		ESP_LOGE(TAG, "libssh2_channel_close failed: %d", rc);
		while(1) { vTaskDelay(1); }
	}

	if(exitsignal)
		ESP_LOGI(TAG, "EXIT: %d, SIGNAL: %s, bytecount: %d", exitcode, exitsignal, bytecount);
		//ESP_LOGI(TAG, "Got signal: %s", exitsignal);
	else
		ESP_LOGI(TAG, "EXIT: %d, bytecount: %d", exitcode, bytecount);

	libssh2_channel_free(channel);
	channel = NULL;

	// Close a session
	libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
	libssh2_session_free(session);

	// Close socket
	close(sock);
	ESP_LOGI(TAG, "[%s] done", ssh_connect_task_para->shell_command);

	libssh2_exit();

	vTaskDelete( NULL );
}
