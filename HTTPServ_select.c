#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<unistd.h>
#include<netinet/in.h>
#include<errno.h>
#include<sys/select.h>
#include<sys/time.h>

#define PORT 8080


void exit_failure(unsigned char *msg);
void handle_client(int cli_fd);

int main(int argc, char **argv)
{
  int i, ret;
  int sock_fd, accept_fd;
  struct sockaddr_in addr, cli_addr;
  socklen_t len;
  int option = 1;
  fd_set fds, orgfds;
  struct timeval waitval;

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

  FD_ZERO(&orgfds);
  FD_SET(sock_fd, &orgfds);
  

  /*Handle Client Loop*/
  while (1) {
    waitval.tv_sec = 2;
    waitval.tv_usec = 500;
    memcpy(&fds, &orgfds, sizeof(orgfds));
    len = sizeof(cli_addr);
    if ((ret = select(FD_SETSIZE, &fds, NULL, NULL, &waitval)) == -1) {
      if (errno == EINTR)
	continue;
      else 
	exit_failure("select");
    } else if (ret == 0) { /*timeout*/
      continue;
    } else { 
      for (i = 0; i < FD_SETSIZE; i++) {
	if (FD_ISSET(i, &fds)) {
	  if (i == sock_fd) {
	    if ((accept_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &len)) == -1)
	      exit_failure("accept");
	    FD_SET(accept_fd, &orgfds);
	  } else {
	    handle_client(i);
	    FD_CLR(i, &orgfds);
	  }
	}
      }
    }
    
  }

  close(sock_fd);

  return 0;
}

void exit_failure(unsigned char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

void handle_client(int cli_fd)
{
  unsigned char recvBuf[1024];
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

  memset(recvBuf, 0, strlen(recvBuf));
  if (recv(cli_fd, recvBuf, 1024, 0) == -1) {
    exit_failure("recv");
  }
  printf("%s", recvBuf);
  
  if (send(cli_fd, HTML, strlen(HTML), 0) == -1) {
    exit_failure("send");
  }

  close(cli_fd);

  return ;
}

