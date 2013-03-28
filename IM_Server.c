#include "include.h"

int online,user;
struct Account userList[LISTENQ];
struct OnlineAccount onlineUser[LISTENQ];
int listenfd, n;
socklen_t clilen;
struct Package sendpkg,recvpkg;
struct sockaddr_in cliaddr, servaddr;
char username[10];
char password[20];
int k;
struct mythread childthread[LISTENQ];
pthread_t heartthread;
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

	int ret = pthread_create(&heartthread,NULL,(void*)heartBeatThread,NULL);
	if (ret != 0){
		printf("Create thread error!\r\n");
		exit(1);
	}

	while(1){
		clilen = sizeof(cliaddr);
		k = findValid();
		childthread[k].connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
		printf("Received request ...\n");
		if(k != -1){
			childthread[k].used = 1;
			int ret = pthread_create(&(childthread[k].handlethread),NULL,(void*)handleThread,(void *)k);
			if (ret != 0){
				printf("Create thread error!\r\n");
				exit(1);
			}
			else{
				printf("Succeed to create a thread!\r\n");
			}	
		}
		else{
			printf("Sorry, the server is busy!!!\n");
			exit(1);
		}
	}
	pthread_join(heartthread,NULL);
	return 0;
}

void handleThread(void * k){
	int index = (int)k;
	int connfd = childthread[index].connfd;
	while((n = recv(connfd, (void *)&childthread[index].recvpkg, MAXLINE, 0) > 0)){
		switch(childthread[index].recvpkg.service){
			case(REGIST):	{handleRegist(connfd,index);break;}
			case(LOGON):	{handleLogon(connfd,index);	break;}
			case(LOGOFF):	{handleLogoff(connfd,index);break;}
			case(HEARTBEAT):{handleHeartbeat(connfd,index);	break;}
			case(MESSAGE):	{handleMessage(connfd,index);	break;}
			case(INFORM):	{handleInform(connfd,index);	break;}
			case(ONLINEFRIENDS):{handleOnlinefriends(connfd,index);	break;}
			default:		{printf("Unknown package!!!\n");}
			fflush(stdout);
		}
		memset(&recvpkg, 0 , MAXLINE);
	}
	
	printf("The thread is dead\n");
	childthread[index].used = 0;
	
	if(childthread[index].loged){
		char offuser[10];
		strcpy(offuser,childthread[index].username);
		memset(&(childthread[index].username),0,10);
		abnormal_logoff(offuser);
	}
	
	pthread_exit(NULL);
}

void handleRegist(int connfd,int index){

	strcpy(username,childthread[index].recvpkg.srcuser);
	strcpy(password,childthread[index].recvpkg.message);
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
		memset(&childthread[index].sendpkg, 0 , MAXLINE);
		init_pkg(&childthread[index].sendpkg);
		childthread[index].sendpkg.service = REGIST;
		strcpy(childthread[index].sendpkg.srcuser, username);
		childthread[index].sendpkg.status = ACCEPT;
		send(connfd, (void *)&childthread[index].sendpkg, HEADLINE, 0);
		printf("Regist successfully\n");
	}
	else{
		memset(&childthread[index].sendpkg, 0 , MAXLINE);
		init_pkg(&childthread[index].sendpkg);
		childthread[index].sendpkg.service = REGIST;
		strcpy(childthread[index].sendpkg.srcuser, username);
		childthread[index].sendpkg.status = REFUSE;
		send(connfd, (void *)&childthread[index].sendpkg, HEADLINE, 0);
		printf("Regist failed\n");
	}
}

void handleLogon(int connfd,int index){
	strcpy(username,childthread[index].recvpkg.srcuser);
	strcpy(password,childthread[index].recvpkg.message);
	printf("User logon, the username id %s,the password is %s   ",username,password);
	int i;
	int success = 0;
	for(i = 0 ; i < online ; i++){
		if(strcmp(onlineUser[i].username,username) == 0)
			break;
	}
	if(i < online){
		memset(&childthread[index].sendpkg, 0 , MAXLINE);
		init_pkg(&childthread[index].sendpkg);
		childthread[index].sendpkg.service = LOGON;
		strcpy(childthread[index].sendpkg.srcuser, username);
		childthread[index].sendpkg.status = REPEAT;
		send(connfd, (void *)&childthread[index].sendpkg, HEADLINE, 0);
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
				memset(&childthread[index].sendpkg, 0 , MAXLINE);
				init_pkg(&childthread[index].sendpkg);
				childthread[index].sendpkg.service = LOGON;
				strcpy(childthread[index].sendpkg.srcuser, username);
				strcpy(childthread[index].username,username);
				childthread[index].sendpkg.status = ACCEPT;
				send(connfd, (void *)&childthread[index].sendpkg, HEADLINE, 0);
				success = 1;
				childthread[index].loged = 1;
				broadcast(username,1);
				printf("Logon successfully\n");			
			}
		}
		if(!success){
			memset(&childthread[index].sendpkg, 0 , MAXLINE);
			init_pkg(&childthread[index].sendpkg);
			childthread[index].sendpkg.service = LOGON;
			strcpy(childthread[index].sendpkg.srcuser, username);
			childthread[index].sendpkg.status = REFUSE;
			send(connfd, (void *)&childthread[index].sendpkg, HEADLINE, 0);
			printf("Logon failed\n");
		}
	}
}

void handleLogoff(int connfd,int index){
	strcpy(username,childthread[index].recvpkg.srcuser);
	printf("User logoff, the username id %s   ",username);
	memset(&(childthread[index].username),0,10);
	int i,j;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(userList[i].username, username) == 0)
			break;
	}

	for(j = i ; i < online-2 ; j++){
		strcpy(onlineUser[j].username,onlineUser[j+1].username);
		onlineUser[j].connfd = onlineUser[j+1].connfd;
	}
	online--;
	close(childthread[index].connfd);
	childthread[index].loged = 0;
	pthread_mutex_unlock(&mutex);
	broadcast(username,0);
	printf("Logoff successfully\n");
}

void abnormal_logoff(char *username){
	printf("User abnormal disconnection, the username id %s   ",username);
	int i,j;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(userList[i].username, username) == 0)
			break;
	}
	for(j = i ; i < online-2 ; j++){
		strcpy(onlineUser[j].username,onlineUser[j+1].username);
		onlineUser[j].connfd = onlineUser[j+1].connfd;
	}
	online--;
	pthread_mutex_unlock(&mutex);
	broadcast(username,0);
}

void handleMessage(int connfd,int index){
	char desuser[10];
	strcpy(username,childthread[index].recvpkg.srcuser);
	strcpy(desuser,childthread[index].recvpkg.desuser);
	printf("%s send message to %s : %s   ",username,desuser,childthread[index].recvpkg.message);
	memset(&childthread[index].sendpkg, 0 , MAXLINE);
	init_pkg(&childthread[index].sendpkg);
	childthread[index].sendpkg.service = MESSAGE;
	strcpy(childthread[index].sendpkg.srcuser, username);
	strcpy(childthread[index].sendpkg.desuser, desuser);
	strcpy(childthread[index].sendpkg.message, childthread[index].recvpkg.message);
	childthread[index].sendpkg.length = strlen(childthread[index].sendpkg.message);
	int fd,i;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(desuser,onlineUser[i].username)==0){
			fd = onlineUser[i].connfd;
			send(fd, (void *)&childthread[index].sendpkg, childthread[index].sendpkg.length + HEADLINE, 0);
			printf("send successfully\n");
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
}

void handleInform(int connfd,int index){
	int i;
	memset(&sendpkg, 0 , MAXLINE);
	init_pkg(&sendpkg);
	sendpkg.service = INFORM;
	strcpy(sendpkg.srcuser, childthread[index].recvpkg.srcuser);
	strcpy(sendpkg.message, childthread[index].recvpkg.message);
	sendpkg.length = childthread[index].recvpkg.length;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		if(strcmp(onlineUser[i].username,childthread[index].recvpkg.srcuser)!=0){
			send(onlineUser[i].connfd, (void *)&sendpkg, sendpkg.length + HEADLINE, 0);
		}
	}
	pthread_mutex_unlock(&mutex);
}

void handleOnlinefriends(int connfd,int index){
	strcpy(username,childthread[index].recvpkg.srcuser);
	printf("%s want to know online friends    ",username);
	memset(&childthread[index].sendpkg, 0 , MAXLINE);
	init_pkg(&childthread[index].sendpkg);
	childthread[index].sendpkg.service = ONLINEFRIENDS;
	strcpy(childthread[index].sendpkg.srcuser, username);
	childthread[index].sendpkg.status = 0x00 + online;
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0 ; i < online ; i++){
		strcpy(&childthread[index].sendpkg.message[10*i],onlineUser[i].username);
	}
	send(connfd, (void *)&childthread[index].sendpkg, HEADLINE + 10*online , 0);
	pthread_mutex_unlock(&mutex);	
	printf("success \n");
}

void handleHeartbeat(int connfd,int index){
	printf("Receive heartbeat from %s \n",childthread[index].recvpkg.srcuser);
	if(childthread[index].recvpkg.status == REPLY){
		pthread_mutex_lock(&mutex);
		strcpy(onlineUser[online].username,childthread[index].recvpkg.srcuser);
		onlineUser[online].connfd = connfd;
		online++;
		printf("online %d\n",online);
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
	alarm(30);
}

void sendheartbeat(){
	int i;
	pthread_mutex_lock(&mutex);
	//online = 0;
	for(i = online-1 ; i >= 0 ; i--){
		printf("Send heartbeat to %s \n",onlineUser[i].username);
		strcpy(sendpkg.srcuser, onlineUser[i].username);
		sendpkg.service = HEARTBEAT;
		sendpkg.status = REQUEST;
		send(onlineUser[i].connfd, (void *)&sendpkg, HEADLINE, 0);
		online -- ;
	}
	pthread_mutex_unlock(&mutex);
	//sleep(5);
	checkheartbeat();
	alarm(30);
}

void checkheartbeat(){
}

void init_pkg(struct Package *pkg){
	strcpy(pkg->proname, "ZRQP");
	pkg->length = 0;
	pkg->service = INIT;
}

int findValid(){
	int i;
	for( i = 0 ; i < LISTENQ ; i++){
		if(childthread[i].used != 1)
			return i;
	}
	return -1;
}
