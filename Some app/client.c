
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "protocol.h"

#define FIFO_NAME_SEND "MyTest_FIFO"
#define FIFO_NAME_RECEIVE "MyTest_FIFO2"

#define MAX_BUFFER_SIZE 10005

int isNumber(char s[])
{
    for (int i = 0; s[i]!= '\0'; i++)
    {
        if (isdigit(s[i]) == 0)
              return 0;
    }
    return 1;
}

int main()
{
    char commandBuffer[MAX_BUFFER_SIZE],username[MAX_BUFFER_SIZE];
    int num, fd_send , fd_receive;
 

    printf("Astept cititori...\n");
    fd_send = open(FIFO_NAME_SEND, O_WRONLY);
    fd_receive = open(FIFO_NAME_RECEIVE, O_RDONLY);
    printf("Am un cititor....introduceti ceva..\n");

    while (gets(commandBuffer), !feof(stdin)) {
        int len = strlen(commandBuffer);
        
        /// parse words from cmd
        char* words[100];
        int tokensCounter = 0;

        words[tokensCounter] = strtok(commandBuffer," ");

        while( words[tokensCounter] != NULL){
            words[++tokensCounter] = strtok(NULL," ");
        }

        if(words[0] != NULL && strcmp(words[0],"quit") == 0){
            header command;
            command.from = FROM_CLIENT;
            command.numberOfBytes = 0;
            command.operation = OPERATION_QUIT;

            if ((num = write(fd_send,&command,SIZEOF_HEADER)) == -1) 
                perror("Problema la scriere in FIfFO!");
            else {
                printf("Cererea a fost trimisa !\n");
                if (read(fd_receive, commandBuffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din FIFO!\n");

                header response = *((header*)commandBuffer);

                if(response.from == FROM_FATHER && response.operation == OPERATION_QUIT){
                    if(response.respone_code == RESPONSE_SUCCESS)
                        printf("Operatia de quit a avut succes !!!!\n");
                    else printf("Operatia de quit a esuat :( \n");
                }
            }

        }
        else if(words[0] != NULL && strcmp(words[0],"logout") == 0){
            username[0] = 0;
            printf("Logout successfully \n");
        }
        else if(words[0] != NULL && strcmp(words[0],"get-logged-users") == 0){
            
            if(strlen(username) != 0 ){
                // daca userul e logat
                    header command;
                    command.from = FROM_CLIENT;
                    command.numberOfBytes = 0;
                    command.operation = OPERATION_GET_LOGGED_USERS;

                    if (write(fd_send,&command,SIZEOF_HEADER) == -1) 
                        perror("Problema la scriere in FIfFO!");

                    printf("Cererea a fost trimisa !\n");
                    // trebuie sa astept raspunsul 
                    if (read(fd_receive, commandBuffer, SIZEOF_HEADER) == -1)
                        perror("Eroare la citirea din FIFO!\n");

                    header response = *((header*)commandBuffer);

                    if(response.from == FROM_FATHER && response.operation == OPERATION_GET_LOGGED_USERS){
                        if(response.respone_code == RESPONSE_SUCCESS){
                            printf("Operatia de GET-LOGGED-USERS a avut succes !!!!\n");
                            if (read(fd_receive, commandBuffer, response.numberOfBytes) == -1)
                                perror("Eroare la citirea din FIFO!\n");
                            commandBuffer[response.numberOfBytes] = 0;
                            printf("%s\n",commandBuffer);
                        }
                        else printf("Operatia de GET-LOGGED-USERS a esuat :( \n");
                    }
                    bzero(commandBuffer,sizeof(commandBuffer));
            }
            else{
                printf("Please Login first \n");
            }
        }
        else if(words[0] != NULL && strcmp(words[0],"get-proc-info") == 0 && strcmp(words[1],":") == 0){
            if(isNumber(words[2])){

                if(strlen(username) != 0 ){
                    // daca userul e logat
                    header command;
                    command.from = FROM_CLIENT;
                    command.numberOfBytes = strlen(words[2]);
                    command.operation = OPERATION_GET_PID_INFO;

                    if (write(fd_send,&command,SIZEOF_HEADER) == -1) 
                        perror("Problema la scriere in FIfFO!");
                    if (write(fd_send,words[2],command.numberOfBytes) == -1) 
                        perror("Problema la scriere in FIfFO!");

                    printf("Cererea a fost trimisa !\n");
                    // trebuie sa astept raspunsul 
                    if (read(fd_receive, commandBuffer, SIZEOF_HEADER) == -1)
                        perror("Eroare la citirea din FIFO!\n");

                    header response = *((header*)commandBuffer);

                    if(response.from == FROM_FATHER && response.operation == OPERATION_GET_PID_INFO){
                        if(response.respone_code == RESPONSE_SUCCESS){
                            printf("Operatia de GET-PID a avut succes !!!!\n");
                            if (read(fd_receive, commandBuffer, response.numberOfBytes) == -1)
                                perror("Eroare la citirea din FIFO!\n");
                            commandBuffer[response.numberOfBytes] = 0;
                            printf("%s\n",commandBuffer);
                        }
                        else printf("Operatia de GET-PID a esuat :( \n");
                    }
                }
                else {
                    printf("Please Login first !\n");
                }
                
            }
            else printf("Please enter a number as pid\n");

        }
        else if(words[0] != NULL && strcmp(words[0],"login")  == 0 && strcmp(words[1],":") == 0 && strlen(words[2]) != 0 ){

            if(strlen(username) == 0) {
                
                //printf("len username : %d \n",strlen(username));

                header command;
                command.from = FROM_CLIENT;
                command.numberOfBytes = strlen(words[2]);
                command.operation = OPERATION_LOGIN;

                if (write(fd_send,&command,SIZEOF_HEADER) == -1) 
                    perror("Problema la scriere in FIfFO!");
                if (write(fd_send,words[2],command.numberOfBytes) == -1) 
                    perror("Problema la scriere in FIfFO!");

                printf("Cererea a fost trimisa !\n");
                bzero(commandBuffer,sizeof(commandBuffer));
                // trebuie sa astept raspunsul 
                if (read(fd_receive, commandBuffer, SIZEOF_HEADER) == -1)
                    perror("Eroare la citirea din FIFO!\n");

                //printf("Am primit raspunsul !\n");

                header response = *((header*)commandBuffer);

                if(response.from == FROM_FATHER && response.operation == OPERATION_LOGIN){
                    if(response.respone_code == RESPONSE_SUCCESS){

                        printf("Acum ai 2 optiuni : \n *Pentru a introduce un cod apasa 1 \n *Pentru a trimite o notificare de cerere apasa 2 \n ");

                        char option[10];
                        char cod[3];
                        int success = 0;
                        read(0,option,5);
                        header command2;
                        command2.from = FROM_CLIENT;
                        command2.operation = OPERATION_LOGIN;

                        printf("optiunea aleasa : %s \n" ,option);

                        if(option[0] == '1'){
                            
                            read(0,cod,sizeof(cod));
    
                            command2.numberOfBytes = sizeof(cod);

                        }
                        else if(option[0] == '2'){
                            
                            command2.numberOfBytes = 0;
                        
                        }
                        else {
                            printf("Ai ales o optiune invalida \n");
                            return 0;
                        }

                        if (write(fd_send,&command2,SIZEOF_HEADER) == -1) 
                            perror("Problema la scriere in FIfFO!");
                        if (command2.numberOfBytes > 0){
                            if (write(fd_send,cod,command2.numberOfBytes) == -1) 
                                perror("Problema la scriere in FIfFO!");
                        }

                        if (read(fd_receive, &command2, SIZEOF_HEADER) == -1)
                            perror("Eroare la citirea din FIFO!\n");

                        success = command2.respone_code;

                        if(success == RESPONSE_SUCCESS || success == RESPONSE_ACCEPTED){

                            printf("Operatia de LOGIN a avut succes !!!!\n");
                            strcpy(username,words[2]);
                            username[command.numberOfBytes] = 0;
                        }
                        else printf("Operatia de LOGIN a esuat :((((( \n");
                        
                    }
                    else printf("Operatia de LOGIN a esuat :( \n");
                }
            }
            else printf("You are already logged !\n");
        }
    }
}

