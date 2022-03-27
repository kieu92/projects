/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char prompt[] = "ATM: ";

int main(int argc, char**argv)
{
    if (argc != 2) {
        printf("Error opening ATM initialization file\n");

        return 64;
    }

    char user_input[10000]; // changed 1000 to 10000 since thats whats used in atm.c
    FILE *fp;

    char sub_string[5];
    int file_len = strlen(argv[1]);
    memcpy(sub_string, &argv[1][file_len - 4], 5); // check for .atm

    if (strcmp(sub_string, ".atm") != 0 || (fp = fopen(argv[1],"r")) == NULL) {
        printf("Error opening ATM initialization file\n");

        return 64;
    }

    ATM *atm = atm_create();

    printf("%s", prompt);
    fflush(stdout);

    while (fgets(user_input, 10000,stdin) != NULL) 
    {
        atm_process_command(atm, user_input);
        printf("%s", prompt);
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
