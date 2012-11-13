/*************************************************************************
    > File Name: chat_room.c
    > Author: wxiaodong
    > Mail  : wxiaodong0829@163.com 
    > 自由  谐作   创造   沟通
    > Created Time: 2012年09月11日 星期二 20时58分24秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include "err.h"
#include "debug.h"

#define SERV_PORT 8000
void err_sys(const char *str)
{
	perror(str);
	exit(1);
}
#define BUFSIZE 1024

#define UP 1
#define DOWN 2

#define INIT 0
#define DO 1
#define DONE 2 

typedef struct usr {
	struct sockaddr_in addr;	/* 对应的地址结构 */
	int type;					/* 1:上传，2:下载*/
	int status;					/* 状态机 */
	int socket;					/* 只下载用 对方套接字*/
	int fd;						/* 本地文件描述符 */
	int cfd;					/* 套接字描述符*/
	struct usr *next;			/* 简单的 链表操作 */
}usr_t;

usr_t * head = NULL;

usr_t *new_node(const struct sockaddr_in *addr, int cfd)
{
	usr_t *p;

	p = malloc(sizeof(usr_t));
	if (p == NULL)
		return NULL;
	
	p->addr = *addr;
	p->type = 0;
	p->status = INIT;
	p->socket = -1;
	p->fd = -1;
	p->cfd = cfd;
	p->next = NULL;

	return p;
}

int insert(const usr_t * p)
{
	usr_t *q;

	if (p == NULL)
		return -1;
	if (head == NULL) {
		head = p;
		return 0;
	}
	for (q = head; q->next != NULL; q = q->next)
		;
	q->next = p;

	return 0;
}

int is_equal(struct sockaddr_in *p, struct sockaddr_in *q)
{
	return p->sin_family == q->sin_family && 
		   p->sin_port ==	q->sin_port  &&
		   p->sin_addr.s_addr == q->sin_addr.s_addr;
}


usr_t * search_by_sockfd(int cfd)
{
	usr_t *p;

	for (p = head; p != NULL; p = p->next)
		if (p->cfd == cfd)
			return p;

	return NULL;
}
void delete_by_addr(struct sockaddr_in *addr)
{
	usr_t *p, *q;
	
	if (is_equal(&head->addr, addr)) {
		p = head;
		head = head->next;
		free(p);
		return ;
	}

	for (p = head; p->next != NULL; p = p->next) {
		if (is_equal(&p->next->addr, addr)) {
			q = p->next;
			p->next = p->next->next;
			free(q);
			return ;
		}
	}
}

void print(void)
{
	usr_t *p;
	char ip[20];
	int i;

	for (p = head, i = 0; p != NULL; p = p->next, i++) {
		printf("[%d] cfd:%d", i, p->cfd);
		if (p->type == UP)
			printf("UP  client ---file-sendto-server---> server, ");
		else if (p->type == DOWN)
			printf("DOWN  server ---file-sendto-client---> client, ");
		inet_ntop(AF_INET, &p->addr.sin_addr.s_addr, ip, 20);
		printf("client_ip: %s, ", ip);
		printf("client_port: %u", ntohs(p->addr.sin_port));
		printf("fd:%d, ", p->fd);
		if (p->status == INIT)
			printf("status: INIT\n");
		else if (p->status == DO)
			printf("status: DO\n");
		else if (p->status == DONE)
			printf("status: DONE\n");
		else 
			printf("status: impossible...\n");
	}
}

void destroy(void)
{
	usr_t *p, *q;

	for (p = head; p != NULL; p = q) {
		q = p->next;
		free(p);
	}

	head = NULL;
}

/*int sock_init(in_port_t port, const char *ip)
{
	int sfd;
	struct sockaddr_in serv_addr;

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd == -1) {
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);

	if (bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		return -1;

	return sfd;
}
*/


int is_end(char *buf)
{
	if (strncmp(buf, "UP_END", 6) == 0){ /* 收到  结束 */

		return 1;
	}
	return  0;
}
#define MODE_LEN 5
#define FILE_LEN 255
#define ROOT "./serv"
/* 前三个是传出参数*/
//int fill_struct(int *type, int *status, int *fd, char *buf, int efd, int cfd)
int fill_struct(usr_t *usr, char *buf, int efd)
{
	char mode[MODE_LEN]; char filename[FILE_LEN];
	char path[FILE_LEN];
	char *p;
	int i, n; int file_d;
	char root[FILE_LEN];
	struct stat st;
	struct epoll_event ep;
	int cfd = usr->cfd;

	for (p = buf, i = 0; p = strtok(p, " "); p = NULL, i++) {
		if (i == 0) {
			strncpy(mode, p, strlen(p) + 1);
		} else if (i == 1) {
			strncpy(root, p, strlen(p) + 1);
		} else if (i == 2)
			strncpy(filename, p, strlen(p) + 1);
	}
	sprintf(path, "%s/%s", root, filename);
	PRINT_S(path);

    lstat(path, &st);
	if(S_ISDIR(st.st_mode)) {
		printf("%s is dir file\n", path);
		return -1;
	}

	if (strcmp(mode, "up") == 0) {           /* 上传文件 */
		usr->type = UP;
		usr->fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
		if (usr->fd == -1)
			error_sys("open up");
		usr->status = DO;

//		ep.events = EPOLLOUT; ep.data.fd = cfd;
//		epoll_ctl(efd, EPOLL_CTL_DEL, cfd, &ep);
//		ep.events = EPOLLIN; ep.data.fd = cfd;
//		n = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ep);
	}
	else if (strcmp(mode, "down") == 0) {    /* 下载文件 */
		usr->type = DOWN;
		printf("down path:%s\n", path);
		usr->fd = open(path, O_RDONLY);
		if (usr->fd == -1)
			error_sys("open down");
		usr->status = DO;
#if 0
		ep.events = EPOLLIN; ep.data.fd = cfd;
		epoll_ctl(efd, EPOLL_CTL_DEL, cfd, &ep);
		ep.events = EPOLLOUT; ep.data.fd = cfd;
		n = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ep);
#endif
	}

	return 1;
}

void init(int *cfd, int *e_fd);
int main(void)
{
	int lfd, efd;
	struct epoll_event ep;
	struct epoll_event events[FD_SETSIZE];
	int n, i;
	struct sockaddr_in cli_addr;
	int len_cli;
	usr_t *p;
	usr_t *new;
	char buf[BUFSIZ];
	int nready, j, cfd;

	init(&lfd, &efd);
	while(1){
//		printf("epoll_wait .....\n");
		nready = epoll_wait(efd, events, FD_SETSIZE, -1);
//		for (i = 0; i < nready; i++)
//			printf("nready:%d i:%d  enents.data.fd:%d\n", nready, i, events[i].data.fd);
		if(nready == -1)
			err_sys("select error");

		for (i = 0; i < nready; i++) {
			if (events[i].data.fd == lfd) { /* 监听套接字可用 , 创立链接，并注册到监听器 */
				len_cli = sizeof(cli_addr);
				cfd = accept(lfd, (struct sockaddr *)&cli_addr, &len_cli);
				if (cfd == -1)
					error_sys("accept");

				ep.events = EPOLLIN; ep.data.fd = cfd;
				n = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ep);
				if (n == -1)
					error_sys("epoll_ctl");

				new = new_node(&cli_addr, cfd);
				insert(new);
				printf("........create new socket\n");
		//		print();
		//		printf("nready: %d, i=%d\n", nready, i);

			} else { /* 普通套接字 可用 */

#if 1
				int cfd = events[i].data.fd;
				p = search_by_sockfd(cfd);
				if (p->status == INIT) {				/* 第一次发数据   
														   发送 up  or  down ,
														   如果数据还没结束，
														   来了第二个链接请求，
														   进不到这里 `*/
					n = read(p->cfd, buf, BUFSIZ);
					buf[n] = '\0';						
					printf("..............first translent:%s\n",buf);

					fill_struct(p, buf, efd); /* */
					n = write(p->cfd, "OK", strlen("OK"));
					if (n != strlen("OK"))
						printf("write OK is error\n");
			//		print();

				} else if (p->status == DO){                                /* 正常的文件内容 */
					if (events[i].events == EPOLLIN) {  
						if (p->type == DOWN) {					/* 下载预处理 */
						    printf("wait for READY\n");
						    n = read(p->cfd, buf, BUFSIZ);
						    buf[n] = '\0';
						    if (strcmp(buf, "READY") != 0) {
								printf("can't get READY\n");
						    }
							write(p->cfd, "OK", strlen("OK"));
							
							ep.events = EPOLLIN; ep.data.fd = cfd;
							epoll_ctl(efd, EPOLL_CTL_DEL, cfd, &ep);
							ep.events = EPOLLOUT; ep.data.fd = cfd;
							epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ep);
						} else if (p->type == UP) {				/* 上传文件 */
							if ((n = read(p->cfd, buf, BUFSIZ)) == 0) {
								close(p->cfd);
								close(p->fd);
								delete_by_addr(&p->addr);

								ep.events = EPOLLIN; ep.data.fd = p->cfd;
								epoll_ctl(efd, EPOLL_CTL_DEL, p->cfd, &ep);
								printf("....up end...events.data.fd:%d \n", events[i].data.fd);
							} else { /* 正常文件内容读写  */
								write(p->fd, buf, n);
							}
						}

					} else if (events[i].events == EPOLLOUT) { /* 下载文件 */
						if ((n = read(p->fd, buf, BUFSIZ)) == 0) {
			//				print();
							ep.events = EPOLLOUT; ep.data.fd = cfd;
							epoll_ctl(efd, EPOLL_CTL_DEL, cfd, &ep);
							delete_by_addr(&p->addr);
							close(p->fd);
							close(p->cfd);
							printf("....down end...events.data.fd:%d \n", events[i].data.fd);
						} else {
							write(p->cfd, buf, n);
						}

					}

				}

#endif
			}

		} /* for */
	} /* while */

	close(lfd);
	printf("should not be here\n");
	return 0;
}

void init(int *cfd, int *e_fd)
{
	struct sockaddr_in serv_addr;
	int n, i;
	struct epoll_event ep;
	struct epoll_event events[FD_SETSIZE];
	int efd;
	int lfd;

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if(lfd == -1)
		err_sys("socket error");
	*cfd = lfd;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	inet_pton(AF_INET, INADDR_ANY, &serv_addr.sin_addr.s_addr);

	n = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if(n == -1)
		err_sys("bind error");
	n = listen(lfd, 20);
	if(n == -1)
		err_sys("listen error");

	efd = epoll_create(FD_SETSIZE);
	if (efd == -1)
		error_sys("epoll_create");
	*e_fd = efd;

	ep.events = EPOLLIN; ep.data.fd = lfd;
	n = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &ep);
	if (n == -1)
		error_sys("epoll_ctl");

	return ;
}
