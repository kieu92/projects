#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void clean_up(FILE **fp_one, FILE **fp_two, char **init_atm, char **init_bank) {
    if (fp_one) {
        fclose(*fp_one);
    }
    if (fp_two) {
        fclose(*fp_two);
    }
    free(*init_atm);
    free(*init_bank);
}

int main (int argc, char *argv[]) {
    FILE *fp_one;
    FILE *fp_two;

    if (argc != 2) {
        printf("Usage:  init <filename>\n");

        return 62;
    } 
    else {
        char *init_atm;
        char *init_bank;
        int len = strlen(argv[1]); // argv[1] is the file name

        init_atm = (char*) malloc((len+4+1)*sizeof(char)); // +4 for ".atm" and +1 for "\0"
        init_bank = (char*) malloc((len+5+1)*sizeof(char)); // +5 for ".bank" and +1 for "\0"

        strcpy(init_atm, argv[1]);
        strcpy(init_bank, argv[1]);
        strcat(init_atm, ".atm");
        strcat(init_bank, ".bank");

        fp_one = fopen(init_atm, "r");
        fp_two = fopen(init_bank, "r");
    
        if (fp_one || fp_two) {
            printf("Error: one of the files already exists\n");
            clean_up(&fp_one, &fp_two, &init_atm, &init_bank);

            return 63;
        } 
        else {
            fp_one = fopen(init_atm, "w");
            fp_two = fopen(init_bank, "w");

            if (fp_one == NULL && fp_two == NULL) {
                printf("Error creating initialization files\n");
                free(init_atm);
                free(init_bank);
                
                return 64;
            } 
            else {
                printf("Successfully initialized bank state\n");
                clean_up(&fp_one, &fp_two, &init_atm, &init_bank);

                return 0;
            }
        }
    }
}
