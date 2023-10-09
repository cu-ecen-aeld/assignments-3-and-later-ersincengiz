#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define TCP_PORT "9000"
#define SOCKET_LISTEN_BACKLOG_NUMBER 5
#define MAX_READ_SIZE 1024
#define USE_AESD_CHAR_DEVICE 1


const char* TEMP_FILE = (USE_AESD_CHAR_DEVICE == 1) ? "/dev/aesdchar" : "/var/tmp/aesdsocketdata";
int sockfd;

struct LinkedList {
	struct LinkedList *next;
	pthread_t thread_id;
	struct SocketData *socket;
};


struct SocketData {
	bool socket_completed;
	pthread_mutex_t *mutex;
	int accepted_fd;
	char *message;
};

//init mutex
pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;

bool server_is_running = true;

struct LinkedList *HEAD = NULL;

void signal_handler(int signum);
void create_daemon();
void cleanup_socket(bool socket_was_terminated);
void linked_list_add_node(struct LinkedList *node);
void timer_10sec(int signal_number);


void signal_handler(int signum)
{
    if (signum == SIGTERM || signum == SIGINT)
    {
    server_is_running = false;
		cleanup_socket(true);
		close(sockfd);
		if (0 == USE_AESD_CHAR_DEVICE)
		{
			remove(TEMP_FILE);
		}
		syslog(LOG_DEBUG, "Killed aesdsocket");
		exit(EXIT_SUCCESS);

    }
    else
    {

    syslog(LOG_DEBUG, "Failed to kill aesdsocket");
		exit(EXIT_FAILURE);
    
    }

}



void create_daemon() 
{
    pid_t pid;
    
    pid = fork();
    
    if (pid < 0)
    {
      exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
      exit(EXIT_SUCCESS);
    }
    
    if (setsid() < 0)
    {
      exit(EXIT_FAILURE);
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    pid = fork();
    if (pid < 0)
    {
      exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
      exit(EXIT_SUCCESS);
    }
    
    umask(0);
    
    chdir("/");
    int x;
    for ( x = 2; x>=0; x--)
    {
      close(x);
    }
}

void cleanup_socket(bool socket_was_terminated)
{
	struct LinkedList *current_head = HEAD;
	struct LinkedList *prev_head = NULL;
	while (NULL != current_head)
	{
		if (socket_was_terminated || current_head->socket->socket_completed)
		{
			pthread_join(current_head->thread_id, NULL);
			if (0 <= current_head->socket->accepted_fd)
			{
				close(current_head->socket->accepted_fd);
			}
			if (NULL != current_head->socket->message)
			{
				free(current_head->socket->message);
			}
			free(current_head->socket);
			if (NULL == prev_head)
			{
				HEAD = current_head->next;
				free(current_head);
				current_head = HEAD;
			}
			else
			{
				prev_head->next = current_head->next;
				free(current_head);
				current_head = prev_head->next;
			}
			
		}
		else
		{
			prev_head = current_head;
			current_head = prev_head->next;
		}
	}
}


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void timer_10sec(int signal_number)
{
	char timestamp[200];
	char output_timestamp[300];
	time_t abs_time;
	struct tm *local_time;
	FILE *fp;

	syslog(LOG_DEBUG, "aesdsocket testing time script");
	strcpy(output_timestamp, "timestamp:");
	abs_time = time(NULL);
	local_time = localtime(&abs_time);
	strftime(timestamp, sizeof(timestamp), "%a, %d %b %Y %T %z", local_time);
	strcat(output_timestamp, timestamp);
	strcat(output_timestamp, "\n");
	pthread_mutex_lock(&MUTEX);
	fp = fopen(TEMP_FILE, "a+");
	fwrite(output_timestamp, sizeof(char), strlen(output_timestamp), fp);
	fclose(fp);
	pthread_mutex_unlock(&MUTEX);
	syslog(LOG_DEBUG, "aesdsocket ending time script");
}


void linked_list_add_node(struct LinkedList *node)
{
	if (NULL == HEAD)
	{
		HEAD = node;
	}
	else
	{
		struct LinkedList *current_head = HEAD;
		while (current_head->next)
		{
			current_head = current_head->next;
		}
		current_head->next = node;
	}
}

void *socket_thread(void *socket_param)
{
	syslog(LOG_DEBUG, "aesdsocket in accepted connection");
	struct SocketData* socket = (struct SocketData*) socket_param;
	int msg_len = 0;
	int bytes_received = 0;
	char msg_buffer[MAX_READ_SIZE];
	char output_buffer[MAX_READ_SIZE];
	int bytes_read;
	bool is_receiving_message = true;
	FILE *fp;
	
	syslog(LOG_DEBUG, "aesdsocket in connection: variables assigned");
	
	while (is_receiving_message)
	{
		bytes_received = recv(socket->accepted_fd, msg_buffer, MAX_READ_SIZE, 0);
		if (bytes_received < 0)
		{
			perror("Bytes received: ");
			break;
		}
		else if (bytes_received == 0)
		{
			is_receiving_message = false;
		}
		else
		{
			
			socket->message = realloc(socket->message, msg_len + bytes_received);
		
			if (NULL == socket->message)
			{
				printf("Not enough memory\n\r");
			}
			else
			{
				for (int i = 0; i<bytes_received;i++)
				{
					socket->message[msg_len] = msg_buffer[i];
					++msg_len;
					if (msg_buffer[i] == '\n')
					{
						pthread_mutex_lock(socket->mutex);
						fp = fopen(TEMP_FILE, "a+");
						fwrite(socket->message, sizeof(char), msg_len, fp);
						rewind(fp);
						while ((bytes_read = fread(output_buffer, 1, MAX_READ_SIZE, fp)) > 0)
						{
							send(socket->accepted_fd, output_buffer, bytes_read, 0);
						}
						fclose(fp);
						pthread_mutex_unlock(socket->mutex);
						msg_len = 0;
					}
				}
			}
		}
	}
	
	return NULL;
}

int main(int argc, char *argv[])
{
	struct sockaddr_storage received_addr;
  struct addrinfo send_configure, *recv_configure;
	socklen_t addr_size;
	int status;
	char received_IP[INET6_ADDRSTRLEN];
	bool is_there_daemon = false;
	int opt;
	
	struct itimerval delay;
	
	delay.it_value.tv_sec = 10;
	delay.it_value.tv_usec = 0;
	delay.it_interval.tv_sec = 10;
	delay.it_interval.tv_usec = 0;
	
	
	//Is there daemon?
	while ((opt = getopt(argc, argv, "d")) != -1)
	{
		if (opt == 'd')
		{
			is_there_daemon = true;
		}
	}
	
	memset(&send_configure, 0 , sizeof(send_configure));
	send_configure.ai_family = AF_UNSPEC;
	send_configure.ai_socktype = SOCK_STREAM;
	send_configure.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(NULL, TCP_PORT, &send_configure, &recv_configure);
	
	if (status == -1)
	{
		perror("getaddrinfo failed: ");
		return -1;
	}
	
	sockfd = socket(recv_configure->ai_family, recv_configure->ai_socktype, recv_configure->ai_protocol);
	
	if (sockfd == -1)
	{
		perror("socket failed: ");
		return -1;
	}
	
	status = bind(sockfd, recv_configure->ai_addr, recv_configure->ai_addrlen);
	
	if (status == -1)
	{
		perror("bind failed: ");
		return -1;
	}
	
	if (is_there_daemon)
	{
		create_daemon();
	}
	else
	{
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);
	}
	
	syslog(LOG_DEBUG, "aesdsocket started daemon");
	
	if (USE_AESD_CHAR_DEVICE != 1)
	{
		signal(SIGALRM, timer_10sec);
		setitimer(ITIMER_REAL, &delay, NULL);
	}	
	
	freeaddrinfo(recv_configure);
	
	
	
	status = listen(sockfd, SOCKET_LISTEN_BACKLOG_NUMBER);
	
	if (status == -1)
	{
		perror("listen failed: ");
		return -1;
	}

	
	syslog(LOG_DEBUG, "aesdsocket starting server");
	
	while (server_is_running)
	{
		
		addr_size = sizeof(received_addr);
		
		syslog(LOG_DEBUG, "Setting the aesdsocket structure.");
		
		struct LinkedList *node = (struct LinkedList *) malloc(sizeof(struct LinkedList));
		node->next = NULL;
		node->thread_id = 0;
		node->socket = (struct SocketData *) malloc(sizeof(struct SocketData));
		node->socket->socket_completed = false;
		node->socket->mutex = &MUTEX;
		node->socket->message = NULL;
		node->socket->accepted_fd = -1;
		linked_list_add_node(node);
		
		node->socket->accepted_fd = accept(sockfd, (struct sockaddr *)&received_addr, &addr_size);
		
		syslog(LOG_DEBUG, "aesdsocket has been configured and added to Linkedlist");
		
		if (node->socket->accepted_fd == -1)
		{
			syslog(LOG_DEBUG, "aesdsocket failed to accept connection");
			perror("accept failed: ");
			continue;
		}
		
		inet_ntop(received_addr.ss_family, get_in_addr((struct sockaddr *) &received_addr), received_IP, sizeof(received_IP));
		syslog(LOG_DEBUG, "Accepted connection from %s\n", received_IP);
		
		syslog(LOG_DEBUG, "aesdsocket adding node");
		
		
		syslog(LOG_DEBUG, "aesdsocket adding node finished");
		
		pthread_create(&(node->thread_id), NULL, socket_thread, (void *) node->socket);
		
		cleanup_socket(false);
			
	}
	return 0;
}
