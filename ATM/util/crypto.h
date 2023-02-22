
#ifndef CRYPTO_GUARD
#define CRYPTO_GUARD

#define PACKETPROTOCOL


#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*#include "protocol.h"*/
#include <openssl/aes.h>

#include <openssl/evp.h>

#include <openssl/sha.h>

typedef struct _Packet {
  char checksum[32]; //sha256 hash of packet contents not including length
  int len;
  char content[0];
}
Packet;


ssize_t crypto_send(int fd,struct sockaddr* addr, char *data, size_t data_len);
ssize_t crypto_recv(int fd, char *data, size_t max_data_len);



#endif