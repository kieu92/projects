#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <regex.h>
#include "hash_table.h"
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "crypto.h"

int num_incorrect_pin_attempts = 0; //vulnerability 4
const int MAX_INCORRECT_PIN_ATTEMPTS = 5; //vulnerability 4

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        printf("Error 1\n");

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        printf("Error 2\n");

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        printf("Error 3\n");

    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        printf("Error 4\n");

    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        printf("Error 1\n");

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        printf("Error 2\n");

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        printf("Error 3\n");
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        printf("Error 4\n");
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

Bank* bank_create()
{
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    }

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));

    // Set up the protocol state
    // TODO set up more, as needed

    return bank;
}

void bank_free(Bank *bank)
{
    if (bank != NULL)
    {
        close(bank->sockfd);
        free(bank);
    }
}


ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{


    #ifdef PACKETPROTOCOL
        return crypto_send(bank->sockfd,(struct sockaddr*) &bank->rtr_addr,data,data_len);
    #else

    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));

    #endif
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{

    #ifdef PACKETPROTOCOL

    return crypto_recv(bank->sockfd,data,max_data_len);

    #else

    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);

    #endif
}

void bank_process_local_command(Bank *bank, char *command, size_t len, HashTable *ht) {
    int length;
    char command_type[BUFSIZ];
    int user_cap = 1000;

    const char* pattern_one = "([a-zA-Z-]+)";
    const char* pattern_two = "create-user ([a-zA-Z]+) ([0-9][0-9][0-9][0-9]) ([0-9]+)";
    const char* pattern_three = "deposit ([a-zA-Z]+) ([0-9]+)";
    const char* pattern_four = "balance ([a-zA-Z]+)";

    regex_t regex_one;
    regex_t regex_two;
    regex_t regex_three;
    regex_t regex_four;

    regmatch_t pmatch_one[10];
    regmatch_t pmatch_two[10];
    regmatch_t pmatch_three[10];
    regmatch_t pmatch_four[10];

    regcomp(&regex_one, pattern_one, REG_EXTENDED);
    regcomp(&regex_two, pattern_two, REG_EXTENDED);
    regcomp(&regex_three, pattern_three, REG_EXTENDED);
    regcomp(&regex_four, pattern_four, REG_EXTENDED);

    if (regexec(&regex_one, command, 10, pmatch_one, REG_NOTBOL) != REG_NOMATCH) {
        length = pmatch_one[0].rm_eo - pmatch_one[0].rm_so;
        memcpy(command_type, command + pmatch_one[0].rm_so, length);
        command_type[length] = 0;

        if (strcmp(command_type, "create-user") == 0) {            
            if (hash_table_size(ht) == user_cap){
                printf("Too many users created already\n");
            } else if (regexec(&regex_two, command, 10, pmatch_two, REG_NOTBOL) != REG_NOMATCH) {
                char *user_name;
                char *balance;
                char *pin;

                length = pmatch_two[1].rm_eo - pmatch_two[1].rm_so;
                // Valid user names are at most 250 characters
                if (length > 250){
                    goto createuser_usageerror;
                }
                user_name = (char*) malloc((length+1)*sizeof(char));
                memcpy(user_name, command + pmatch_two[1].rm_so, length);
                user_name[length] = 0;

                length = pmatch_two[2].rm_eo - pmatch_two[2].rm_so;
                // four-digit number (regex already checks but why not)
                if (length != 4){
                    goto createuser_usageerror;
                }
                pin = (char*) malloc((length+1)*sizeof(char));
                memcpy(pin, command + pmatch_two[2].rm_so, length);
                pin[length] = 0;

                length = pmatch_two[3].rm_eo - pmatch_two[3].rm_so;
                // at most what can be represented with an int
                if (length > 9){
                    goto createuser_usageerror;
                }
                balance = (char*) malloc((length+1)*sizeof(char));
                memcpy(balance, command + pmatch_two[3].rm_so, length);
                balance[length] = 0;

                if (hash_table_find(ht, user_name) == NULL) {

                    FILE *fp;
                    char *user_card;
                    user_card = (char*) malloc((strlen(user_name)+7+1)*sizeof(char));
                    strcpy(user_card, "./");
                    strcat(user_card, user_name);
                    strcat(user_card, ".card"); // creating <user_name>.card in the current directory

                    fp = fopen(user_card, "w");

                    if (fp == NULL) {
                        printf("Error creating card file for user %s\n", user_name);
                    }
                    else {
                        char *user_pin;
                        user_pin = (char*) malloc((strlen(user_name)+6+1)*sizeof(char));
                        strcpy(user_pin, user_name);
                        strcat(user_pin, "'s Pin");

                        hash_table_add(ht, user_name, balance); // storing user's name and balance
                        hash_table_add(ht, user_pin, pin);      // storing user's pin

                        printf("Created user %s\n", user_name);

                        if (fp) {
                            fclose(fp);
                        }
                    }
                } 
                else {
                    printf("Error:  user %s already exists\n", user_name);
                }
            }
            else {
                createuser_usageerror:
                printf("Usage:  create-user <user-name> <pin> <balance>\n");
            }
        }
        else if (strcmp(command_type, "deposit") == 0) {
            if (regexec(&regex_three, command, 10, pmatch_three, REG_NOTBOL) != REG_NOMATCH) {
                char *user_name;
                char *amt;

                length = pmatch_three[1].rm_eo - pmatch_three[1].rm_so;

                // Valid user names are at most 250 characters.)
                if (length > 250) {
                    goto deposit_usageerror;
                }
                user_name = (char*) malloc((length+1)*sizeof(char));
                memcpy(user_name, command + pmatch_three[1].rm_so, length);
                user_name[length] = 0;

                length = pmatch_three[2].rm_eo - pmatch_three[2].rm_so;

                // at most what can be represented with an int
                if (length > 9) {
                    goto deposit_usageerror;
                }
                amt = (char*) malloc((length + 1)*sizeof(char));
                memcpy(amt, command + pmatch_three[2].rm_so, length);
                amt[length] = 0;

                if (hash_table_find(ht, user_name) == NULL) {
                    printf("No such user\n");
                }
                else {
                    int total = atoi(hash_table_find(ht, user_name)) + atoi(amt);

                    if ((INT_MAX - total) < 0){
                      printf("Too rich for this program\n");
                    }
                    else {
                        char *new_bal = (char*) malloc((length+1)*sizeof(char));;
                        sprintf(new_bal, "%d", total);
                        hash_table_del(ht, user_name);
                        hash_table_add(ht, user_name, new_bal);

                        printf("$%d added to %s's account\n", atoi(amt), user_name);
                    }
                }
            }
            else {
                deposit_usageerror:
                printf("Usage:  deposit <user-name> <amt>\n");
            }
        }
        else if (strcmp(command_type, "balance") == 0) {
            if (regexec(&regex_four, command, 10, pmatch_four, REG_NOTBOL) != REG_NOMATCH) {
                char user_name[251];

                length = pmatch_four[1].rm_eo - pmatch_four[1].rm_so;
                if (length > 250){
                    goto balance_usageerror;
                }
                memcpy(user_name, command + pmatch_four[1].rm_so, length);
                user_name[length] = 0;

                if (hash_table_find(ht, user_name) == NULL) {
                    printf("No such user\n");
                }
                else {
                    printf("$%s\n", (char *) hash_table_find(ht, user_name));
                }
            }
            else {
                balance_usageerror:
                printf("Usage:  balance <user-name>\n");
            }
        }
        else {
            printf("Invalid command\n");
        }

    } else {
        printf("Invalid command\n");
    }

    printf("\n");

    regfree(&regex_one);
    regfree(&regex_two);
    regfree(&regex_three);
    regfree(&regex_four);
}

void bank_process_remote_command(Bank *bank, char *command, size_t len, HashTable *ht)
{
     // TODO: Implement the bank side of the ATM-bank protocol

    /* A 256 bit key */
    unsigned char *key = (unsigned char *)"01234567890123456789012345678969";

    /* A 128 bit IV */
    unsigned char *iv = (unsigned char *)"0123456789012420";

    unsigned char decryptedtext[128];
    unsigned char ciphertext[128];
    unsigned char message[128];

    int decryptedtext_len;
    int ciphertext_len;

    command[len]=0;
    decryptedtext_len = decrypt((unsigned char*) command, len, key, iv, decryptedtext);
    decryptedtext[decryptedtext_len] = '\0';

    const char* pattern_one = "isuservalid ([a-zA-Z]+)";
    const char* pattern_two = "checkpin ([a-zA-Z]+) ([0-9][0-9][0-9][0-9])";
    const char* pattern_three = "balance ([a-zA-Z]+)";
    const char* pattern_four = "withdraw ([a-zA-Z]+) ([0-9]+)";

    regex_t regex_one;
    regex_t regex_two;
    regex_t regex_three;
    regex_t regex_four;

    regmatch_t pmatch_one[10];
    regmatch_t pmatch_two[10];
    regmatch_t pmatch_three[10];
    regmatch_t pmatch_four[10];

    regcomp(&regex_one, pattern_one, REG_EXTENDED);
    regcomp(&regex_two, pattern_two, REG_EXTENDED);
    regcomp(&regex_three, pattern_three, REG_EXTENDED);
    regcomp(&regex_four, pattern_four, REG_EXTENDED);

    int length;

    if (regexec(&regex_one, (char*) decryptedtext, 10, pmatch_one, REG_NOTBOL) != REG_NOMATCH) {
        char *user_name;

        length = pmatch_one[1].rm_eo - pmatch_one[1].rm_so;
        user_name = (char*) malloc((length+1)*sizeof(char));
        memcpy(user_name, (char*) decryptedtext + pmatch_one[1].rm_so, length);
        user_name[length] = 0;

        if (hash_table_find(ht, user_name) == NULL) {
            memset(message, 0, sizeof(message));
            strcpy((char*) message, "0");
            /* Encrypt the message */
            ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
            bank_send(bank, (char*) ciphertext, ciphertext_len);
        }
        else {
            memset(message, 0, sizeof(message));
            strcpy((char*) message, "1");
            /* Encrypt the message */
            ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
            bank_send(bank, (char*) ciphertext, ciphertext_len);
        }
    }
    else if (regexec(&regex_two, (char*) decryptedtext, 10, pmatch_two, REG_NOTBOL) != REG_NOMATCH) {
        char *pin;
        char *user_name;

        length = pmatch_two[1].rm_eo - pmatch_two[1].rm_so;
        user_name = (char*) malloc((length+1)*sizeof(char));
        memcpy(user_name, (char*) decryptedtext + pmatch_two[1].rm_so, length);
        user_name[length] = 0;

        length = pmatch_two[2].rm_eo - pmatch_two[2].rm_so;
        pin = (char*) malloc((length+1)*sizeof(char));
        memcpy(pin, (char*) decryptedtext + pmatch_two[2].rm_so, length);
        pin[length] = 0;

        if (hash_table_find(ht, user_name) == NULL) {
            memset(message, 0, sizeof(message));
            strcpy((char*) message, "0");
            /* Encrypt the message */
            ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
            bank_send(bank, (char*) ciphertext, ciphertext_len);
        }
        else {
            char check_user_pin[strlen(user_name) + 6 + 1];
            strcpy(check_user_pin, user_name);
            strcat(check_user_pin, "'s Pin");

            if (strcmp((char *) hash_table_find(ht, check_user_pin), pin) != 0 || num_incorrect_pin_attempts >= MAX_INCORRECT_PIN_ATTEMPTS) {//vulnerability 4 - hard lock them out
                //pin is incorrect
                //increment the number of incorrect pins
                num_incorrect_pin_attempts++;//vulnerability 4
                memset(message, 0, sizeof(message));
                strcpy((char*) message, "0");
                /* Encrypt the message */
                ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
                bank_send(bank, (char*) ciphertext, ciphertext_len);
            }
            else {
                //pin is correct
                //reset the number of incorrect pin attempts
                num_incorrect_pin_attempts = 0;//vulnerability 4
                memset(message, 0, sizeof(message));
                strcpy((char*) message, "1");
                /* Encrypt the message */
                ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
                bank_send(bank, (char*) ciphertext, ciphertext_len);
            }
        }
    }
    else if (regexec(&regex_three, (char*) decryptedtext, 10, pmatch_three, REG_NOTBOL) != REG_NOMATCH) {
        char *user_name;

        length = pmatch_three[1].rm_eo - pmatch_three[1].rm_so;
        if (length > 250) {
            memset(message, 0, sizeof(message));
            strcpy((char*) message, "0");
            /* Encrypt the message */
            ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
            bank_send(bank, (char*) ciphertext, ciphertext_len);

            return;
        }
        user_name = (char*) malloc((length+1)*sizeof(char));
        memcpy(user_name, (char*) decryptedtext + pmatch_three[1].rm_so, length);
        user_name[length] = 0;

        memset(message, 0, sizeof(message));
        strcpy((char*) message, (char *) hash_table_find(ht, user_name));
        /* Encrypt the message */
        ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
        bank_send(bank, (char*) ciphertext, ciphertext_len);
    }
    else if (regexec(&regex_four, (char*) decryptedtext, 10, pmatch_four, REG_NOTBOL) != REG_NOMATCH) {
        char *user_name;
        char *amt;

        length = pmatch_four[1].rm_eo - pmatch_four[1].rm_so;
        user_name = (char*) malloc((length+1)*sizeof(char));
        memcpy(user_name, (char*) decryptedtext + pmatch_four[1].rm_so, length);
        user_name[length] = 0;

        length = pmatch_four[2].rm_eo - pmatch_four[2].rm_so;
        amt = (char*) malloc((length+1)*sizeof(char));
        memcpy(amt, (char*) decryptedtext + pmatch_four[2].rm_so, length);
        amt[length] = 0;

        if (hash_table_find(ht, user_name) == NULL) {
            memset(message, 0, sizeof(message));
            strcpy((char*) message, "0");
            /* Encrypt the message */
            ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
            bank_send(bank, (char*) ciphertext, ciphertext_len);
        }
        else {
            if (atoi((char *) hash_table_find(ht, user_name)) < atoi(amt)) { //prevent the user from withdrawing more than is in their acct - Vulnerability 6
                memset(message, 0, sizeof(message));
                strcpy((char*) message, "0");
                /* Encrypt the message */
                ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
                bank_send(bank, (char*) ciphertext, ciphertext_len);
            }
            else {
                memset(message, 0, sizeof(message));
                strcpy((char*) message, "1");
                /* Encrypt the message */
                ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);
                bank_send(bank, (char*) ciphertext, ciphertext_len);

                int total = atoi(hash_table_find(ht, user_name)) - atoi(amt);
                char *new_bal = (char*) malloc((length+1)*sizeof(char));;
                sprintf(new_bal, "%d", total);
                hash_table_del(ht, user_name);
                hash_table_add(ht, user_name, new_bal);
            }
        }
    }
    else {
        printf("Communication error\n");
    }
	
    // char sendline[1000];
    // command[len]=0;
    // sprintf(sendline, "Bank got: %s", command);
    // bank_send(bank, sendline, strlen(sendline));
    // printf("Received the following:\n");
    // fputs(command, stdout);
}
