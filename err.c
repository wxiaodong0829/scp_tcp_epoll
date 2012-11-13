/*************************************************************************
    > File Name: err.c
    > Author: wxiaodong
    > Mail  : wxiaodong0829@163.com 
    > 自由  谐作   创造   沟通
    > Created Time: 2012年08月21日 星期二 16时15分28秒
 ************************************************************************/
#include "err.h"
void error_sys(const char *err)
{
	 perror(err);
	 exit(1);
}
void error_user(const char *err)
{
	puts(err);
	exit(2);
}

void print_exit(int status)
{
    if(WIFEXITED(status)){
        printf("child exit normally, eixt no : %u\n", WEXITSTATUS(status));
    }else if(WIFSIGNALED(status)){
        printf("child killed by signal, signo : %d\n", WTERMSIG(status));
    }else
        printf("other...\n");
}
