#include "atm.h"
#include "ports.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "crypto.h"

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

ATM* atm_create()
{
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL)
    {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&atm->rtr_addr,sizeof(atm->rtr_addr));
    atm->rtr_addr.sin_family = AF_INET;
    atm->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&atm->atm_addr, sizeof(atm->atm_addr));
    atm->atm_addr.sin_family = AF_INET;
    atm->atm_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->atm_addr.sin_port = htons(ATM_PORT);
    bind(atm->sockfd,(struct sockaddr *)&atm->atm_addr,sizeof(atm->atm_addr));

    // Set up the protocol state
    // TODO set up more, as needed

    return atm;
}

void atm_free(ATM *atm)
{
    if(atm != NULL)
    {
        close(atm->sockfd);
        free(atm);
    }
}


ssize_t atm_send(ATM *atm, char *data, size_t data_len)
{

    #ifdef PACKETPROTOCOL

        return crypto_send(atm->sockfd,(struct sockaddr*) &atm->rtr_addr,data,data_len);
    #else
       // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));

    #endif

 
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{   

    #ifdef PACKETPROTOCOL

        return crypto_recv(atm->sockfd,data,max_data_len);

    #else

        // Returns the number of bytes received; negative on error
        return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);

    #endif
}

void atm_process_command(ATM *atm, char *command)
{
    /* A 256 bit key */
    unsigned char *key = (unsigned char *)"01234567890123456789012345678969";

    /* A 128 bit IV */
    unsigned char *iv = (unsigned char *)"0123456789012420";

    int n, length;
    char recvline[10000];
    char command_type[BUFSIZ];

    const char* pattern_one = "([a-zA-Z-]+)";
    const char* pattern_two = "begin-session ([a-zA-Z]+)";
    const char* pattern_three = "withdraw ([0-9]+)";
    const char* pattern_four = "balance";
    const char* pattern_five = "end-session";

    regex_t regex_one;
    regex_t regex_two;
    regex_t regex_three;
    regex_t regex_four;
    regex_t regex_five;

    regmatch_t pmatch_one[10];
    regmatch_t pmatch_two[10];
    regmatch_t pmatch_three[10];
    regmatch_t pmatch_four[10];
    regmatch_t pmatch_five[10];

    regcomp(&regex_one, pattern_one, REG_EXTENDED);
    regcomp(&regex_two, pattern_two, REG_EXTENDED);
    regcomp(&regex_three, pattern_three, REG_EXTENDED);
    regcomp(&regex_four, pattern_four, REG_EXTENDED);
    regcomp(&regex_five, pattern_five, REG_EXTENDED);

    if (regexec(&regex_one, command, 10, pmatch_one, REG_NOTBOL) != REG_NOMATCH) {
        length = pmatch_one[0].rm_eo - pmatch_one[0].rm_so;
        memcpy(command_type, command + pmatch_one[0].rm_so, length);
        command_type[length] = 0;

        //Check the input here:
        //Four possible commands: begin-session, withdraw, balance, end-session
        if (strcmp(command_type, "begin-session") == 0) { //takes arguments, so just compare the first part for now
            //Using regex documentation found at: http://web.archive.org/web/20160308115653/http://peope.net/old/regex.html
            //compile regex
            if (regexec(&regex_two, command, 10, pmatch_two, REG_NOTBOL) != REG_NOMATCH) {
                FILE *fp;
                char *user_name;
                char *check_card;
                unsigned char message[10000];
                unsigned char decryptedtext[128];
                unsigned char ciphertext[128];

                int decryptedtext_len;
                int ciphertext_len;

                length = pmatch_two[1].rm_eo - pmatch_two[1].rm_so;
                // Valid user names are at most 250 characters
                if (length > 250){
                    goto createuser_usageerror;
                }
                user_name = (char*) malloc((length+1)*sizeof(char));
                memcpy(user_name, command + pmatch_two[1].rm_so, length);
                user_name[length] = 0;

                check_card = (char*) malloc((length+5+1)*sizeof(char));
                strcpy(check_card, user_name);
                strcat(check_card, ".card");

                strcpy((char*) message, "isuservalid ");
                strcat((char*) message, user_name);
                message[strlen((char*) message)] = 0;

                /* Encrypt the message */
                ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);

                // atm_send(atm, (char*) message, strlen((char*) message));
                atm_send(atm, (char*) ciphertext, ciphertext_len);
                n = atm_recv(atm,recvline,10000);
                recvline[n]=0;
                decryptedtext_len = decrypt((unsigned char*) recvline, n, key, iv, decryptedtext);
                decryptedtext[decryptedtext_len] = '\0';
                
                if (strcmp((char*) decryptedtext, "0") == 0) {
                    printf("No such user\n");
                } 
                else if ((fp = fopen(check_card, "r")) == NULL) {
                    printf("Not authorized\n\n");

                    if (fp) {
                        fclose(fp);
                    }

                    return;
                }
                else {
                    printf("PIN? ");
                    char pin[BUFSIZ];
                    fgets(pin,BUFSIZ,stdin);
                    if (strlen(pin) != 5) {
                        goto pin_error;
                    }
                    pin[4] = 0;

                    memset(message, 0, sizeof(message));
                    strcpy((char*) message, "checkpin ");
                    strcat((char*) message, user_name);
                    strcat((char*) message, " ");
                    strcat((char*) message, pin);
                    message[strlen((char*) message)] = 0;

                    /* Encrypt the message */
                    ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);

                    // atm_send(atm, (char*) message, strlen((char*) message));
                    atm_send(atm, (char*) ciphertext, ciphertext_len);
                    n = atm_recv(atm,recvline,10000);
                    recvline[n]=0;
                    decryptedtext_len = decrypt((unsigned char*) recvline, n, key, iv, decryptedtext);
                    decryptedtext[decryptedtext_len] = '\0';

                    if (strcmp((char*) decryptedtext, "0") == 0) {
                        pin_error:
                        printf("Not authorized\n");
                    } 
                    else {
                        printf("Authorized\n\n");

                        while (1) {
                            char user_input[10000];
                            char cmd_type[BUFSIZ];

                            const char* pattern_cmd = "([a-zA-Z-]+)";
                            const char* pattern_withdraw = "withdraw ([0-9]+)";
                            const char* pattern_balance = "balance";
                            const char* pattern_session = "begin-session ([a-zA-Z]+)";

                            regex_t regex_cmd;
                            regex_t regex_withdraw;
                            regex_t regex_balance;
                            regex_t regex_session;

                            regmatch_t pmatch_cmd[10];
                            regmatch_t pmatch_withdraw[10];
                            regmatch_t pmatch_balance[10];
                            regmatch_t pmatch_session[10];

                            regcomp(&regex_cmd, pattern_cmd, REG_EXTENDED);
                            regcomp(&regex_withdraw, pattern_withdraw, REG_EXTENDED);
                            regcomp(&regex_balance, pattern_balance, REG_EXTENDED);
                            regcomp(&regex_session, pattern_session, REG_EXTENDED);

                            printf("ATM (%s): ", user_name);
                            fflush(stdout);
                            fgets(user_input, 10000,stdin);

                            if (regexec(&regex_cmd, user_input, 10, pmatch_cmd, REG_NOTBOL) != REG_NOMATCH) {
                                length = pmatch_cmd[0].rm_eo - pmatch_cmd[0].rm_so;
                                memcpy(cmd_type, user_input + pmatch_cmd[0].rm_so, length);
                                cmd_type[length] = 0;

                                if (strcmp(cmd_type, "end-session") == 0) {
                                    printf("User logged out\n");

                                    break;
                                }
                                else if (strcmp(cmd_type, "balance") == 0) {
                                    if (regexec(&regex_balance, user_input, 10, pmatch_balance, REG_NOTBOL) != REG_NOMATCH) {
                                        memset(message, 0, sizeof(message));
                                        strcpy((char*) message, "balance ");
                                        strcat((char*) message, user_name);
                                        message[strlen((char*) message)] = 0;

                                        /* Encrypt the message */
                                        ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);

                                        // atm_send(atm, (char*) message, strlen((char*) message));
                                        atm_send(atm, (char*) ciphertext, ciphertext_len);
                                        n = atm_recv(atm,recvline,10000);
                                        recvline[n]=0;
                                        decryptedtext_len = decrypt((unsigned char*) recvline, n, key, iv, decryptedtext);
                                        decryptedtext[decryptedtext_len] = '\0';

                                        printf("$%s\n", (char*) decryptedtext);
                                    }
                                    else {
                                        printf("Usage: balance\n");
                                    }
                                }
                                else if (strcmp(cmd_type, "begin-session") == 0) {
                                    if (regexec(&regex_session, user_input, 10, pmatch_session, REG_NOTBOL) != REG_NOMATCH) {
                                        printf("A user is already logged in\n");
                                    }
                                    else {
                                        printf("Usage: begin-session <user-name>\n");
                                    }
                                }
                                else if (strcmp(cmd_type, "withdraw") == 0) {
                                    if (regexec(&regex_withdraw, user_input, 10, pmatch_withdraw, REG_NOTBOL) != REG_NOMATCH) {
                                        length = pmatch_withdraw[1].rm_eo - pmatch_withdraw[1].rm_so;
                                        // at most what can be represented with an int
                                        if (length > 9){
                                            printf("Insufficient funds\n");
                                        }
                                        else {
                                            char *amt = (char*) malloc((length+1)*sizeof(char));
                                            memcpy(amt, user_input + pmatch_withdraw[1].rm_so, length);
                                            amt[length] = 0;

                                            memset(message, 0, sizeof(message));
                                            strcpy((char*) message, "withdraw ");
                                            strcat((char*) message, user_name);
                                            strcat((char*) message, " ");
                                            strcat((char*) message, amt);
                                            message[strlen((char*) message)] = 0;

                                            /* Encrypt the message */
                                            ciphertext_len = encrypt (message, strlen ((char *)message), key, iv, ciphertext);

                                            // atm_send(atm, (char*) message, strlen((char*) message));
                                            atm_send(atm, (char*) ciphertext, ciphertext_len);
                                            n = atm_recv(atm,recvline,10000);
                                            recvline[n]=0;
                                            decryptedtext_len = decrypt((unsigned char*) recvline, n, key, iv, decryptedtext);
                                            decryptedtext[decryptedtext_len] = '\0';

                                            if (strcmp((char*) decryptedtext, "0") == 0) {
                                                printf("Insufficient funds\n");
                                            } 
                                            else {
                                                printf("$%s dispensed\n", amt);
                                            }
                                        }
                                    }
                                    else {
                                        printf("Usage: withdraw <amt>\n");
                                    }
                                }
                                else {
                                    printf("Invalid command\n");
                                }
                            }
                            else {
                                printf("Invalid command\n");
                            }

                            printf("\n");
                        }
                    }

                    printf("\n");
                    return;
                }
            }
            else {
                createuser_usageerror:
                printf("Usage: begin-session <user-name>\n");
            }
        }
        else if (strcmp(command_type, "withdraw") == 0) {
            if (regexec(&regex_three, command, 10, pmatch_three, REG_NOTBOL) != REG_NOMATCH) {
                printf("No user logged in\n");
            }
            else {
                printf("Usage: withdraw <amt>\n");
            }
        }
        else if (strcmp(command_type, "balance") == 0) {
            if (regexec(&regex_four, command, 10, pmatch_four, REG_NOTBOL) != REG_NOMATCH) {
                printf("No user logged in\n");
            }
            else {
                printf("Usage: balance\n");
            }

        }
        else if (strcmp(command_type, "end-session") == 0) {
            if (regexec(&regex_five, command, 10, pmatch_five, REG_NOTBOL) != REG_NOMATCH) {
                printf("No user logged in\n");
            }
        }
        else {
            //Invalid command
            printf("Invalid command\n");
        }
    }
    else {
        //Invalid command
        printf("Invalid command\n");
    }

    printf("\n");

    regfree(&regex_one);
    regfree(&regex_two);
    regfree(&regex_three);
    regfree(&regex_four);
    regfree(&regex_five);

	/*
	 * The following is a toy example that simply sends the
	 * user's command to the bank, receives a message from the
	 * bank, and then prints it to stdout.
	 */

	
    // char recvline[10000];
    // int n;

    // atm_send(atm, command, strlen(command));
    // n = atm_recv(atm,recvline,10000);
    // recvline[n]=0;
    // fputs(recvline,stdout);
}
