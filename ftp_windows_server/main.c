// tcp_server.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define BUFLEN_SEND 65536
#define DEFAULT_PORT "1153"

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult = 0;
	unsigned long long nbdatatotal = 0, nbdata = 0;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

//	int iSendResult;
	int i = 0;
	char recvbuf[BUFLEN_SEND];

	time_t difftime = 0, previoustime = 0, starttime = 0, totaltime = 0, before = 0, endtime = 0;


	struct client_info{
		int nb_sockets;
		int chunk_size;
		char file_path[1024];
	};
	struct client_info client_parameters;
	memset(&client_parameters, 0, sizeof(client_parameters));

	FILE* fh=NULL;
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

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Waiting for client...\n");

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Client connected...\n");

	// No longer need server socket
	closesocket(ListenSocket);

	iResult = recv(ClientSocket, (char*) &client_parameters, sizeof(client_parameters), 0);

	if (fh = fopen_s(NULL, client_parameters.file_path, "r") != NULL){
		printf("error openning file\n");
		return -1;
	}

	char file_buf[BUFLEN_SEND]="";
	size_t nb_octets_read_from_file=0;

	fseek(fh, 0, SEEK_END);
	int lengthOfFile = ftell(fh)/2;
	fseek(fh, 0, SEEK_SET);

	difftime = before = starttime = time(NULL);
	// Receive until the file is read
	do
	{
		difftime = time(NULL) - before;
		nb_octets_read_from_file = fread(file_buf, BUFLEN_SEND, 1, fh);
		iResult = send(ClientSocket, file_buf, BUFLEN_SEND, 0);
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		nbdatatotal += nb_octets_read_from_file;
		nbdata += nb_octets_read_from_file;
		if (difftime == 1 /*(clock_t) CLOCKS_PER_SEC/10*/)
		{
			wprintf(L"throughput %lf Mo/s datas received %lf Mo \n", nbdata / (1024 * 1024.0), nbdatatotal / (1024 * 1024.0));
			before = time(NULL);
			nbdata = 0;
		}
	} while (nbdatatotal<lengthOfFile);

	printf("data sent : %llu\n", nbdatatotal);

	// send 0 length msg to client to close the connection
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	printf("sent 0 length msg to close connection...\n");
	// cleanup
	closesocket(ClientSocket);
	
	WSACleanup();
	printf("connection closed \n");

	return 0;
}
