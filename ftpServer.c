#ifndef __unp_h
#define __unp_h
#include <sys/types.h>
/* open and close  */
#include <fcntl.h> // for open 
#include <unistd.h> // for close
/* bcopy  */
#include <strings.h>
#include <string.h>
/* basic system data types */
#include <sys/socket.h>
/* basic socket definitions */
#include <netinet/in.h>
/* sockaddr_in{} and other Internet defns */
#include <sys/time.h>
/* timeval{} for select() */
#include <time.h>
/* timespec{} for pselect() */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#define MAX_PACKET_SIZE 128 //bytes
#define MAX_PACKET_DATA_SIZE 122 // bytes

struct header {
    char acknowledgement; // 2 bytes
    unsigned short checksum; // 2 bytes
    char sequenceNum; // 2 bytes
};

struct packet {
    struct header headerData; // 6 bytes
    char data[MAX_PACKET_DATA_SIZE]; // remaining 122 bytes
};

//prototypes
int errorDetectionServer(struct packet *packetPtr);

int main() {
	char *buffer;
	int n, sd, packetCount;
	unsigned int addr_length;
    struct sockaddr_in server;
    struct sockaddr from;
    struct packet packet;
    // use this packet to send NAKs. It will be initiliazed with just the acknowledgment set to 0
    struct packet nakPack;
    // use this packet to send ACKSs
    struct packet ackPack;
    struct packet *ackPackPointer = &ackPack;
    struct packet *nakPackPointer = &nakPack;
    struct packet *packetPointer = &packet;
    struct hostent *hp;

    // set NAK for nakPack
    nakPack.headerData.acknowledgement = '0';
    // set ACK for ackPack
    ackPack.headerData.acknowledgement = '1';

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(12345);

    from.sa_family = AF_INET;
    hp = gethostbyname("192.168.1.7"); // client's IP
    bcopy(hp->h_addr, &(server.sin_addr), hp->h_length);
    addr_length = sizeof(from);

	buffer = (char*)calloc(MAX_PACKET_DATA_SIZE, sizeof(char));

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	bind(sd, (struct sockaddr *) &server, sizeof(server));

	char sequenceCheck = '0'; // first packet will have sequence number of 0
	packetCount = 0;
	do {
		recvfrom(sd, packetPointer, MAX_PACKET_SIZE, 0, &from, &addr_length);
		printf("packet received\n");
		printf("Sequence Number: %c\n", packetPointer->headerData.sequenceNum);
		printf("Data: %s\n", packetPointer->data);
		if (packetPointer->headerData.sequenceNum != sequenceCheck || errorDetectionServer(packetPointer) == 0) {
			sendto(sd, nakPackPointer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &from, sizeof(from));
			printf("NAK sent\n");
		}
		else {
			sendto(sd, ackPackPointer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &from, sizeof(from));
			printf("No erros in packet. ACK sent\n");
			printf("Writing packet data to file...\n");
			// 
			//
			// do the file writing here
			//
			//
			packetCount++;
			if (sequenceCheck == '0') {
				sequenceCheck = '1';
			}
			else {
				sequenceCheck = '0';
			}
		}
	} while(packetPointer->data[0] != '\0');
	free(buffer);
	close(sd);
}

// error checking server side packet. returns 1 if checksums equal, 0 if there was an error
int errorDetectionServer(struct packet *packetPtr) {
	int i;
	unsigned short cksum;

	cksum = 0;
	cksum += (unsigned short)packetPtr->headerData.acknowledgement;
	cksum += (unsigned short)packetPtr->headerData.sequenceNum;
	for (i = 0; i < MAX_PACKET_DATA_SIZE; i++) {
		cksum += (unsigned short)packetPtr->data[i];
	}
	printf("Acutal checksum: %d\n", packetPtr->headerData.checksum);
	printf("Calculated checksum: %d\n", cksum);
	if (cksum == packetPtr->headerData.checksum) {
		return 1;
	}
	else {
		printf("error found in packet with sequence number %c\n", packetPtr->headerData.sequenceNum);
		return 0;
	}
}





























