// tcp_server.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
//#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>      /* Needed only for _O_RDWR definition */  
#include <io.h>  
#include <stdio.h>  
#include <share.h>  

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define BUFLEN_READ_DISK 1024*1024*12 //IDEAL SIZE OF 40Mo on LabVIEW
#define BUFLEN_SEND 65536//*100
#define DEFAULT_PORT 1153
#define MAX_FIBER_NB 4

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult = 0;
	long long nbdatatotal = 0, nbdata = 0;

	struct sockaddr_in server;

	SOCKET MasterListenSocket = INVALID_SOCKET;
	SOCKET MasterClientSocket = INVALID_SOCKET;
	SOCKET ListenSockets[MAX_FIBER_NB];// INVALID_SOCKET;
	SOCKET ClientSockets[MAX_FIBER_NB]; // INVALID_SOCKET;
	USHORT port = DEFAULT_PORT;

	int i = 0;
	for (i = 0; i < MAX_FIBER_NB; i++){
		ListenSockets[i] = INVALID_SOCKET;
		ClientSockets[i] = INVALID_SOCKET;
	}

//	int iSendResult;
	int count = 0;
	//char recvbuf[BUFLEN_SEND];

	time_t difftime = 0, previoustime = 0, starttime = 0, totaltime = 0, before = 0, endtime = 0;
	int useless = 0;


	struct transfer_info{
		int nb_sockets;
		int chunk_size;
		long long sizeof_file;
		long long file_block_size_read;
		int record;
		char file_path[1024];
	};
	struct transfer_info client_parameters;
	struct transfer_info server_parameters;
	memset(&client_parameters, 0, sizeof(client_parameters));
	memset(&server_parameters, 0, sizeof(server_parameters));

	/*struct server_info{
		int nb_sockets;
		int chunk_size;
		char file_path[1024];
	};
	struct server_info server_parameters;
	memset(&server_parameters, 0, sizeof(server_parameters));*/


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	// Create a SOCKET for connecting to server
	MasterListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (MasterListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	// Setup the TCP listening socket
	iResult = bind(MasterListenSocket, (struct sockaddr *) &server, sizeof(server));
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(MasterListenSocket);
		WSACleanup();
		return 1;
	}
	iResult = listen(MasterListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(MasterListenSocket);
		WSACleanup();
		return 1;
	}
	// Accept a client socket
	MasterClientSocket = accept(MasterListenSocket, NULL, NULL);
	if (MasterClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(MasterClientSocket);
		WSACleanup();
		return 1;
	}
	printf("Client master connected ...client sending init data\n");
	
	// No longer need server socket
	closesocket(MasterListenSocket);
	// No longer need master connection
	//closesocket(MasterClientSocket);

	//get clients parameters
	iResult = recv(MasterClientSocket, (char*)&client_parameters, sizeof(client_parameters), 0);

	memcpy(&server_parameters, &client_parameters, sizeof(struct transfer_info));

	printf("iResult from client : %d\n", iResult);
	printf("file_path_asked:%s strlen%d\n", client_parameters.file_path, strlen(client_parameters.file_path));
	printf("nb_clients_to_connect : %d\n", client_parameters.nb_sockets);
	printf("Waiting for %d clients...\n", client_parameters.nb_sockets);

	int fh = 0;
	if (_sopen_s(&fh, client_parameters.file_path, _O_BINARY, _SH_DENYWR, 0))
	{
		perror("error openning file0\n");
		exit(1);
	}
	/* Set the end of the file: */
	long long lengthOfFile = _lseeki64(fh, 0L, SEEK_END);
	if (lengthOfFile == -1L)
		perror("_lseeki64 to end failed");
	else
		printf("Position for end of file seek = %lld\n", lengthOfFile);
	long long tmp;
	/* Seek the beginning of the file: */
	tmp = _lseeki64(fh, 0L, SEEK_SET);
	if (tmp == -1L)
		perror("_lseek to beginning failed");
	else
		printf("Position for beginning of file seek = %ld\n", tmp);

	printf("length of file (octets) %llu\n", lengthOfFile);
	server_parameters.sizeof_file = lengthOfFile;
	server_parameters.file_block_size_read = BUFLEN_READ_DISK;
	iResult = send(MasterClientSocket, (char*)&server_parameters, sizeof(server_parameters), 0);
	printf("I have answered to client master channel !\n");

	//setvbuf((FILE*)fh, NULL, _IONBF, 0); //disable buffering on disk
	
	//Establishing connection with N clients

	int cnt_clients_sockets = 0;
	for (i=0; i < client_parameters.nb_sockets; i++)
	{
		// Create a SOCKET for connecting to server
		ListenSockets[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (ListenSockets[i] == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
		port++;
		server.sin_port = htons(port);
		printf("port num : %d ", port);
		// Setup the TCP listening socket
		iResult = bind(ListenSockets[i], (struct sockaddr *) &server, sizeof(server));
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSockets[i]);
			WSACleanup();
			return 1;
		}
		iResult = listen(ListenSockets[i], SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSockets[i]);
			WSACleanup();
			return 1;
		}
		// Accept a client socket
		ClientSockets[i] = accept(ListenSockets[i], NULL, NULL);
		if (ClientSockets[i] == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSockets[i]);
			WSACleanup();
			return 1;
		}
		cnt_clients_sockets++;
		printf("client num : %d\n", cnt_clients_sockets);
	}

	printf("BUFLEN_SEND %d \n", BUFLEN_SEND);
	printf("BUFLEN_READ_DISK %d \n", BUFLEN_READ_DISK);

	long long nb_blocks_read_from_file[MAX_FIBER_NB];
	unsigned int nb_octets_per_read_disk[MAX_FIBER_NB];
	int nb_octets_per_socket = BUFLEN_SEND;
	//char file_buf[MAX_FIBER_NB];

	for (i = 0; i < cnt_clients_sockets; i++)
		nb_octets_per_read_disk[i] = BUFLEN_READ_DISK/cnt_clients_sockets;

	char **file_buf = {NULL};
	if ((file_buf = malloc(MAX_FIBER_NB*sizeof(char*))) == NULL)
	{ /* error */
		printf("error malloc");
		return -1;
	}
	for (i = 0; i < cnt_clients_sockets; i++)
	{
		if ((file_buf[i] = (char*) malloc(nb_octets_per_read_disk[i])) == NULL)
		{ /* error */
			printf("error malloc");
			return -1;
		}
	}

	difftime = before = starttime = time(NULL);
	//printf("sizeof filebuf %d cnt_clients_sockets %d \n", sizeof(file_buf), cnt_clients_sockets);
	// Receive until the file is read
	//printf("nb_octets_per_block %d \n", nb_octets_per_block);
	printf("nb_octets_per_read_disk %lu \n", nb_octets_per_read_disk[0]);
	long long size_to_send[MAX_FIBER_NB];
	while (nbdatatotal < lengthOfFile && cnt_clients_sockets > 0)
	{
		difftime = time(NULL) - before;
		//send data depending on nb sockets opened
		for (i = 0; i < cnt_clients_sockets ; i++)
		{
			if (fh != 0) nb_blocks_read_from_file[i] = _read(fh, file_buf[i], nb_octets_per_read_disk[i]);
			size_to_send[i] = nb_blocks_read_from_file[i];
			//size_to_send[i] = nb_octets_per_read_disk[i]; // for no read file purpose bench
			//printf("size_to_send[client %d] %lli\n", i, size_to_send[i]);
			
			
			nbdatatotal += size_to_send[i];
			nbdata += size_to_send[i];

			iResult = -1;
			nb_octets_per_socket = BUFLEN_SEND;
			while (size_to_send[i] >= 0 && iResult != 0)
			{	// for each socket
				if (size_to_send[i] <= BUFLEN_SEND)
					nb_octets_per_socket = size_to_send[i];
				iResult = send(ClientSockets[i], (const char *) file_buf[i], nb_octets_per_socket, 0);
				if (iResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(ClientSockets[i]);
					WSACleanup();
					return 1;
				}
				//printf("size_to_send[client %d] %lli size_sent %d\n", i, size_to_send[i], iResult);
				size_to_send[i] -= iResult;
				//printf(" nbdatatotal %llu nboctets_per_block %d \n", nbdatatotal, nb_octets_per_block);
				nbdatatotal += iResult;
				nbdata += iResult;
			} 
		}
		if (difftime >= 1)
		{
			wprintf(L"throughput %llf o/s data sent %llf o \n", nbdata / (1024 * 1024.0 * difftime), nbdatatotal / (1024 * 1024.0 * difftime));
			before = time(NULL);
			nbdata = 0;
		}
	} 
	difftime = time(NULL) - starttime;
	wprintf(L"final throughput %llf o/s data sent %llf o \n", nbdatatotal / (difftime * 1024 * 1024.0), nbdatatotal / (1024 * 1024.0));


	if (_close(fh) != 0)
	{
		printf("error closing file\n");
	}

	if (file_buf != NULL && (sizeof(file_buf) != 4 || sizeof(file_buf) != 8))
		free(file_buf);
	
	printf("data sent : %llu\n", nbdatatotal);

	for (i = 0; i < cnt_clients_sockets; i++)
	{
		// send 0 length msg to client to close the connection
		iResult = shutdown(ClientSockets[i], SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSockets[i]);
			WSACleanup();
			return 1;
		}
		// cleanup
		closesocket(ClientSockets[i]);
	}

	// send 0 length msg to client to close the connection
	iResult = shutdown(MasterClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(MasterClientSocket);
		WSACleanup();
		return 1;
	}
	printf("sent 0 length msg to close connection...\n");
	
	// cleanup
	closesocket(MasterClientSocket);
	
	WSACleanup();
	printf("connection closed \n");

	return 0;
}
