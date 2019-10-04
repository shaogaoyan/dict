#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <strings.h>
#include <unistd.h>
#include <sqlite3.h>
#include <string.h>


#define REGISTER  1
#define QUIRE     2
#define QUIT      3
#define SERVEID   "192.168.1.189"
#define PORT      54321
#define NUM       100
#define WORD_LEN  20
#define PATH      "./dict.txt"

typedef struct{
	char name[20];
	char password[20];
}im;


typedef struct{
	int  flag;
	char word[20];
	im   reg;
	im   log;
}recv_im;

typedef struct{
	int  jud;
	char name[20];
	char password[20];
}quire;

typedef struct{
	int  jud;
	char exits[256];
}send_c;

int create_socket();
void deal_register(recv_im,sqlite3*,struct epoll_event);
void deal_login(recv_im,sqlite3*,struct epoll_event);
void deal_quire(recv_im,struct epoll_event);
void do_uinsert(sqlite3 *db,recv_im);         //插入数据
int  do_rquery(sqlite3 *db,const char* name,quire*); //查询数据
void do_delete(sqlite3 *db);                  //删除数据
void do_update(sqlite3 *db);                  //更新数据
void Trim(char *src);


int main(int argc, char *argv[])
{
	int sfd,cfd;
	int epfd,epct;
	recv_im rec;
	sqlite3 *db;
	struct sockaddr_in severaddr,clientaddr;
	struct epoll_event event;
	struct epoll_event serve[20];
	socklen_t len = sizeof(severaddr);
	socklen_t len_recv = sizeof(rec);
	bzero(serve,sizeof(serve));
	epfd = epoll_create(1);
	sfd = create_socket();
	if((sqlite3_open("user.db",&db))!=SQLITE_OK){
		printf("Open failed : %s\n",sqlite3_errmsg(db));
		return -1;
	}
	event.data.fd = sfd;
	event.events = EPOLLIN|EPOLLET;
	epoll_ctl(epfd,EPOLL_CTL_ADD,sfd,&event);
	while(1){
		epct = epoll_wait(epfd,serve,20,-1);
		for(int i=0;i<epct;++i){
			if(serve[i].data.fd == sfd){
				cfd = accept(serve[i].data.fd,(struct sockaddr*)&clientaddr,&len);
				event.data.fd = cfd;
				event.events = EPOLLIN|EPOLLET;
				epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&event);
			}
			else{
				bzero(&rec,len_recv);
				if((read(serve[i].data.fd,&rec,len_recv))<=0){
					printf("User is quit!\n");
					close(serve[i].data.fd);
					epoll_ctl(epfd,EPOLL_CTL_DEL,serve[i].data.fd,&event);
					continue;
				}
				//处理客户端消息
				if(rec.flag == 1){
					deal_register(rec,db,serve[i]);
				}
				else if(rec.flag == 2){
					deal_login(rec,db,serve[i]);
				}
				else if(rec.flag ==3){
					deal_quire(rec,serve[i]);
				}

			}
		}

	}

	return 0;
}

int create_socket()
{
	int sfd;
	struct sockaddr_in severaddr;
	socklen_t len = sizeof(severaddr); 
	if((sfd = socket(AF_INET,SOCK_STREAM,0))<0){
		perror("socket");
		exit(-1);
	}
	severaddr.sin_family = AF_INET;
	severaddr.sin_port = htons(54321);
	severaddr.sin_addr.s_addr = 0;
	if((bind(sfd,(struct sockaddr*)&severaddr,len))<0){
		perror("bind");
		exit(-1);
	}

	if((listen(sfd,10))<0){
		perror("listen");
		exit(-1);
	}

	return sfd;
}

void deal_register(recv_im rec,sqlite3 *db,struct epoll_event serve)
{
	quire  quire_u;
	send_c sc;
	do_rquery(db,rec.reg.name,&quire_u);
	if(1 == quire_u.jud){
		sc.jud = 0;
		strcpy(sc.exits,"The user is exits");
		if((send(serve.data.fd,&sc,sizeof(sc),0))<0){
			perror("register send");
			return;
		}
	}
	else if(0 == quire_u.jud){
		strcpy(sc.exits,"Register is succed");
		sc.jud = 1;
		do_uinsert(db,rec);
		send(serve.data.fd,&sc,sizeof(sc),0);
	}
}


void deal_login(recv_im rec,sqlite3* db,struct epoll_event serve)
{
	printf("login:\n");
	quire quire_u;
	send_c sc;
	size_t len_sc = sizeof(sc);
	do_rquery(db,rec.log.name,&quire_u);
	if(0 == quire_u.jud){
		strcpy(sc.exits, "The user does not exis");
		sc.jud = 0;
		if((send(serve.data.fd,&sc,len_sc,0))<0){
			perror("register send");
			return;
		}
	}
	else{
		if(!(strcmp(rec.log.password,quire_u.password))){
			strcpy(sc.exits, "Login succed");
			sc.jud = 1;
			send(serve.data.fd,&sc,len_sc,0);
		}
		else{
			sc.jud = 0;
			send(serve.data.fd,&sc,len_sc,0);
		}
	}
}

int do_rquery(sqlite3 *db,const char* name,quire *quire_u)
{
	char sql[128]={0};
	char *errmsg;
	char **rep;
	int  n_row;
	int  n_column;
	sprintf(sql,"select * from user where name='%s'",name);
	if(sqlite3_get_table(db,sql,&rep,&n_row,&n_column,&errmsg)!=SQLITE_OK){
		printf("qurey %s\n",errmsg);
		return -1;
	}
	else{
		if(n_row==0){
			quire_u->jud = 0;
		}
		else{
			printf("%s\n",*++rep);
			printf("%s\n",*++rep);
			quire_u->jud = 1;
			strcpy(quire_u->password,*(++rep));
			strcpy(quire_u->password,*(rep));
		}
	}
	return 0;
}


void do_uinsert(sqlite3 *db,recv_im rec)//插入数据
{
	char sql[128]={0};
	char *errmsg;
	sprintf(sql,"insert into user values('%s','%s')",rec.reg.name,rec.reg.password);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return;
	}
	puts("insert ok!");
	return;

}

void deal_quire(recv_im rec,struct epoll_event serve)
{
	FILE *dp;
	char *p;
	send_c sc;
	size_t len = sizeof(sc);
	char buf[1024]={0};
	if((dp=fopen(PATH,"r"))==NULL){
		perror("fail to fopen");
		strcpy(sc.exits,"Fail");
		send(serve.data.fd,&sc,len,0);
		return;
	}
	while(fgets(buf,sizeof(buf),dp)!=NULL)// 一行一行的去比较单词。
	{
		int ret;
		ret = strncmp(buf,rec.word,strlen(rec.word));//比较穿过来的单词和BUF
		//
		if(ret<0)//buf中的单词比   发来的单词小  继续比较
		{
			continue;
		}
		else if(ret>0)//buf中的单词  比发来的大  退出（找不到）
		{
			break;
		}
		//   abcd   abcde
		else if(ret==0 && buf[strlen(rec.word)]!=' ')//与BUF中的几个相同 但是后一个不是空格 退出
		{
			break;
		}
		p=buf+strlen(rec.word);//把指针定位到第一个空格位置
		while(*p==' ')//定位到单词的意思
			p++;
		sc.jud = 1;
		strcpy(sc.exits,p);
		send(serve.data.fd,&sc,len,0);
		fclose(dp);
		return;
	}
	strcpy(sc.exits,"no found");
	send(serve.data.fd,&sc,len,0);
	fclose(dp);
	return;
}
