#include "include.h"

int online,user;
struct Account userList[100];
struct OnlineAccount onlineUser[100];
int listenfd, n , connfd[100];
socklen_t clilen;
struct Package sendpkg,recvpkg;
struct sockaddr_in cliaddr, servaddr;
char username[10];
char password[20];
int k;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv){

	user = 0;
	online = 0;
	k = 0;

	if((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		perror("Problem in creating the socket\n");
		exit(1);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind (listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	printf("Server running ... Waiting for connections.\n");

	pthread_t heartthread;
	int ret = pthread_create(&heartthread,NULL,(void*)heartBeatThread,NULL);
	if (ret != 0){
		printf("Create thread error!\r\n");
		exit(1);
	}

	pthread_t handlethread[100];

	while(1){
		clilen = sizeof(cliaddr);
		connfd[k] = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
		printf("Received request ...\n");

		int ret = pthread_create(&handlethread[k],NULL,(void*)handleThread,(void *)connfd[k]);
		if (ret != 0){
			printf("Create thread error!\r\n");
			exit(1);
		}
		k++ ;
	}
	pthread_join(heartthread,NULL);
	return 0;
}

void handleThread(void * fd){
	int connfd = (int)fd;
	while((n = recv(connfd, (void *)&recvpkg, MAXLINE, 0) > 0)){
		switch(recvpkg.service){
			case(REGIST):	{handleRegist(connfd);break;}
			case(LOGON):	{handleLogon(connfd);	break;}
			case(LOGOFF):	{handleLogoff(connfd);break;}
			case(HEARTBEAT):{handleHeartbeat(connfd);	break;}
			case(MESSAGE):	{handleMessage(connfd);	break;}
			case(INFORM):	{handleInform(connfd);	break;}
			case(ONLINEFRIENDS):{handleOnlinefriends(connfd);	break;}
			default:		{printf("Unknown package!!!\n");}
		}
		memset(&recvpkg, 0 , MAXLINE);
	}

}

void handleRegist(int connfd){

	strcpy(username,recvpkg.srcuser);
	strcpy(password,recvpkg.message);
	printf("User regeist, the username id %s,the password is %s   ",username,password);
	int i;
	for(i = 0 ; i < user ; i++){
		if(strcmp(userList[i].username,username) == 0)
			break;
	}
	if(i == user){
		strcpy(userList[user].username,username);
		strcpy(userList[user].password,password);
		user++;
		memset(&sendpkg, 0 , MAXLINE);
		init_pkg(&sendpkg);
		sendpkg.service = REGIST;
		strcpy(sendpkg.srcuser, username);
		sendpkg.status = ACCEPT;
		send(connfd, (void *)&sendpkg, HEADLINE, 0);
		printf("Regist successfully\n");
	}
	else{
		memset(&sendpkg, 0 , MAXLINE);
		init_pkg(&sendpkg);
		sendpkg.service = REGIST;
		strcpy(sendpkg.srcuser, username);
		sendpkg.status = REFUSE;
		send(connfd, (void *)&sendpkg, HEADLINE, 0);
		printf("Regist failed\n");
	}
}

void handleLogon(int connfd){
	strcpy(username,recvpkg.srcuser);
	strcpy(password,recvpkg.message);
	printf("User logon, the username id %s,the password is %s   ",username,password);
	int i;
	int success = 0;
	for(i = 0 ; i < online ; i++){
		if(strcmp(onlineUser[i].username,username) == 0)
			break;
	}
	if(i < online){
		memset(&sendpkg, 0 , MAXLINE);
		init_pkg(&sendpkg);
		sendpkg.service = LOGON;
		strcpy(sendpkg.srcuser, username);
		sendpkg.status = REPEAT;
		send(connfd, (void *)&sendpkg, HEADLINE, 0);
	}
	else{
		for(i = 0 ; i < user ; i++){
			if(strcmp(userList[i].username,username) == 0)
				break;
		}
		if(i < user){
			if(strcmp(userList[i].password,password) == 0){
				pthread_mutex_lock(&mutex);
				strcpy(onlineUser[online].username,username);
				onlineUser[online].connfd = connfd;
				online++;
				pthread_mutex_unlock(&mutex);
				memset(&sendpkg, 0 , MAXLINE);
				init_pkg(&sendpkg);
				sendpkg.service = LOGON;
				strcpy(sendpkg.srcuser, username);
				sendpkg.status = ACCEPT;
				send(connfd, (void *)&sendpkg, HEADLINE, 0);
				success = 1;
				broadcast(username,1);
				printf("Logon successfully\n");			
			}
		}
		if(!success){
			memset(&sendpkg, 0 , MAXLINE);
			init_pkg(&sendpkg);
			sendpkg.service = LOGON;
			strcpy(sendpkg.srcuser, username);
			sendpkg.status = REFUSE;
			send(connfd, (void *)&sendpkg, HEADLINE, 0);
			printf("Logon failed\n");
		}
	}
}

void handleLogoff(int connfd){
	strcpy(username,recvpkg.srcuser);
	printf("User logoff, the username id %s   ",username);
	int i,j;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(userList[i].username, username) == 0)
			break;
	}
	//close(onlineUser[i].connfd);
	for(j = i ; i < online-2 ; j++){
		strcpy(onlineUser[j].username,onlineUser[j+1].username);
		onlineUser[j].connfd = onlineUser[j+1].connfd;
	}
	online--;
	pthread_mutex_unlock(&mutex);
	broadcast(username,0);
	printf("Logoff successfully\n");
}

void handleMessage(int connfd){
	char desuser[10];
	strcpy(username,recvpkg.srcuser);
	strcpy(desuser,recvpkg.desuser);
	printf("%s send message to %s : %s   ",username,desuser,recvpkg.message);
	memset(&sendpkg, 0 , MAXLINE);
	init_pkg(&sendpkg);
	sendpkg.service = MESSAGE;
	strcpy(sendpkg.srcuser, username);
	strcpy(sendpkg.desuser, desuser);
	strcpy(sendpkg.message, recvpkg.message);
	sendpkg.length = strlen(sendpkg.message);
	int fd,i;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(desuser,onlineUser[i].username)==0){
			fd = onlineUser[i].connfd;
			send(fd, (void *)&sendpkg, sendpkg.length + HEADLINE, 0);
			printf("send successfully\n");
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
}

void handleInform(int connfd){
	int i;
	memset(&sendpkg, 0 , MAXLINE);
	init_pkg(&sendpkg);
	sendpkg.service = INFORM;
	strcpy(sendpkg.srcuser, username);
	strcpy(sendpkg.message, recvpkg.message);
	sendpkg.length = recvpkg.length;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(onlineUser[i].username,username)!=0){
			send(onlineUser[i].connfd, (void *)&sendpkg, sendpkg.length + HEADLINE, 0);
		}
	}
	pthread_mutex_unlock(&mutex);
}

void handleOnlinefriends(int connfd){
	strcpy(username,recvpkg.srcuser);
	printf("%s want to know online friends    ",username);
	memset(&sendpkg, 0 , MAXLINE);
	init_pkg(&sendpkg);
	sendpkg.service = ONLINEFRIENDS;
	strcpy(sendpkg.srcuser, username);
	sendpkg.status = 0x00 + online;
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		strcpy(&sendpkg.message[10*i],onlineUser[i].username);
	}
	pthread_mutex_unlock(&mutex);
	send(connfd, (void *)&sendpkg, HEADLINE + 10*online , 0);	
	printf("success \n");
}

void handleHeartbeat(int connfd){
	printf("Receive heartbeat from %s \n",recvpkg.srcuser);
	if(recvpkg.status == REPLY){
		pthread_mutex_lock(&mutex);
		strcpy(onlineUser[online].username,recvpkg.srcuser);
		onlineUser[online].connfd = connfd;
		online++;
		pthread_mutex_unlock(&mutex);
	}
}

void broadcast(char *username, int type){		//通知所有在线用户，某用户上线或下线，上线1，下线0
	int i;
	memset(&sendpkg, 0 , MAXLINE);
	init_pkg(&sendpkg);
	sendpkg.service = UPDATE;
	if(type)	sendpkg.status = LOGON;
	else sendpkg.status = LOGOFF;
	strcpy(sendpkg.srcuser, username);
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(onlineUser[i].username,username)!=0){
			send(onlineUser[i].connfd, (void *)&sendpkg, HEADLINE, 0);
		}
	}
	pthread_mutex_unlock(&mutex);
}

void heartBeatThread(){
	signal(SIGALRM,sendheartbeat);
	alarm(60);
}

void sendheartbeat(){
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		printf("Send heartbeat to %s \n",onlineUser[i].username);
		strcpy(sendpkg.srcuser, onlineUser[i].username);
		sendpkg.service = HEARTBEAT;
		sendpkg.status = REQUEST;
		send(onlineUser[i].connfd, (void *)&sendpkg, HEADLINE, 0);
	}
	online = 0;
	pthread_mutex_unlock(&mutex);
	sleep(5);
	checkheartbeat();
	alarm(60);
}

void checkheartbeat(){
}

void init_pkg(struct Package *pkg){
	strcpy(pkg->proname, "ZRQP");
	pkg->length = 0;
	pkg->service = INIT;
}
