/*************************************************************************
    > File Name: err.h
    > Author: wxiaodong
    > Mail  : wxiaodong0829@163.com 
    > 自由  谐作   创造   沟通
    > Created Time: 2012年08月21日 星期二 16时13分03秒
 ************************************************************************/

#ifndef ERR_H_
#define ERR_H_
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
void error_sys(const char *err);

void error_user(const char *err);

void print_exit(int status);

#endif
