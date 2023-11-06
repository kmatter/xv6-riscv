#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
// stdin: 0 stdout: 1

typedef struct command {
    char* cmd;
    char ** argv;
    char redir;
    char* redir_file;
    int* pipe_i;
    int pipe_read;
    int pipe_write;
    int is_exit;
    int is_cd;

}command;

char* gettok(char* raw,char* wordbuf){ // future reference, you know when it's done when wordbuf is blank when called
    if(raw[0] == 0 || (raw[0] == '\n')){return 0;}
    if(raw[0] == '<' || raw[0] == '>' || raw[0] == '|'){
        wordbuf[0] = raw[0];
        return raw + 1;
    }
    char* b = 0;
    char* e = 0;
    int i = 0;
    for(;raw[i] != 0;i++ ){
        if(raw[i] != ' ' && b == 0){
            b = &raw[i];
            if(raw[i] == '<' || raw[i] == '>' || raw[i] == '|'){
                e = b;
                break;
            }
        }if((raw[i+1] == ' ' && b != 0) || (raw[i+1] == 0) || (b !=0 && ((raw[i+1] == '\n') || raw[i+1] == '<' || raw[i+1] == '>' || raw[i+1] == '|'))) // stop if the next char is blank and we haven't reached the end, or the next is null, or the next is a newline, or if the next is one of the special chars
        {
            e = &raw[i];
            break;
        }
    }
    if(b == 0 || e == 0){return 0;}
    for(int f = 0; b!= e+1; f++){ // could replace by memcpy probably
        wordbuf[f] = *b;
        b++;
    }
    return raw + i +1;
}

int execcmd(command* cmd){ // 0 read 1 write
    if(cmd == 0){
        return -2;
    }
    if(cmd->is_exit){
        return -1;
    }
    if(cmd->is_cd){
        chdir(cmd->argv[1]);
        return -2;
    }
    int pid = fork();
    if(pid == 0){
        if(cmd->pipe_write == 1 && cmd->pipe_read == 1){
            close(1);
            dup(*(cmd->pipe_i+1));
            close(0);
            dup(*(cmd->pipe_i));
        }
        else if(cmd->pipe_write == 1){
            close(1);
            dup(*(cmd->pipe_i+1));
            close(*cmd->pipe_i);
            close(*(cmd->pipe_i+1));
        }
        else if(cmd->pipe_read == 1){
            close(0);
            dup(*(cmd->pipe_i));
            close(*cmd->pipe_i);
            close(*(cmd->pipe_i+1));
        }
        if(cmd->redir == '<'){
            close(0);
            int fd = open(cmd->redir_file,O_RDONLY);
            dup(fd);
        }else if(cmd->redir == '>'){
            close(1);
            int fd = open(cmd->redir_file,O_WRONLY | O_CREATE);
            dup(fd);
        }
        exec(cmd->cmd,cmd->argv);
    }
    return pid;
}

char* getcmd(char* buf, command *n_cmd, command* last_command){
    char* offset;
    offset = gettok(buf,n_cmd->cmd);
    if(strcmp(n_cmd->cmd,"cd") == 0){
        n_cmd->is_cd = 1;
    }
    if(strcmp(n_cmd->cmd,"exit") == 0){
        n_cmd->is_exit = 1;
    }
    int i = 1;
    if(last_command != 0){
        if(last_command->pipe_write == 1){ // ok this is a x | y and we're y so we need to read from the pipe
            n_cmd->pipe_read = 1;
            n_cmd->pipe_i = last_command->pipe_i;
        }
    }
    while(offset != 0){
        char* v = malloc(50);
        offset = gettok(offset,v);
        if(offset == 0){break;}
        if(v[0] == '|'){ // pipin' time. this is an x | y and we're x so we need to write to the pipe (and set it up)
            pipe(n_cmd->pipe_i);
            n_cmd->pipe_write = 1;
            break;
        }
        if(v[0] == '<' || v[0] == '>'){
            n_cmd->redir = v[0];
            continue;
        }
        if(n_cmd->redir == '<' || n_cmd->redir == '>'){
            n_cmd->redir_file = v;
            break;
        }
        n_cmd->argv[i] = v;
        i++;
    }
    n_cmd->argv[0] = n_cmd->cmd;
    n_cmd->argv[i] = 0; // need to null terminate it
    return offset;
}

command* initcommand(){
    command *ret = malloc(sizeof(command));
    ret->cmd = malloc(50);
    ret->argv = malloc(500);
    ret->redir_file = malloc(50);
    ret->pipe_i = malloc(16);
    ret->pipe_read = 0;
    ret->pipe_write = 0;
    ret->is_cd = 0;
    ret->is_exit = 0;
    ret->redir = ' ';
    return ret;
}

void destroycommand(command* c){
    free(c->cmd);
    free(c->argv);
    free(c->redir_file);
    free(c->pipe_i);
}

void runcmd(char* buf){
    command** command_buffer = malloc(500);
    int* pidbuf = malloc(500);
    int c = 1;
    int exit_after_done = 0;
    char* offset = buf;
    command *first = initcommand();
    offset = getcmd(offset,first,0);
    command_buffer[0] = first;
    for(;offset!=0;c++){
        command* new_com = initcommand();
        offset = getcmd(offset,new_com,command_buffer[c-1]);
        command_buffer[c] = new_com;
    }
    for(int i = 0;i<c;i++){
        pidbuf[i] = execcmd(command_buffer[i]);
        if(pidbuf[i] == -1){
            exit_after_done = 1;
        }
    }
    for(int i = 0;i<c;i++){
        printf("waiting for %d to exit\n",pidbuf[i]);
        wait(&pidbuf[i]);
        //if(command_buffer[i]->pipe_write ==1){
        //    printf("closing pipe.. \n");
        //    //write(*command_buffer[i]->pipe_i+1,"\n",2);
        //    close(*command_buffer[i]->pipe_i+1);
        //}
        //if(command_buffer[i]->pipe_read ==1){
        //    printf("closing the read end...\n");
        //    close(*command_buffer[i]->pipe_i);
        //}
    }
    for(int i=0;i<c;i++){
        destroycommand(command_buffer[i]);
    }
    free(pidbuf);
    free(command_buffer);
    if(exit_after_done){
        exit(0);
    }
    //free(argv); // free rest of stuff here lol
}


void
main(void){
    for(;;){
        char buf[512] = {0};
        printf(">>>");
        read(0,buf,sizeof buf);
        char* last = buf;
        int buflen = strlen(buf);
        for(int i = 0; i<buflen;i++){
            if(buf[i] == ';'){
                char buf2[256] = {0};
                buf[i] = 0;
                strcpy(buf2,last);
                buf[i] = ';';
                runcmd(buf2);
                last = &buf[i+1];
            }
        }
        runcmd(last);
    }
}