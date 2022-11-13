# DNS tunneling
**Autor**: Nikola Machalkova  
**Login**: xmacha80  
**Date**: 14/11/2022


## Content
Program for managing DNS tunelling, includes sender and receiver. 
Works _only_ for IPv4 and text files, as it doesn't support '\0' (null byte) in its messages.


## Usage
For running the program, use command 
```bash 
make
```
to create binary files. Then run these files with the following arguments:

### dns_sender.c
```bash 
./dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]
```

### dns_receiver.c
```bash 
./dns_receiver {BASE_HOST} {DST_FILEPATH}
```

## Examples
### dns_sender.c
1. `./dns_sender -u 127.0.0.1 example.com datatxt ./data.txt`
### dns_receiver.c
2. `./dns_receiver example.com ./data`


## List of files
- sender
    * dns_sender.c
    * dns_sender_events.c
    * dns_sender_events.h
- receiver
    * dns_receiver.c
    * dns_receiver_events.c
    * dns_receiver_events.h
- README
- Makefile 
