#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<unistd.h>
#include<netinet/in.h>
#include<errno.h>
#include<pthread.h>

#define PORT 8080


void exit_failure(unsigned char *msg);
void * handle_client(void *args);

typedef struct thread_arg_t {
  int fd;
  struct sockaddr_in addr;
} thread_arg;

int main(int argc, char **argv)
{
  int sock_fd, cli_fd;
  struct sockaddr_in addr, cli_addr;
  socklen_t len;
  pthread_t thread;
  thread_arg * th_arg;
  int option = 1;
  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    exit_failure("socket");

  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option,sizeof(option)) == -1)
    exit_failure("setsockopt");
  if (bind(sock_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1)
    exit_failure("bind");

  if (listen(sock_fd, 10) == -1) 
    exit_failure("listen");

  /*Handle Client Loop*/
  while (1) {
    len = sizeof(cli_addr);    
    if ((cli_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &len)) == -1)
      exit_failure("accept");
    
    if ((th_arg = (thread_arg *)malloc(sizeof(thread_arg))) == NULL) 
      exit_failure("malloc");
    
    th_arg->fd = cli_fd;
    th_arg->addr = cli_addr;

    if (pthread_create(&thread, NULL, handle_client, (void *) th_arg) != 0) 
      exit_failure("pthread_create");

  }

  close(sock_fd);

  return 0;
}

void exit_failure(unsigned char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

void * handle_client(void *args)
{
  pthread_detach(pthread_self());
  unsigned char recvBuf[1024];
  int cli_fd;
  struct sockaddr_in cli_addr;
  char *HTML = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
    "<head>"
    "<title>HTTP Test</title>"
    "</head>"
    "<body>"
    "<h1>It works</h1>"
    "</body>"
    "</html>";

  cli_fd = ((thread_arg *) args)->fd;
  cli_addr = ((thread_arg *) args)->addr;

  memset(recvBuf, 0, strlen(recvBuf));
  if (recv(cli_fd, recvBuf, 1024, 0) == -1) {
    exit_failure("recv");
  }
  printf("%s\n", recvBuf);
  
  if (send(cli_fd, HTML, strlen(HTML), 0) == -1) {
    exit_failure("send");
  }

  close(cli_fd);
  free(args);

  pthread_exit((void *) 0);
}

