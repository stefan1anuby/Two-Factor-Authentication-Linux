/* cliTCPIt.c - Exemplu de client TCP
   Trimite un numar la server; primeste de la server numarul incrementat.

   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"

#define TIME_TO_UPDATE 5

struct userInfo
{
	char userID[30];
	struct
	{
		char name[30];
		unsigned int code;
		unsigned int needsApproval;
	} apps[10];
	unsigned int appsCount;
};

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port = 2908;
char ip[]="127.0.0.1";

int main(int argc, char *argv[])
{
  int sd;                    // descriptorul de socket
  struct sockaddr_in server; // structura folosita pentru conectare
                             // mesajul trimis
  int nr = 0;
  char buf[SIZEOF_PACKET];
  char name[20];

  /* exista toate argumentele in linia de comanda? */
  /*
  if (argc != 3)
  {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }
  */

  /* stabilim portul */
  //port = atoi(argv[2]);

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(ip);
  /* portul de conectare */
  server.sin_port = htons(port);

  /* ne conectam la server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    return errno;
  }

  /* citirea mesajului */
  bzero(name,sizeof(name));
  printf("[client]Introduceti un nume: ");
  fflush(stdout);
  read(0, name, sizeof(name));

  int size_name = strlen(name);
	if(name[size_name - 1] == '\n') name[size_name - 1] = 0;

  // scanf("%d",&nr);
  printf("[client] Am citit %s \n", name);

  while(1){
      bzero(buf,MAX_SIZE_OF_CONTENT);

      Packet request;
      request.from = FROM_2FA_CLIENT;
      request.operation = OPERATION_GET_CODES;
      memcpy(request.userID,name,sizeof(name));

      /* trimiterea mesajului la server */
      if (write(sd, &request, MAX_SIZE_OF_CONTENT) <= 0)
      {
        perror("[client]Eroare la write() (Packet request) spre server.\n");
        return errno;
      }

      //printf("S a trimis requestul \n");
      /* citirea raspunsului dat de server
        (apel blocant pina cind serverul raspunde) */

      Packet response;
      if (read(sd, &response, SIZEOF_PACKET) < 0)
      {
        perror("[client]Eroare la read() de la server.\n");
        return errno;
      }
    
      /* afisam mesajul primit */
      //printf("[client]Mesajul primit este: %d %d \n",response.from,response.respone_code);
      //printf("COntentul primit este : %s \n",response.content);

      if(response.respone_code == RESPONSE_SUCCESS){

          struct userInfo userinfo = *((struct userInfo*)response.content);

          //printf("userinfo : %d , %s \n",userinfo.appsCount ,  userinfo.userID);

          for(int j = 0 ; j < userinfo.appsCount ; j++){
                
            printf("{ app : %s , code : %d ,approval : %d} ; ", userinfo.apps[j].name , userinfo.apps[j].code , userinfo.apps[j].needsApproval);
           
            if(userinfo.apps[j].needsApproval == RESPONSE_WAIT){
              printf("aplicatia %s a facut o cerere de aprobare , o accepti ? [Y/N] \n",userinfo.apps[j].name);
              
              char raspuns[10];
              int raspuns_cod = 0;
              fflush(stdout);
              read(0, raspuns, sizeof(raspuns));

              if(raspuns[0] == 'Y'){
                printf("Ai raspuns Da \n");
                raspuns_cod = OPERATION_APPROVE_REQ;
              }
              else if(raspuns[0] == 'N'){
                printf("Ai raspuns Nu \n");
                raspuns_cod = OPERATION_DECLINE_REQ;
              }
              else{
                printf("Raspunsul nu este suportat \n");
              }
              
              if(raspuns_cod != 0 ){
                
                Packet request;
                request.from = FROM_2FA_CLIENT;
                request.operation = raspuns_cod;
                memcpy(request.userID,name,sizeof(name));
                memcpy(request.appID,userinfo.apps[j].name,sizeof(userinfo.apps[j].name));

                if (write(sd, &request, MAX_SIZE_OF_CONTENT) <= 0)
                {
                  perror("[client]Eroare la write() (Packet request aprobare/respingere) spre server.\n");
                  return errno;
                }
                printf("S a trimis requestul \n");

                Packet response;
                if (read(sd, &response, SIZEOF_PACKET) < 0)
                {
                  perror("[client]Eroare la read() de la server.\n");
                  return errno;
                }
              }
            }

          } 
          printf("\n");
      }
      else printf("Nasol :( \n");

      sleep(TIME_TO_UPDATE);
  }

  /* inchidem conexiunea, am terminat */
  close(sd);
}
