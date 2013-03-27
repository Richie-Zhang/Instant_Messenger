#include "include.h"

int online,user;
struct Account userList[100];
struct OnlineAccount onlineUser[100];
int listenfd, connfd, n;
socklen_t clilen;
struct Package sendpkg,recvpkg;
struct sockaddr_in cliaddr, servaddr;
char username[10];
char password[20];
int k;

int main(int argc, char **argv){

	init_pkg(&sendpkg);
	init_pkg(&recvpkg);
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
	
	pthread_t handlethread[100];
	
	while(1){
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
		printf("Received request ...\n");
		
		
		int ret = pthread_create(&handlethread[k],NULL,(void*)handleThread,(void *)connfd);
		if (ret != 0){
			printf("Create thread error!\r\n");
			exit(1);
		}
		
		pthread_join(handlethread[k],NULL);
		k++ ;
		
	}
	
	return 0;
}

void handleThread(void * connfd){
	//pthread_mutex_t sendmutex = PTHREAD_MUTEX_INITIALIZER;
	//pthread_mutex_t recvmutex = PTHREAD_MUTEX_INITIALIZER;
	
	//pthread_mutex_lock (&recvmutex);
	while((n = recv((int)connfd, (void *)&recvpkg, MAXLINE, 0) > 0)){
		printf("%c",recvpkg.service + 'a');
		switch(recvpkg.service){
			case(REGIST):	{handleRegist();break;}
			case(LOGON):	{handleLogon();	break;}
			case(LOGOFF):	{handleLogoff();break;}
			//case(HEARTBEAT):{handleHeartbeat();	break;}
			case(MESSAGE):	{handleMessage();	break;}
			case(ONLINEFRIENDS):{handleOnlinefriends();	break;}
			default:		{printf("Unknown package!!!\n");}
		}
	}
	if(n < 0)	printf("Read error\n");
	//pthread_mutex_unlock (&recvmutex);
	exit(0);
}

void handleRegist(){
	
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
		sendpkg.service = REGIST;
		strcpy(sendpkg.srcuser, username);
		sendpkg.status = ACCEPT;
		send(connfd, (void *)&sendpkg, HEADLINE, 0);
		printf("Regist successfully\n");
	}
	else{
		sendpkg.service = REGIST;
		strcpy(sendpkg.srcuser, username);
		sendpkg.status = REFUSE;
		send(connfd, (void *)&sendpkg, HEADLINE, 0);
		printf("Regist failed\n");
	}
}

void handleLogon(){
	strcpy(username,recvpkg.srcuser);
	strcpy(password,recvpkg.message);
	printf("User logon, the username id %s,the password is %s   ",username,password);
	int i;
	int success = 0;
	for(i = 0 ; i < user ; i++){
		if(strcmp(userList[i].username,username) == 0)
			break;
	}
	if(i < user){
		if(strcmp(userList[i].password,password) == 0){
			strcpy(onlineUser[online].username,username);
			onlineUser[online].connfd = connfd;
			online++;
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
		sendpkg.service = LOGON;
		strcpy(sendpkg.srcuser, username);
		sendpkg.status = REFUSE;
		send(connfd, (void *)&sendpkg, HEADLINE, 0);
		printf("Logon failed\n");
	}
}

void handleLogoff(){
	strcpy(username,recvpkg.srcuser);
	printf("User logoff, the username id %s   ",username);
	int i;
	for(i = 0 ; i < user ; i++){
		if(strcmp(onlineUser[i].username,username) == 0)
			break;
	}
	if(i < user){
		int i,j;
		for(i = 0 ; i < online ; i++){
			if(strcmp(userList[i].username, username) == 0)
				break;
		}
		close(onlineUser[i].connfd);
		for(j = i ; i < online-2 ; j++){
			strcpy(onlineUser[j].username,onlineUser[j+1].username);
			onlineUser[j].connfd = onlineUser[j+1].connfd;
		}
		online--;
		broadcast(username,0);
		printf("Logoff successfully\n");
	}
}

void handleMessage(){
	char desuser[10];
	strcpy(username,recvpkg.srcuser);
	strcpy(desuser,recvpkg.desuser);
	printf("%s send message to %s : %s   ",username,desuser,recvpkg.message);
	sendpkg.service = MESSAGE;
	strcpy(sendpkg.srcuser, username);
	strcpy(sendpkg.desuser, desuser);
	strcpy(sendpkg.message, recvpkg.message);
	sendpkg.length = strlen(sendpkg.message);
	int fd,i;
	for(i = 0 ; i < online ; i++){
		if(strcmp(desuser,onlineUser[i].username)==0){
			fd = onlineUser[i].connfd;
			send(fd, (void *)&sendpkg, sendpkg.length + HEADLINE, 0);
			printf("send successfully\n");
			break;
		}
	}	
}

void handleOnlinefriends(){
	strcpy(username,recvpkg.srcuser);
	printf("%s want to know online friends    ",username);
	sendpkg.service = ONLINEFRIENDS;
	strcpy(sendpkg.srcuser, username);
	sendpkg.status = 0x00 + online;
	int i;
	for(i = 0 ; i < online ; i++){
		strcpy(&sendpkg.message[10*i],onlineUser[i].username);
	}
	send(connfd, (void *)&sendpkg, HEADLINE + 10*online , 0);	
	printf("success \n");
}

void broadcast(char *username, int type){		//通知所有在线用户，某用户上线或下线，上线1，下线0
	int i;
	sendpkg.service = UPDATE;
	if(type)	sendpkg.status = LOGON;
	else sendpkg.status = LOGOFF;
	strcpy(sendpkg.srcuser, username);
	for(i = 0 ; i < online ; i++){
		if(strcmp(onlineUser[i].username,username)!=0){
			send(onlineUser[i].connfd, (void *)&sendpkg, HEADLINE, 0);
		}
	}
}

void heartBeatThread(){
	while(1){
		int i,success;
		for(i = 0 ; i < online ; i++){
			success = 0;
			strcpy(sendpkg.srcuser, onlineUser[i].username);
			sendpkg.service = HEARTBEAT;
			sendpkg.status = REQUEST;
			send(onlineUser[i].connfd, (void *)&sendpkg, HEADLINE, 0);
			if(recv(onlineUser[i].connfd, (void *)&recvpkg, MAXLINE, 0) > 0){
				if(recvpkg.service == HEARTBEAT && recvpkg.status == REPLY)
					success = 1;
			}
			if(!success){
				close(onlineUser[i].connfd);
				int j;
				for(j = i ; j < online-2 ; j++){
					strcpy(onlineUser[j].username,onlineUser[j+1].username);
					onlineUser[j].connfd = onlineUser[j+1].connfd;
				}
				online--;
				broadcast(username,0);
			}
		}
	}
}

void init_pkg(struct Package *pkg){
	strcpy(pkg->proname, "ZRQP");
	pkg->length = 0;
	pkg->service = INIT;
}

