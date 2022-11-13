/*/////////////////
Name: DNS tunelling -- dns_receiver
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
#include <stdbool.h>
#include  "dns_receiver_events.h"

// DNS RECEIVER //

// dns_receiver {BASE_HOST} {DST_FILEPATH}
// $ dns_receiver example.com ./data

// define macros
#define MAXLINE 1000000

char domain[MAXLINE];
int domainlen = 0;

char query[MAXLINE];
int chunksize = 0;
int chunkID = 1;

char* uselessquery(char *b) {
    int len = query[0];
    int ind = 0;
    static char new[MAXLINE];
    memset(new, 0, MAXLINE);
    for (int i = 1; i <= len; i++) {
        if (i == len) {
            new[ind] = query[i];
            ind++;
            i++;
            new[ind] = '.';
            ind++;
            len += query[i] + 1;
        }
        else {
            new[ind] = query[i];
            ind++;
        }
    }
    strcat(new, b);

    return new;
}

int decode(char *data, char *output) {
    for (int i = 0; i < strlen(data); i++) {
        char hex[3];
        hex[0] = data[i];
        hex[1] = data[i+1];
        hex[2] = '\0';
        int number = (int)strtol(hex, NULL, 16);
        output[i/2] = number;
        i++;
    }
}

int domainextract() {
    char temp[MAXLINE];
    memset(temp, 0, MAXLINE);
    int index = 0;
    for (int i = 0; i < strlen(domain); i++) {
        if (domain[i] == '.') {
            continue;
        }
        temp[index] = domain[i];
        index++;
    }
    memset(domain, 0, MAXLINE);
    memcpy(domain, temp, strlen(temp));
    domainlen = strlen(domain);
}

int extractdata(char *data, char *temp, int len) {
    int ind = 0;
    for (int i = 1; i <= len ; i++) {
        if (i == len) {
            temp[ind] = data[i];
            ind++;
            i++;
            len += data[i] + 1;
        }
        else {
            temp[ind] = data[i];
            ind++;
        }
    }

    if (strstr(temp, domain) != NULL) {
        char input[strlen(temp) - domainlen + 1];
        memset(input, 0, sizeof(input));
        memcpy(input, temp, (strlen(temp) - domainlen));

        char output[(strlen(input) / 2) + 1];
        memset(output, 0, sizeof(output));
        decode(input, output);
        memset(input, 0, strlen(temp) - domainlen);

        memset(temp, 0, strlen(temp));
        memcpy(temp, output, strlen(output));
        return 0;
    }
    return 1;
}




int main (int argc, char *argv[]) {
    char *b = NULL;
    char *dst = NULL;
    struct sockaddr_in sockaddr; // socket address
    int opt = 1;

    for (int i = 1; i < argc; i++) {
        if (b == NULL) {
            b = argv[i];
        }
        else if (dst == NULL) {
            dst = argv[i];
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

    memcpy(domain, b, strlen(b));
    domainextract();

    // socket creation 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)  {
        fprintf(stderr, "ERROR: Socket couldn't be created.\n");
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        fprintf(stderr, "ERROR: Couldn't set sockopt.\n");
        return -1;
    }    

    // assingning port & IP address
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(53);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        fprintf(stderr, "ERROR: Couldn't bind to port.\n");
        return -1;
    }

    socklen_t socklen = sizeof(sockaddr);
    struct sockaddr_in clientaddr;


    bool isFilename = false;
    bool isEnd = false;

    char buffer[MAXLINE];
    memset(buffer, 0, MAXLINE);
    int indbuffer = 0;

    char filename[MAXLINE];
    memset(filename, 0, MAXLINE);

    char filedata[MAXLINE];
    memset(filedata, 0, MAXLINE);

    char filepath[MAXLINE];
    memset(filepath, 0, MAXLINE);

    memset(query, 0, MAXLINE);

    while (true) {
        char data[MAXLINE];
        memset(data, 0, MAXLINE);

        char temp[MAXLINE];
        memset(temp, 0, MAXLINE);

        char output[MAXLINE];
        memset(output, 0, MAXLINE);

        // determine based on the first byte which type of packet was sent
        int length = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&clientaddr, &socklen);
        if (buffer[0] == '\t') {
            isFilename = true;
        }
        else if (buffer[0] == '\n') {
            isEnd = true;
        }

        memcpy(data, buffer+12, length - 12);
        int len = (int)data[0];

        if (isFilename) {
            if (extractdata(data, temp, len)) {
                memset(temp, 0, len);
                isFilename = false;
                continue;
            }

            dns_receiver__on_transfer_init(&clientaddr.sin_addr);

            memcpy(filename, temp, strlen(temp) + 1);
            isFilename = false;
            memset(temp, 0, MAXLINE);

            memset(filepath, 0, MAXLINE);
            strcpy(filepath, dst);
            strcat(filepath,"/");
            strcat(filepath, filename);
        }
        else if (isEnd) {
            memcpy(query, data, strlen(data) - strlen(b) - 1);

            if (extractdata(data, temp, len)) {
                memset(temp, 0, len);
                isEnd = false;
                continue;
            }
            dns_receiver__on_chunk_received(&clientaddr.sin_addr, filepath, chunkID, strlen(query) - (strlen(query) / 63 + 1) );
            dns_receiver__on_query_parsed(filepath, uselessquery(b));
            
            memcpy(filedata+indbuffer, temp, strlen(temp));
            indbuffer = 0;
            
            FILE *fptr;
            fptr = fopen(filepath, "w");
            if (fptr == NULL) {
                fprintf(stderr, "ERROR: Couldn't create new file.\n");
                return -1;
            }

            fwrite(filedata, 1, strlen(filedata), fptr);
            fclose(fptr);
            dns_receiver__on_transfer_completed(filepath, strlen(filedata));

            memset(filename, 0, MAXLINE);
            memset(filepath, 0, strlen(filepath));
            memset(buffer, 0, strlen(buffer));
            memset(filedata, 0, strlen(filedata));

            memset(query, 0, strlen(query));
            isEnd = false;
            chunkID = 1;
            chunksize = 0;
        }
        else {      
            memcpy(query, data, strlen(data) - strlen(b) - 1);

            if (extractdata(data, temp, len)) {
                memset(temp, 0, len);
                continue;
            }

            dns_receiver__on_chunk_received(&clientaddr.sin_addr, filepath, chunkID, strlen(query) - (strlen(query) / 63 + 1) );
            dns_receiver__on_query_parsed(filepath, uselessquery(b));

            memcpy(filedata+indbuffer, temp, strlen(temp));
            indbuffer += strlen(temp);

            chunkID++;
            memset(query, 0, MAXLINE);
        }
    }
}
