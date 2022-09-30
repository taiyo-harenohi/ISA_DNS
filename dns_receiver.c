/*/////////////////
Name: DNS tunelling -- DNS_receiver
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
    1. Extrahuje argumenty
    2. Otevrit server/socket
    3. Prijme packety (ve smycce)
        - kazdy packet musi rozsifrovat
        - ziskat jmeno souboru a data
        - ulozit data do DST_FILEPATH/jmenosouboru
*/