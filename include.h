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
void handleThread(void* connfd);
void handleRegist(int connfd);
void handleLogon(int connfd);
void handleLogoff(int connfd);
void handleMessage(int connfd);
void handleInform(int connfd);
void handleOnlinefriends(int connfd);
void broadcast(char *username, int type);
void mainThread();
void showlist();
void handleHeartbeat(int connfd);
void checkheartbeat();
void sendheartbeat();
void heartBeatThread();

