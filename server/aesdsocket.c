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

#define TCP_PORT "9000"
#define SOCKET_LISTEN_BACKLOG_NUMBER 5
#define TEMP_FILE "/var/tmp/aesdsocketdata"
#define MAX_READ_SIZE 1024


int captured_signal = 0;


void signal_handler(int signum)
{
    if (signum == SIGTERM || signum == SIGINT)
    {
    captured_signal = signum;
    }

}



void create_daemon() 
{
  pid_t process_id = 0;
  pid_t sid = 0;
  process_id = fork();

  if (process_id < 0) 
  {
    syslog(LOG_ERR, "Line number = %d : Fork failed",__LINE__);
    exit(1);
  }
  if (process_id > 0) 
  {
    exit(0);
  }
  umask(0);
  sid = setsid();
  if (sid < 0)
    exit(1);

  chdir("/");
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

void set_up_signals()
{
    struct sigaction new_sigaction;

    memset(&new_sigaction, 0, sizeof(struct sigaction));
    new_sigaction.sa_handler = signal_handler;

    if (sigaction(SIGTERM, &new_sigaction, NULL) != 0) 
    {
    syslog(LOG_ERR, "Line number = %d : %d (%s) registeing for SIGTERM",__LINE__, errno, strerror(errno));
    }
    if (sigaction(SIGINT, &new_sigaction, NULL) != 0) 
    {
    syslog(LOG_ERR, "Line number = %d : Error %d (%s) registeing for SIGINT",__LINE__, errno,strerror(errno));
    }
}


int main(int argc, const char **argv) 
{

  openlog(NULL, 0, LOG_USER);

  set_up_signals();

  int socketfd;
  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
  {
    syslog(LOG_ERR, "Line number = %d : Error create socket %d (%s)",__LINE__, errno, strerror(errno));
    exit(1);
  }
  const int enable = 1;
  if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
  {
    syslog(LOG_ERR, "Line number = %d : Error setsockopt(SO_REUSEADDR) %d (%s)",__LINE__ , errno, strerror(errno));
    perror("setsockopt(SO_REUSEADDR) failed");
  }
  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct sockaddr client;

  socklen_t addrlen = sizeof(client);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, TCP_PORT, &hints, &servinfo)) != 0) {
    syslog(LOG_ERR, "Line number = %d getaddrinfo error: %s\n",__LINE__, gai_strerror(status));
    exit(1);
  }

  if ((status = bind(socketfd, servinfo->ai_addr, sizeof(struct sockaddr))) != 0) 
  {
    syslog(LOG_ERR, "Line number = %d : bind error: %s\n",__LINE__, gai_strerror(status));
    exit(1);
  }

  freeaddrinfo(servinfo);

  if (argc ==2 && strcmp(argv[1], "-d") == 0) 
  {
    create_daemon();
  }

  listen(socketfd, SOCKET_LISTEN_BACKLOG_NUMBER);

  while (!captured_signal) 
  {
    int accept_fd = accept(socketfd, &client, &addrlen);
    if (accept_fd == -1) 
    {
      break;
    }

    if (client.sa_family != AF_INET) {
        // Client connection AF_INET (IPv4) changes error.
        close(socketfd);
       break;
    }

    struct sockaddr_in *addr_in = (struct sockaddr_in *)(&client);
    syslog(LOG_DEBUG, "Accepted connection from  %s", inet_ntoa(addr_in->sin_addr));

    char *buffer = malloc(MAX_READ_SIZE);
    memset(buffer, 0, MAX_READ_SIZE);

    FILE *fp = fopen(TEMP_FILE, "ab");
    if (fp == NULL) 
    {
      syslog(LOG_ERR, "Line number = %d : Error opening file aesdsocketedata %s",__LINE__, strerror(errno));
      exit(1);
    }

    size_t totalSize = 0;
    size_t readSize = 0;
    char *ret = NULL;

    while (ret == NULL) 
    {
      readSize = recv(accept_fd, buffer + totalSize, MAX_READ_SIZE, 0);
      totalSize += readSize;
      buffer = realloc(buffer, totalSize + MAX_READ_SIZE);
      memset(buffer + totalSize, 0, MAX_READ_SIZE);
      ret = strchr(buffer, '\n');
    }

    fwrite(buffer, 1, totalSize, fp);
    fclose(fp);
    free(buffer);

    fp = fopen(TEMP_FILE, "r");
    if (fp == NULL) 
    {
      syslog(LOG_ERR, "Line number = %d : Error opening file aesdsocketedata %s",__LINE__, strerror(errno));
      exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    size_t n;

    while ((n = getline(&line, &len, fp)) != -1) 
    {
      send(accept_fd, line, n, 0);
    }

    fclose(fp);
    free(line);
    shutdown(accept_fd, 2);
    syslog(LOG_DEBUG, "Closed connection from  %s",inet_ntoa(addr_in->sin_addr));
  }

  syslog(LOG_DEBUG, "Caught signal, exiting");
  shutdown(socketfd, 2);
  remove(TEMP_FILE);

  return 0;
}
