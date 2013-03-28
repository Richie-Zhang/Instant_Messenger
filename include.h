#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define INIT 0x00
#define REGIST 0x01
#define LOGON 0x02
#define LOGOFF 0x03
#define HEARTBEAT 0x04
#define MESSAGE 0x05
#define ONLINEFRIENDS 0x06
#define UPDATE 0x07
#define INFORM 0x08

#define REFUSE 0x00
#define ACCEPT 0x01
#define REQUEST 0x03
#define REPLY 0x04
#define REPEAT 0x05


#define SERV_PORT 3000
#define MAXLINE 1024
#define HEADLINE 28
#define LISTENQ 100



struct Package{
	char proname[4];
	short length;
	char service;
	char status;
	char srcuser[10];
	char desuser[10];
	char message[996];
};

struct Account{
	char username[10];
	char password[20];
};

struct OnlineAccount{
	char username[10];
	int connfd;
};

struct mythread{
	char username[10];
	int connfd;
	pthread_t handlethread;
	struct Package sendpkg;
	struct Package recvpkg;
	int used;
	int loged;
};

void regist();
void logon();
void listfri();
void chat();
void inform();
void logoff();
void sendheart();
void updatelist();
void receiveMsg();
void init_pkg(struct Package *pkg);
void handleThread(void* l);
void handleRegist(int connfd,int index);
void handleLogon(int connfd,int index);
void handleLogoff(int connfd,int index);
void handleMessage(int connfd,int index);
void handleInform(int connfd,int index);
void handleOnlinefriends(int connfd,int index);
void broadcast(char *username, int type);
void handleHeartbeat(int connfd,int index);
void mainThread();
void showlist();
void checkheartbeat();
void sendheartbeat();
void heartBeatThread();
int findValid();
void abnormal_logoff(char *username);
