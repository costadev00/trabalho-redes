/*
** server.c -- a stream socket server demo
** Server side program that is able to wait for a connection and answer with a
** Hello World string to a client program
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MYPORT 3490     // the port users will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once

#define BACKLOG 10 // how many pending connections queue will hold

void sigchld_handler(int s) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

int login(char *user, char *pass) {
  FILE *f = fopen("login.fms", "r");
  int r = 0;
  char sUser[50], sPass[50];
  if (f != NULL) {
    while (fscanf(f, " %s %s", sUser, sPass) != EOF) {
      if (!strcmp(user, sUser) && !strcmp(pass, sPass)) {
        r = 1;
        break;
      }
    }
    fclose(f);
    if (r) {
      int c = 0;
      f = fopen("logados.fms", "r+");
      if (f == NULL)
        return 0;
      while (fscanf(f, " %s", sUser) != EOF) {
        if (strcmp(sUser, user) == 0) {
          printf("%s\n", sUser);
          fprintf(f, " ON\n");
          c = 1;
          break;
        }
        fscanf(f, " %s", sUser);
      }
      if (!c) {
        fprintf(f, "%s ON\n", user);
      }
      fclose(f);
    }
  }
  if (r) {
    f = fopen("log.fms", "a");
    if (f != NULL) {
      fprintf(f, "Login: %s\n", user);
      fclose(f);
    }
  }
  return r;
}

int registerUser(char *user, char *pass) {
  FILE *f = fopen("login.fms", "r");
  int r = 1;
  char sUser[50], sPass[50];
  if (f != NULL) {
    while (fscanf(f, " %s %s", sUser, sPass) != EOF) {
      if (!strcmp(user, sUser)) {
        r = 0;
        break;
      }
    }
    fclose(f);
  }
  if (r) {
    f = fopen("login.fms", "a");
    if (f != NULL) {
      fprintf(f, "%s %s\n", user, pass);
      fclose(f);
    }
    f = fopen("log.fms", "a");
    if (f != NULL) {
      fprintf(f, "%s registred\n", user);
      fclose(f);
    }
    f = fopen(user, "w+");
    if (f != NULL) {
      fclose(f);
    }
  }
  return r;
}

int is_loged(char *user) {
  int r = 0;
  char sUser[50];
  FILE *f = fopen("logados.fms", "r");
  if (f == NULL)
    return 0;
  while (fscanf(f, " %s", sUser) != EOF) {
    if (strcmp(sUser, user) == 0) {
      fscanf(f, " %s", sUser);
      if (strcmp(sUser, "ON") == 0) {
        r = 1;
      }
      break;
    }
  }
  fclose(f);
  return r;
}

void sendMsg(char *dest, char *msg, char *rem) {
  FILE *f = fopen(dest, "a");
  if (f == NULL) {
    return;
  }
  fprintf(f, "%s %s\n", rem, msg);
  fclose(f);
}

void logout(char *user) {
  char sUser[50];
  FILE *f = fopen("logados.fms", "r+");
  if (f == NULL)
    return;
  while (fscanf(f, " %s", sUser) != EOF) {
    if (strcmp(sUser, user) == 0) {
      fprintf(f, " OF\n");
      fscanf(f, " %s", sUser);
      break;
    }
  }
  fclose(f);
  f = fopen("log.fms", "a");
  if (f != NULL) {
    fprintf(f, "Logoff: %s\n", user);
    fclose(f);
  }
}

void listUser(int fd) {
  char msg[10002] = "\n";
  char sUser[50], st[3];
  FILE *f = fopen("logados.fms", "r+");
  if (f == NULL)
    return;
  while (fscanf(f, " %s %s", sUser, st) != EOF) {
    printf("%s %s\n", sUser, st);
    if (strcmp(st, "ON") == 0) {
      strcat(msg, sUser);
      strcat(msg, "\n");
    }
  }
  fclose(f);
  if (write(fd, msg, strlen(msg)) == -1) {
    perror("Sending");
    close(fd);
    // sleep(3);
    exit(-1);
  }
}

void receber(char *user, int fd) {
  FILE *f = fopen(user, "r");
  char msgs[16000] = "\n";
  if (f != NULL) {
    char msg[210];
    while (fgets(msg, 210, f)) {
      strcat(msgs, msg);
      // puts(msg);
    }
    fclose(f);
    f = fopen(user, "w+");
    fclose(f);
  }
  if (write(fd, msgs, strlen(msgs)) == -1) {
    perror("Sending");
    close(fd);
    // sleep(3);
    exit(-1);
  }
}

/*void delete(int fd,user **logons){
  FILE *f = fopen("login.fms", "r");
  if(f==NULL)return;
  char user[50],pss[50];
  int pos = is_loged(fd, logons);
  while(fscanf(f, " %s %s",user,pss)){
    if(strcmp(user,logons[pos]->nome)){
      int len = strlen(user) + strlen(pss)+1;

    }
  }
  logout(fd, logons);
}*/

int main(int argc, char **argv) {
  int sockfd, new_fd;            // listen on sock_fd, new connection on new_fd
  struct sockaddr_in my_addr;    // my address information
  struct sockaddr_in their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  int myPort;
  int retVal;
  char buf[MAXDATASIZE];
  char usr[50] = "";

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <Port Number>\n", argv[0]);
    exit(-1);
  }
  myPort = atoi(argv[1]);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(-2);
  }
  /*
          if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
     -1) { perror("setsockopt"); exit(-3);
          }
  */

  my_addr.sin_family = AF_INET;         // host byte order
  my_addr.sin_port = htons(myPort);     // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) ==
      -1) {
    perror("bind");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  while (1) { // main accept() loop

    printf("Accepting ....\n");
    retVal = 0;
    sin_size = sizeof(struct sockaddr_in);
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) ==
        -1) {
      perror("accept");
      continue;
    }
    printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
    if (!fork()) { // this is the child process
      signal(SIGCHLD, SIG_IGN);
      sleep(5);
      while (1) {
        char resp[100];
        memset(buf, 0, MAXDATASIZE);
        memset(resp, 0, MAXDATASIZE);
        if ((retVal = read(new_fd, buf, MAXDATASIZE)) == -1) {
          perror("Receiving");
          close(new_fd);
          exit(-1);
        } else {
          char op[10];
          sscanf(buf, " %s", op);
          if (!strcmp(op, "quit")) {
            if (!is_loged(usr))
              strcpy(resp, "quit");
            else
              strcpy(resp, "Voce esta logado");
          } else if (!strcmp(op, "login")) {
            char user[50], pass[50];
            sscanf(buf + 5, " %s %s", user, pass);
            if (strlen(usr)) {
              strcpy(resp, "Você já está logado");
            } else if (login(user, pass)) {
              strcpy(usr, user);
              receber(usr, new_fd);
              strcpy(resp, "Logado");
            } else {
              strcpy(resp, "Falha no Login");
            }
          } else if (!strcmp(op, "register")) {
            char user[50], pass[50];
            sscanf(buf + 8, " %s %s", user, pass);
            if (registerUser(user, pass)) {
              strcpy(resp, "Usuario Registrado");
            } else {
              strcpy(resp, "Nome de usuario já existe");
            }
          } else if (!strcmp(op, "send")) {
            char dest[50];
            char msg[100];
            sscanf(buf + 4, " %s %[^\n]s", dest, msg);
            if (strlen(usr)) {
              sendMsg(dest, msg, usr);
              strcpy(resp, "Mensagem encaminhada");
            } else {
              strcpy(resp, "Faça login primeiro");
            }
          } else if (!strcmp(op, "isOnline")) {
            char user[50];
            sscanf(buf + 8, " %s", user);
            if (is_loged(user)) {
              strcpy(resp, "Online");
            } else {
              strcpy(resp, "Offline");
            }
          } else if (!strcmp(op, "listUsers")) {
            listUser(new_fd);
            strcpy(resp, "\0");
          } else if (!strcmp(op, "logout")) {
            logout(usr);
            strcpy(usr, "");
            strcpy(resp, "Logoff realizado");
          } else if (!strcmp(op, "recive")) {
            if (strlen(usr)) {
              receber(usr, new_fd);
              strcpy(resp, "\0");
            }
            strcpy(resp, "Faça login primeiro");
          }
          /*else if (!strcmp(op, "apagar")){
            delete(new_fd,logons);
            strcpy(resp, "Usuario Apagado");
          }*/
          else {
            strcpy(resp, " ");
          }
        }
        if ((retVal = write(new_fd, resp, strlen(resp))) == -1) {
          perror("Sending");
          close(new_fd);
          exit(-1);
        }
        if (!strcmp(resp, "quit")) {
          printf("Connection closed by foreign host\n");
          sleep(1);
          close(new_fd);
          exit(-1);
        }
      }
    }
    close(new_fd); // parent doesn't need this
  }

  return 0;
}
