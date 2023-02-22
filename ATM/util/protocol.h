#ifndef PROTOCOL_GUARD
#define PROTOCOL_GUARD
#include <stdbool.h>


typedef enum _atm_to_bank_command{
    invalid = 0,
    isuservalid = 1,
    withdraw = 2,
    balance = 3,
    checkpin = 4,

} atm_to_bank_command;

typedef struct _atm_to_bank_request
{
    atm_to_bank_command cmd;
    union {
        struct {
            char username[251];
        } isuservalid ;

        struct {
            char username[251];
            int amt;
        } withdraw;

        struct {
            char username[251];
        } balance;

        struct {
            char username[251];
            char pin[5];
        } checkpin;

    };

} atm_to_bank_request;


typedef struct _atm_to_bank_response
{
    union {
        struct {
            bool uservalid; // 1 if user exists else 0
        } isuservalid ;

        struct {
            bool success; //0 if insufficient funds. else 1
        } withdraw;

        struct {
            bool error; //0 if there was an 0 error getting balance else 1 and balance = user balance
            int balance;
        } balance;

        struct {
            bool validpin; //1 if pin is valid for user
        } checkpin;

    };

} atm_to_bank_response;



#endif