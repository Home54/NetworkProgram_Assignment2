#ifndef FUNCTIONS_H
#define FUNCTIONS_H

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

int jobID;//need for inter-process communication

void alarm_handler(int sig){//called if time out
	printf("Child:Client time out. At ID:%d\n",jobID);
	exit(0);
}


void genQues(char * ques , char ** ans){
	double fresult;
        int iresult;
        const char *arith[] = {"add", "div", "mul", "sub", "fadd", "fdiv", "fmul", "fsub"};
        srand(time(NULL));
                    /* Figure out HOW many entries there are in the list.
                       First we get the total size that the array of pointers use, sizeof(arith). Then we divide with
                       the size of a pointer (sizeof(char*)), this gives us the number of pointers in the list.
                    */
        int Listitems = sizeof(arith) / (sizeof(char *));
        int itemPos = rand() % Listitems;
        char itemPosStr[10];
        char resultStr[MAX_QUES_LEN]; // asume that the length of answer is less than 20 char  
        sprintf(itemPosStr, "%d", itemPos);

                    /* As we know the number of items, we can just draw a random number and modulo it with the number
                       of items in the list, then we will get a random number between 0 and the number of items in the list

                       Using that information, we just return the string found at that position arith[itemPos];
                    */

        const char *ptr;
        ptr = arith[itemPos]; // Get a random arithemtic operator.

        int i1, i2;
        double f1, f2;
        i1 = rand() % 100;
        i2 = rand() % 100;
        f1 = (double) rand() / (double) (RAND_MAX / 100.0);
        f2 = (double) rand() / (double) (RAND_MAX / 100.0);
        if (ptr != NULL && strlen(ptr) > 0) {
        if (ptr[0] == 'f') {
            sprintf( ques, "%s %.5f %.5f", ptr, f1, f2);
                            if (strcmp(ptr, "fadd") == 0) {
                                fresult = f1 + f2;
                            } else if (strcmp(ptr, "fsub") == 0) {
                                fresult = f1 - f2;
                            } else if (strcmp(ptr, "fmul") == 0) {
                                fresult = f1 * f2;
                            } else if (strcmp(ptr, "fdiv") == 0) {
                                fresult = f1 / f2;
                            }
// 将浮点数 fresult 转换为字符串
                            sprintf(resultStr, "%f", fresult);

// 发送结果字符串给客户端
                        } else {
                            sprintf( ques, "%s %d %d", ptr, i1, i2);
                            if (strcmp(ptr, "add") == 0) {
                                iresult = i1 + i2;
                            } else if (strcmp(ptr, "sub") == 0) {
                                iresult = i1 - i2;
                            } else if (strcmp(ptr, "mul") == 0) {
                                iresult = i1 * i2;
                            } else if (strcmp(ptr, "div") == 0) {
                                iresult = i1 / i2;
                            } else {
                                printf("No match\n");
                            }
                            sprintf(resultStr, "%d", iresult);

                        }
                        //end the job
                        //set the timer for the each
                    }
                    *ans = strdup(resultStr);
}

int main(int argc, char *argv[]){//userID pipe_In(for P write) pipe_Out(for P read) should be passed by argv
  struct sockaddr_in server_addr,client_addr;
  int sockfd;
  jobID=atoi(argv[1]);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//open another port

  client_addr.sin_family=AF_INET;
  client_addr.sin_port = htons(atoi(argv[3]));
  client_addr.sin_addr.s_addr = inet_addr(argv[2]);//serve one specific client
  
  sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
     perror("bind");
     exit(0);
  }
  
  int rv=connect(sockfd,(struct sockaddr_in *)&client_addr,sizeof(client_addr));//point to the target client
  if(rv<0) {
  	perror("connect");
  	exit(0);
  }//in the parent or outer handler may handle the problem 
  
  char ques[MAX_QUES_LEN];
  memset(ques,0,sizeof(ques));
  char* ans=NULL;
  union dataSent fromParent;
  //sentToParent.jobID=jobID;//fixed the arg
   
  signal(SIGALRM,alarm_handler); 
  getchar();
  while(1){
  	 //if the answer list is none:generate the questions
     if(ans==NULL){
     	 
     }else{//expect for an answer for client
     	//memset(&fromParent,0,sizeof(fromParent));
     	char msg[MAX_QUES_LEN];memset(msg,0,sizeof(msg));
     	alarm(114514);
     	ssize_t num_bytes = recv(sockfd,msg,sizeof(msg),0);//character like msg
     	printf("recieve %s from client.\n",msg);
     	alarm(0);//free the alarm
     	if (num_bytes == -1) {
            perror("recv");
            exit(0);
        }
        //handle the answer : compare to the answer
        if(fabs(atof(msg) - atof(ans)) >= PRICISION){
            	send(sockfd,"WRONG ANSWER.",strlen("WRONG ANSWER."),0);
        }else{//do good
            	send(sockfd,"CORRECT ANSWER.",strlen("CORRECT ANSWER."),0);
        }
        free(ans);
     }
     //generate the question
     //response to the client
     memset(ques,0,sizeof(ques));
     genQues(&ques[0],&ans);
     printf("send question:%s and answer:%s\n",ques,ans);
     
	 send(sockfd,&ques[0],strlen(ques),0);
  }
}
#endif

#ifdef AAA
  				
  				question.CP.type=htons((uint16_t)TYPE_STC_TEXT);
  				question.CP.major_version=htons((uint16_t)1);
  				question.CP.minor_version=htons((uint16_t)0);
  				question.CP.id=htonl(currJob);
  				question.CP.arith=htonl((uint16_t)ARITH_MUL);
  				question.CP.inValue1=htonl(72);
  				question.CP.inValue2=htonl(86);
  				question.CP.inResult=htonl(0);//should not exceed INT_MAX
  				question.CP.inValue1=0.0;
  				question.CP.inValue2=0.0;
  				question.CP.inResult=0.0;
#endif
