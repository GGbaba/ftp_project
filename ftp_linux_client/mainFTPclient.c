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
    int sock=-1;
    struct sockaddr_in server;
    int iResult=-1;
    char message[BUFLEN]="";
    time_t difftime=0, before=0;
    unsigned long long nbdata=0, nbdatatotal=0;
    char* SERVER_ADDRESS=NULL;
    int PORT_NUMBER=0;

	struct client_info{
		int nb_sockets;
		int chunk_size;
		char file_path[1024];
	};
	struct client_info client_parameters;
	memset(&client_parameters, 0, sizeof(client_parameters));
	
    //parse args
    if(argc != 3)
    {
                printf("%s usage: '%s + Server IP + port'\n", argv[0], argv[0]);
                fflush(stdout);
                return 1;
        }
        SERVER_ADDRESS=argv[1];
        PORT_NUMBER=atoi(argv[2]);
	if(PORT_NUMBER<1 || PORT_NUMBER>65535)
    {
		printf("bad port number %d\n",PORT_NUMBER);
		fflush(stdout);
		return 1;
	}
	
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT_NUMBER );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
    puts("Connected\n");
	
	iResult = send(sock , (char*) &client_parameters , sizeof(client_parameters), 0);
	
    puts("Sended init msg\n");
    before = time(NULL); 
    //keep communicating with server
	do
	{
		difftime = time(NULL)-before;
		//Send some data
		iResult = recv(sock , message , BUFLEN , 0);
		if(iResult < 0)
		{
			puts("Send failed");
			return 1;
		}
		nbdata+=iResult;
		nbdatatotal+=iResult;
		if(difftime == 1 /*(clock_t) CLOCKS_PER_SEC/10*/)
		{
			printf("throughput %lf Mo/s datas sent %lf Mo \n", nbdata/(1024*1024.0), nbdatatotal/(1024*1024.0) );
			before=time(NULL);
			nbdata=0;
		}
	} while(iResult > 0);
		
	iResult = shutdown(sock , SHUT_RDWR);
	//iResult = send(sock , "nexxt" , BUFLEN , 0);
	puts("done\n");
	
    printf("Bytes received: %llu\n", nbdatatotal);	
     
    close(sock);
    return 0;
}