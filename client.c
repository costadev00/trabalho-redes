/*
** client.c -- a stream socket client demo
** Client side program that is able to establish a connection with a server
** receive a message and print the received string.
*/

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 3490 // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

enum t_state { IDLE, CONNECTED, HHHH } state;

int sockfd, numbytes;

void sigControlC() {
  printf("Control C pressed\n");
  close(sockfd);
  exit(0);
}

void sigHandler() { fprintf(stderr, "*** Pegou um sinal ***\n"); }

int main(int argc, char *argv[]) {
  char iBuf[MAXDATASIZE + 1];
  char oBuf[MAXDATASIZE + 1];
  struct hostent *he;
  struct sockaddr_in their_addr; // connector's address information
  int myPort;

  signal(1, sigHandler);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, sigControlC);

  state = IDLE;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <Hostname> <Port Number>\n", argv[0]);
    exit(1);
  }

  if ((he = gethostbyname(argv[1])) == NULL) { // get the host info
    perror("gethostbyname");
    exit(1);
  }

  myPort = atoi(argv[2]);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  their_addr.sin_family = AF_INET;     // host byte order
  their_addr.sin_port = htons(myPort); // short, network byte order
  their_addr.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(their_addr.sin_zero), '\0', 8); // zero the rest of the struct

  printf("Going to be connected...\n");

  if (connect(sockfd, (struct sockaddr *)&their_addr,
              sizeof(struct sockaddr)) == -1) {
    perror("connect");
    fprintf(stderr, "connect: ERRNO %d\n", errno);
    exit(1);
  }

  char usr[50] = "";

  printf("Connected ....\n");
  while (1) {

    memset(oBuf, 0, MAXDATASIZE);
    printf("> ");
    fgets(oBuf, MAXDATASIZE - 1, stdin);
    oBuf[strlen(oBuf) - 1] = 0;

    write(sockfd, oBuf, strlen(oBuf));

    printf("Waiting....\n");

    char op[50];
    sscanf(oBuf, " %s", op);

    memset(iBuf, 0, MAXDATASIZE);

    //sleep(1);
    // if (strcmp(op, "login")==0)
    //   sleep(2);

    if ((numbytes = read(sockfd, iBuf, MAXDATASIZE)) > 0) {
      printf("Received [%s] with %d bytes\n", iBuf, numbytes);

      if (strcmp(op, "login") == 0) {
        if (strlen(usr) == 0) {
          sscanf(oBuf + 6, " %s", usr);
        }
        if (strcmp(iBuf, "Voce esta logado") != 0 &&
            strcmp(iBuf, "Você já está logado") != 0 &&
            strcmp(iBuf, "Falha no Login") != 0) {
          char user[50], msg[160];
          int p=0;
          while (sscanf(iBuf+p, "%s %[^\n]s", user, msg) > 1) {
            FILE *f = fopen(user, "a");
            if (f != NULL) {
              fprintf(f, "%s: %s\n", user, msg);
              p+= strlen(user)+strlen(msg)+2;
            }
          }
        }
      } else if (strcmp(op, "send") == 0) {
        if (strlen(usr)) {
          char user[50], msg[160];
          sscanf(oBuf + 5, " %s %[^\n]s", user, msg);
          FILE *f = fopen(user, "a");
          if (f != NULL) {
            fprintf(f, "%s: %s\n", usr, msg);
          }
        }
      } else if (strcmp(op, "recive") == 0) {
        char user[50], msg[160];
        while (sscanf(iBuf, "%s %[^\n]s", user, msg) > 1) {
          FILE *f = fopen(user, "a");
          if (f != NULL) {
            fprintf(f, "%s: %s\n", user, msg);
          }
        }
      } else if (strcmp(op, "logout") == 0) {
        strcpy(usr, "");
      }
      op[0]=0;
    }
    if (!strcmp(iBuf, "quit")) {
      printf("Connection closed\n");
      close(sockfd);
      exit(0);
    }
  }
}
