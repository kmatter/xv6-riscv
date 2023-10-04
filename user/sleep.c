#include "kernel/types.h"
#include "user/user.h"
int main(int argc, char *argv[]) {
    if(argc != 2){
        printf("usage: sleep (secs)\n");
        exit(1);
    }
    int time = atoi(argv[1]);
    sleep(time);
    exit(0);
};

