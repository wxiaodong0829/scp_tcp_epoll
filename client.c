/*************************************************************************
    > File Name: client.c
    > Author: wxiaodong
    > Mail  : wxiaodong0829@163.com 
    > 自由  谐作   创造   沟通
    > Created Time: 2012年09月11日 星期二 22时00分10秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <regex.h>
#include "err.h"
#include "debug.h"

#define DOWN 0
#define UP 1

#define BUFSIZE 1024

#define SERV_PORT 8000

#define ROOT "./cli"

int is_down_end(char *buf)
{
	PRINT_S(buf);
	if (strncmp(buf, "DOWN_END", strlen("DOWN_END")) == 0){ /* 收到  结束 */

		return 1;
	}
	return  0;
}
int full(char *argv1, char *argv2, char *serv_ip, char *src, char *dest, int *up_or_down, char *filename)
{
	char *p;
	int i, n;


	int len_argv = strlen(argv1);
	for (i = 0, n = 0; i < len_argv; i++) {
		if (argv1[i] == '.')
			n++;
	}
	p = strrchr(argv1, '/');
	if (p != NULL) {
		strncpy(filename, p + 1, strlen(p + 1));
	} else {
		strncpy(filename, argv1, strlen(argv1));
	}

	if (n >= 3) {
		*up_or_down = DOWN; /* 下载 */
		if (argv1[len_argv - 1] == '/')
			argv1[len_argv - 1] = '\0';
		for (p = argv1, i = 0; p = strtok(p, ":"); p = NULL, i++) {
			if (i == 0) {
				strncpy(serv_ip, p, strlen(p) + 1);
			} else if (i == 1){
				strncpy(src, p, strlen(p) + 1);
				p = strrchr(src, (int)'/');
				*p = '\0';
			}
		}
		if (argv2[strlen(argv2) - 1] == '/')
		    argv2[strlen(argv2) - 1] = '\0';
		if (argv2[0] == '.') {
			PRINT_S("--------here------------\n");
			strncpy(dest, argv2, strlen(argv2));
		} else {
			PRINT_S("--------here 1 ------------\n");
			sprintf(dest, "./%s", argv2);
		}
	}
	else  {
		*up_or_down = UP; /* 上传 */
		if (argv1[0] == '.') {
			strncpy(src, argv1, strlen(argv1));
		} else if (argv1[0] == '/') {
			strncpy(src, argv1, strlen(argv1) + 1);
		}else{
			sprintf(src, "./%s", argv1);
		}
		len_argv = strlen(argv2);
		if (argv2[len_argv - 1] == '/')
			argv2[len_argv - 1] = '\0';
		for (p = argv2, i = 0; p = strtok(p, ":"); p = NULL, i++) {
			if (i == 0) {
				strncpy(serv_ip, p, strlen(p) + 1);
			} else if (i == 1){
				strncpy(dest, p, strlen(p) + 1);
			}
		}
	}
	return 1;
}

int initsocket(char *serv_ip)
{
	struct sockaddr_in serv_addr;
	int cfd; int n;

	cfd = socket(AF_INET, SOCK_STREAM, 0);
	if (cfd == -1)
		error_sys("socket");

	memset(&serv_addr, 0, sizeof(serv_addr)); //用sizeof可使更通用
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr.s_addr);

	n = connect(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (n == -1)
		error_sys("connect");
	return cfd;

}

int main(int argc, char **argv)
{
	int n;
	char *usr_name;
	int sfd, i, nsend;
	struct sockaddr_in serv_addr;
	char buf[BUFSIZ];
	int fd, len_argv;
	char path[1024];
	int up_or_down = -1;	/* 0：up   1：down */
	int cfd;

	char serv_ip[1024];
	char src[1024];
	char dest[1024];
	char file_name[1024];

	struct stat st;
	regex_t preg;
	char *regex = "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}";
	int reg;
	regmatch_t pmatch[2];
	size_t nmatch = 2;

	memset(pmatch, 0, sizeof(pmatch));
	memset(&preg, 0, sizeof(preg));
	regcomp(&preg, regex, 0);
	
	full(argv[1], argv[2], serv_ip, src, dest, &up_or_down, file_name);
	//reg = regexec(&preg, serv_ip, nmatch, pmatch, 0);
	reg = regexec(&preg, "127.0.0.1", nmatch, pmatch, 0);                  /*      如何做呢？？？？       */
	if (reg == 0) {
		printf("serv_ip is error\n");
		error_sys("regexec");
	}

#if 0
	printf("serv_ip:%s src:%s  dest:%s ", serv_ip, src, dest);
	if (up_or_down == UP)
		printf("upload\n");
	else if (up_or_down == DOWN)
		printf("download\n");
	printf("filename:%s\n", file_name);
#endif

#if 1
	sfd = initsocket(serv_ip);


	if (up_or_down == UP) {							/* 上传 */
		lstat(src, &st);
		if(S_ISDIR(st.st_mode)) {
			printf("%s is dir\n", src);
			return -1;
		}

		sprintf(buf, "up %s %s", dest, file_name);							/* 上传标志 */
		PRINT_S(buf); PRINT_S(dest); PRINT_S(file_name);

		write(sfd, buf, strlen(buf));

	//	PRINT_S("here1");
		n = read(sfd, buf, BUFSIZ);				/* 等待服务器回复 确认信息,同时可保证下面将要发送的数据的正确性 */
		buf[n] = '\0';
		if (strcmp(buf, "OK")) { 
			printf("fail to up link ...\n");
			return 0;
		} else {
		    printf("up %s link ok?: %s\n", file_name, buf);
		}

		fd = open(src, O_RDONLY);									/* 数据 */
		if (fd == -1)
			error_sys("open");
		while ((n = read(fd, buf, 1024)) != 0) {
			write(sfd, buf, n);
		}
		close(fd);
		close(sfd);
		printf("..........................up %s is end\n", file_name);

	} else if (up_or_down == DOWN) {						 /* 下载 */
		lstat(dest, &st);
		if(S_ISREG(st.st_mode)) {
			printf("%s is reg file \n", dest);
			return -1;
		}

		sprintf(buf, "down %s %s", src, file_name);
		PRINT_S(buf);
		PRINT_S(src);
		PRINT_S(file_name);

		write(sfd, buf, strlen(buf));

		n = read(sfd, buf, BUFSIZ);
		buf[n] = '\0';
		if (strcmp(buf, "OK")) {
			printf("fail to down link ...\n");
			return 0;
		} else {
		    printf("down %s link ok?: %s\n", file_name, buf);
		}


		if (dest[strlen(dest) - 1] == '/')
			dest[strlen(dest) - 1] = '\0';
		sprintf(path, "%s/%s", dest, file_name);
		PRINT_S(path);
		PRINT_S(dest);
		PRINT_S(file_name);
		fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

		write(sfd, "READY", strlen("READY"));
		n = read(sfd, buf, BUFSIZ);
		buf[n] = '\0';
		if (strcmp(buf, "OK")) {
			printf("fail to down link ...\n");
			return 0;
		} else {
		    printf("down %s is ready, is doing...\n", file_name);
		}

		while ((n = read(sfd, buf, BUFSIZ)) != 0){
			write(fd, buf, n);
		}

		close(fd);
		close(cfd);
		printf("......................down %s end\n", file_name);
		 
	} else { /* 不合法 */
		printf("useage: ./client up/down file1\n");
	}

#endif

	return 0;
}
