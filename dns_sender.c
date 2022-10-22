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

/*
    1. ✓ Extrahovat argumenty na cmd
    2. ✓ Poslat data serveru
        ✓ musim otevrit socket na -u
            ✓ vytvorit socket
            ✓ musim zjistit -u, pokud neni zadane
            ✓ jinak se prenese -u
        ✓ musim poslat soubor v packetu
            ✓ vytvorit packet
                ✓ musim nacist soubor/STDIN
                ✓ pokud je moc velky -- UDP
                ✓ kontrolovat, jak velky soubor je
            ✓ zakodovat content soubor/STDIN, aby obsahoval znaky, 
               ktere se muzou objevovat v domene (base64 ci podobne)
            ✓ tecka po tecku je jenom 63 B a dohromady data 255 B)
            ✓ poslat zakodovany soubor packetem  
    3. Ošetřit errory
    4. Lépe rozdělený kód na podproblémy (=funkce)
*/

// Examples of the input
// $ dns_sender -u 127.0.0.1 example.com data.txt ./data.txt
// $ echo "abc" | dns_sender -u 127.0.0.1 example.com data.txt

// basic version of the input
// dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]

// define macros
#define MAXLINE 10000 

// function for encoding
// encode is done via: dec -> hex
int encode(char *data, char *output) {
    for (int i = 0; i < strlen(data); i++) {
        sprintf(output+2*i, "%02x", data[i]);
    }
}

// count the length of each stream of data
char countlength(char *data) {
    for (int i = 1; i <= 63; i++) {
        if (data[i] == '\0') {
            return i;
        }
    }
    return 63;
}

int stripdomain(char *domain, char *message, int index) {
    const char delim[2] = ".";
    char *data;
    char *buffer = malloc(strlen(domain) + 1);
    strcpy(buffer, domain);
    data = strtok(buffer, delim);
    while (data != NULL) {
        message[index] = countlength(data);
        index++;
        for (int j = 0; j < strlen(data); j++) {
            message[index] = data[j];
            index++;
        }

        data = strtok(NULL, delim);
    }
}

// function for sending packets
int sendingpackets(int socket, char *data, char *domain, int domainlength, struct sockaddr_in sockaddr) {
    char *message = NULL;
    message = calloc(1, MAXLINE);
    // 2 places are reserved for a length before first data stream 
    // and before domain is written
    int packetlength = 255 - domainlength - 2;
    int packetnum = strlen(data) / packetlength + 1;

    int crntindex = 0;

    for (int i = 0; i < packetnum; i++) {
        // dns header
        char buffer[MAXLINE];
        memcpy(buffer, "\n\t\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00", 12);

        int index = 0;
        for (int j = index; j < packetlength; j++) {
            if ((crntindex % 63) == 0) {
                printf("%d\n", crntindex);
                message[index++] = countlength(data);
                printf("%s\n", data);
                crntindex++;
                continue;
            }
            else if (data[crntindex - 1] == '\0') {
                break;
            }
            else {
                message[index] = data[crntindex - 1];
            }
            index++;
            crntindex++;
        }

        printf("%s\n", message);

        // the reserved index for '.'
        message[index] = countlength(domain);
        index++;
        stripdomain(domain, message, index);

        strcpy(buffer+12, message);

        // dns tail
        memcpy(buffer+12+strlen(message)+1, "\x00\x01\x00\x01", 4);
        // sending packet
        sendto(socket, buffer, 12+strlen(message)+1+4, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        memset(message, 0, strlen(message));
    }
    free(message);
}

int main (int argc, char *argv[]) {
    // definition of variables
    char *b = NULL;
    char *u = NULL;
    char *dst = NULL;
    char *source = NULL;
    char *data = NULL;
    struct sockaddr_in sockaddr; // socket address
    bool isFile = false;

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
            isFile = true;
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

    // this legth is used to determine how many packets will be sent
    int domainlength = strlen(b);

    // if the input for source filepath is on stdin
    if (source == NULL) {
        data = malloc(MAXLINE);
        if (data == NULL) {
            // todo: better error handling
            fprintf(stderr, "Error: Not enough memory.\n");
            return 1;
        }
        fread(data, 1, MAXLINE, stdin);
    }
    else {
        // open the file
        FILE *fptr;
        fptr = fopen(source,"rb");
        data = calloc(1, MAXLINE);

        if (fptr == NULL) {
            // todo: better error handling
            fprintf(stderr, "Error: File doesn't exist.\n");
            return 1;
        }
        // save the data from file to "data" variable
        fread(data, 1, MAXLINE, fptr);
        if (data == NULL) {
             // todo: better error handling
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

    char outputdata[MAXLINE];
    encode(data, outputdata);

    // socket creation 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        fprintf(stderr, "ERROR: Socket couldn't be created.\n");
        return -1;
    }  

    // assingning port & IP address
    sockaddr.sin_family = AF_INET;
    // TODO: port 53
    sockaddr.sin_port = htons(1234);
    // address is value of -u argument (or is extracted from resolv.conf)
    sockaddr.sin_addr.s_addr = inet_addr(u);

    if (isFile) {
        char nameencode[MAXLINE];
        encode(source, nameencode);
        // sending filename
        sendingpackets(sockfd, nameencode, b, domainlength, sockaddr);
    }
    // sending packets
    sendingpackets(sockfd, outputdata, b, domainlength, sockaddr);

    // freeing allocated data
    free(data);
    return 0;
}
