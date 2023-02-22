#include "crypto.h"

#include <stdio.h>

#include <string.h>

#include <stdlib.h>

//#define CRYPTO_DBG
//#define CRYPTO_SOLOCOMPILE

#ifdef CRYPTO_DBG
#define DBGPRINT(...) printf(__VA_ARGS__)
#define SASRT(expr, dbg) _Static_assert(expr, dbg)

#include <signal.h>

#define BRK(...)\
DBGPRINT(__VA_ARGS__);\
raise(SIGINT)

#else
#define DBGPRINT(...)
#define BRK(...)
#define SASRT(expr, dbg)
#endif

/*outhash must be at least 32 bytes*/
void getsha256hash(char * input, int inputlen, char * outhash);

void aesencrypt(char * input, int inputlen, char ** ciphertext, int * ciphertextlen);
void aesdecrypt(char * input, int inputlen, char ** plaintext, int * plaintextlen);

void printbytes(char * p, int bytes);
int getpacketlen(int expectedlen);

Packet * createpacket(char * content, int contentlen, int * packetlenout);
int decodepacket(Packet * p, int packetlen);
void freepacket(Packet * p);


char aes_key[] = "dfkkg4d9300000000000000000000000";
char aes_iv[AES_BLOCK_SIZE] = "0000000000000000";

int getpacketlen(int expectedlen) {
  int packetlen = expectedlen;
  int res = packetlen % AES_BLOCK_SIZE;

  //DBGPRINT("packetlen: %d res: %d blksz: %d new: %d\n",packetlen,res,AES_BLOCK_SIZE,packetlen+(AES_BLOCK_SIZE-res));

  if (res) {
    packetlen += (AES_BLOCK_SIZE - res);
  }
  return packetlen;
}

ssize_t crypto_send(int fd,struct sockaddr* addr, char *data, size_t data_len){
	int newlen = 0;
    Packet* p = createpacket(data,data_len,&newlen);

    ssize_t retval = sendto(fd, p, newlen, 0,(struct sockaddr*) addr, sizeof(struct sockaddr_in));

    freepacket(p);

    return retval;
}

ssize_t crypto_recv(int fd, char *data, size_t max_data_len){

		Packet* p = (Packet*)malloc(max_data_len);
        int packetlen = recvfrom(fd, (void*)p, max_data_len, 0, NULL, NULL);

        if(!decodepacket(p,packetlen)){
            DBGPRINT("failed to decoded message from bank\n");
            free(p);
            return 0;
        }

        int msglen = p->len;

        //this should not happen (with checksum) but might as well check
        if(msglen > max_data_len){
            DBGPRINT("invalid msg len\n");
            free(p);
            return 0;
        }

        memcpy(data,&p->content[0],msglen);
        

        return msglen;
}



//Packet memory must be freed. 0 on failure else pointer to packet structure that must be freed
Packet * createpacket(char * content, int contentlen, int * packetlenout) {

  if (!content || !contentlen) {
    DBGPRINT("createpacket [invalid-params]\n");
    return 0;
  }

  int packetlen = getpacketlen(sizeof(Packet) + contentlen); //it is important total packetsize is divisible by the AES block size

  Packet * p = malloc(packetlen);
  if (!p) {
    DBGPRINT("createpacket [out-of-memory]\n");
    return 0;
  }

  memset(p, 0, packetlen);

  p -> len = contentlen;
  memcpy( & p -> content[0], content, contentlen);

  /*	char outhash[32];
  	memset(outhash,0,sizeof(outhash));*/
  //hash length and content

  getsha256hash((char * ) & p -> len, packetlen - sizeof(p -> checksum), (char * ) & p -> checksum);

  DBGPRINT("checksum: ");
  printbytes( (char*)& p -> checksum, 32);

  DBGPRINT("pre-encryption: ");
  printbytes((char*)p, packetlen);

  char * ciphertext = 0;
  int ciphertextlen = 0;
  //encrypt entire packet
  aesencrypt((char * ) p, packetlen, & ciphertext, & ciphertextlen);

  /*if(packetlen != ciphertextlen){
  	DBGPRINT("packetlen %d != ciphertextlen %d\n",packetlen,ciphertextlen);
  }*/

  memcpy(p, ciphertext, packetlen);
  free(ciphertext);

  DBGPRINT("post-encryption: ");
  printbytes((char*)p, packetlen);
  * packetlenout = packetlen;

  return p;
}

void freepacket(Packet * p) {
  free(p);
}

int decodepacket(Packet * p, int packetlen) {

  if (!p) {
    DBGPRINT("decodepacket [invalid-params]\n");
    return 0;
  }

  if (packetlen % AES_BLOCK_SIZE) {
    DBGPRINT("packet has been tampered with abort\n");
    return 0;
  }

  DBGPRINT("decrypting packet of size: %d\n", packetlen);
  //decrypt packet
  char * plaintext = 0;
  int plaintextlen = 0;
  aesdecrypt((char*)p, packetlen, & plaintext, & plaintextlen);

  /*if(plaintextlen!=packetlen){
  	DBGPRINT("plaintext %d != packetlen %d\n",plaintextlen,packetlen);
  }*/

  memcpy(p, plaintext, packetlen);
  free(plaintext);

  DBGPRINT("decrypted packet: ");
  printbytes((char*)p, packetlen);

  //verify hash
  char outhash[32];
  memset(outhash, 0, sizeof(outhash));

  getsha256hash((char * ) & p -> len, packetlen - sizeof(p -> checksum), outhash);
  /*DBGPRINT("decodedpacket checksum: ");
  printbytes(outhash,32);*/

  if (memcmp(outhash, & p -> checksum[0], sizeof(outhash))) {
    DBGPRINT("y u do dis :( [INVALID-HASH]\n");
    return 0;
  }

  /*DBGPRINT("2\n");*/

  //packet decrypted and validated
  return 1;
}

//assume inputlen divisible by aes blocklen
void aesencrypt(char * input, int inputlen, char ** ciphertext, int * ciphertextlen) {

  int dlen = inputlen + AES_BLOCK_SIZE, flen = 0;

  char * encbuffer = malloc(dlen);

  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();

  EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), 0, (void*)aes_key, (void*)aes_iv);

  //DBGPRINT("%p %p %d %p %d\n",ctx,encbuffer,dlen,input,inputlen); 
  EVP_EncryptUpdate(ctx, (void*)encbuffer, & dlen, (void*)input, inputlen);

  //DBGPRINT("2\n");

  EVP_EncryptFinal_ex(ctx, (void*)(encbuffer + dlen), & flen);

  //DBGPRINT("3\n");

  * ciphertext = encbuffer;
  * ciphertextlen = dlen + flen;

}

//assume block size divisible by aesblocksz
void aesdecrypt(char * input, int inputlen, char ** plaintext, int * plaintextlen) {

  int l = inputlen, flen = 0;

  char * decbuffer = malloc(inputlen);

  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();

  EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), 0, (void*)aes_key, (void*)aes_iv);

  EVP_DecryptUpdate(ctx, (void*)decbuffer, & l, (void*)input, inputlen);

  EVP_DecryptFinal_ex(ctx, (void*)(decbuffer + l), & flen);

  * plaintext = decbuffer;
  * plaintextlen = l + flen;

}

void getsha256hash(char * input, int inputlen, char * outhash) {

  //unsigned char hash[SHA256_DIGEST_LENGTH];

  SHA256_CTX sha;
  SHA256_Init( & sha);

  //char buffer[SHA256_DIGEST_LENGTH];

  SHA256_Update( & sha, input, inputlen);
  SHA256_Final((void*)outhash, & sha);
  /*char c[(SHA256_DIGEST_LENGTH*2)+1];
  memset(c,0,sizeof(c));
  char* p = c;
  for(int i=0;i<SHA256_DIGEST_LENGTH;i++){
  	int x = hash[i];
  	

  	//DBGPRINT("%02x\n",x);
  	sDBGPRINT(p,"%02x",x);
  	p+=2;

  }
  *p = '\n';*/

}

#ifdef CRYPTO_DBG

void printbytes(char * p, int bytes) {

  for (int i = 0; i < bytes; i++) {
    char x = p[i];
    DBGPRINT("%02hhx", x);

  }

  DBGPRINT("\n");
}

#else
	
	void printbytes(char* p, int bytes){
		return;
	}
#endif

#ifdef CRYPTO_SOLOCOMPILE

void fuzz(int times) {

  DBGPRINT("-4\n");

  int maxmsg = 1000;

  DBGPRINT("-3\n");

  void * space = malloc(maxmsg);
  int size = 0;

  DBGPRINT("-2\n");

  srandom(time(NULL));

  for (int i = 0; i < times; i++) {

    size = (random() % (maxmsg - sizeof(int) - 1)) + sizeof(int);

    memset(space, 0, size);

    DBGPRINT("-1\n");

    for (int j = 0; j < size / sizeof(int); j++) {

      ((int * ) space)[j] = rand();
    }

    DBGPRINT("0\n");
    int u;
    Packet * p = createpacket(space, size, & u);
    if (!p) {
      DBGPRINT(":( 1\n");
      return;
    }

    int receivedlen = getpacketlen(sizeof(Packet) + size);
    if (!decodepacket(p, receivedlen)) {
      printbytes( & p -> content, size);
      DBGPRINT(":( 2\n");
      return;
    }

    freepacket(p);
    DBGPRINT("success %d\n", i);

  }
}

void main() {

  SASRT(sizeof(Packet) == 32 + sizeof(int), "Packet structure alignment check failure\n");

  char * hello = "hello";

  DBGPRINT("hello: ");
  printbytes(hello, sizeof(hello) - 1);

  Packet * p = createpacket(hello, sizeof(hello) - 1);
  if (!p) {
    DBGPRINT(":( 1\n");
    return;
  }

  DBGPRINT("encrypted hello: ");
  printbytes( & p -> content, sizeof(hello) - 1);

  /*packet now encrypted and ready to send*/

  //in a normal scenario we would have the exact number of received bytes and wouldn't use this
  int receivedlen = getpacketlen(sizeof(Packet) + sizeof(hello) - 1);
  if (!decodepacket(p, receivedlen)) {
    printbytes( & p -> content, sizeof(hello));
    DBGPRINT(":( 2\n");
    return;
  }

  DBGPRINT("decrypted hello: ");
  printbytes( & p -> content, sizeof(hello) - 1);

  freepacket(p);

  fuzz(10);

}
#endif