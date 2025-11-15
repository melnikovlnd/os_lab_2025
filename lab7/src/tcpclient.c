#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {

  char *ip = argv[1];
  int port = atoi(argv[2]);
  int bufsize = atoi(argv[3]);

  // Проверка валидности размера буфера
  if (bufsize <= 0) {
    printf("Error: BUFSIZE must be positive number\n");
    exit(1);
  }

  int fd;
  int nread;
  char *buf = malloc(bufsize);
  
  struct sockaddr_in servaddr;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    exit(1);
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("bad address");
    exit(1);
  }

  servaddr.sin_port = htons(port);

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    exit(1);
  }

  printf("Connected to server %s:%d\n", ip, port);
  printf("Input message to send (Ctrl+D to exit):\n");
  
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      exit(1);
    }
  }

  if (nread < 0) {
    perror("read");
  }

  free(buf);
  close(fd);
  printf("Connection closed\n");
  return 0;
}