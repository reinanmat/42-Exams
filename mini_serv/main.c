#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

const int MAX_CONNECTIONS = 1024;
const int BUFFER_SIZE = 2000000;

typedef struct s_client {
	int id;
	int fd;
	char *msg;
}	t_client;

typedef struct s_server {
	int fd;
	struct sockaddr_in	addr_in;
	struct sockaddr *addr;
	socklen_t	addrlen;
}	t_server;

t_client clients[1024] = {0};
t_server server;

int g_fds = 0;
int g_id = 0;

char recv_buffer[1024 * 8] = {0};
char server_msg[100] = {0};
char *send_msg = NULL;

fd_set  active_fds;
fd_set  read_fds;

void    err(char *msg) {
	write(2, msg, strlen(msg));
	write(2, "\n", 1);
	exit(1);
}

void	free_and_close_all() {
	if (server.fd != -1) {
		close(server.fd);
		server.fd = -1;
	}

	if (send_msg) {
		free(send_msg);
		send_msg = NULL;
	}
	
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		clients[i].id = -1;
		if (clients[i].fd != -1) {
			close(clients[i].fd);
			clients[i].fd = -1;
		}
		if (clients[i].msg) {
			free(clients[i].msg);
			clients[i].msg = NULL;
		}
	}
}

void    send_for_all(char *msg, size_t msg_size, int fd_except) {
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		int curr_fd = clients[i].fd;
		if (curr_fd != -1 && FD_ISSET(curr_fd, &active_fds) && curr_fd != fd_except) {
			send(curr_fd, msg, msg_size, MSG_NOSIGNAL);
		}
	}
}

void    add_new_client() {
	int new_client_fd = accept(server.fd, 0, 0);
	if (new_client_fd < 0) {
		free_and_close_all();
		err("Fatal error");
	}

	// SERVER FULL
	if (new_client_fd - 4 > MAX_CONNECTIONS - 1) {
		close(new_client_fd);
		return ;
	}

	if (new_client_fd > g_fds)
		g_fds = new_client_fd;

	FD_SET(new_client_fd, &active_fds);
	clients[new_client_fd - 4] = (t_client){
		.id = g_id,
		.fd = new_client_fd,
		.msg = calloc(sizeof(char), BUFFER_SIZE),
	};

	if (!clients[new_client_fd - 4].msg) {
		free_and_close_all();
		err("Fatal error");
	}

	sprintf(server_msg, "server: client %d just arrived\n", g_id);
	send_for_all(server_msg, strlen(server_msg), new_client_fd);
	bzero(server_msg, sizeof(server_msg));

	g_id++;
}

void    read_client_msg(t_client *client) {
	int bytes = recv(client->fd, recv_buffer, sizeof(recv_buffer), 0);

	if (bytes <= 0) {
		sprintf(server_msg, "server: client %d just left\n", client->id);
		send_for_all(server_msg, strlen(server_msg), client->fd);
		bzero(server_msg, sizeof(server_msg));

		client->id = -1;
		FD_CLR(client->fd, &active_fds);
		close(client->fd);
		free(client->msg);
		client->fd = -1;
	} else {
		int j = strlen(client->msg);

		for (int i = 0; i < bytes; i++) {
			client->msg[j++] = recv_buffer[i];
			if (recv_buffer[i] == '\n') {
				client->msg[j] = '\0';
				send_msg = calloc(sizeof(char), strlen(client->msg) + 100);
				if (!send_msg) {
					free_and_close_all();
					err("Fatal error");
				}
				sprintf(send_msg, "client %d: %s", client->id, client->msg);
				send_for_all(send_msg, strlen(send_msg), client->fd);
				free(send_msg);
				bzero(client->msg, BUFFER_SIZE);
				j = 0;
			}
		}
	}

	bzero(recv_buffer, sizeof(recv_buffer));
}

int main(int argc, char **argv) {
	if (argc != 2)
		err("Wrong number of arguments");

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		err("Fatal error");

	server = (t_server){
		.fd = server_fd,
		.addrlen = sizeof(server.addr_in),
		.addr_in.sin_family = AF_INET,
		.addr_in.sin_addr.s_addr = INADDR_ANY,
		.addr_in.sin_port = htons(atoi(argv[1])),
		.addr = (struct sockaddr *)&server.addr_in,
	};

	if (bind(server.fd, server.addr, server.addrlen) < 0 
		|| listen(server.fd, MAX_CONNECTIONS) < 0) {
		close(server.fd);
		err("Fatal error");
	}

	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		clients[i].id = -1;
		clients[i].fd = -1;
		clients[i].msg = NULL;
	}

	FD_ZERO(&active_fds);
	FD_SET(server.fd, &active_fds);
	g_fds = server.fd;

	while (42) {
		read_fds = active_fds;
		if (select(g_fds + 1, &read_fds, 0, 0, 0) == -1) {
			continue ;
		}

		if (FD_ISSET(server.fd, &read_fds))
			add_new_client();
		for (int i = 0; i < MAX_CONNECTIONS; i++) {
			if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds)) {
				read_client_msg(&clients[i]);
			}
		}
	}
}
