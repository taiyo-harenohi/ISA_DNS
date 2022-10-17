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


// DNS RECEIVER //

// dns_receiver {BASE_HOST} {DST_FILEPATH}
// $ dns_receiver example.com ./data

/*
    1. ✓ Extrahuje argumenty
    2. Otevrit server/socket
    3. Prijme packety (ve smycce)
        - kazdy packet musi rozsifrovat
        - ziskat jmeno souboru a data
        - ulozit data do DST_FILEPATH/jmenosouboru
*/

int main (int argc, char const *argv[]) {
    char *b = NULL;
    char *dst = NULL;

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
        fprintf(stderr, "Error: BASE_HOST must be defined.");
        return 1;   
    }
    if (dst == NULL) {
        fprintf(stderr, "Error: DST_FILEPATH must be defined.");
        return 1;
    }
}
