/*/////////////////
Name: DNS tunelling -- dns_sender
Subject: ISA
Author: Nikola Machalkova
Login: xmacha80
Date: 14/11/2022
*//////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dns_sender_events.h"

// Examples of the input
// $ dns_sender -u 127.0.0.1 example.com data.txt ./data.txt
// $ echo "abc" | dns_sender -u 127.0.0.1 example.com data.txt

// basic version of the input
// dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]

// define macros
#define MAXLINE 1000000 

char* filename;
int chunksize = 0;

// function for encoding
// encode is done via: dec -> hex
int encode(char *data, char *output, int filesize) {
    for (int i = 0; i < filesize; i++) {
        sprintf(output+2*i, "%02x", data[i]);
    }
}

// function for dealing with domain
int stripdomain(char *domain, char *message, int index) {
    const char delim[2] = ".";
    char *data;
    char *buffer = malloc(strlen(domain) + 1);
    strcpy(buffer, domain);
    data = strtok(buffer, delim);

    index++;
    while (data != NULL) {
        message[index] = (char)strlen(data);
        index++;
        strcpy(message+index, data);
        index += strlen(data);
        data = strtok(NULL, delim);
    }
    free(buffer);
}

// function for dns_sender__on_chunk_encoded
void chunkencoded(char* domain, int chunkID, char* data){
    strcat(data, domain);
    dns_sender__on_chunk_encoded(filename, chunkID, data);
}

// function for transfering data into message at the size of packetLength
int getdata(char* data, char* message, int allsize, int nchunks, int packetlength, char* domain, int chunkID, bool isName) {
    char buffer[MAXLINE];
    memset(buffer, 0, MAXLINE);
    for (int j = 0; j < nchunks; j++) {
        char ar[63];
        memset(ar, 0, 63);
        int length = strlen(data+63*j);
        if (length > 63) {
            length = 63;
        }
        memcpy(ar, data+63*j, length);
        if (strlen(ar) == 0) {
            break;
        }
        
        memcpy(buffer+(64*j), data+63*j, length);
        buffer[64*j + length] = '.';

        allsize += length;
        chunksize += length;

        message[64*j] = (char)length;
        memcpy(message+(64*j + 1), data+(63*j), sizeof(ar));
    }
    int thissize = nchunks * 63;
    if (thissize < packetlength && strlen(data) > thissize) {
        int arraylength = packetlength - thissize;
        if (strlen(data) < packetlength) {
            arraylength = strlen(data) - thissize;
        }
        char ar[arraylength];
        memcpy(ar, data+63*(nchunks), sizeof(ar));

        memcpy(buffer+64*nchunks, data+63*nchunks, sizeof(ar));
        buffer[64*nchunks + sizeof(ar)] = '.';

        allsize += sizeof(ar);
        chunksize += sizeof(ar);

        message[64*(nchunks)] = (char)sizeof(ar);
        memcpy(message+(64*nchunks + 1), data+(63*nchunks), sizeof(ar));
    }    
    if (!isName){
        chunkencoded(domain, chunkID, buffer);
    }
    message[strlen(message)] = '\0';
    return allsize;
}

// function for sending packets
int sendingpackets(int socket, char *data, char *domain, struct sockaddr_in sockaddr, bool isName) {
    // one char is reserved for the length of the first part of domain
    // and for '\0'
    int packetlength = 255 - strlen(domain) - 2 - 4;
    int nchunks = packetlength / 63;
    int packetnum = strlen(data) / packetlength + 1;
    int allsize = 0;
    char message[MAXLINE];
    memset(message, 0, MAXLINE);
    int chunkID = 0;


    if (packetlength % 2 == 1) {
        packetlength -= 1;
    }

    for (int i = 0; i < packetnum; i++) {
        if (i % 100 == 99) {
            sleep(1);
        }
        // dns header
        char buffer[MAXLINE];
        // begin t
        if (isName) {
            memcpy(buffer, "\t\n\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00", 12);
        }
        // end n
        else if (i == packetnum - 1) {
            memcpy(buffer, "\n\n\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00", 12);
            chunkID++;
        }
        // middle r
        else{
            memcpy(buffer, "\r\n\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00", 12);
            chunkID++;
        }

        allsize = getdata(data+allsize, message, allsize, nchunks, packetlength, domain, chunkID, isName);

        stripdomain(domain, message, strlen(message) - 1);

        strcpy(buffer+12, message);
        // dns tail
        memcpy(buffer+12+strlen(message)+1, "\x00\x01\x00\x01", 4);
        // sending packet
        sendto(socket, buffer, 12+strlen(message)+1+4, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        
        if (!isName)
        {
            dns_sender__on_chunk_sent(&sockaddr.sin_addr, filename, chunkID, chunksize);
        }
        chunksize = 0;

        memset(message, 0, strlen(message));
    }
}

int main (int argc, char *argv[]) {
    // definition of variables
    char *b = NULL;
    char *u = NULL;
    char *dst = NULL;
    char *source = NULL;
    char *data = NULL;
    struct sockaddr_in sockaddr; // socket address
    bool isName = false;

    // argument handling
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0) {
            i++;
            u = argv[i];
        }
        else if (b == NULL) {
            b = argv[i];
        }
        else if (dst == NULL) {
            dst = argv[i];
        }
        else {
            source = argv[i];
        }
    }

    if (b == NULL) {
        fprintf(stderr, "Error: BASE_HOST must be defined.\n");
        return 1;   
    }
    if (dst == NULL) {
        fprintf(stderr, "Error: DST_FILEPATH must be defined.\n");
        return 1;
    }

    filename = dst;

    int filesize = 0;
    // if the input for source filepath is on stdin
    if (source == NULL) {
        data = malloc(MAXLINE);
        if (data == NULL) {
            fprintf(stderr, "Error: Not enough memory.\n");
            return 1;
        }
        filesize = fread(data, 1, MAXLINE, stdin);
    }
    else {
        // open the file
        FILE *fptr;
        fptr = fopen(source,"rb");
        data = calloc(1, MAXLINE);

        if (fptr == NULL) {
            fprintf(stderr, "Error: File doesn't exist.\n");
            return 1;
        }
        // save the data from file to "data" variable
        filesize = fread(data, 1, MAXLINE, fptr);
        if (data == NULL) {
            fprintf(stderr, "Error: No data was loaded.\n");
            return 1;
        }
        fclose(fptr);
    }

    // determine -u
    if (u == NULL) {
        FILE *fptr;
        char *tmp = NULL;
        fptr = fopen("/etc/resolv.conf", "rb");
        if (fptr == NULL) {
            fprintf(stderr, "Error: File not found; IP address not found.\n");
            return 1;
        }

        char buffer[MAXLINE];
        while (fgets(buffer, MAXLINE, fptr) != NULL) {
            if(strstr(buffer, "nameserver") != NULL) {
                strtok(buffer, " ");
                u = strtok(NULL, " \n");
                break;
            }
        }
        fclose(fptr);
    }

    // socket creation 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        fprintf(stderr, "ERROR: Socket couldn't be created.\n");
        return -1;
    }  

    // assingning port & IP address
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(53);
    // address is value of -u argument (or is extracted from resolv.conf)
    sockaddr.sin_addr.s_addr = inet_addr(u);

    dns_sender__on_transfer_init(&sockaddr.sin_addr);

    char outputdata[MAXLINE];
    encode(data, outputdata, filesize);

    // encode filename
    char outputname[MAXLINE];
    memset(outputname, 0, MAXLINE);
    encode(dst, outputname, strlen(dst));

    // sending filename
    sendingpackets(sockfd, outputname, b, sockaddr, true);

    // sending packets
    sendingpackets(sockfd, outputdata, b, sockaddr, false);

    dns_sender__on_transfer_completed(dst, filesize);

    // freeing allocated data
    free(data);
    return 0;
}
