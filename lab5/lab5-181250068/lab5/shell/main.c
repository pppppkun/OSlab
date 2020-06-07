#include "lib.h"
#include "types.h"
#define NULL ((void*)0)


char path[64];
char ls_[2];
char cd_[2];

void cd(char *path_){
    path[0]='/';
    path[1]='b';
    path[2]='o';
    path[3]='o';
    path[4]='t';
    path[5]='/';
}

void execute(char *code){
    int flag=1;
    for(int i = 0;i<2;i++){
        if(code[i]!=ls_[i]){
            flag=0;
            break;
        }
    }
    if(flag==1){
        printf("%s",path);
        ls(path);
        return;
    }

    flag=1;
    for(int i = 0;i<2;i++){
        if(code[i]!=cd_[i]){
            flag=0;
            break;
        }
    }
    if(flag==1){
        cd(code);
        return;
    }

}

void init(){
    ls_[0]='l'; ls_[1]='s';
    cd_[0]='c'; cd_[1]='d';

    for(int i = 0;i<64;i++){
        path[i]='\0';
    }
    path[0]='/';
}

int main(void)
{
    init();
    /*char buffer[6];
    while(1){
        printf("pkun@qemu:");
        scanf(" Test %6s", buffer);
        execute(buffer);

    }*/

	char str[6];
	char cha = 0;
	while(1){
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		scanf("Test %c Test %6s", &cha, str);
        printf("%s", str);
        execute(str);
	} 
}
