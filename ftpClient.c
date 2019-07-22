/* Reads file into a buffer  */

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

// prototypes
void removeNewline(char *array);
void segmentData(char *buffer, int bufferLength, struct packet *packetArray);
void errorDetectionClient(struct packet *packetArray, int numOfPackets);

int main() {
    FILE *infile;
    char *buffer;
    char filename[50];
    long numbytes;
    int bufferLength;
    int numOfPackets;
    unsigned int addr_length;
    struct packet *packetArray;
    int sd;
    struct sockaddr_in server;
    struct hostent *hp;
    struct packet receivePacket;
    struct packet *receivePacketPointer = &receivePacket;
    time_t start, end;

    printf("Enter the file name to be read into the buffer:\n");
    
    // fgets keeps the newline character so it will need to be removed in order to be read 
    fgets(filename, 50, stdin);
    removeNewline(filename);
    infile = fopen(filename, "r");

    if(infile == NULL) {
        printf("error: file doesn't exist\n");
        return 1;
    }

    //  Find the length of the file in bytes
    fseek(infile, 0, SEEK_END); 
    numbytes = ftell(infile);

    // bring position indicator back to start of file
    fseek(infile, 0, SEEK_SET);

    buffer = (char*)calloc(numbytes, sizeof(char));

    if (buffer == NULL) {
        printf("memory error");
        return 1;
    }

    fread(buffer, sizeof(char), numbytes, infile);
    fclose(infile);

    bufferLength = strlen(buffer);
    numOfPackets = (bufferLength + MAX_PACKET_DATA_SIZE - 1)/MAX_PACKET_DATA_SIZE;
    /* 
    add one extra packet for the one byte null message which tells
    the receiver that the entire file has been transmitted.
    */
    numOfPackets++;  
    packetArray = calloc(numOfPackets, sizeof(char));

    segmentData(buffer, bufferLength, packetArray);
    errorDetectionClient(packetArray, numOfPackets);

    sd = socket(AF_INET, SOCK_DGRAM, 0);

    hp = gethostbyname("192.168.1.7"); 
    bcopy(hp->h_addr, &(server.sin_addr), hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(12345);

    addr_length = sizeof(server);
    int remainingLength = numOfPackets*MAX_PACKET_SIZE;
    char buf[MAX_PACKET_SIZE];
    struct packet *packetPointer = packetArray;
    while (remainingLength > 0) {
        size_t length = sendto(sd, packetPointer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &server, sizeof(server));
        printf("Packet sent\n");
        printf("Data: %s\n", packetPointer->data);
        start = clock();
        while (end - start < 20) {
            recvfrom(sd, receivePacketPointer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &server, &addr_length);
            end = clock();
        }
        packetPointer += MAX_PACKET_SIZE;
        remainingLength -= MAX_PACKET_SIZE;
    }
    close(sd);

    /*
    for (int i = 0; i < numOfPackets; i++) {
        printf("\nPacket number %d\n", i);
        printf("Header ACK: %d", packetArray[i].headerData.acknowledgement);
        printf("\nHeader cksm: %d", packetArray[i].headerData.checksum);
        printf("\nHeader sequenceNum: %d\n", packetArray[i].headerData.sequenceNum);
        printf("Data: %s\n", packetArray[i].data);
    }
    */

    free(buffer);
    free(packetArray);
}

// method to segment data into packets of 2048 bits
void segmentData(char *buffer, int bufferLength, struct packet *packetArray) {
    int i, j, k;
    int numOfPackets = (bufferLength + MAX_PACKET_DATA_SIZE - 1)/MAX_PACKET_DATA_SIZE;
    numOfPackets++;
    for (i = 0; i < numOfPackets - 1; i++) {
        packetArray[i].headerData.acknowledgement = '1';
        packetArray[i].headerData.checksum = '0';
        if (i%2 == 0) {
            packetArray[i].headerData.sequenceNum = '0';
        } 
        else {
            packetArray[i].headerData.sequenceNum = '1';
        }
        k = 0;
        for (j = i*MAX_PACKET_DATA_SIZE; j < (i*MAX_PACKET_DATA_SIZE+MAX_PACKET_DATA_SIZE); j++) {
            packetArray[i].data[k] = buffer[j];
            k++;
        }
    }
    // final packet gets null character for data
    packetArray[numOfPackets].data[0] = '\0';
    packetArray[numOfPackets].headerData.acknowledgement = '1';
    if (numOfPackets%2 == 0) {
        packetArray[numOfPackets].headerData.sequenceNum = '0';
    } 
    else {
        packetArray[numOfPackets].headerData.sequenceNum = '1';
    }
}

void errorDetectionClient(struct packet *packetArray, int numOfPackets) {
    int i, j;
    unsigned short sum;
    for (i = 0; i <  numOfPackets; i++) {
        sum = 0;
        sum += (unsigned int)packetArray[i].headerData.acknowledgement;
        sum += (unsigned int)packetArray[i].headerData.sequenceNum;
        for (j = 0; j < MAX_PACKET_DATA_SIZE; j++) {
            sum += (unsigned int)packetArray[i].data[j];
        }
        packetArray[i].headerData.checksum = sum;
    }
}

// method to remove the newline after using fgets
void removeNewline(char *array) {
    int length, i;
    length = strlen(array);
    for(i = 0; i < length; i++) {
        if (array[i] == '\n') {
            array[i] = '\0';
        }
    }
}


















