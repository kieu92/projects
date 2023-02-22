/* 
 * The main program for the Bank.
 *
 * You are free to change this as necessary.
 */

#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include "bank.h"
#include "ports.h"
#include "hash_table.h"

static const char prompt[] = "BANK: ";

int main(int argc, char**argv)
{
    if (argc != 2) {
        printf("Error opening bank initialization file\n");

        return 64;
    }

    int n;
    char sendline[1000];
    char recvline[1000];
    FILE *fp;

    char sub_string[6];
    int file_len = strlen(argv[1]);
    memcpy(sub_string, &argv[1][file_len - 5], 6); // check for .bank

    if (strcmp(sub_string, ".bank") != 0 || (fp = fopen(argv[1],"r")) == NULL) {
        printf("Error opening bank initialization file\n");

        return 64;
    }

    Bank *bank = bank_create();
    HashTable *ht = hash_table_create(BUFSIZ);

    printf("%s", prompt);
    fflush(stdout);

   while(1)
   {
       fd_set fds;
       FD_ZERO(&fds);
       FD_SET(0, &fds);
       FD_SET(bank->sockfd, &fds);
       select(bank->sockfd+1, &fds, NULL, NULL, NULL);

       if(FD_ISSET(0, &fds))
       {
           fgets(sendline, 10000,stdin);
           bank_process_local_command(bank, sendline, strlen(sendline), ht);
           printf("%s", prompt);
           fflush(stdout);
       }
       else if(FD_ISSET(bank->sockfd, &fds))
       {
           n = bank_recv(bank, recvline, 10000);
           bank_process_remote_command(bank, recvline, n, ht);
       }
   }

   return EXIT_SUCCESS;
}
