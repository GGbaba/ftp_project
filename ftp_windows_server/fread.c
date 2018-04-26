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
	int iResult = 0;
	long long nbdatatotal = 0, nbdata = 0;
	int count = 0;
	//char recvbuf[BUFLEN_SEND];

	time_t difftime = 0, previoustime = 0, starttime = 0, totaltime = 0, before = 0, endtime = 0;
	int useless = 0;
	/*
	int fh = 0;
	if (_sopen_s(&fh, "E:\\bench\\toto.tdms" , _O_BINARY, _SH_DENYWR, 0))
	{
		perror("error openning file0\n");
		exit(1);
	}*/

	FILE* fh;

	// Open for read (will fail if file "crt_fopen_s.c" does not exist)  
	fh = fopen("E:\bench\toto.tdms", "rb");
	if (fh == 0)
		printf("The file 'crt_fopen_s.c' was opened\n");
	else
	{
		printf("The file 'crt_fopen_s.c' was not opened\n");
		return -1;
	}

	/* Set the end of the file: */
//	lseek(fh, 0L, SEEK_SET);// = _lseeki64(fh, 0L, SEEK_END);
	unsigned long long lengthOfFile;
	lengthOfFile = 1024 * 1024 * 1024 * 2;
	if (lengthOfFile < 0)
		perror("_lseeki64 to end failed");
	else
		printf("Position for end of file seek = %llu\n", lengthOfFile);

	long long nb_blocks_read_from_file[MAX_FIBER_NB];
	unsigned int nb_octets_per_read_disk[MAX_FIBER_NB];
	int nb_octets_per_socket = BUFLEN_SEND;
	//char file_buf[MAX_FIBER_NB];

	int cnt_clients_sockets = 1;
	int i = 0;
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

	printf("nb_octets_per_read_disk = %lu\n", nb_octets_per_read_disk);
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
			if (fh != 0) nb_blocks_read_from_file[i] = read(fh, file_buf[i], 1024*1024*1024/*nb_octets_per_read_disk[i]*/);
			size_to_send[i] = nb_blocks_read_from_file[i];
			//size_to_send[i] = nb_octets_per_read_disk[i]; // for no read file purpose bench
			//printf("size_to_send[client %d] %lli\n", i, size_to_send[i]);
			nbdatatotal += size_to_send[i];
			nbdata += size_to_send[i];

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


	if (close(fh) != 0)
	{
		printf("error closing file\n");
	}

	if (file_buf != NULL && (sizeof(file_buf) != 4 || sizeof(file_buf) != 8))
		free(file_buf);
	
	printf("data sent : %llu\n", nbdatatotal);

	return 0;
}
