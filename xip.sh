gcc -Wall -c cjson/cJSON.c  -lm
gcc -Wall -c libqqwry/qqwry.c
gcc -Wall -c xip.c   -levent -L /usr/local/libevent2/lib/ -I /usr/local/libevent2/include/
gcc -Wall -o xip qqwry.o cJSON.o xip.o   -levent -L /usr/local/libevent2/lib/ -I /usr/local/libevent2/include/ -lm -liconv

