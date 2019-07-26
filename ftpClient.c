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
#define MAX_PACKET_DATA_SIZE 124 // bytes

struct header {
    char acknowledgement; // 1 byte
    unsigned short checksum; // 2 bytes
    char sequenceNum; // 1 byte
};

struct packet {
    struct header headerData; // 4 bytes
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

    printf("Enter the file name to be read into the buffer: ");
    
    // fgets keeps the newline character so it will need to be removed in order to be read 
    fgets(filename, 50, stdin);
    removeNewline(filename);
    infile = fopen(filename, "r");

    if(infile == NULL) {
        printf("error: file doesn't exist\n");
        return 1;
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);

    hp = gethostbyname("127.0.0.1"); 
    bcopy(hp->h_addr, &(server.sin_addr), hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(10012);

    addr_length = sizeof(server);

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


    int remainingLength = numOfPackets*MAX_PACKET_SIZE;
    char buf[MAX_PACKET_SIZE];
    struct packet *packetPointer = packetArray;
    size_t length;



    while (packetPointer->data[0] != '\0') {
        length = sendto(sd, packetPointer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &server, sizeof(server));
        printf("Packet sent\n");
        printf("Sequence Number: %c\n", packetPointer->headerData.sequenceNum);
        printf("Data: %s\n", packetPointer->data);
        printf("Checksum: %d\n", packetPointer->headerData.checksum);
        
        // loop while it hasn't timed out and the length of the message is still zero
        length = recvfrom(sd, receivePacketPointer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &server, &addr_length);
        /* 
           Timeout, so the current packet will be resent.
           This keeps the pointer pointing to the same packet for the next time 
           the program enters the loop.
        */ 
        if ((end - start)/CLOCKS_PER_SEC >= 0.02) {
            printf("timeout receving ACK or NAK from server\n");
            printf("resending packet\n");
        }
        else {
            if (receivePacketPointer->headerData.acknowledgement == '1') {
                printf("ACK received. Moving on to next packet.\n");
                packetPointer += MAX_PACKET_SIZE;
                remainingLength -= MAX_PACKET_SIZE;
            }
            else {
                printf("NAK received: ");
                printf("Error in packet with sequence number %c\nResending packet\n", packetPointer->headerData.sequenceNum);
            }
            
        }
    }
    // send null packet
    sendto(sd, packetPointer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &server, sizeof(server));
    printf("Final Packet Sent\n");
    close(sd);

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
        packetArray[i].headerData.checksum = 0;
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
        packetArray[numOfPackets].headerData.sequenceNum = '1';
    } 
    else {
        packetArray[numOfPackets].headerData.sequenceNum = '0';
    }
}

// method for calculating checksum to place in packet's header
void errorDetectionClient(struct packet *packetArray, int numOfPackets) {
    int i, j;
    unsigned short sum;
    for (i = 0; i < numOfPackets; i++) {
        sum = 0;
        sum += (unsigned short)packetArray[i].headerData.acknowledgement;
        sum += (unsigned short)packetArray[i].headerData.sequenceNum;
        for (j = 0; j < strlen(packetArray[i].data); j++) {
            sum += (unsigned short)packetArray[i].data[j];
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