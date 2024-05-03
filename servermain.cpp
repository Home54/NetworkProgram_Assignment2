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
#include <sys/wait.h>

//cpp headers 
#include <stack>
#include <map>

//local headers
#include "protocol.h"

#define PORT 2000
#define MAX_JOB 10
#define EXPIRE_TIME 10
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
socklen_t client_struct_length;//for recv the client 
uint32_t currJob;
map<client,int> joblist;
map<int,client> joblistR;
int pipefd[MAX_JOB][2];//for inter-process communication
int socket_desc;
struct sockaddr_in server_addr,client_addr;


// 声明处理消息的函数
void processMessage(union dataSent* cp, union Result**  result) {
	Result* temp =(Result*)malloc(sizeof(union Result));
    uint32_t random_arith = rand() % 8 + 1;  // 生成 1 到 8 之间的随机数
    cp->CP.arith = htonl(random_arith);  // 将随机数转换为网络字节顺序后赋给 cp->arith
	printf("Set the arith:%d ", random_arith);
    if (random_arith >= 1 && random_arith <= 4) {
        // 生成整数值并转换为网络字节顺序
        int32_t random_int1 = htonl(rand());  
        int32_t random_int2 = htonl(rand());  
        cp->CP.inValue1 = random_int1;
        cp->CP.inValue2 = random_int2;
        printf("oprand %d and %d ",ntohl(random_int1) , ntohl(random_int2));
    } else if (random_arith >= 5 && random_arith <= 8) {
        // 生成浮点数值
        double random_float1 = rand() / (double)RAND_MAX;
        double random_float2 = rand() / (double)RAND_MAX;
        cp->CP.flValue1 = random_float1;
        cp->CP.flValue2 = random_float2;
		printf("oprand %f and %f ",random_float1 , random_float2);
		
        switch(ntohl(cp->CP.arith)) {
            case 1: /*add */
                temp->inserverResult = htonl(ntohl(cp->CP.inValue1) + ntohl(cp->CP.inValue2));
                printf("result:%d.\n",ntohl(temp->inserverResult));
                break;
            case 2: /*sub */
                temp->inserverResult = htonl(ntohl(cp->CP.inValue1) - ntohl(cp->CP.inValue2));
printf("result:%d.\n",ntohl(temp->inserverResult));
                break;	
            case 3: /*mul */
                temp->inserverResult = htonl(ntohl(cp->CP.inValue1) * ntohl(cp->CP.inValue2));
printf("result:%d.\n",ntohl(temp->inserverResult));
                break;
            case 4: /*div */
                temp->inserverResult = htonl(ntohl(cp->CP.inValue1) / ntohl(cp->CP.inValue2));
printf("result:%d.\n",ntohl(temp->inserverResult));
                break;
            case 5: /*fadd */
                temp->flserverResult = cp->CP.flValue1 + cp->CP.flValue2;
                printf("result:%f.\n",temp->flserverResult);
                break;
            case 6: /*fsub */
                temp->flserverResult = cp->CP.flValue1 - cp->CP.flValue2;
                printf("result:%f.\n",temp->flserverResult);
                break;	
            case 7: /*fmul */
                temp->flserverResult = cp->CP.flValue1 * cp->CP.flValue2;
                printf("result:%f.\n",temp->flserverResult); 
                break;
            case 8: /*fdiv */
                temp->flserverResult = cp->CP.flValue1 / cp->CP.flValue2;
                printf("result:%f.\n",temp->flserverResult);
                break;
        }
        cp->CP.type = htons(TYPE_STC_BIN);
        cp->CP.major_version = htons(1);
    	cp->CP.minor_version = htons(0);
        *result=temp;	
    }
}

void sendStatusMessage( int type, int status) {
    struct calcMessage myMessage;
	memset(&myMessage,0,sizeof(myMessage));
    // 根据类型设置消息类型
    switch (type) {
        case TYPE_STC_TEXT:
			myMessage.type = htons(1); //1 = server-to-client, text protocol
            break;
        case TYPE_STC_BIN:
			myMessage.type = htons(2); // 2 =server-to-client, binary protocol
            break;
        case TYPE_STC_NA:
			myMessage.type = htons(3); // 3 = server-to-client, N/A
            break;
        case TYPE_CTS_TEXT:
			myMessage.type = htons(21); // 21 = client-to-server, text protocol
            break;
        case TYPE_CTS_BIN:
			myMessage.type = htons(22); //    22 = client-to-server, binary protocol
            break;
        case TYPE_CTS_NA:
            myMessage.type = htons(23);  // 23  client-to-serve, N/A

            break;
        default:
            fprintf(stderr, "Invalid message type\n");
            exit(EXIT_FAILURE);
    }

    // 根据状态设置消息内容
    switch (status) {
        case MES_NOT_AVAI:
            myMessage.message = htonl(0); // MES_NOT_AVAI
            break;
        case MES_OK:
            myMessage.message = htonl(1); // MES_OK
            break;
        case MES_NOK:
            myMessage.message = htonl(2); // MES_NOK
            break;
        default:
            fprintf(stderr, "Invalid status code\n");
            exit(EXIT_FAILURE);
    }

    // 设置协议类型
    myMessage.protocol = htons(UDP);

    // 设置版本号
    myMessage.major_version = htons(1);
    myMessage.minor_version = htons(0);

    // 发送消息
    ssize_t numbytes = sendto(socket_desc, &myMessage, sizeof(myMessage), 0, (struct sockaddr*)&client_addr, client_struct_length);
    if (numbytes == -1) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

void intepreteCP(cProtocol *cp){
	printf("\nType: %u\n", ntohs(cp->type));
    	printf("Major Version: %u\n", ntohs(cp->major_version));
    	printf("Minor Version: %u\n", ntohs(cp->minor_version));
    	printf("ID: %u\n", ntohl(cp->id));
    	printf("Arithmetic Operation: %u\n", ntohl(cp->arith));
    	printf("Integer Value 1: %d\n", ntohl(cp->inValue1));
    	printf("Integer Value 2: %d\n", ntohl(cp->inValue2));
    	printf("Integer Result: %d\n", ntohl(cp->inResult));
    	printf("Float Value 1: %f\n", cp->flValue1);
    	printf("Float Value 2: %f\n", cp->flValue2);
    	printf("Float Result: %f\n\n", cp->flResult);
}

char assignResultToAns(union dataSent* fromParent, union Result * ans) {
    if ( fromParent->CP.inResult != 0) {
        // 如果 inResult 不为零，将其赋值给 ans，并返回
        ans->inserverResult = ntohl(fromParent->CP.inResult);
        return 0;
    } else if ( fabs(fromParent->CP.flResult) > 0.0001  ) {
        // 如果 flResult 不为零，将其赋值给 ans，并返回
        ans->flserverResult = fromParent->CP.flResult;
        return 1;
    }
	return -1;
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
	while(waitpid(-1,NULL,WNOHANG)>0);
}

int main(int argc, char *argv[]){
  union dataSent client_message;
  socklen_t client_len;//for recv the client 
  fd_set readfds, selectfds;
  FD_ZERO( & readfds);
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
  
  signal(SIGCHLD,child_handler);//deal with the died children

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
  				union dataSent fromParent;
				union dataSent toClient;
				union Result* ans =NULL;//the answer generated by the server
				union Result ansC;//the answer sent by the client
  				signal(SIGALRM,alarm_handler);
  				while(1){
     				if(ans==NULL){
     	 
     				}else{//expect for an answer for client
     					memset(&fromParent,0,sizeof(fromParent));
     					alarm(EXPIRE_TIME);//setitimer?
     					ssize_t num_bytes = read( pipefd[currJob][0], &fromParent, sizeof(fromParent) );//character like msg
     					//read from the parent process to avoid competition
     					alarm(0);//free the alarm
     					if (num_bytes == -1) {
           					perror("read");
            				exit(0);
        				}
						//write a function to interpret the message
        				intepreteCP( &fromParent.CP );
    					//handle the answer : compare to the answer
						memset(&ansC,0,sizeof(ansC));
	 					char res = assignResultToAns(&fromParent, &ansC);//abstrct the answer from the client message // res represent the type of the result
						if(res == 0){
							if(fabs((ansC.inserverResult - ans->inserverResult)!=0)){
								sendStatusMessage( TYPE_STC_BIN, MES_OK);//do good
							}
							else{
								sendStatusMessage( TYPE_STC_BIN, MES_NOT_AVAI);

							}
						}
						else if(res == 1){
							if(fabs(ansC.flserverResult - ans->flserverResult) >= PRICISION){//use CM to answer MES_NOT_AVAI to represent the wrong answer
								sendStatusMessage( TYPE_STC_BIN, MES_NOT_AVAI);
        					}
							else{
								sendStatusMessage( TYPE_STC_BIN, MES_OK);//do good
        					}
						}
						else{
       						fprintf(stderr, "Unable to determine result type\n");
       						sendStatusMessage( TYPE_STC_BIN, MES_NOT_AVAI);
						}
        				
        					free(ans);
     				}
     //generate the question
     //response to the client
     				memset(&toClient,0,sizeof(toClient));
     				toClient.CP.id=htonl(currJob);
     				processMessage(&toClient, &ans);//wirte the message sent to the client
	 				sendto(socket_desc,&toClient,sizeof(toClient),0,(struct sockaddr*)&client_addr, client_struct_length);
  				}
  			}
	   }else{
	   		sendto(socket_desc,"ERROR",strlen("ERROR"),0,(struct sockaddr*)&client_addr, client_struct_length);
	   		printf("The server does not support the form of C.\n");
	   }
	}else if( recvLen == proLength ){//for the further communication
		//should not happened
		int id=ntohl(client_message.CP.id);
		if(joblistR.find(id) == joblistR.end() ){
			printf("should sign up first.\n");
			sendStatusMessage( TYPE_STC_TEXT, MES_NOK);
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
		sendto(socket_desc,"ERROR",strlen("ERROR"),0,(struct sockaddr*)&client_addr, client_struct_length);
	   printf("protocal length error.\n");
	}
  }
  exit(0);
}
