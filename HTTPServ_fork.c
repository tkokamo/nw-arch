#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<unistd.h>
#include<netinet/in.h>
#include<errno.h>

#define PORT 22636

static int code;

void exit_failure(unsigned char *msg);
unsigned char*  request_check(unsigned char *buf);
void handle_client(int cli_fd, struct sockaddr_in cli_addr);


int main(int argc, char **argv)
{
  int sock_fd, cli_fd;
  struct sockaddr_in addr, cli_addr;
  socklen_t len;
  pid_t pid, wait_pid;
  int status;
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

    /*fork*/
    if ((pid = fork()) == -1) //error
      exit_failure("fork");

    if (pid == 0) { /*Child Process*/
      close(sock_fd);
      handle_client(cli_fd, cli_addr);

      return 0;
    } else { /*Parent Process*/      
      while ((wait_pid = waitpid(-1, &status, WNOHANG)) > 0) ;
      //while ((wait_pid = waitpid(-1, &status, 0)) > 0) ;
      
      if (wait_pid < 0 && errno != ECHILD) {
	exit_failure("waitpid");
      }
    }
    close(cli_fd);
  }

  close(sock_fd);

  return 0;
}

void exit_failure(unsigned char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

/*Header Check*/
unsigned char* request_check(unsigned char *buf)
{
  unsigned char *p = buf, *path_point, *path;
  unsigned char c;
  unsigned int depth = 0;
  unsigned int pathLen = 0;

  /*allow only GET request*/
  if (strncmp(p, "GET ", 4)) return NULL;
  p += 4;
  
  while ((c = *(p++)) == ' ') {
    ;
  }
  if (c != '/') return NULL;
  path_point = p;
  /*get path length and depth */
  /*common*/
  do {
    if (c == '/') {
      if (strncmp(p, "../", 3) == 0) 
	depth--;
      else 
	depth++;
    }
    pathLen++;
  } while ((c = *(p++)) != ' ' && c != '\n');
  
  if (*(p-2) == '/') return NULL;
  /*forbidden refering to files under document root*/
  if (depth <= 0) return NULL;

  /*copy path*/
  path = (unsigned char *)malloc(sizeof(unsigned char)*pathLen);
  memset(path, 0, pathLen);
  memcpy(path, path_point, pathLen-1);

  /*HTTP check*/
  while ((c = *(p++)) == ' ') {
    ;
  }


  if (strncmp(p-1, "HTTP/1.1", 8)) return NULL;
  return path;
}

void handle_client(int cli_fd, struct sockaddr_in cli_addr)
{
  FILE *fp;
  int size;
  unsigned char recvBuf[1024];
  unsigned char *path;
  unsigned char *header_ok = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "\r\n";
  unsigned char *header_notfound = 
    "HTTP/1.0 404 Not Found\r\n"
    "Content-Type: text/html\r\n"
    "\r\n";
  unsigned char html_ok[1024];
  unsigned char *html_notfound =
    "<html>"
    "<head>"
    "<title>Not Found</title>"
    "</head>"
    "<body>"
    "<h1>Not Found</h1>"
    "</body>"
    "</html>";

  memset(recvBuf, 0, 1024);
  if (recv(cli_fd, recvBuf, 1024, 0) == -1) {
    exit_failure("recv");
  }
  printf("%s", recvBuf);
  
  /*validate request*/
  if ((path = request_check(recvBuf)) == NULL) { /*Bad Request*/
    if (send(cli_fd, header_notfound, strlen(header_notfound), 0) == -1) {
      exit_failure("send header");
    }
    if (send(cli_fd, html_notfound, strlen(html_notfound), 0) == -1) {
      exit_failure("send html");
    }
  } else { /*Good Request*/
    if ((fp = fopen(path, "r")) == NULL && errno == ENOENT) { /*file not found*/

      if (send(cli_fd, header_notfound, strlen(header_notfound), 0) == -1) {
	exit_failure("send header");
      }
      if (send(cli_fd, html_notfound, strlen(html_notfound), 0) == -1) {
	exit_failure("send html");
      }
    } else if (fp == NULL && errno != ENOENT) { /*could not open file*/
      exit_failure("fopen");
    } else { /*file exists*/
      if (send(cli_fd, header_ok, strlen(header_ok), 0) == -1) {
	exit_failure("send header");
      }
      while ((size = fread(html_ok, sizeof(unsigned char), 1024, fp)) != 0) {
	if (send(cli_fd, html_ok, size, 0) == -1) {
	  exit_failure("send html");
	}
      }
      if (ferror(fp)) {
	exit_failure("fread");
      }
      fclose(fp);
    }

  }  

  close(cli_fd);

  return ;
}

