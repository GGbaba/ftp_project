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
#define DEFAULT_PORT "1153"
#define MAX_FIBER_NB 8

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult = 0;
	unsigned long long nbdatatotal = 0, nbdata = 0;

	SOCKET MasterListenSocket;// INVALID_SOCKET;
	SOCKET MasterClientSocket; // INVALID_SOCKET
	SOCKET ListenSockets[MAX_FIBER_NB];// INVALID_SOCKET;
	SOCKET ClientSockets[MAX_FIBER_NB]; // INVALID_SOCKET
	memset(ListenSockets, 0, sizeof(ListenSockets));
	memset(ClientSockets, 0, sizeof(ClientSockets));

	struct addrinfo *masterresult;
	struct addrinfo *clientsresult[MAX_FIBER_NB];
	struct addrinfo masterhints;
	struct addrinfo clientshints[MAX_FIBER_NB];

//	int iSendResult;
	int count = 0;
	//char recvbuf[BUFLEN_SEND];

	time_t difftime = 0, previoustime = 0, starttime = 0, totaltime = 0, before = 0, endtime = 0;


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
	fflush(stdout);

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

	ZeroMemory(&masterhints, sizeof(masterhints));
	masterhints.ai_family = AF_INET;
	masterhints.ai_socktype = SOCK_STREAM;
	masterhints.ai_protocol = IPPROTO_TCP;
	masterhints.ai_flags = AI_PASSIVE;

	//Establishing connection with the master

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &masterhints, &masterresult);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	MasterListenSocket = socket(masterresult->ai_family, masterresult->ai_socktype, masterresult->ai_protocol);
	if (MasterListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(masterresult);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(MasterListenSocket, masterresult->ai_addr, (int)masterresult->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(masterresult);
		closesocket(MasterListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(masterresult);
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
	iResult = recv(MasterClientSocket, (char*)&client_parameters, sizeof(client_parameters), 0);

	printf("file_path_asked:%s\n", client_parameters.file_path);
	printf("nb_clients_to_connect : %d\n", client_parameters.nb_sockets);
	printf("Waiting for %d clients...\n", client_parameters.nb_sockets);


	FILE* fh = NULL;
	if (fopen_s(&fh, client_parameters.file_path, "r") != 0)
	{
		perror("error openning file\n");
		return -1;
	}

	char file_buf[BUFLEN_SEND][MAX_FIBER_NB];
	int nb_octets_per_block = BUFLEN_SEND;//sizeof(char);
	int nb_blocks_to_read = sizeof(char);//1;
	int nb_blocks_read_from_file = 0;

	fseek(fh, 0, SEEK_END);
	int lengthOfFile = ftell(fh);
	fseek(fh, 0, SEEK_SET);

	printf("length of file (octets) %d\n", lengthOfFile);

	memcpy(&server_parameters, &client_parameters, sizeof(struct transfer_info));

	iResult = send(MasterClientSocket, (char*)&server_parameters, sizeof(server_parameters), 0);

	// No longer need server socket
	closesocket(MasterListenSocket);
	// No longer need server clien socket
	closesocket(MasterClientSocket);
	printf("I have answered to client master channel !\n");

	//Establishing connection with N clients

	ZeroMemory(&clientshints, sizeof(clientshints));
	int cnt_clients_sockets = 0;
	for (cnt_clients_sockets; cnt_clients_sockets < (client_parameters.nb_sockets - 1); cnt_clients_sockets++)
	{
		clientshints[cnt_clients_sockets].ai_family = AF_INET;
		clientshints[cnt_clients_sockets].ai_socktype = SOCK_STREAM;
		clientshints[cnt_clients_sockets].ai_protocol = IPPROTO_TCP;
		clientshints[cnt_clients_sockets].ai_flags = AI_PASSIVE;
		// Resolve the server address and port
		iResult = getaddrinfo(NULL, DEFAULT_PORT + cnt_clients_sockets + 1, &clientshints[cnt_clients_sockets], &clientsresult[cnt_clients_sockets]);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}
		// Create a SOCKET for connecting to server
		ListenSockets[cnt_clients_sockets] = socket(clientsresult[cnt_clients_sockets]->ai_family, clientsresult[cnt_clients_sockets]->ai_socktype, clientsresult[cnt_clients_sockets]->ai_protocol);
		if (ListenSockets[cnt_clients_sockets] == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(clientsresult[cnt_clients_sockets]);
			WSACleanup();
			return 1;
		}

		// Setup the TCP listening socket
		iResult = bind(ListenSockets[cnt_clients_sockets], clientsresult[cnt_clients_sockets]->ai_addr, (int)clientsresult[cnt_clients_sockets]->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(clientsresult[cnt_clients_sockets]);
			closesocket(ListenSockets[cnt_clients_sockets]);
			WSACleanup();
			return 1;
		}
		freeaddrinfo(clientsresult[cnt_clients_sockets]);
		iResult = listen(ListenSockets[cnt_clients_sockets], SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSockets[cnt_clients_sockets]);
			WSACleanup();
			return 1;
		}
		ClientSockets[cnt_clients_sockets] = accept(ListenSockets[cnt_clients_sockets], NULL, NULL);
		if (ClientSockets[cnt_clients_sockets] == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSockets[cnt_clients_sockets]);
			WSACleanup();
			return 1;
		}
		printf("client num : %d", cnt_clients_sockets);
	}



	difftime = before = starttime = time(NULL);
	
	count = 0;
	// Receive until the file is read
	do
	{
		count++;
		difftime = time(NULL) - before;
		if ( (lengthOfFile - nbdatatotal) <= nb_octets_per_block)
		{
			nb_octets_per_block = lengthOfFile - nbdatatotal;
		}
		//read file per N blocks
		nb_blocks_read_from_file = fread(file_buf, nb_octets_per_block*cnt_clients_sockets, nb_blocks_to_read, fh);
		//send file depending on nb sockets opened
		for (cnt_clients_sockets = 0; cnt_clients_sockets < client_parameters.nb_sockets; cnt_clients_sockets++)
		{
			iResult = send(ClientSockets[cnt_clients_sockets], file_buf, nb_blocks_read_from_file*nb_octets_per_block, 0);
			if (iResult == SOCKET_ERROR)
			{
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSockets[cnt_clients_sockets]);
				WSACleanup();
				return 1;
			}
		}
		printf("nb_blocks_read %d nbdatatotal %llu nboctets_per_block %d \n", nb_blocks_read_from_file, nbdatatotal, nb_octets_per_block);
		nbdatatotal += nb_blocks_read_from_file * nb_octets_per_block;
		nbdata += nb_blocks_read_from_file * nb_octets_per_block;
		if (difftime == 1)
		{
			wprintf(L"throughput %lf Mo/s datas sent %lf Mo nb_blocks read : %d \n", nbdata / (1024 * 1024.0), nbdatatotal / (1024 * 1024.0), nb_blocks_read_from_file);
			before = time(NULL);
			nbdata = 0;
		}
		//printf("nbdatatotal %d nb_octets_per_blocks%d", nbdatatotal, nb_octets_per_block);
	} while (nbdatatotal<lengthOfFile && nb_octets_per_block != 0 );

	if (fclose(fh) != 0)
	{
		printf("error closing file\n");
		return -1;
	}

	printf("data sent : %llu\n", nbdatatotal);

	// send 0 length msg to client to close the connection
	iResult = shutdown(ClientSockets, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSockets);
		WSACleanup();
		return 1;
	}
	printf("sent 0 length msg to close connection...\n");
	
	// cleanup
	closesocket(ClientSockets);
	
	WSACleanup();
	printf("connection closed \n");

	return 0;
}
