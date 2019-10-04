#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define REGISTER  1 
#define QUIRE     2
#define QUIT      3
#define SERVEID   "192.168.1.188"
#define PORT      54321
#define NUM       100

int sever_socket(struct sockaddr_in * severaddr,int port,const char* addr);
void printf_main_s();     //输出主界面
void printf_branch_1();
void Register(struct sockaddr_in*,int,socklen_t*,struct sockaddr_in*);
void Login(struct sockaddr_in*,int,socklen_t*,struct sockaddr_in*);
void Quire(int);
void History(int,socklen_t*,struct sockaddr_in*);


typedef struct{
	char name[20];
	char password[20];
}im;


typedef struct{
	int  flag;
	char word[20];
	im   reg;
	im   log;
}send_im;

typedef struct{
	int jud;
	char exits[256];
}recv_s;

int main(int argc, char *argv[])
{

	int choose = 0;
	int cfd;
	struct sockaddr_in clientaddr,severaddr;
	cfd = sever_socket(&severaddr,PORT,SERVEID);
	socklen_t len = sizeof(clientaddr);
	if(connect(cfd,(struct sockaddr*)&severaddr,len)<0){
		perror("connect");
		exit(-1);
	}
	while(1){
		printf_main_s();
		scanf("%d",&choose);
		switch(choose){
			case 1:
				Register(&severaddr,cfd,&len,&clientaddr);
				break;
			case 2:
				Login(&severaddr,cfd,&len,&clientaddr);
				break;
			case 3:
				printf("quit the programe!\n");
				return 0;
			default:
				printf("Please input correct command!\n");
				break;
		}
		getchar();
	}
	return 0;
}

void printf_main_s()
{
	char main_s[3][50] = {
		"*********************************************",
		"     1.registered     2.login     3.quit     ",
		"*********************************************"
	};
	for(int i=0;i<3;++i)
	{
		printf("%s\n",main_s[i]);
	}
	printf("input>");

}


void printf_branch_1()
{
	char main_s[3][50] = {
		"*********************************************",
		"     1.quire     2.history      3.quit       ",
		"*********************************************"
	};
	for(int i=0;i<3;++i)
	{
		printf("%s\n",main_s[i]);
	}

}

void Register(struct sockaddr_in* severaddr,int cfd,socklen_t* len,struct sockaddr_in* clientaddr)
{
	send_im registered;
	recv_s  rec;
	size_t length_r = sizeof(rec);
	size_t length = sizeof(registered); 
Loop:
	bzero(&registered,length);
	registered.flag = 1;
	printf("Please input your username:\n");
	scanf("%s",registered.reg.name);
	printf("Please input your password\n");
	scanf("%s",registered.reg.password);
	if((sendto(cfd,&registered,length,0,(struct sockaddr*)severaddr,*len))<0){
		printf("Fail to sendto: %s\n",strerror(errno));
		exit(-1);
	}
	bzero(&rec,length_r);
	if((recvfrom(cfd,&rec,length_r,0,(struct sockaddr*)clientaddr,len))<0){
		perror("recvfrom");
		exit(-1);
	}
	if(1 == rec.jud){
		printf("%s\n",rec.exits);
		return;
	}
	else{
		puts("The name is exits");
		goto Loop;
	}

}


void Login(struct sockaddr_in* severaddr,int cfd,socklen_t* len,struct sockaddr_in* clientaddr)
{
	send_im user;
	recv_s  rec;
	size_t length_r = sizeof(rec);
	size_t length = sizeof(user);
	int choose = 0;
	bzero(&user,length);
	user.flag = 2;
	printf("Please input your username:\n");
	scanf("%s",user.log.name);
	printf("Please input your password\n");
	scanf("%s",user.log.password);
	if((sendto(cfd,&user,length,0,(struct sockaddr*)severaddr,*len))<0){
		perror("send");
		exit(-1);
	}
	bzero(&rec,length_r);
	if((recvfrom(cfd,&rec,length_r,0,(struct sockaddr*)clientaddr,len))<0){
		perror("recvfrom");
		exit(-1);
	}
	if(1 == rec.jud){
		printf("Succesful Login !\n");
		while(1){
			printf_branch_1();
			scanf("%d",&choose);
			switch(choose){
				case 1:
					Quire(cfd);
					break;
				case 2:
					History(cfd,len,clientaddr);
					break;
				case 3:
					close(cfd);
					return;
				default:
					printf("Please input correct command!\n");
					break;
			}
		}
	}
	else{
		puts("password is wrong");
	}
	return ;
}


int sever_socket(struct sockaddr_in * severaddr,int port,const char * addr)
{
	int cfd;
	if((cfd = socket(AF_INET,SOCK_STREAM,0))<0){
		perror("socket");
		exit(-1);
	}
	severaddr->sin_family = AF_INET;
	severaddr->sin_port = htons(port);
	severaddr->sin_addr.s_addr = inet_addr(addr);
	return cfd;
}


void Quire(int cfd)
{
	send_im quire;
	recv_s rec;
	size_t length = sizeof(quire);
	size_t length_r = sizeof(rec);
	bzero(&quire,length);
	bzero(&rec,length_r);
	quire.flag = 3;
	printf("Please input word which you want to quire\n");
	scanf("%s",quire.word);
	if((send(cfd,&quire,length,0))<0){
		perror("quire send");
		exit(-1);
	}
	if((recv(cfd,&rec,length_r,0))<0){
		perror("quire recv");
		exit(-1);
	}
	if(rec.jud == 1){
		printf("find\n");
		printf("%s	%s\n",quire.word,rec.exits);
	}
	return;
}

void History(int cfd,socklen_t* len,struct sockaddr_in* clientaddr)
{
	char history[NUM][20];
	size_t length = sizeof(history);
	bzero(history,length);
	if((recvfrom(cfd,history,length,0,(struct sockaddr*)clientaddr,len))<0){
		perror("recvfrom");
		exit(-1);
	}
	for(int i=0;i<NUM;++i){
		if(strlen(history[i])){
			break;
		}
		printf("%s\n",history[i]);
	}
	return;
}

