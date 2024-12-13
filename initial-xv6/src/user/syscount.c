#include"kernel/types.h"
#include"user/user.h"

int main(int argc , char * argv[]){

    if(argc < 3){
        printf("Usage: syscount <mask> <command> [args]\n");
        exit(0);
    }

    int mask=atoi(argv[1]);

    int f= fork();

    if(f==0){
        int res = exec(argv[2],&argv[2]);
        if(res==-1){
            printf("exec %s failed\n",argv[2]);
        }
        exit(0);
    }

    wait(&f);
    getSysCount(mask);
    exit(0);
}