/*
    C ECHO client example using sockets
*/
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<stdlib.h> //exit(0);
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<time.h>
#include<unistd.h>    //write
 
//#define SERVER_ADDRESS "127.0.0.1"
//#define PORT_NUMBER 1153
#define BUFLEN 1024*1024*2

int main(int argc , char *argv[])
{
    int sockmaster=-1;
	int sockclients[4];
	int i=0;
    struct sockaddr_in server;
    int iResult=-1;
    //char message[BUFLEN]="";
    time_t difftime=0, before=0;
    unsigned long long nbdata=0, nbdatatotal=0;
    char SERVER_ADDRESS[16][16]={"127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1"};
    int PORT_NUMBER=1153;
    char* message = (char *) malloc(BUFLEN);
    if (message ==NULL){
		perror("malloc issue");
		exit(1);
	}
	
    //FILE *fh = fopen ("RT_test0.bin", "wb");
	FILE *fh = NULL;

	struct transfer_info{
		int nb_sockets;
		int chunk_size;
		long long sizeof_file;
		int record;
		char file_path[1024];
	};
	struct transfer_info client_parameters;
	struct transfer_info server_parameters;
	memset(&client_parameters, 0, sizeof(client_parameters));
	memset(&server_parameters, 0, sizeof(server_parameters));
	
    //parse args
    if(argc != 3)
    {
		printf("%s usage: '%s + FILE_PATH + nb sockets'\n", argv[0], argv[0]);
		fflush(stdout);
		return 1;
    }
    /*for(i=0;i<4;i++)
    {
		printf("toto");
		strcpy(SERVER_ADDRESS[i], argv[i+2]);
	}*/
	//strcpy(FILE_PATH, argv[1]);
	strcpy(client_parameters.file_path,argv[1]);	
	client_parameters.nb_sockets=atoi(argv[2]);
		
	/*int record = 0;
	if (record !=0) fh = fopen ("RT_test0.bin", "wb");*/
	
	if(PORT_NUMBER<1 || PORT_NUMBER>65535)
    {
		printf("bad port number %d\n",PORT_NUMBER);
		fflush(stdout);
		return 1;
	}
	printf("port number : %d\n",PORT_NUMBER);
	
    //Create socket
    sockmaster = socket(AF_INET , SOCK_STREAM , 0);
    if (sockmaster == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS[0]);
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT_NUMBER );
 
    //Connect to remote server
    if (connect(sockmaster , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
    puts("Connected .. will send init msg\n");
		
	iResult = send(sockmaster , (char*) &client_parameters , sizeof(client_parameters), 0);
	printf("iResult %d\n",iResult);
    puts("Sended init msg\n");	
	iResult = recv(sockmaster , (char*) &server_parameters , sizeof(server_parameters), 0);
	printf("iResult %d\n",iResult);
	puts("response from master received !\n");
	
	int cnt_clients_sockets=0;
	unsigned short port=PORT_NUMBER;
	printf("for loop : clients trying to connect to server\n");
	for(i=0; i < server_parameters.nb_sockets; i++)
	{
		sleep(1);
		//Create socket
		sockclients[i] = socket(AF_INET , SOCK_STREAM , 0);
		if (sockclients[i] == -1)
		{
			printf("Could not create socket nb %d ", i);
			break;
		}
		printf("Socket number %d created ", i);
		fflush(stdout);
		port++;	
		printf("add num %d : %s",i,SERVER_ADDRESS[i]);
		server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS[i]);
		server.sin_port = htons( port);
		fflush(stdout);
		//Connect to remote server
		if (connect(sockclients[i] , (struct sockaddr *)&server , sizeof(server)) < 0)
		{
			printf("connect failed for port num %d\n",port);
			fflush(stdout);
			perror("connect failed. Error");
			break;
		}
		printf("Connected via port %d \n", port);
		fflush(stdout);
		cnt_clients_sockets++;
	}
	printf("%d scokets opened\n", cnt_clients_sockets);
    before = time(NULL);
    //keep communicating with server
	//for(cnt_clients_sockets=0; cnt_clients_sockets < server_parameters.nb_sockets; cnt_clients_sockets++)
	//{
	int size_to_retrieve = 0;
	//int size_to_retrieve_per_ite = 0;
	printf("sizeoffile : %lli\n",server_parameters.sizeof_file);
	fflush(stdout);
	while( nbdatatotal < server_parameters.sizeof_file && cnt_clients_sockets > 0)//(iResult = recv(sockclients[cnt_clients_sockets] , message , BUFLEN , 0)) > 0 )
	{
		difftime = time(NULL)-before;
		for (i=0; i < cnt_clients_sockets ;i++)
		{
			//printf("msg received for client [%d] \n", i);
			fflush(stdout);
			iResult = -1;
			size_to_retrieve = BUFLEN;
			//size_to_retrieve_per_ite = BUFLEN;
			//pour chaque socket
			//while( (iResult = recv(sockclients[i] , message , BUFLEN , 0)) >0)
			while ( size_to_retrieve > 0 && iResult !=0)
			{
				//if (size_to_retrieve <= BUFLEN)
					//size_to_retrieve_per_ite = size_to_retrieve;
				iResult = recv(sockclients[i] , message , size_to_retrieve, 0);
				if(iResult < 0)	{puts("Recv failed (iResult<0");return 1;}
				size_to_retrieve-= iResult;
				nbdata+=iResult;
				nbdatatotal+=iResult;
				if (fh != NULL)	fwrite (&message, iResult, 1, fh);
			}
		}
		if(difftime == 1 /*(clock_t) CLOCKS_PER_SEC/10*/)
		{
			printf("throughput %lf Mo/s  \n", nbdata/(1024*1024.0) );
			printf("%llu octets \n", nbdatatotal );
			before=time(NULL);
			nbdata=0;
		}
	}
	
	puts("done\n");
	printf("Bytes received: %llu\n", nbdatatotal);	
	fflush(stdout);
	
    if (message != NULL){
		free(message);
	}
	
	if (fh!=NULL){ fclose (fh); puts("file closed");}
	
	for(i=0; i < cnt_clients_sockets; i++)
	{
		iResult = shutdown(sockclients[i] , SHUT_RDWR);
		close(sockclients[i]);
	}		
	iResult = shutdown(sockmaster , SHUT_RDWR);     
    close(sockmaster);
    
    return 0;
}
