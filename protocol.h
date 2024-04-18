#ifndef MACROS_H
#define MACROS_H

#ifdef __GCC_IEC_559 
#pragma message("GCC ICE 559 defined...")

#else

#error *** do not use this platform

#endif


#include <stdint.h>


#define MES_NOT_AVAI 0
#define MES_OK 1
#define MES_NOK 2

#define TYPE_STC_TEXT 1
#define TYPE_STC_BIN 2
#define TYPE_STC_NA 3
#define TYPE_CTS_TEXT 21
#define TYPE_CTS_BIN 22
#define TYPE_CTS_NA 23

#define UDP 17
#define TCP 6

#define ARITH_ADD 1
#define ARITH_SUB 2
#define ARITH_MUL 3
#define ARITH_DIV 4
#define ARITH_FADD 5
#define ARITH_FSUB 6
#define ARITH_FMUL 7
#define ARITH_FDIV 8

#define STATE_TIMEOUT 1
#define STATE_CORRECT 2
#define STATE_WARONG 3
#define STATE_QUESTION 4

#define MAX_QUES_LEN 120
#define MAX_MESSAGE_LEN 1200

/* 
   Used in both directions; if 
   server->client,type should be set to 1, 
   client->server type = 2. 
 */
struct  __attribute__((__packed__)) calcProtocol{
  uint16_t type;  // What message is this, 1 = server to client, 2 client to server, 3... reserved , conversion needed 
  uint16_t major_version; // 1, conversion needed 
  uint16_t minor_version; // 0, conversion needed 
  uint32_t id; // Server side identification with operation. Client must return the same ID as it got from Server., conversion needed 
  uint32_t arith; // What operation to perform, see mapping below. 
  int32_t inValue1; // integer value 1, conversion needed 
  int32_t inValue2; // integer value 2, conversion needed 
  int32_t inResult; // integer result, conversion needed 
  double flValue1;  // float value 1,NO NEED TO do host to Network or Network to Host conversion here, we are using equivalent platforms        
  double flValue2;  // float value 2,NO NEED TO do host to Network or Network to Host conversion here, we are using equivalent platforms
  double flResult;  // float result,NO NEED TO do host to Network or Network to Host conversion here, we are using equivalent platforms
};

  
struct  __attribute__((__packed__)) calcMessage {
  uint16_t type;    // See below, conversion needed 
  uint32_t message; // See below, conversion needed 
  
  // Protocol, UDP = 17, TCP = 6, other values are reserved. 
  uint16_t protocol; // conversion needed 
  uint16_t major_version; // 1, conversion needed 
  uint16_t minor_version; // 0 , conversion needed 

};

union dataSent{
	char message[MAX_MESSAGE_LEN];
	struct calcMessage CM;
	struct calcProtocol CP;
};//must ensure the length comes first
#endif

//time out 
//wrong 
//correct

/* arith mapping in calcProtocol
1 - add
2 - sub
3 - mul
4 - div
5 - fadd
6 - fsub
7 - fmul
8 - fdiv

other numbers are reserved

*/


/* 
   calcMessage.type
   1 - server-to-client, text protocol
   2 - server-to-client, binary protocol
   3 - server-to-client, N/A
   21 - client-to-server, text protocol
   22 - client-to-server, binary protocol
   23 - client-to-serve, N/A
   
   calcMessage.message 

   0 = Not applicable/availible (N/A or NA)
   1 = OK   // Accept 
   2 = NOT OK  // Reject 

*/
