#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "time.h"
#include <math.h>
//cpp headers 
#include <stack>
#include <map>

//local headers
#include "protocol.h"

#define PORT 2000
#define MAX_JOB 10
#define EXPIRE_TIME 100
#define PRICISION 0.001

#define MSG_WRG_PROT "THE CLIENT GOT A WRONG FORM OF PROTOCOL"

using namespace std;
typedef struct calcMessage cMessage;
typedef struct calcProtocol cProtocol;
typedef struct client{
	in_addr_t ip_addr;
	short port;
	
	bool operator<(const struct client & other) const {
        if (ip_addr == other.ip_addr) {
            return port < other.port;
        }
        return ip_addr < other.ip_addr;
    }
}client;//used in map

int mesLength=sizeof(cMessage);
int proLength=sizeof(cProtocol);
uint32_t currJob;
map<client,int> joblist;
map<int,client> joblistR;
int pipefd[MAX_JOB][2];//for inter-process communication

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
  }
     *ans = strdup(resultStr);
}

void alarm_handler(int sig){//called if time out
	if (write(pipefd[currJob][1], &currJob, sizeof(currJob)) == -1) {
            perror("write");
            exit(0);
    }
	printf("Child:Client time out. At ID:%d\n",currJob);
	exit(0);
}

void child_handler(int sig){
	int state;	
	pid_t pid;
	
	while(pid=waitpid(-1,&state,WNOHANG)>=0){
		printf("pid on:%d expired.\n",pid);
	}
	return;
}

int main(int argc, char *argv[]){
  int socket_desc;
  struct sockaddr_in server_addr,client_addr;
  //union dataSent client_message;
  char client_message[120];
  
  fd_set readfds, selectfds;
  FD_ZERO( & readfds);
  socklen_t client_struct_length;//for recv the client 
  struct timeval timeToWait;
    // Create UDP socket:
  socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
  if(socket_desc < 0){
      printf("Error while creating socket\n");
     exit(0);
   }
   printf("Socket created successfully\n");
    
    // Set port and IP:
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
  if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
      printf("Couldn't bind to the port\n");
      exit(0);
    }
  printf("Done with binding\n");//create one udp in parent process
  
  int jobNum=0;
  stack<int> availableID;//container
  
  for( int i = 0 ; i < MAX_JOB ; i++ ){//0 for read 1 for write
   	if (pipe(pipefd[i]) == -1) {
        perror("pipe");
        exit(0);
    }
    FD_SET(pipefd[i][0], &readfds);
    availableID.push(MAX_JOB-i);
  }//for each child process create a pipe to communicate
 
  while(1){
  	int pid;//before add the job, in the parent process add the job number
  	int recvLen=0;
  	
	printf("Parent:Before recv\n");
	memset(&client_message, 0 , sizeof(client_message));
  	if ((recvLen=recvfrom(socket_desc, &client_message, sizeof(client_message),0, (struct sockaddr*)&client_addr, &client_struct_length)) < 0){
     	printf("Couldn't receive. At line %d\n",__LINE__);
     	printf("errno=%s\n",strerror(errno));
		continue;
    }
    //select on the socket_desc and the pipe for read
    
	//to decide if the message is hello using the message length.
	//if so, register a job number for the client
	if(recvLen==mesLength){
	   if( ntohs(client_message.CM.type) == TYPE_CTS_BIN && ntohs(client_message.CM.message) == MES_NOT_AVAI && ntohs(client_message.CM.protocol)== UDP && ntohs(client_message.CM.major_version)== 1 && ntohs(client_message.CM.minor_version)==0 ){
	   		selectfds=readfds;
	   		timeToWait.tv_sec=0;
    		timeToWait.tv_usec=0;
	   		int res=select(MAX_JOB+1,&selectfds,NULL,NULL,&timeToWait);
	    	if(res<0) {
	    		perror("select");
	    	} 
	    
	    	for(int i = 0 ; i < MAX_JOB ; i++ ){//handle the expire the child
	    		if(FD_ISSET(pipefd[i][0],&selectfds)){
	    			int newID;
	    			if(read(pipefd[i][0],&newID,sizeof(newID)) < 0 ){
	    				perror("read");
	    			}
	    			availableID.push(newID);
	    			jobNum--;
	    			joblist.erase(joblistR[newID]);
	    			joblistR.erase(newID);
	    		}
	    	}//before alloc the new client free the old ones
	    	
	    	if(availableID.empty()){
	   			printf("The server is full.\n");
	   			continue;
	   		}
	   		if(joblist.find({client_addr.sin_addr.s_addr,client_addr.sin_port}) != joblist.end()){
	   			printf("Muti handshake.\n");
	   			continue;
	   		}
  		
	   		//the new handshake 
	   		
	   		jobNum++;
    		currJob=availableID.top();//share the memoary by the parent and the child.
    		availableID.pop();
    		joblist.insert({{client_addr.sin_addr.s_addr,client_addr.sin_port},currJob});
    		joblistR.insert({currJob,{client_addr.sin_addr.s_addr,client_addr.sin_port}});
    		printf("New terminal add on IP: %s port: %i jobID:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),currJob);
    		
    		pid=fork();
  			if(pid<0){
  				perror("fork()");
  				exit(0);
  			} 
  			else if(pid==0){//in the child space 
  				//replace the child with another image 
  				printf("In the client.\n");
 				char ques[MAX_QUES_LEN];
  				memset(ques,0,sizeof(ques));
  				char* ans=NULL;
  				union dataSent fromParent;
   
  				signal(SIGALRM,alarm_handler);
  				signal(SIGCHLD,child_handler);//deal with the died children
  				while(1){
  	 //if the answer list is none:generate the questions
     				if(ans==NULL){
     	 
     				}else{//expect for an answer for client
     					memset(&fromParent,0,sizeof(fromParent));
     			//char msg[MAX_QUES_LEN];memset(msg,0,sizeof(msg));
     					alarm(EXPIRE_TIME);//settitimer?
     					ssize_t num_bytes = read( pipefd[currJob][0], fromParent, sizeof(fromParent) );//character like msg
     					//read from the parent process to avoid competition
     					alarm(0);//free the alarm
     					if (num_bytes == -1) {
           					perror("read");
            				exit(0);
        				}
        				msg[num_bytes-1]='\0';printf("recieve %s from client.\n",msg);
        				
     //handle the answer : compare to the answer
        				if(fabs( - atof(ans)) >= PRICISION){
            				sendto(sockfd,"WRONG ANSWER.\n",strlen("WRONG ANSWER. "),0,(struct sockaddr*)&client_addr, &client_struct_length));
        				}else{//do good
            				sendto(sockfd,"CORRECT ANSWER.\n",strlen("CORRECT ANSWER. "),0,(struct sockaddr*)&client_addr, &client_struct_length));
        				}
        					free(ans);
     				}
     //generate the question
     //response to the client
     				memset(ques,0,sizeof(ques));
     				genQues(&ques[0],&ans);
     				printf("send question:%s and answer:%s\n",ques,ans);
     				
     				ques[strlen(ques)]='\n';
	 				sendto(sockfd,&ques[0],strlen(ques),0,(struct sockaddr*)&client_addr, &client_struct_length));
  				}
  			}
	   }else{
	   		printf("The server does not support the form of calcProtocol.\n");
	   }
	}else if( recvLen == proLength ){//for the further communication
		//should not happened
		int id=ntohs(client_message.CP.id);
		if(joblistR.find(id) == joblistR.end() ){
			printf("should sign up first.\n");
			//reject the client
		}else{
			//suceess and write the data to the child process
			ssize_t num_bytes = write( pipefd[id][1], &client_message, sizeof(client_message) );
			if( num_bytes == -1 ){
				perror("write");
				printf("errorno=%s.\n",strerror(errno));
			}
		}
	}else{
	   printf("protocal length error.\n");
	}
  }
  exit(0);
}
