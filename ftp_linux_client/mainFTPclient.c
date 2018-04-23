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
#define BUFLEN 65536

int main(int argc , char *argv[])
{
    int sockmaster=-1;
	int sockclients[4];
	int i=0;
    struct sockaddr_in server;
    int iResult=-1;
    char message[BUFLEN]="";
    time_t difftime=0, before=0;
    unsigned long long nbdata=0, nbdatatotal=0;
    char* SERVER_ADDRESS=NULL;
    int PORT_NUMBER=0;
	
	char* FILE_PATH=NULL;

	struct transfer_info{
		int nb_sockets;
		int chunk_size;
		int sizeof_file;
		char file_path[1024];
	};
	struct transfer_info client_parameters;
	struct transfer_info server_parameters;
	memset(&client_parameters, 0, sizeof(client_parameters));
	memset(&server_parameters, 0, sizeof(server_parameters));
	
    //parse args
    if(argc != 5)
    {
                printf("%s usage: '%s + Server IP + port + FILE_PATH + nb sockets'\n", argv[0], argv[0]);
                fflush(stdout);
                return 1;
    }
	SERVER_ADDRESS=argv[1];
	PORT_NUMBER=atoi(argv[2]);
	FILE_PATH = argv[3];
	client_parameters.nb_sockets=atoi(argv[4]);
		
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
     
    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT_NUMBER );
 
    //Connect to remote server
    if (connect(sockmaster , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
    puts("Connected .. will send init msg\n");
	
	sprintf(client_parameters.file_path,FILE_PATH);	
	iResult = send(sockmaster , (char*) &client_parameters , sizeof(client_parameters), 0);
    puts("Sended init msg\n");	
	iResult = recv(sockmaster , (char*) &server_parameters , sizeof(server_parameters), 0);
	puts("response from master received !\n");
	
	int cnt_clients_sockets=0;
	int port=0;
	sleep(1);
	for(cnt_clients_sockets; cnt_clients_sockets < (server_parameters.nb_sockets-1); cnt_clients_sockets++)
	{
		printf("for loop %d cnt_client_sockets\n",cnt_clients_sockets);
		//Create socket
		sockclients[cnt_clients_sockets] = socket(AF_INET , SOCK_STREAM , 0);
		if (sockclients[cnt_clients_sockets] == -1)
		{
			printf("Could not create socket nb %d ", cnt_clients_sockets);
		}
		printf("Socket number %d created\n", cnt_clients_sockets);
		fflush(stdout);
		 
		//server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
		//server.sin_family = AF_INET;
		port = PORT_NUMBER + cnt_clients_sockets + 1;
		printf("will create port number  %d \n", port);
		server.sin_port = htons( port);
		fflush(stdout);
		//Connect to remote server
		if (connect(sockclients[cnt_clients_sockets] , (struct sockaddr *)&server , sizeof(server)) < 0)
		{
			perror("connect failed. Error");
			return 1;
		}
	}
	printf("caca2\n");
	
    before = time(NULL); 
    //keep communicating with server
	i=0;
	while( (iResult = recv(sockmaster , message , BUFLEN , 0)) > 0)
	{
		i++;
		difftime = time(NULL)-before;
		//Send some data
		if(iResult < 0)
		{
			puts("Recv failed (iResult<0");
			return 1;
		}
		nbdata+=iResult;
		nbdatatotal+=iResult;
		if(difftime == 1 /*(clock_t) CLOCKS_PER_SEC/10*/)
		{
			printf("throughput %lf Mo/s datas received %lf Mo \n", nbdata/(1024*1024.0), nbdatatotal/(1024*1024.0) );
			before=time(NULL);
			nbdata=0;
		}
		printf("message numero%d : '%s' taille recue %d\n",i, message, strlen(message));
	} while(iResult > 0);
	
		
	iResult = shutdown(sockmaster , SHUT_RDWR);
	//iResult = send(sockmaster , "nexxt" , BUFLEN , 0);
	puts("done\n");
	
    printf("Bytes received: %llu\n", nbdatatotal);	
     
    close(sockmaster);
    return 0;
}