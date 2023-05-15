/* servTCPConcTh2.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un numar de la clienti si intoarce clientilor numarul incrementat.
  Intoarce corect identificatorul din program al thread-ului.


   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
 #include <fcntl.h>
#include "protocol.h"

/* portul folosit */
#define PORT 2908

#define MAX_NUM_OF_THREADS 100
#define MAX_NUM_OF_USERS 100
#define MAX_NAME_LEN 100
#define TIME_TO_UPDATE 30

#define DEBUG_LV 100

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData
{
	int idThread; // id-ul thread-ului tinut in evidenta de acest program
	int cl;		  // descriptorul intors de accept
} thData;

static void *treatReq(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
static void *treatUpdate(void *);
void raspunde(void *);

// mutex-ul folosit pentru a accesa "baza de date" de catre threaduri multiple
pthread_mutex_t lock;

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
} users[MAX_NUM_OF_USERS];

struct ContextForAccessingDatabase{
    int operation;
    char userID[50];
    char appID[50];
    int code_to_verify;
};

int accessDatabase(struct ContextForAccessingDatabase context, struct userInfo *userinfo);
unsigned int usersCount;

int main()
{
	struct sockaddr_in server; // structura folosita de server
	struct sockaddr_in from;
	int sd; // descriptorul de socket
	int pid;
	pthread_t th[MAX_NUM_OF_THREADS]; // Identificatorii thread-urilor care se vor crea
	int i = 0;

	/* creez mutexul */
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init has failed\n");
		return 1;
	}

	/* initializez database ul*/
	initDatabase();

	/* creez thread ul de update de coduri*/
	thData *td;
	td = (struct thData *)malloc(sizeof(struct thData));
	td->idThread = i++;

	pthread_create(&th[i], NULL, &treatUpdate, td);	//is doar operatii de update/write din database aici

	/* crearea unui socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("[server]Eroare la socket().\n");
		return errno;
	}
	/* utilizarea optiunii SO_REUSEADDR */
	int on = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	/* pregatirea structurilor de date */
	bzero(&server, sizeof(server));
	bzero(&from, sizeof(from));

	/* umplem structura folosita de server */
	/* stabilirea familiei de socket-uri */
	server.sin_family = AF_INET;
	/* acceptam orice adresa */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	/* utilizam un port utilizator */
	server.sin_port = htons(PORT);

	/* atasam socketul */
	if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
	{
		perror("[server]Eroare la bind().\n");
		return errno;
	}

	/* punem serverul sa asculte daca vin clienti sa se conecteze */
	if (listen(sd, 2) == -1)
	{
		perror("[server]Eroare la listen().\n");
		return errno;
	}
	/* servim in mod concurent clientii...folosind thread-uri */
	while (1)
	{
		int client;
		thData *td; // parametru functia executata de thread
		int length = sizeof(from);

		if( DEBUG_LV > 20 ) printf("[server]Asteptam la portul %d...\n", PORT);
		fflush(stdout);

		// client= malloc(sizeof(int));
		/* acceptam un client (stare blocanta pina la realizarea conexiunii) */
		if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
		{
			perror("[server]Eroare la accept().\n");
			continue;
		}

		/* s-a realizat conexiunea, se astepta mesajul */

		// int idThread; //id-ul threadului
		// int cl; //descriptorul intors de accept

		td = (struct thData *)malloc(sizeof(struct thData));
		td->idThread = i++;
		td->cl = client;

		//is doar operatii de read din database aici
		pthread_create(&th[i], NULL, &treatReq, td);

	} // while
};
static void *treatReq(void *arg)
{
	struct thData tdL;
	tdL = *((struct thData *)arg);
	if( DEBUG_LV > 20 ) printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
	fflush(stdout);
	pthread_detach(pthread_self());
	raspunde((struct thData *)arg);
	/* am terminat cu acest client, inchidem conexiunea */
	close((intptr_t)arg);
	return (NULL);
};

static void *treatUpdate(void *arg)
{
	struct thData tdL;
	tdL = *((struct thData *)arg);
	if( DEBUG_LV > 20 ) printf("[thread]- %d - Thread u de update s a spawnat...\n", tdL.idThread);
	fflush(stdout);
	pthread_detach(pthread_self());

	struct userInfo ui;
	struct ContextForAccessingDatabase context;
	context.operation = OPERATION_UPDATE;

	while(1){
		accessDatabase(context,NULL);
		if( DEBUG_LV > 20 ) printf("[thread]- %d - Updated !!!\n", tdL.idThread);
		sleep(TIME_TO_UPDATE);
	}


	//close((intptr_t)arg);
	return (NULL);
};

void raspunde(void *arg)
{
	int i = 0 , op_response;
	struct thData tdL;
	Packet request,response;

	tdL = *((struct thData *)arg);

	while(1){
		op_response = RESPONSE_SUCCESS;
		bzero(&request,SIZEOF_PACKET);

		if (read(tdL.cl, &request, SIZEOF_PACKET) <= 0)
		{
			if( DEBUG_LV > 20 ) printf("[Thread %d]\n", tdL.idThread);
			perror("Eroare la read() de la client. (header) \n");
			return errno;
		}

		if( DEBUG_LV > 20 ) printf("[Thread %d]Mesajul a fost receptionat... \n", tdL.idThread);
		

		if(request.from == FROM_SOME_SERVER && request.operation == OPERATION_CHECK_APPROVAL){
			
			struct ContextForAccessingDatabase context;
			context.operation = OPERATION_CHECK_APPROVAL;
			memcpy(context.userID,request.userID,sizeof(request.userID));
			memcpy(context.appID,request.appID,sizeof(request.appID));

			const int TIME_TO_WAIT = 60;
			for(int i = 0 ; i < TIME_TO_WAIT ; i++){
				op_response = accessDatabase(context, NULL);
				if(op_response == RESPONSE_WAIT){
					sleep(1);
				}
				else break;
			}

		}
		else if(request.from == FROM_SOME_SERVER && request.operation == OPERATION_CHECK_CODE_REQ){
			
			struct ContextForAccessingDatabase context;
			context.operation = OPERATION_CHECK_CODE_REQ;
			memcpy(context.userID,request.userID,sizeof(request.userID));
			memcpy(context.appID,request.appID,sizeof(request.appID));
			context.code_to_verify = atoi(request.content);

			op_response = accessDatabase(context, NULL);
			
		}
		else if(request.from == FROM_2FA_CLIENT && request.operation == OPERATION_GET_CODES){
			struct userInfo uinfo;
			struct ContextForAccessingDatabase context;
			context.operation = OPERATION_GET_CODES;
			memcpy(context.userID,request.userID,sizeof(request.userID));
			

			op_response = accessDatabase(context, &uinfo);

			memcpy(response.content,(&uinfo),sizeof(uinfo));
		}
		else if(request.from == FROM_2FA_CLIENT && request.operation == OPERATION_APPROVE_REQ){

			struct ContextForAccessingDatabase context;
			context.operation = OPERATION_APPROVE_REQ;
			memcpy(context.userID,request.userID,sizeof(request.userID));
			memcpy(context.appID,request.appID,sizeof(request.appID));

			op_response = accessDatabase(context, NULL);
		}
		else if(request.from == FROM_2FA_CLIENT && request.operation == OPERATION_DECLINE_REQ){
			struct ContextForAccessingDatabase context;
			context.operation = OPERATION_DECLINE_REQ;
			memcpy(context.userID,request.userID,sizeof(request.userID));
			memcpy(context.appID,request.appID,sizeof(request.appID));

			op_response = accessDatabase(context, NULL);
		}
		
		/*pregatim mesajul de raspuns */

		if( DEBUG_LV > 20 ) printf("[Thread %d]Trimitem mesajul inapoi... \n", tdL.idThread);

		/* returnam mesajul clientului */
		response.from = FROM_2FA_SERVER;
		response.operation = request.operation;
		//response.respone_code = op_response ? RESPONSE_SUCCESS : RESPONSE_ERROR;
		response.respone_code = op_response;
		//memcpy(response.content,(&uinfo),sizeof(uinfo));
		
		if (write(tdL.cl, &response, SIZEOF_PACKET) <= 0)
		{
			if( DEBUG_LV > 20 ) printf("[Thread %d] ", tdL.idThread);
			perror("[Thread]Eroare la write() catre client.\n");
		}
		else
			if( DEBUG_LV > 20 ) printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
		
	}
}

void initDatabase()
{
	//---- set the users with dummy data -----
	
	/*
	usersCount = 1;
	strcpy(users[0].userID,"stepopa");
	users[0].appsCount = 3;
	strcpy(users[0].apps[0].name,"discord");
	strcpy(users[0].apps[1].name,"emag");
	strcpy(users[0].apps[2].name,"slack");
	users[0].apps[0].needsApproval = 0;
	users[0].apps[1].needsApproval = 0;
	users[0].apps[2].needsApproval = 0; 
	*/

	//---- END ---------------------------------

	char path[] = "database.txt";
	char contentBuffer[MAX_SIZE_OF_CONTENT];

	int taskSuccess = 1;

	int fd = open(path,O_RDONLY);

	int bytesRead = read(fd,contentBuffer,MAX_SIZE_OF_CONTENT);

	close(fd);

	if(bytesRead < 0) {
		perror("eroare la citire din fisier \n");
		taskSuccess = 0;
	}
	else{
		contentBuffer[bytesRead] = 0;

		char* words[100];
		int tokensCounter = 0;

		words[tokensCounter] = strtok(contentBuffer," ");

		while( words[tokensCounter] != NULL){
			words[++tokensCounter] = strtok(NULL," ");
		}

		int found = 0 , user = 0 , app = 0 ,i = -1, j= 0;

		for(int k = 0 ; k < tokensCounter ; k++){
			if(strcmp("USER",words[k]) == 0) {
				user = k;
				j = 0;
			}
			else if(strcmp("APP",words[k]) == 0) {
				app = k;
			}
			else if(strcmp(";",words[k]) == 0) {
				usersCount = i + 1;

				break;
			}
			else if(user == k - 1){
				i++;
				strcpy(users[i].userID,words[k]);
			}
			else if(app == k - 1){
				strcpy(users[i].apps[j].name,words[k]);
				j++;
				users[i].appsCount = j;

			}
			else{
				printf("CEVA CIUDAT LA INCARCAREA BAZEI DE DATE \n");
				printf("aici : %s , %d \n",words[k],k);
				return;
			}
		}

	}


}

void Update(){

	unsigned int RANGE = 1e3 - 1e2;
	unsigned int num;

	srand(time(NULL));

	for (int i = 0 ; i < usersCount ; i++){
		for(int j = 0 ; j < users[i].appsCount ; j++){

			num = rand() % RANGE;

			users[i].apps[j].code = num + 1e2;

			/*
			TODO:

			if users[i].apps[j].aproval == YES/NO si timpul de cand a fost trimis raspunsul < SOME_TIME => users[i].apps[j].aproval = 0 
			*/
		}
	}
	//if( DEBUG_LV > 20 ) printf("DEBUG update !!\n");
	
	/* log */
	if(DEBUG_LV > 10){
		for(int i = 0 ; i < usersCount ; i++){

			printf("User : %s [ ",users[i].userID);

			for(int j = 0 ; j < users[i].appsCount ; j++){
				
				printf("{ app : %s , code : %d , approval : %d } ; ", users[i].apps[j].name , users[i].apps[j].code ,users[i].apps[j].needsApproval );

			}

			printf(" ] \n");
		}
	}
}

struct userInfo Search(struct ContextForAccessingDatabase context, int *result){
	//iterate over users vector
	//return userInfo of that userID

	for(int i = 0 ; i < usersCount ; i++){

		/* logging */
		if( DEBUG_LV > 20 ) printf("userID : %d , users[i] : %d \n",strlen(context.userID), strlen(users[i].userID));
		if( DEBUG_LV > 20 ) printf("userID : %s , users[i] : %s \n",context.userID, users[i].userID);


		if(strncmp(users[i].userID,context.userID,sizeof(users[i].userID)) == 0){
			

			if(context.operation == OPERATION_GET_CODES ){

				*result = RESPONSE_SUCCESS;
				return users[i];
			}
			else if(context.operation == OPERATION_APPROVE_REQ || context.operation == OPERATION_DECLINE_REQ || context.operation == OPERATION_CHECK_CODE_REQ || context.operation == OPERATION_CHECK_APPROVAL){
				
				for(int j = 0 ; j < users[i].appsCount ; j++){
					
					if(strncmp(users[i].apps[j].name,context.appID,sizeof(users[i].apps[j].name)) == 0){
						
					
						/// right user and right application
						if(context.operation == OPERATION_CHECK_CODE_REQ){
							if(context.code_to_verify == users[i].apps[j].code){
								
								*result = RESPONSE_SUCCESS;
								return;
							}
							else {
								printf("Codul a fost gresit !! \n");
								*result = RESPONSE_ERROR;
								return;

							}
						}
						else if(context.operation == OPERATION_CHECK_APPROVAL){
							if(users[i].apps[j].needsApproval == 0 )
								users[i].apps[j].needsApproval = RESPONSE_WAIT; 
							
							*result = users[i].apps[j].needsApproval;
							/* restore to default value */
							if(*result == RESPONSE_ACCEPTED || *result == RESPONSE_DECLINED)
								users[i].apps[j].needsApproval = 0;
							return;
						}
						else if(users[i].apps[j].needsApproval == RESPONSE_WAIT){
							//cred ca ar trb resooonse approved/declined
							*result = RESPONSE_SUCCESS;
							users[i].apps[j].needsApproval = ( context.operation == OPERATION_APPROVE_REQ ) ? RESPONSE_ACCEPTED : RESPONSE_DECLINED;
							return;
						}
					}

				}
			}
		}
	}
	*result = RESPONSE_ERROR;
	printf("!!!!! NU A GASIT USER ul !!! \n");
}

int accessDatabase(struct ContextForAccessingDatabase context, struct userInfo *userinfo){

	pthread_mutex_lock(&lock);
	if( DEBUG_LV > 20 ) printf("Locked mutex\n");

	int result = 1;

	if(context.operation == OPERATION_GET_CODES){

		*userinfo = Search(context,&result);
	}
	else if(context.operation == OPERATION_APPROVE_REQ || context.operation == OPERATION_DECLINE_REQ){

		Search(context,&result);
	}
	else if(context.operation == OPERATION_CHECK_CODE_REQ){
		Search(context,&result);
	}
	else if(context.operation == OPERATION_CHECK_APPROVAL){
		Search(context,&result);
	}
	else if(context.operation == OPERATION_UPDATE){
		Update();
	}
	else{
		result = RESPONSE_ERROR;
		printf("!!!NU EXISTA ACEST REQUEST");
	}

	if(result == RESPONSE_ERROR) {
		printf("!!!EROARE LA ACEST REQUEST!!! \n");
	}

	pthread_mutex_unlock(&lock);
	if( DEBUG_LV > 20 ) printf("Unlocked mutex\n");

	return result;
}
