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



// DNS SENDER //

/*
    1. ✓ Extrahovat argumenty na cmd
    2. Poslat data serveru
        - musim otevrit socket na -u
            ✓ vytvorit socket
            ✓ musim zjistit -u, pokud neni zadane
            ✓ jinak se prenese -u
        - musim poslat soubor v packetu
            -- vytvorit packet
                ✓ musim nacist soubor/STDIN
                --- pokud je moc velky -- TCP/UDP
                ✓ kontrolovat, jak velky soubor je
            ✓ zakodovat content soubor/STDIN, aby obsahoval znaky, 
               ktere se muzou objevovat v domene (base64 ci podobne)
            -- tecka po tecku je jenom 63 B a dohromady data 255 B)
            -- poslat zakodovany soubor packetem  
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

// function for sending packets
int sendingpackets(int socket, char *data, char *label, int labellength, char *u, struct sockaddr_in sockaddr) {
    char *message = NULL;
    message = calloc(1, MAXLINE);
    // -1 is reserved for a '.' before label is written
    int packetlength = 255 - labellength - 1;
    int packetnum = strlen(data) / packetlength + 1;

    int crntindex = 0;

    for (int i = 0; i < packetnum; i++) {
        // dns header



        // sending corresponding characters from data variable
        int index = 0;
        for (int j = index; j < packetlength; j++) {
            if ((crntindex % 64) == 0 && crntindex != 0) {
                message[index] = '.';
                index++;
                crntindex++;
                continue;
            }
            else {
                message[index] = data[crntindex];
            }
            index++;
            crntindex++;
        }

        // the reserved index for '.'
        message[index] = '.';
        index++;
        // put label into the message
        for (int j = 0; j < labellength; j++) {
            message[index] = label[j];
            index++;
        }

        // dns tail



        // sendto(socket, data, 255, MSG_OOB, sockaddr.sin_addr.s_addr, 255);
        memset(message, 0, MAXLINE);
    }
    printf("%s\n", message);
    free(message);
}

int main (int argc, char const *argv[]) {
    // definition of variables
    char *b = NULL;
    char *u = NULL;
    char *dst = NULL;
    char *source = NULL;
    char *data = NULL;
    int opt = 1;
    struct sockaddr_in sockaddr; // socket address

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

    // todo: better error handling (more codes or smth)
    if (b == NULL) {
        fprintf(stderr, "Error: BASE_HOST must be defined.");
        return 1;   
    }
    if (dst == NULL) {
        fprintf(stderr, "Error: DST_FILEPATH must be defined.");
        return 1;
    }

    // this legth is used to determine how many packets will be sent
    int labellength = strlen(b);

    // if the input for source filepath is on stdin
    if (source == NULL) {
        data = malloc(MAXLINE);
        if (data == NULL) {
            // todo: better error handling
            fprintf(stderr, "Error: Not enough memory.");
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
            fprintf(stderr, "Error: File doesn't exist.");
            return 1;
        }
        // save the data from file to "data" variable
        fread(data, 1, MAXLINE, fptr);
        if (data == NULL) {
             // todo: better error handling
            fprintf(stderr, "Error: No data was loaded.");
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
            fprintf(stderr, "Error: File not found; IP address not found.");
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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    sockaddr.sin_port = htons(53);
    // address is value of -u argument (or is extracted from resolv.conf)
    sockaddr.sin_addr.s_addr = inet_addr(u);

    // sending packets
    sendingpackets(sockfd, outputdata, b, labellength, u, sockaddr);

    // freeing allocated data
    free(data);
    return 0;
}
