# Atm Design Document
Project Team 5:
Andrew Bash,
Amogh Giri,
Maverick Kieu,
Isaac Rattey

## Protocol Overview

The first step in providing security to the bank is by ensuring safety from the start when creating an account for a user. At this point, attacks can happen from user inputs. The bank will only allow valid inputs when creating a user. Our system does this by checking to make sure that the username is at most 250 characters, the pin can only be 4 digits, and the balance has to be a number that is not bigger than INT_MAX. After creating a user, the commands `deposit <user-name> <amt>` and `balance <user-name>` will be operational for that user and the bank will continue to check for valid inputs before executing. On the ATM side, the requirements for a user to be in session is for the user to have their card (represented by XXX.card) and pin number. This simulates in real life where the ATM will read a user's XXX.card and then prompt for the pin. The ATM inputs that the user provides will also be validated by the atm process to prevent the client side from sending the server invalid input. We provided security to the communication between the ATM and the bank by encrypting every message that is sent. When the ATM or the bank receives a message, it decrypts the message and then processes it. If an attacker is able to inspect the message, they would not know the communication protocol between the bank and the ATM without looking at the source code. For example, when the bank receives a message from the ATM, it will go through the process of decrypting. It is then condensed into generic strings that the bank server will later process. Next, to ensure more security these strings are passed to the crypto module and turned into packet structures. The packet structures are encrypted with a hardcoded AES key and include a checksum, the length, and the data. When the bank process receives the packet it decrypts, validates the checksum, revalidates, and processes the strings.

## Vulnerability 1: Buffer Overflow

A common programming error vulnerability is buffer overflows. This could occur if we do not restrict the user inputted commands. In order to defend against this attack, we verify the length of inputted commands, and verify that they fit the format of our accepted commands using regular expressions. Using regular expressions we can easily filter out commands that do not fit our expected formats, and we can check that the length does not exceed the length of the buffer that we are storing them in. We also check the length of usernames to ensure that 

## Vulnerability 2: On-the-wire attack (Packet Integrity)

Simply encrypting the packet is not enough. If an attacker was to determine which bytes correspond to bank balance, for example, an attacker could modify the bank balance to “random” numbers. To prevent this we include a checksum of not only the data sent but also the length of data sent. This way only data that matches the checksum is processed.

## Vulnerability 3: Passive wiretapping (Packet Encryption)

Packet integrity is also useless without encryption. An attacker could simply forge a message and include the checksum of that message at the correct location in the packet. To prevent this we encrypt the entire packet including the checksum. That way an attacker’s message must be encrypted with our hardcoded AES key to bypass this protection. However, in our threat model the attacker cannot read process memory. This protection may be more difficult to bypass even with a hardcoded key.

## Vulnerability 4: Brute force pin attack

This vulnerability occurs if we do not restrict the number of login attempts. Without this restriction, a malicious user could brute force user pins by attempting different pin numbers until one is correct. In order to prevent this attack, we will restrict the login process so that atm users can only attempt to login 5 times before they are locked out. Since the atm process can be shut down, we include this defense on the bank side so that a malicious user could not attempt 5 pins, reboot the atm, attempt 5 pins, and so on in a loop until they guess correctly. Once the malicious user attempts 5 pins, the bank is disabled to new logins until a bank operator manually clears the attempts. The number of pin guesses remaining is also reset if the user inputs the correct pin within their first 5 attempts.

## Vulnerability 5: Denial of Service

The vulnerability in this scenario is when a user tries to create too many users and cause a memory overflow in the bank. Hence, to prevent this, we set a cap on the number of users that can be created with the bank. We have set this value to 1000 as of now. Every time there is a create user command, we check to see if the number of users already in the hash table is less than the user limit. If it is, we proceed as normal, else we deny them the service of creating a user in the bank.

## Vulnerability 6: Excessive withdrawal

This vulnerability would be evident if we do not check the amount that the atm user requests to withdraw from their bank account. We need to verify that the amount they are attempting to withdraw is less than or equal to their current bank balance. Otherwise, this vulnerability would allow their account balance be below 0 and the user would be able to effectively steal money from the bank.

## Vulnerability 7: Integer overflow

The project requires that the program checks for an integer overflow when trying to deposit too much money. However, this fails to account for when a user has already deposited INT_MAX - 1 amount of money and then tries to deposit additional money. This would result in the balance being an integer that is overflowed and incorrectly modified and could eventually lead to a buffer overflow. To prevent this vulnerability, we ensure that we only deposit the requested amount of money if its value combined with the current balance doesn’t result in an integer overflow. This is done by checking whether `(INT_MAX - (deposit_amount + balance)) < 0)` is true. If the deposit will cause an integer overflow, then the bank will not allow it and will return the appropriate response.
