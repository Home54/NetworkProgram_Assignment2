#ifndef FUNCTIONS_C
#define FUNCTIONS_C

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "time.h"
#include <math.h>
#include <signal.h>
//local headers
#include "protocol.h"

#define DEBUG
#define PORT 2001
#define PRICISION 0.001
//for the single process
//use pipe to communicate with the server -> parent (read:expired ID , write: information from client)
//pipes are all created in the parent process 

typedef struct communication State;


int main(int argc, char *argv[]){//userID pipe_In(for P write) pipe_Out(for P read) should be passed by argv
  int jobID;//need for inter-process communication
  struct sockaddr_in server_addr,client_addr;
  memset(&server_addr,0,sizeof(server_addr)); memset(&client_addr,0,sizeof(client_addr));
  int sockfd;
  
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(8888);
  client_addr.sin_addr.s_addr = inet_addr("0.0.0.0");//open another port
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(2001);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//open another port
  
  sockfd=socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
  if(bind(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0){
     perror("bind");
     exit(0);
  }
  
  getchar();
  int rv=connect(sockfd,(struct sockaddr_in *)&server_addr,sizeof(server_addr));//point to the target client
  if(rv<0) {
  	perror("connect");
  	exit(0);
  }//in the parent or outer handler may handle the problem 
  
  
  char msg[1200];
  char msg1[1200];
  
  while(1){
	 memset(msg1,0,sizeof(msg1));
	 int rv=recv(sockfd,&msg1[0],1200,0);
	 if (rv == -1) {
            perror("recv");
            exit(0);
     }
	 msg1[rv]=0;
	 printf("recv from server:%s\n",msg1);
	 
	 rv=recv(sockfd,&msg1[0],1200,0);
	 if (rv == -1) {
            perror("recv");
            exit(0);
     }
	 msg1[rv]=0;
	 printf("recv from server:%s\n",msg1);
	 
     memset(msg,0,sizeof(msg));
     scanf("%s",msg);
	 send(sockfd,&msg[0],1200,0);
	 
  }
}

#endif
