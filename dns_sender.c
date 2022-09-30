/*/////////////////
Name: DNS tunelling -- DNS_sender
Subject: ISA
Author: Nikola Machalkova
Login: xmacha80
Date: 14/11/2022
*//////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>



// DNS SENDER //

// dns_sender [-b BASE_HOST] [-u UPSTREAM_DNS_IP] {DST_FILEPATH} [SRC_FILEPATH]

// $ dns_sender -b example.com -u 127.0.0.1 data.txt ./data.txt
// $ dns_sender -b example.com -u 127.0.0.1 data.txt < echo "abc"

/*
    1. Extrahovat argumenty na cmd
    2. Poslat data serveru
        - musime otevrit socket na -u
            -- musime zjistit, co je -u, pokud neni zadane
        - musime poslat soubor v packetu
            -- musime nacist soubor/STDIN
                --- pokud je moc velky -- TCP/UDP
            -- zakodovat content soubor/STDIN, aby obsahoval znaky, 
               ktere se muzou objevovat v domene (base 64)
            -- poslat zakodovany soubor packetem  
*/

int main (int argc, char const *argv[]) {

    char delim[] = "";

    for (int i = 0; i < 10; i++) {
        printf(argv[i]);
    }
}