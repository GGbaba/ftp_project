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
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define BUFLEN_READ_CACHE_SSD 1024*1024
#define BUFLEN_SEND 65536
#define DEFAULT_PORT 1153
#define MAX_FIBER_NB 8

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult = 0;
	unsigned long long nbdatatotal = 0, nbdata = 0;

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
		long sizeof_file;
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
	
	iResult = recv(MasterClientSocket, (char*)&client_parameters, sizeof(client_parameters), 0);

	memcpy(&server_parameters, &client_parameters, sizeof(struct transfer_info));

	printf("file_path_asked:%s\n", client_parameters.file_path);
	printf("nb_clients_to_connect : %d\n", client_parameters.nb_sockets);
	printf("Waiting for %d clients...\n", client_parameters.nb_sockets);


	FILE* fh = NULL;
	if (fopen_s(&fh, client_parameters.file_path, "rb") != 0)
	{
		perror("error openning file\n");
		return -1;
	}
	/*FILE* fh2;
	if (fopen_s(&fh2, "D:\DATA\toto.bin", "rb") != 0)
	{
		perror("error openning file2\n");
		return -1;
	}*/

	char file_buf[BUFLEN_SEND*MAX_FIBER_NB];
	int nb_octets_per_socket = BUFLEN_SEND;
	int nb_octets_per_block = nb_octets_per_socket*client_parameters.nb_sockets;
	int nb_blocks_to_read = 1;
	int nb_blocks_read_from_file = 0;

	fseek(fh, 0, SEEK_END);
	long lengthOfFile = ftell(fh);
	fseek(fh, 0, SEEK_SET);
	printf("length of file (octets) %d\n", lengthOfFile);

	server_parameters.sizeof_file = lengthOfFile;

	iResult = send(MasterClientSocket, (char*)&server_parameters, sizeof(server_parameters), 0);

	printf("I have answered to client master channel !\n");

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
			closesocket(ClientSockets[cnt_clients_sockets]);
			WSACleanup();
			return 1;
		}
		cnt_clients_sockets++;
		printf("client num : %d\n", cnt_clients_sockets);
	}

	difftime = before = starttime = time(NULL);
	count = 0;
	printf("sizeof filebuf %d cnt_clients_sockets %d \n", sizeof(file_buf), cnt_clients_sockets);
	// Receive until the file is read

	//printf("nb_octets_per_block %d \n", nb_octets_per_block);
	//printf("nb_octets_per_socket %d \n", nb_octets_per_socket);
	while (nbdatatotal<lengthOfFile && nb_octets_per_socket != 0 && cnt_clients_sockets > 0)
	{
		difftime = time(NULL) - before;
		if ((lengthOfFile - nbdatatotal) <= nb_octets_per_block)
		{
			nb_octets_per_block = lengthOfFile - nbdatatotal;
			nb_octets_per_socket = (lengthOfFile - nbdatatotal) / cnt_clients_sockets;
			if (nb_octets_per_socket <= 0) nb_octets_per_socket = 1;
		}
		//read file per N blocks
		if (fh != NULL) nb_blocks_read_from_file = fread(file_buf, nb_octets_per_socket*cnt_clients_sockets, nb_blocks_to_read, fh);
		//if (fh2!= NULL) fwrite(file_buf, nb_octets_per_socket*cnt_clients_sockets, nb_blocks_to_read, fh2);
		//send data depending on nb sockets opened
		for (i = 0; i < cnt_clients_sockets ; i++)
		{
			//printf("client %d\n", i);	//printf("iResult %d\n", iResult);
			iResult = send(ClientSockets[i], (const char *)&file_buf[i*nb_octets_per_socket], nb_octets_per_socket, 0);
			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSockets[i]);
				WSACleanup();
				return 1;
			}		
			//printf(" nbdatatotal %llu nboctets_per_block %d \n", nbdatatotal, nb_octets_per_block);
			nbdatatotal += iResult;
			nbdata += iResult;
		}
		if (difftime == 1)
		{
			wprintf(L"throughput %lf Mo/s datas sent %lf Mo \n", nbdata / (1024 * 1024.0), nbdatatotal / (1024 * 1024.0));
			before = time(NULL);
			nbdata = 0;
		}
	} 

	if (fclose(fh) != 0)
	{
		printf("error closing file\n");
		return -1;
	}
	/*if (fclose(fh2) != 0)
	{
		printf("error closing file\n");
		return -1;
	}*/

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
