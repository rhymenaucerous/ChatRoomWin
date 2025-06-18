/*****************************************************************//**
 * \file   Messages.h
 * \brief  
 * 
 * \author chris
 * \date   September 2024
 *********************************************************************/
#pragma once
#include "pch.h"

//NOTE: Message types
#define TYPE_ACCOUNT 0
#define TYPE_CHAT 1
#define TYPE_LIST 2
#define TYPE_BROADCAST 3
#define TYPE_FAILURE 127

//NOTE: Message sub-types
#define STYPE_EMPTY 0
#define STYPE_LOGIN 1
#define STYPE_LOGOUT 2

//NOTE: Message OPCODES
#define OPCODE_REQ 0
#define OPCODE_RES 1
#define OPCODE_ACK 2

//NOTE: Reject Codes
#define REJECT_SRV_BUSY 0
#define REJECT_SRV_ERR 1
#define REJECT_INVALID_PACKET 2
#define REJECT_UNAME_LEN 3
#define REJECT_USER_LOGGED 4
#define REJECT_USER_NOT_EXIST 5
#define REJECT_MSG_LEN 6
#define REJECT_SRV_FULL 7

#pragma pack(push,1) //NOTE: No spacing within data structure

//NOTE: INT8 will not have to be converted from network byte order. WORD will.
// WCHAR characters will also have to be converted.
// 
//NOTE: By selecting WORD as the data type, the server implicitly limits the
// network packet size at 65535 characters for both data sections. They are 
// explicitly set to BUFF_SIZE (1024 wide characters - 2048 bytes) and below.
typedef struct CHATMSG {
	INT8  iType;
	INT8  iSubType;
	INT8  iOpcode; //NOTE: This will be a reject code if the type is failure.
	WORD  wLenOne; //NOTE: lengths one and two designate the lengths of the
				   //(up to) two strings in the message. This way, the reciever
				   //will always know how many bytes they're going to recieve.
	WORD  wLenTwo;
	PWSTR pszDataOne; 
	PWSTR pszDataTwo;
} CHATMSG, * PCHATMSG;

#pragma pack(pop) //pragma statement at line 35

//NOTE: Even if there isn't data beyond the header - lengths one and two will
//be filled with values.
#define HEADER_LEN 7 //NOTE: Three INT8 and two WORD types. 3*1 + 2*2 = 7.
#define LENGTH_ZERO 0

//NOTE: Used for wsa buffer array classification.
#define ONE_BUFFER 1
#define TWO_BUFFERS 2
#define THREE_BUFFERS 3
#define HEADER_INDEX 0
#define BODY_INDEX_1 1
#define BODY_INDEX_2 2

VOID
WstrNetToHost(PWSTR pszString, INT iLen);

VOID
WstrHostToNet(PWSTR pszString, INT iLen);

//End of file
