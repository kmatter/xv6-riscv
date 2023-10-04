#include "kernel/types.h"
#include "user/user.h"
int main(int argc, char *argv[]) {
    int p[2];
    pipe(p); // p contains fds
    int pid = fork();
    if(pid == 0){
        printf("%d: child\n",getpid());
        exit(0);
    }
    pid = wait((int*)0);
    printf("child exited, pid %d",pid);
    exit(0);
};


