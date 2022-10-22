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


// DNS RECEIVER //

// dns_receiver {BASE_HOST} {DST_FILEPATH}
// $ dns_receiver example.com ./data

/*
    1. ✓ Extrahuje argumenty
    2. ✓ Otevrit server/socket
    3. Prijme packety (ve smycce)
        - kazdy packet musi rozsifrovat
        - ziskat jmeno souboru a data
        - ulozit data do DST_FILEPATH/jmenosouboru
*/

// define macros
#define MAXLINE 512

int decode(char *data, char *output) {
    for (int i = 0; i < strlen(data); i++) {
        char hex[2];
        hex[0] = data[i];
        hex[1] = data[i+1];
        int number = (int)strtol(hex, NULL, 16);
        printf("hex: %s --- dec: %c\n", hex, number);
        i++;
    }
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
    
    // todo: better error handling (more codes or smth)
    if (b == NULL) {
        fprintf(stderr, "Error: BASE_HOST must be defined.\n");
        return 1;   
    }
    if (dst == NULL) {
        fprintf(stderr, "Error: DST_FILEPATH must be defined.\n");
        return 1;
    }

    // socket creation 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        fprintf(stderr, "ERROR: Socket couldn't be created.\n");
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        fprintf(stderr, "ERROR: Couldn't set sockopt.\n");
        return -1;
    }    

    // assingning port & IP address
    sockaddr.sin_family = AF_INET;
    //TODO: port 53
    sockaddr.sin_port = htons(1234);
    // address is value of -u argument (or is extracted from resolv.conf)
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        fprintf(stderr, "ERROR: Couldn't bind to port.\n");
        return -1;
    }

    char buffer[MAXLINE];

    socklen_t socklen = sizeof(sockaddr);
    struct sockaddr clientaddr;

    while (true)
    {
        int length = recvfrom(sockfd, buffer, MAXLINE, 0, &clientaddr, &socklen);

        char data[MAXLINE];
        memcpy(data, buffer+12, length - 12);
        int len = (int)data[0];
        printf("%d\n", len);




        for (int i = 1; i < strlen(data) - 5; i++) {
            char datain[63];
            // kolik je zrovna delka
            // pro tuto delku vytvorit char*
            // vytahnout, kde zacina domena
            // tu dekodovat
            // ulozit do velkeho bufferu/primo souboru uz
            for (int j = 0; j < len; j++) {
                datain[j] = data[i];
                i++;
            }
            char output[MAXLINE];
            
            //decode(datain, output);
            printf("%c\n", data[i]);
            len = data[i];
            char c = data[i];
            printf("\ndata: %d\n", c);
            //printf("\nlength: %d\n", len);

            memset(datain, 0, strlen(datain));
        }
    }
}
