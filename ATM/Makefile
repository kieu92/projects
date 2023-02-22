CC = gcc
SUPRESS = -w
CFLAGS = -fno-stack-protector -z execstack -Wall -Iutil -Iatm -Ibank -Irouter -I.

all: bin bin/atm bin/bank bin/router test bin/init


bin:
	mkdir -p bin

bin/atm : atm/atm-main.c atm/atm.c util/crypto.c
	${CC} ${CFLAGS} atm/atm.c atm/atm-main.c util/crypto.c -o bin/atm -lcrypto

bin/bank : bank/bank-main.c bank/bank.c util/list.c util/hash_table.c util/crypto.c
	${CC} ${CFLAGS} bank/bank.c bank/bank-main.c util/list.c util/hash_table.c util/crypto.c -o bin/bank -lcrypto

bin/router : router/router-main.c router/router.c
	${CC} ${CFLAGS} router/router.c router/router-main.c -o bin/router

test : util/list.c util/list_example.c util/hash_table.c util/hash_table_example.c
	${CC} ${CFLAGS} util/list.c util/list_example.c -o bin/list-test
	${CC} ${CFLAGS} util/list.c util/hash_table.c util/hash_table_example.c -o bin/hash-table-test

bin/init : init.c
	${CC} ${CFLAGS} init.c -o bin/init 

crypto_test: 
	${CC} ${SUPRESS} ${CFLAGS} util/crypto.c -o util/crypto_test -lcrypto 

clean:
	cd bin && rm -f atm bank router list-test hash-table-test init *.atm *.bank *.card
	rm -f *.atm *.bank *.card
