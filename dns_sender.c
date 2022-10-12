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



// DNS SENDER //

/*
    1. ✓ Extrahovat argumenty na cmd
    2. Poslat data serveru
        - musime otevrit socket na -u
            ✓ vytvorit socket
            -- musime zjistit, co je -u, pokud neni zadane
        - musime poslat soubor v packetu
            -- vytvorit packet
            ✓ musime nacist soubor/STDIN
                --- pokud je moc velky -- TCP/UDP
            -- zakodovat content soubor/STDIN, aby obsahoval znaky, 
               ktere se muzou objevovat v domene (base64 ci podobne)
               (po tecku po tecku je jenom 63 B a dohromady data 255 B)
            -- poslat zakodovany soubor packetem  
    3. Ošetřit errory
*/

// Examples of the input
// $ dns_sender -u 127.0.0.1 example.com data.txt ./data.txt
// $ echo "abc" | dns_sender -u 127.0.0.1 example.com data.txt

// basic version of the input
// dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]

// define macros
#define MAXLINE 10000 


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
    // todo: nacist soubor a ulozit data v nem
    else {
        // open the file
        FILE *fptr;
        fptr = fopen(source,"rb");
        data = malloc(MAXLINE);

        if (fptr == NULL) {
            // todo: better error handling
            fprintf(stderr, "Error: File doesn't exist.");
            return 1;
        }
        // save the data from file to "data" variable
        fread(data, 1, MAXLINE, fptr);
        if (data == NULL) {
            printf("what");
        }
        fclose(fptr);
    }


    // determine -u
    if (u == NULL) {
        FILE *fptr;
        char *tmp = NULL;
        tmp = malloc(MAXLINE);
        fptr = fopen("/etc/resolv.conf", "rb");
        fgets(tmp, MAXLINE, fptr);
        if (fptr == NULL) {
            fprintf(stderr, "Error: Reading IP address failed.");
            return 1;
        }

        // lolol, this part is fucked 
        char *token = strtok(tmp, " ");
        while( token != NULL ) {
            printf(" %s\n", token );
            token = strtok(NULL, " ");
        }

        free(tmp);
        fclose(fptr);
    }

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

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(53);
    // k tomuto se vaze -u; inet_addr(-u) == toto je IP adresa
    // pokud neni IP adresa, musi se zjistit ze systemu
    sockaddr.sin_addr.s_addr = INADDR_ANY;




    free(data);
    return 0;
}
