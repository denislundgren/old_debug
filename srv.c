#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "spread_ipc.h"

void func_manage_sig(int sig);

// structure for message queue
struct mesg_buffer {
long mesg_type;
char mesg_text[MAX_MESSAGE];
} message;

int vet_proc[MAX_PROC];

extern int errno;

  int sockfd, connfd;

// Driver function
int sock_manage()
{
        int len;
  struct sockaddr_in servaddr, cli;

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("socket creation failed...\n");
    exit(0);
  }
  else
    printf("Socket successfully created..\n");
  bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  // Binding newly created socket to given IP and verification
  if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
    printf("socket bind failed, errno %d...\n",errno);
    exit(0);
  }
  else
    printf("Socket successfully binded..\n");

  // Now server is ready to listen and verification
  if ((listen(sockfd, 5)) != 0) {
    printf("Listen failed...\n");
    exit(0);
  }
  else
    printf("Server listening..\n");
  len = sizeof(cli);

  // Accept the data packet from client and verification
  connfd = accept(sockfd, (SA*)&cli, &len);
  if (connfd < 0) {
    printf("server accept failed...\n");
    exit(0);
  }
  else
    printf("server accept the client...\n");


// LOOP TCP movido para main()
/*
  // Function for chatting between client and server
  func(connfd);

  // After chatting close the socket
  close(sockfd);

*/
}

void func_manage_sig(int sig)
{
  if (sig == SIGCHLD)
    printf("signal %d captured\n", sig);
  else if (sig == SIGTERM)
  {
    printf("signal %d captured\n", sig);
    exit(0);
  }
}

int main()
{
int ret_code;
int ret;
int nproc=0;
int own_pid;
int parent_pid;

key_t key;
int msgid;
  
FILE *fp;
char buf_text[MAX_BUFFER];
int len;
int total_len;
int i;

char buf_tcp[MAX_MESSAGE];
//char buf_debug[500];


// inibe sinais de saida dos processos filhos.
signal(SIGCHLD, func_manage_sig);
signal(SIGTERM, func_manage_sig);

//  void (*signal(int signo, void (*func )(int)))(int);

own_pid = getpid();

// ftok to generate unique key
key = ftok("spread", 200);

// msgget creates a message queue
// and returns identifier
msgid = msgget(key, 0666 | IPC_CREAT);


while (nproc < MAX_PROC) {
  ret_code = fork();

printf("NPROC %d, ret_code %d\n", nproc, ret_code);

  if (ret_code == 0)
    break;
  else
    vet_proc[nproc++] = ret_code;
}

if (ret_code != 0) {

  // SERVER SIDE

  sock_manage();

  for (;;) {
    bzero(buf_tcp, MAX_MESSAGE);

    // read the message from client and copy it in buf_tcp
    read(connfd, buf_tcp, sizeof(buf_tcp));
 
    // print buf_tcp which contains the client contents
    printf("DEBUG SRV TCP: From client: [%s]\n", buf_tcp);

    // if msg contains "Exit" then server exit and chat ended.
    if (strncmp("exit", buf_tcp, 4) == 0) {
      printf("Server Exit...\n");
  
      // if cli sends exit string, srv will kill chield process.
      for (i = 0; i < MAX_PROC; i++) {
        kill(vet_proc[i], SIGTERM);
      }

      // After chatting close the socket
      close(sockfd);

      exit(0);
    }

    total_len = 0;
    for (i = 0; i < MAX_PROC; i++) {
      printf("SRV side: runnig. own_pid %d\n", own_pid);
      
      message.mesg_type = vet_proc[i];

      strcpy(message.mesg_text, "req");
      
      // msgsnd to send message
      msgsnd(msgid, &message, sizeof(message), 0);
      
      sleep(1);

      bzero(&message, sizeof(message));

      // msgrcv to receive message
      msgrcv(msgid, &message, sizeof(message), own_pid, 0);
      
      // display the message
      printf("PARENT received, CONCAT Data received is : [[%s]] \n", message.mesg_text);

      len = strlen(message.mesg_text);
      strcpy(&(buf_tcp[total_len]), message.mesg_text);
      total_len += len;


//  END LOOP MESSAGE QUEUE
    }

    // and send that buffer to client
    if ((ret = write(connfd, buf_tcp, total_len + 1)) < 0) 
      printf("WRITE RET %d errno %d\n", ret, errno); 

    printf("DEBUG CONCAT [[%s]]\n", buf_tcp);

// END LOOP DAEMON TCP
 }
 
  // After chatting close the socket
  close(sockfd);
}
else {
  // CHIELD ROLE
  
  own_pid = getpid();
  parent_pid = getppid();
  
  printf("CHIELD side: runnig. nproc %d own_pid %d parent pid %d\n", nproc, own_pid, parent_pid);
  
  bzero(&message, sizeof(message));
  
  // msgrcv to receive message
  msgrcv(msgid, &message, sizeof(message), own_pid, 0);
  
  printf("CHIELD side, after msgrcv(); \n");
  
  switch(nproc) {
    case 0:
      if ((ret = system("top -b -n 1 | head -17 | tail -11 > /tmp/spread_top")) < 0)
      {
      printf ("CHIELD TOP error, erno %d\n", errno);
      exit (1);
      }
  
  
      errno = 0;
      if ((fp = fopen("/tmp/spread_top", "r")) == NULL)
      {
        printf("01 erro no arq spread_top errno %d\n", errno);
        exit(2);
      }
  
      total_len = 0;
      while (fgets(buf_text, MAX_BUFFER - 1, fp) != NULL)
      {
        len = strlen(buf_text);
  
  //printf("DEBUG TOP [%s]\n", buf_text);
  
        strcpy(&(message.mesg_text[total_len]), buf_text);
        total_len += len;
      }
  
      message.mesg_text[total_len] = 0;
  
      fclose(fp);
  
    break;
    case 1:
      if ((ret = system("free > /tmp/spread_free")) < 0)
      {
      printf ("CHIELD FREE error, erno %d\n", errno);
      exit (1);
      }
  
      errno = 0;
      if ((fp = fopen("/tmp/spread_free", "r")) == NULL)
      {
        printf("01 erro no arq spread_top errno %d\n", errno);
        exit(2);
      }
  
      total_len = 0;
      while (fgets(buf_text, MAX_BUFFER - 1, fp) != NULL)
      {
        len = strlen(buf_text);
  
  //printf("DEBUG FREE [%s]\n", buf_text);
  
        strcpy(&(message.mesg_text[total_len]), buf_text);
        total_len += len;
      }
  
      message.mesg_text[total_len] = 0;
  
      fclose(fp);
  
    break;
    // STUB DAEMON FOR FUTURE INPLMENTATION
    case 2:
      strcpy(message.mesg_text, "stub response, to be developed");
    break;
  }
  
  message.mesg_type = parent_pid;
  
  // msgsnd to send message
  msgsnd(msgid, &message, total_len + 10 + sizeof(long), 0);
  
  // display the message
  printf("CHIELD SIDE DEBUG %d Data send is : [%s] \n", nproc, message.mesg_text); 
  
}

exit(0);

}

