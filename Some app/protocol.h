#ifndef PROTOCOL_H
#define PROTOCOL_H

#define SIZEOF_HEADER 16
/// am pus typedef ca sa nu trb sa scriu struct cand declar o variabila de tip struct
typedef struct header{
    unsigned int numberOfBytes;
    unsigned int from;
    unsigned int operation;
    unsigned int respone_code;
}header;

#define SIZEOF_PACKET 1112
#define MAX_SIZE_OF_CONTENT 1000

typedef struct Packet{
    //unsigned int numberOfBytes;
    unsigned int from;
    unsigned int operation;
    unsigned int respone_code;
    char userID[50];
    char appID[50];
    char content[MAX_SIZE_OF_CONTENT];
}Packet;

#define FROM_FATHER 1
#define FROM_CLIENT 2
#define FROM_CHILD_PID 3
#define FROM_ACCOUNT_PID 4
#define FROM_GET_LOGGED_USERS 5
#define FROM_SOME_SERVER 6

#define OPERATION_QUIT 1
#define OPERATION_GET_PID_INFO 2
#define OPERATION_LOGIN 3
#define OPERATION_LOGOUT 4
#define OPERATION_GET_LOGGED_USERS 5
#define OPERATION_CHECK_CODE_REQ 6
#define OPERATION_CHECK_APPROVAL 7
#define OPERATION_CHOOSE 8

#define RESPONSE_ERROR 0
#define RESPONSE_SUCCESS 1
#define RESPONSE_ACCEPTED 2
#define RESPONSE_DECLINED 3
#define RESPONSE_WAIT 4

#endif