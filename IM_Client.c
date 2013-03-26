#include "include.h"

int sockfd;
struct sockaddr_in servaddr;
struct Package sendpkg,recvpkg;
char username[10];
char password[20];
int vailduser;
int func;
int loged;
int onlineUser;
char userList[100][10];
char usertochat[10];

char SERV_IP[15] = "172.26.58.97";

int main(int argc, char **argv){

/*-----------------------------------------------------------------------------------------------------------------------*/
//创建socket，并连接到服务器上

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){		//创建socket
		perror("Socket created error\n");
		exit(1);
	}

	memset(&servaddr, 0, sizeof(servaddr));			//设置服务器套接字地址
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(SERV_IP);
	servaddr.sin_port = htons(SERV_PORT);

	if( connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){	//连接服务器
		perror("Connect error\n");
	}
	
	init_pkg(sendpkg);
	init_pkg(recvpkg);

	printf("Connect to the server successfully!!!\n");

/*---------------------------------------------------------------------------------------------------------------------*/
//用户注册帐号，或者登录过程，成功登录则进行下一步操作

	while(!loged){
		printf("Please choose the function(1--Regist; 2--Logon): \n");
		scanf("%d", &func);
		if(func == 1)	regist();	//用户注册帐号过程
		else if(func == 2)	logon();	//用户登录过程
		else printf("Enter error!!!\n");
	}

/*---------------------------------------------------------------------------------------------------------------------*/
//登录成功的用户选择查看在线好友，以及聊天功能

	pthread_t recvthread;
	int ret = pthread_create(&recvthread,NULL,(void*)receiveMsg,NULL);
	if (ret != 0){
		printf("Create thread error!\r\n");
		exit(1);
	}
	pthread_join(recvthread,NULL);

	system("clear");
	printf("You are online,and your username is %s", username);
	printf("Please choose the function\n(1--List online friends; 2--Send messages to one online friend; 3--Log off and exit): \n");
	while(1){
		scanf("%d", &func);
		switch(func){
			case(1):{listfri();	break;}
			case(2):{chat();	break;}
			case(3):{logoff();	break;}
			default:{printf("Input error,Please enter again!!\n");}
		}
	}
	
	return 0;
}

void receiveMsg(){
	while(1){
		if(recv(sockfd, (void *)&recvpkg, MAXLINE, 0) == 0){	//
			perror("Receive error\n");
			exit(1);
		}
		switch (recvpkg.service){
			case(HEARTBEAT):	{sendheart();	break;}
			case(MESSAGE):		{printf("%s : %s\n", recvpkg.desuser, recvpkg.message);	break;}
			case(UPDATE):	{updatelist();	break;}
			default:{printf("Unknown package");}
		}
	}
}


void init_pkg(struct Package pkg){
	strcpy(pkg.proname, "ZRQP");
	pkg.length = 0;
	pkg.service = INIT;
}


void regist(){
	system("clear");
	printf("Please enter your username and password you want to regist.\n");
	while(!vailduser){
		printf("username(less than 10 char):");
		scanf("%s", username);
		printf("password(less than 20 char):");
		scanf("%s", password);
		sendpkg.service = REGIST;
		strcpy(sendpkg.srcuser, username);
		strcpy(sendpkg.message, password); 
		sendpkg.length = strlen(password);
		send(sockfd, (void *)&sendpkg, sendpkg.length + HEADLINE, 0);	//将用户名和密码信息send到服务器
		
		if(recv(sockfd, (void *)&recvpkg, MAXLINE, 0) == 0){	//receive服务器返回的数据	
			perror("Receive error\n");
			exit(1);
		}
		if(recvpkg.status == REFUSE)
			printf("The username you entered has been registed, please try another one.\n");
		else if(recvpkg.status == ACCEPT){
			vailduser = 1;
			printf("Register successfully !!!\n");
		}
	}
}


void logon(){
	system("clear");
	printf("Please enter your username and password to login on.\n");
	while(!vailduser){
		printf("username(less than 10 char):");
		scanf("%s", username);
		printf("password(less than 20 char):");
		scanf("%s", password);
		sendpkg.service = LOGON;
		strcpy(sendpkg.srcuser, username);
		strcpy(sendpkg.message, password); 
		sendpkg.length = strlen(password);
		send(sockfd, (void *)&sendpkg, sendpkg.length + HEADLINE, 0);	//将用户名和密码信息send到服务器
		if(recv(sockfd, (void *)&recvpkg, MAXLINE, 0) == 0){	//receive服务器返回的数据	
			perror("Receive error\n");
			exit(1);
		}
		if(recvpkg.status == REFUSE)
			printf("Username or password is wrong, please try again.\n");
		else if(recvpkg.status == ACCEPT){ 
			vailduser = 1;
			printf("Login on successfully !!!\n");
			loged = 1;
		}
	}
}

void listfri(){
	sendpkg.service = ONLINEFRIENDS;
	strcpy(sendpkg.srcuser, username);
	sendpkg.status = REQUEST;
	send(sockfd, (void *)&sendpkg, HEADLINE, 0);	//向服务器请求在线好友列表
	if(recv(sockfd, (void *)&recvpkg, MAXLINE, 0) == 0){	//receive服务器返回的好友列表
		perror("Receive error\n");
		exit(1);
	}
	if(recvpkg.service == ONLINEFRIENDS){
		onlineUser = recvpkg.status - 0x00;
		int i;
		for(i = 0 ; i < onlineUser ; i++){
			sscanf(&recvpkg.message[10*i],"%s",userList[i]); 
		}
		printf("There are %d friends online, their username follows:\n", onlineUser);
		for(i = 0 ; i < onlineUser ; i++){
			printf("%d.%s,	",  i+1 , userList[i]);
			if(i%5 == 4)
				printf("\n");
		}
	}
	else{
		printf("Faild to get the list of online friends, please try later !!!\n");
	}
}

void chat(){
	printf("Please choose a friend to chat with(Input the username):\n");
	scanf("%s",usertochat);
	int i;
	for(i = 0 ; i < onlineUser ; i++){
		if(strcmp(usertochat, userList[i]) == 0)
			break;
	}
	if( i == onlineUser)
		printf("The friend you entered is offline, you can't send message to him(her)\n");
	else{
		printf("Please enter the message you want to sent to him(her):\n");
		scanf("%s", sendpkg.message);
		sendpkg.service = MESSAGE;
		strcpy(sendpkg.srcuser, username);
		strcpy(sendpkg.desuser, usertochat);
		sendpkg.length = strlen(sendpkg.message);
		send(sockfd, (void *)&sendpkg, sendpkg.length + HEADLINE, 0);	//发送消息至服务器
	}
}

void logoff(){
	sendpkg.service = LOGOFF;
	strcpy(sendpkg.srcuser, username);
	send(sockfd, (void *)&sendpkg, HEADLINE, 0);	//向服务器通知下线
	close(sockfd);
	system("clear");
	printf("Log off successfully!!!\n");
	exit(0);
}

void sendheart(){
	sendpkg.service = HEARTBEAT;
	sendpkg.status = REPLY;
	strcpy(sendpkg.srcuser, username);
	send(sockfd, (void *)&sendpkg, HEADLINE, 0);	//向服务器发送心跳响应包
}

void updatelist(){
	if(recvpkg.status == LOGON){
		strcpy(userList[onlineUser],recvpkg.srcuser);
		printf("User %s is online:\n", userList[onlineUser]);
		onlineUser++;
		printf("\n");
	}
	else if(recvpkg.status == LOGOFF){
		char temp[10];
		strcpy(temp,recvpkg.srcuser);
		printf("User %s is offline:\n", temp);
		int i ;
		for(i = 0 ; i < onlineUser ; i++){
			if(strcmp(userList[i], temp) == 0)
				break;
		}
		int j;
		for(j = i ; i < onlineUser-2 ; j++){
			strcpy(userList[j],userList[j+1]);
		}
		onlineUser--;
	}
}


