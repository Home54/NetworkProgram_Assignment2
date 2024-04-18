#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//cpp headers 
#include <stack>

//local headers
#include "protocol.h"

#define PORT 2000
#define MAX_JOB 10
#define MAX_ID 1024

#define MSG_WRG_PROT "THE CLIENT GOT A WRONG FORM OF PROTOCOL"

using namespace std;
typedef struct calcMessage cMessage;
typedef struct calcProtocol cProtocol;

int loopCount=0;
int terminate=0;
int mesLength=sizeof(cMessage);
int proLength=sizeof(cProtocol);
uint32_t currJob;

//TO-DO: reject list
//map jobID to Client 
//


int main(int argc, char *argv[]){
  int socket_desc;
  struct sockaddr_in server_addr,client_addr;
  union dataSent client_message;
  
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
  server_addr.sin_port = htons(2000);
  server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    
    // Bind to the set port and IP:
  if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
      printf("Couldn't bind to the port\n");
      exit(0);
    }
  printf("Done with binding\n");//create one udp in parent process
  

  int jobNum=0;
  stack<int> availableID;//container
  for(int i = MAX_ID - 1 ; i >= 0  ; i-- ){
  	availableID.push(i);
  }
  
  while(1){
  	int pid;//before add the job, in the parent process add the job number
  	int recvLen=0;
  	
	printf("Parent:Before recv\n");
	memset(&client_message, 0 , sizeof(client_message));
  	if ((recvLen=recvfrom(socket_desc, &client_message, sizeof(client_message), 0, (struct sockaddr*)&client_addr, &client_struct_length)) < 0){
     	printf("Couldn't receive. At line %d\n",__FUNC__);
		continue;
    }
    //select on the socket_desc and the pipe for read
    
	//to decide if the message is hello using the message length.
	//if so, register a job number for the client
	if(recvLen==mesLength){	
	   if( ntohs(client_message.CM.type) == TYPE_CTS_BIN && ntohs(client_message.CM.message) == MES_NOT_AVAI && ntohs(client_message.CM.protocol)== UDP && ntohs(client_message.CM.major_version)== 1 && ntohs(client_message.CM.minor_version)==0 ){
	   		selectfd=readfd;
	   		timeToWait.tv_sec=0;
    		timeToWait.tv_usec=0;
	   		int res=select(MAX_JOB+1,&selectfd,NULL,NULL,&timeToWait);
	    	if(res<0) {
	    		perror("select");
	    	} 
	    
	    	for(int i = 0 ; i < MAX_JOB ; i++ ){//handle the expire the child
	    		if(FD_ISSET(pipefd[i][0],&selectfd)){
	    			int newID;
	    			if(read(pipefd[i][0],newID,sizeof(newID)) < 0 ){
	    				perror("read");
	    			}
	    			availableID.push(newID);
	    			jobNum--;
	    		}
	    	}
	    	//before alloc the new client free the old ones
	   		if(availableID.empty()){
	   			printf("The server is full.\n");
	   			continue;
	   		}
	   		//the new handshake 
	   		printf("New terminal add on IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	   		jobNum++;
    		currJob=availableID.top();//share the memoary by the parent and the child.
    		availableID.pop();
    		
  			if(pid<0){
  				perror("fork()");
  				exit(0);
  			} 
  			else if(pid==0){//in the child space 
  				//replace the child with another image 
  				char argv[3][20];
  				sprintf(argv[0], "%u", currJob);//jobID
  				sprintf(argv[1], "%d", pipefd[currjob][1]);//pipe_In(for P write)
  				sprintf(argv[2], "%d", pipefd[currjob][0]);// pipe_Out(for P read)
  				
  				exec("./doStaff.c",argv[0],argv[1],argv[2],NULL);
  			}
	   }else{
	   		printf("The server does not support the form of calcProtocol.\n");
	   }
	}else if( recvLen == proLength ){//for the further communication
	   	//just copy the whole to child space
	   	if()
	   	write(piprfd[client_message.CP.id][1],)
	   	
	}else{
	   printf("protocal length error.\n");
	}
  }
  
  wait(NULL);
  exit(0);
}
