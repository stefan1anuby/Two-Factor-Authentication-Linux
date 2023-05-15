
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/time.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protocol.h"

#define FIFO_NAME_SEND "MyTest_FIFO2"
#define FIFO_NAME_RECEIVE "MyTest_FIFO"

#define MAX_BUFFER_SIZE 10005


int DEBUG_INFO = 0;

int sockp[2],sockp2[2];
int fd_loginPC[2],fd_loginCP[2];

void ListenerProcess(){

    char buffer[MAX_BUFFER_SIZE],contentBuffer[MAX_BUFFER_SIZE];
    int numBytesRead, fd_receive , fd_send , fd_rec, fd_sen;
    char APP_NAME[] = "slack";

    mkfifo(FIFO_NAME_RECEIVE, S_IFIFO | 0666);
    mkfifo(FIFO_NAME_SEND, S_IFIFO | 0666);

    fd_sen = fd_loginPC[1];
    fd_rec = fd_loginCP[0];

    if(DEBUG_INFO > 0 ) printf("Astept sa scrie cineva...\n");
    fd_receive = open(FIFO_NAME_RECEIVE, O_RDONLY);
    fd_send = open(FIFO_NAME_SEND,O_WRONLY);
    if(DEBUG_INFO > 0 ) printf("A venit cineva:\n");

    do {
        if ((numBytesRead = read(fd_receive, buffer, SIZEOF_HEADER)) == -1)
            perror("Eroare la citirea din FIFO!\n");
        else {
            buffer[numBytesRead] = '\0';
            // ar merge niste verificari la marimea lui num
            
            header message = *((header*)buffer);

            //if(DEBUG_INFO > 1 ) printf("s a citit headerul si contine %d bytes si string %s \n",numBytesRead,buffer);
   
            bzero(contentBuffer,sizeof(contentBuffer));
            if(message.numberOfBytes > 0){
                int bytsread = read(fd_receive, contentBuffer, message.numberOfBytes);
                if(message.numberOfBytes != bytsread)
                    perror("Something went wrong!!\n");
                //if(DEBUG_INFO > 1 ) printf("S-au citit din FIFO %d bytes: \"%s\"\n", message.numberOfBytes , contentBuffer);  

            }

            if(message.from == FROM_CLIENT && message.operation == OPERATION_QUIT) {
                header request;
                request.from = FROM_FATHER;
                request.numberOfBytes = 0;
                request.operation = OPERATION_QUIT;

                /// send to child the request to quit
                if (write(sockp[1], &request , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                if (write(fd_sen, &request , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                if (write(sockp2[1], &request , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");

                /// wait for the child to answer
                if (read(sockp[1], buffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din SOCKET!\n");

                buffer[SIZEOF_HEADER] = '\0';
                header responeFromChildPID = *((header*)buffer);

                if (read(fd_rec, buffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din PIPE!\n");

                buffer[SIZEOF_HEADER] = '\0';
                header responeFromChildAcc = *((header*)buffer);

                if (read(sockp2[1], buffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din SOCKET2!\n");

                buffer[SIZEOF_HEADER] = '\0';
                header responeFromGetLoggedUsers = *((header*)buffer);

                int isChildPIDProcDead = 0, taskSuccess = 0 , isChildAccProcDead = 0 , isChildGetLoggedUsersDead = 0;
                
                if(responeFromChildPID.from == FROM_CHILD_PID && responeFromChildPID.operation == OPERATION_QUIT && responeFromChildPID.respone_code == RESPONSE_SUCCESS){
                    //printf("AM PRIMIS ACEST MESAAAJ\n");
                    
                    isChildPIDProcDead = 1;

                    //return;
                }

                if(responeFromChildAcc.from == FROM_ACCOUNT_PID && responeFromChildAcc.operation == OPERATION_QUIT && responeFromChildAcc.respone_code == RESPONSE_SUCCESS){
                    //printf("AM PRIMIS ACEST MESAAAJ\n");
                    
                    isChildAccProcDead = 1;

                    //return;
                }                    
                if(responeFromGetLoggedUsers.from == FROM_GET_LOGGED_USERS && responeFromGetLoggedUsers.operation == OPERATION_QUIT && responeFromGetLoggedUsers.respone_code == RESPONSE_SUCCESS){
                    //printf("AM PRIMIS ACEST MESAAAJ\n");
                    
                    isChildGetLoggedUsersDead = 1;

                    //return;
                }
                
                /// check if task succeded
                if(isChildPIDProcDead == 1 && isChildAccProcDead == 1 && isChildGetLoggedUsersDead == 1 ){
                    //child is dead , tasks is a succes
                    taskSuccess = 1;
                }

                header responseToClient;
                responseToClient.from = FROM_FATHER;
                responseToClient.operation = OPERATION_QUIT;
                responseToClient.respone_code = taskSuccess ? RESPONSE_SUCCESS : RESPONSE_ERROR;

                if (write(fd_send, &responseToClient , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");

                if(taskSuccess == 1) return;
            }
            if(message.from == FROM_CLIENT && message.operation == OPERATION_GET_PID_INFO && message.numberOfBytes > 0){
                
                int taskSuccess = 0;

                if(DEBUG_INFO > 2) printf("Comanda urmeaza sa fie procesata!!\n");

                header request;
                request.from = FROM_FATHER;
                request.numberOfBytes = message.numberOfBytes;
                request.operation = OPERATION_GET_PID_INFO;

                /// send request to child
                if (write(sockp[1], &request , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                if (write(sockp[1], contentBuffer , request.numberOfBytes) < 0) perror("[parinte]Err.....write\n");

                /// wait for the child to answer
                if (read(sockp[1], buffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din SOCKET!\n");

                header response = *((header*)buffer);

                if(response.respone_code == RESPONSE_SUCCESS) taskSuccess = 1;

                if (read(sockp[1], contentBuffer, response.numberOfBytes) == -1)
                    perror("Eroare la citirea din SOCKET!\n");

                ///send the full response back to client
                header responseToClient;
                responseToClient.from = FROM_FATHER;
                responseToClient.operation = OPERATION_GET_PID_INFO;
                responseToClient.respone_code = taskSuccess ? RESPONSE_SUCCESS : RESPONSE_ERROR;
                responseToClient.numberOfBytes = taskSuccess ? response.numberOfBytes : 0 ;

                if (write(fd_send, &responseToClient , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                if (write(fd_send, contentBuffer , responseToClient.numberOfBytes) < 0) perror("[parinte]Err...write\n");

                
            }
            if(message.from == FROM_CLIENT && message.operation == OPERATION_LOGIN && message.numberOfBytes > 0){
                int taskSuccess = 0;

                if(DEBUG_INFO > 2) printf("Comanda urmeaza sa fie procesata!!\n");

                header request;
                request.from = FROM_FATHER;
                request.numberOfBytes = message.numberOfBytes;
                request.operation = OPERATION_LOGIN;

                /// send request to child
                if (write(fd_sen, &request , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                if (write(fd_sen, contentBuffer , request.numberOfBytes) < 0) perror("[parinte]Err.....write\n");

                /// wait for the child to answer
                bzero(buffer,sizeof(buffer));
                if (read(fd_rec, buffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din PIPE!\n");

                header response = *((header*)buffer);

                if(response.respone_code == RESPONSE_SUCCESS) {
                    

                    header responseToClient;
                    responseToClient.from = FROM_FATHER;
                    responseToClient.operation = OPERATION_LOGIN;
                    responseToClient.respone_code = RESPONSE_SUCCESS;
                    responseToClient.numberOfBytes = 0 ;

                    if (write(fd_send, &responseToClient , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                    else printf("adsda am trimis raspunsul la client de a alege o varianta !!\n");

                    /* in content buffer ai numele utiizatorului */
                    header option_from_client;


                    if (read(fd_receive, &option_from_client , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                    else printf("adsda am trimis raspunsul la client !!\n");

                    int operation = 0;
                    char code_to_receive[10];

                    if(option_from_client.numberOfBytes == 0){
                        operation = OPERATION_CHECK_APPROVAL;
                    }
                    else if(option_from_client.numberOfBytes <= 4){
                        operation = OPERATION_CHECK_CODE_REQ;
                        if (read(fd_receive, code_to_receive, option_from_client.numberOfBytes) < 0) perror("[parinte]Err...write\n");

                    }
                    else{
                        printf("IDK WHAT YOU DID THERE \n");
                    }

                    


                    /*CONECTEAZA TE LA SERVER UL 2FA */
                    /* cream socketul */

                    int sd_twofa;
                    struct sockaddr_in server_twofa;

                    int port = 2908;
                    char ip[]="127.0.0.1";

                    if ((sd_twofa = socket(AF_INET, SOCK_STREAM, 0)) == -1)
                    {
                        perror("Eroare la socket().\n");
                        return errno;
                    }

                    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
                    /* familia socket-ului */
                    server_twofa.sin_family = AF_INET;
                    /* adresa IP a serverului */
                    server_twofa.sin_addr.s_addr = inet_addr(ip);
                    /* portul de conectare */
                    server_twofa.sin_port = htons(port);

                    /* ne conectam la server */
                    if (connect(sd_twofa, (struct sockaddr *)&server_twofa, sizeof(struct sockaddr)) == -1)
                    {
                        perror("[client]Eroare la connect().\n");
                        return errno;
                    }
                    else printf("M am conectat la servererul 2FA !! \n");


                    /* FA REQUESTUL CATRE SERVERUL 2FA */
                    Packet TwoFactorAuthRequest;
                    strcpy(TwoFactorAuthRequest.appID,APP_NAME);
                    strcpy(TwoFactorAuthRequest.userID,contentBuffer);
                    TwoFactorAuthRequest.operation = operation;
                    TwoFactorAuthRequest.from = FROM_SOME_SERVER;
                    strcpy(TwoFactorAuthRequest.content,code_to_receive);

                    /* SCRIE REQUESTUL */    

                    if (write(sd_twofa, &TwoFactorAuthRequest , SIZEOF_PACKET) < 0) perror("[parinte]Err...write pentru 2FA request \n");



                    /* CITESTE REQUESTUL */
                    Packet TwoFactorAuthResponse;
                    if (read(sd_twofa, &TwoFactorAuthResponse , SIZEOF_PACKET) < 0) perror("[parinte]Err...read pentru 2FA response \n");

                    if(TwoFactorAuthResponse.respone_code == RESPONSE_ACCEPTED || TwoFactorAuthResponse.respone_code == RESPONSE_SUCCESS) 
                        taskSuccess = 1;

                    close(sd_twofa);
                }

                ///send the full response back to client
                header responseToClient;
                responseToClient.from = FROM_FATHER;
                responseToClient.operation = OPERATION_LOGIN;
                responseToClient.respone_code = taskSuccess;
                responseToClient.numberOfBytes = 0 ;

                if (write(fd_send, &responseToClient , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                else printf("am trimis raspunsul la client !!\n");
            }
            if(message.from == FROM_CLIENT && message.operation == OPERATION_GET_LOGGED_USERS) {
                int taskSuccess = 0;

                //if(DEBUG_INFO > 2) printf("Comanda urmeaza sa fie procesata!!\n");

                header request;
                request.from = FROM_FATHER;
                request.numberOfBytes = message.numberOfBytes;
                request.operation = OPERATION_GET_LOGGED_USERS;

                /// send request to child
                if (write(sockp2[1], &request , SIZEOF_HEADER) < 0) perror("[parinte]Err...,write\n");

                /// wait for the child to answer
                if (read(sockp2[1], buffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din SOCKET!\n");

                header response = *((header*)buffer);

                if(response.respone_code == RESPONSE_SUCCESS) taskSuccess = 1;

                if (read(sockp2[1], contentBuffer, response.numberOfBytes) == -1)
                    perror("Eroare la citirea din SOCKET!\n");

                ///send the full response back to client
                header responseToClient;
                responseToClient.from = FROM_FATHER;
                responseToClient.operation = OPERATION_GET_LOGGED_USERS;
                responseToClient.respone_code = taskSuccess ? RESPONSE_SUCCESS : RESPONSE_ERROR;
                responseToClient.numberOfBytes = taskSuccess ? response.numberOfBytes : 0 ;

                if (write(fd_send, &responseToClient , SIZEOF_HEADER) < 0) perror("[parinte]Err...write\n");
                if (write(fd_send, contentBuffer , responseToClient.numberOfBytes) < 0) perror("[parinte]Err...write\n");
            }
        }
    } while (numBytesRead > 0);
}

void GetPIDInfoProcess(){
    char buffer[MAX_BUFFER_SIZE],contentBuffer[MAX_BUFFER_SIZE];

    int messageLength;
    char job_name[] = "COPILU CU PIDU\0";
    do{
        if ((messageLength = read(sockp[0], buffer, SIZEOF_HEADER)) == -1)
            perror("<> : Eroare la citirea din sockets pair!\n");
        else{
            buffer[SIZEOF_HEADER] = '\0';
            // ar merge niste verificari la marimea lui num
            
            //int numBytesToReadContent = *((int*)buffer);
            header request = *((header*)buffer);
            //if(DEBUG_INFO > 3) printf("<%s> : a ajuns un request la mine \n",job_name);

            if(request.from == FROM_FATHER && request.operation == OPERATION_QUIT){
                //send response

                header response;
                response.from = FROM_CHILD_PID;
                response.numberOfBytes = 0;
                response.operation = OPERATION_QUIT;
                response.respone_code = RESPONSE_SUCCESS;

                //printf("<%s> : incerc sa trimit raspunsu \n",job_name);

                if (write(sockp[0], &response, SIZEOF_HEADER ) == -1) printf("<> : Err...write\n");
                //else if(DEBUG_INFO > 10) printf("<%s> : raspunsul a fost trimis , eu ma inchid !!!!\n",job_name);

                return;
            }
            else if(request.from == FROM_FATHER && request.operation == OPERATION_GET_PID_INFO && request.numberOfBytes > 0){
                
                int taskSuccess = 1;

                if (read(sockp[0], contentBuffer, request.numberOfBytes) == -1){
                    perror("<> : Eroare la citirea din sockets pair!\n");
                    taskSuccess = 0;
                }
                
                contentBuffer[request.numberOfBytes] = 0;
                //if(DEBUG_INFO > 10) printf("<%s> : contentbuferru e : %s \n",job_name,contentBuffer);



                /// TODO trebuie sa concatenez aceste 3 stringuri ca altfel crapa
                char path[100] ="/proc/";

                strcat(contentBuffer,"/status");
                strcat(path,contentBuffer);
                
                if(DEBUG_INFO > 10) printf("path : %s\n",path);
                int fd = open(path,O_RDONLY);

                int bytesRead = read(fd,contentBuffer,MAX_BUFFER_SIZE);

                close(fd);

                if(bytesRead < 0) {
                    perror("eroare la citire din fisier \n");
                    taskSuccess = 0;
                }

                int len = 0; 
                char responseBuffer[MAX_BUFFER_SIZE];

                if(taskSuccess == 1){

                    contentBuffer[bytesRead] = 0;

                    //if(DEBUG_INFO > 10) printf("numarul de bytes cititi din proc/2/status este : %d \n",bytesRead);

                    //if(DEBUG_INFO > 3 ) printf("buffer : %s \n",buffer);

                    int row = 1;

                    for(int i = 0 ; i < bytesRead ; i++){
                        /// idk where the fuck is "vmsize"
                        if(row == 1 || row == 3 || row == 7 || row == 9)
                            responseBuffer[len++] = contentBuffer[i]; 
                        if(contentBuffer[i] == '\n')
                            row++;
                    }

                    responseBuffer[len] = 0;


                    //if(DEBUG_INFO > 10) printf("<%s> : response buffer : \n%s\n",job_name,responseBuffer);
                }

                header response;
                response.from = FROM_CHILD_PID;
                response.numberOfBytes = len;
                response.operation = OPERATION_GET_PID_INFO;
                response.respone_code = taskSuccess ? RESPONSE_SUCCESS : RESPONSE_ERROR;

                //if(DEBUG_INFO > 10) printf("<%s> : incerc sa trimit raspunsuuuu \n",job_name);

                if (write(sockp[0], &response, SIZEOF_HEADER ) == -1) perror("<> : Err...write\n");
                if (response.respone_code == RESPONSE_SUCCESS && write(sockp[0], responseBuffer, response.numberOfBytes ) == -1) perror("<> : Err.write\n");
                //else if(DEBUG_INFO > 10) printf("<%s> : raspunsul a fost trimis la parinte !!!!\n",job_name);
                
            }
        }

    }while(messageLength > 0);
}

void AccountsProcess(){
    char buffer[MAX_BUFFER_SIZE],contentBuffer[MAX_BUFFER_SIZE];
    int numBytesRead, fd_receive , fd_send;
    char job_name[] = "COPILU CU CONTURILE\0";

    fd_receive = fd_loginPC[0];
    fd_send = fd_loginCP[1];

    //printf("<%s> : eu am luat viata  !!\n",job_name);

    do {
        if ((numBytesRead = read(fd_receive, buffer, SIZEOF_HEADER)) == -1)
            perror("Eroare la citirea din PIPE!!!\n");
        else {
            buffer[numBytesRead] = '\0';
            
            header request = *((header*)buffer);

            //if(DEBUG_INFO > 1 ) printf("s a citit headerul si contine %d bytes si string %s \n",numBytesRead,buffer);
   
            if(request.numberOfBytes > 0){
                int bytsread = read(fd_receive, contentBuffer, request.numberOfBytes);
                if(request.numberOfBytes != bytsread)
                    perror("Something went wrong!!\n");
                //if(DEBUG_INFO > 1 ) printf("S-au citit din PIPE %d bytes: \"%s\"\n", request.numberOfBytes , contentBuffer);  
                contentBuffer[bytsread] = 0;
            }

            if(request.from == FROM_FATHER && request.operation == OPERATION_QUIT){
                //send response

                header response;
                response.from = FROM_ACCOUNT_PID;
                response.numberOfBytes = 0;
                response.operation = OPERATION_QUIT;
                response.respone_code = RESPONSE_SUCCESS;

                printf("<%s> : incerc sa trimit raspunsu \n",job_name);

                if (write(fd_send, &response, SIZEOF_HEADER ) == -1) printf("<> : Err...write\n");
                else if(DEBUG_INFO > 10) printf("<%s> : raspunsul a fost trimis , eu ma inchid !!!!\n",job_name);

                return;
            }           
            if(request.from == FROM_FATHER && request.operation == OPERATION_LOGIN && request.numberOfBytes > 0){
                
                char path[] = "project_passwords.txt";
                char nameToFind[MAX_BUFFER_SIZE];
                strcpy(nameToFind,contentBuffer);

                int taskSuccess = 1;

                int fd = open(path,O_RDONLY);

                int bytesRead = read(fd,contentBuffer,MAX_BUFFER_SIZE);

                close(fd);

                if(bytesRead < 0) {
                    perror("eroare la citire din fisier \n");
                    taskSuccess = 0;
                }
                else{
                    contentBuffer[bytesRead] = 0;

                    char* words[100];
                    int tokensCounter = 0;

                    words[tokensCounter] = strtok(contentBuffer,"\n");

                    while( words[tokensCounter] != NULL){
                        words[++tokensCounter] = strtok(NULL,"\n");
                    }

                    int found = 0 ;
                    for(int i = 0 ; i < tokensCounter ; i++){
                        if(strcmp(nameToFind,words[i]) == 0) found = 1;
                    }

                    if(found == 0) taskSuccess = 0;
                }

                header response;
                response.from = FROM_ACCOUNT_PID;
                response.numberOfBytes = 0;
                response.operation = OPERATION_LOGIN;
                response.respone_code = taskSuccess ? RESPONSE_SUCCESS : RESPONSE_ERROR;

                //printf("<%s> : incerc sa trimit raspunsu \n",job_name);

                if (write(fd_send, &response, SIZEOF_HEADER ) == -1) printf("<> : Err...write\n");
                //else if(DEBUG_INFO > 10) printf("<%s> : raspunsul a fost trimis !!!!\n",job_name);
                
            }
        }
    } while (numBytesRead > 0);
}

void GetLoggedUsersInfoProcess(){
    char buffer[MAX_BUFFER_SIZE],contentBuffer[MAX_BUFFER_SIZE];
    int numBytesRead, fd_receive , fd_send;
    char job_name[] = "COPILU CU GET-LOGGED-USERS\0";

    fd_receive = sockp2[0];
    fd_send = sockp2[0];

    //printf("<%s> : eu am luat viata  !!\n",job_name);

    do {
        if ((numBytesRead = read(sockp2[0], buffer, SIZEOF_HEADER)) == -1)
            perror("Eroare la citirea din SOCKETS2!!!\n");
        else {
            buffer[numBytesRead] = '\0';
            
            header request = *((header*)buffer);

            //printf("sa citit headerul si contine %d bytes si string %s \n",numBytesRead,buffer);
            //printf("<%s> : a ajuns un request la mine \n",job_name);
   
            if(request.numberOfBytes > 0){
                int bytsread = read(fd_receive, contentBuffer, request.numberOfBytes);
                if(request.numberOfBytes != bytsread)
                    perror("Something went wrong!!\n");
                //if(DEBUG_INFO > 1 ) printf("S-au citit din SOCKETS2 %d bytes: \"%s\"\n", request.numberOfBytes , contentBuffer);  

            }

            if(request.from == FROM_FATHER && request.operation == OPERATION_QUIT){
                //send response

                header response;
                response.from = FROM_GET_LOGGED_USERS;
                response.numberOfBytes = 0;
                response.operation = OPERATION_QUIT;
                response.respone_code = RESPONSE_SUCCESS;

                //printf("<%s> : incerc sa trimit raspunsu \n",job_name);

                if (write(fd_send, &response, SIZEOF_HEADER ) == -1) perror("<> : Err..,,.write\n");
                //else if(DEBUG_INFO > 10) printf("<%s> : raspunsul a fost trimis , eu ma inchid !!!!\n",job_name);

                return;
            }           
            else if(request.from == FROM_FATHER && request.operation == OPERATION_GET_LOGGED_USERS){
                
                struct utmp *n;
                setutent();
                n=getutent();
                int len = 0;

                contentBuffer[0] = 0;

                while(n) {
                    if(n->ut_type==USER_PROCESS) {
                        
                        time_t time;
                        struct tm *tim;
                        char tmbuf[64], buf[MAX_BUFFER_SIZE];


                        time = n->ut_tv.tv_sec;
                        tim = localtime(&time);
                        strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", tim);
                        sprintf(buf, "user : %s \nhostname : %s \ntime entry : %s \n \n",n->ut_user , n->ut_host ,tmbuf);

                        //printf("%s \n",buf);
                        len += strlen(buf);

                        strcat(contentBuffer,buf);

                        }
                    n=getutent();
                }

                header response;
                response.from = FROM_GET_LOGGED_USERS;
                response.numberOfBytes = len;
                response.operation = OPERATION_GET_LOGGED_USERS;
                response.respone_code = RESPONSE_SUCCESS;

                //printf("<%s> : incerc sa trimit raspunsu \n",job_name);

                if (write(fd_send, &response, SIZEOF_HEADER ) == -1) perror("<> : Err..,,.write\n");
                if (write(fd_send, contentBuffer, response.numberOfBytes ) == -1) perror("<> : Err..,,.write\n");
                //else if(DEBUG_INFO > 10) printf("<%s> : raspunsul a fost trimis , eu ma inchid !!!!\n",job_name);

            }
        }
    } while (numBytesRead > 0);
}


int main()
{

    pid_t pid,pid1,pid2;
    int rv;

    pipe(fd_loginPC);
    pipe(fd_loginCP);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp2) < 0) 
    { 
        perror("Err..... socketpair"); 
        exit(1); 
    }
  
    pid = fork();
    if (pid == 0) {
        
        printf("child[2] --> pid = %d and ppid = %d\n",
               getpid(), getppid());

        close(sockp2[1]); 
        GetLoggedUsersInfoProcess();
        close(sockp2[0]);
        exit(rv); 
    }
    else {

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0) 
        { 
            perror("Err... socketpair"); 
            exit(1); 
        }


        pid1 = fork();
        if (pid1 == 0) {
            
            printf("child[0] --> pid = %d and ppid = %d\n",
               getpid(), getppid());

            close(sockp[1]); 
            GetPIDInfoProcess();
            close(sockp[0]);
            exit(rv); 
        }
        else {
            pid2 = fork();
            if (pid2 == 0) {
                
                printf("child[1] --> pid = %d and ppid = %d\n",
               getpid(), getppid());

                //copilul inchide capatul de scriere
                close(fd_loginPC[1]);
                
                close(fd_loginCP[0]);

                AccountsProcess();
                close(fd_loginPC[0]);

                close(fd_loginCP[1]);

                exit(rv);
            }

            else {

                //parintele inchide capatul de citire
                close(fd_loginPC[0]);

                close(fd_loginCP[1]);

                close(sockp[0]);
                close(sockp2[0]); 
                ListenerProcess();
                close(sockp[1]);
                close(sockp2[1]);
                close(fd_loginPC[1]);
                close(fd_loginCP[0]);

                wait(&rv);
                wait(&rv);
                wait(&rv);
            }
        }

    }
}

