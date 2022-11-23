#include <stdio.h>
#include <string.h>
#include <winsock2.h> 
#include <windows.h>
#include <ctime>
#include <thread>
#include <iostream>
#define RECV_SIZE 32//禁止修改
#define SEND_SIZE 256//根据情况修改
#define PORT 37982//根据情况修改
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))
using namespace std;
//客户端服务端反转模式
int client_addr_len;
WORD winsock_version;
WSADATA wsa_data;
SOCKET server_socket,client_socket;
struct sockaddr_in server_addr,client_addr;
const char echo[SEND_SIZE+1]="!Connected!!";//回应客户端
const char ques[RECV_SIZE+1]="!Connected??";//接收客户端的心跳
const char done[RECV_SIZE+1]="!Finished!!";
const char stop[RECV_SIZE+1]="#<STOP>";
bool locked;

void print(const char *str){printf("[% *.3f] %s\n",10,(double)clock()/1000,str);}
void print(const char *str1,const char *str2){printf("[% *.3f] %s%s\n",10,(double)clock()/1000,str1,str2);}
bool cmp(const char *str1,const char *str2,int len){
	while(len--){
		if(*str1!=*str2)return 1;
		str1++,str2++;
	}
	return 0;
}
void init(){
	winsock_version=MAKEWORD(2,2);
	if(WSAStartup(winsock_version,&wsa_data)!=0)print("Failed to init socket dll!"),exit(1);
	server_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(server_socket==INVALID_SOCKET)print("Failed to create server socket!"),exit(1);
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(PORT);
	server_addr.sin_addr.S_un.S_addr=INADDR_ANY;
	if(bind(server_socket,(LPSOCKADDR)&server_addr,sizeof(server_addr))==SOCKET_ERROR)print("Failed to bind port!"),exit(1);
	if(listen(server_socket,10))print("Failed to listen!"),exit(1);
	client_addr_len=sizeof(client_addr);
	print("Waiting for connecting...");
	client_socket=accept(server_socket,(SOCKADDR*)&client_addr,&client_addr_len);
	if(client_socket==INVALID_SOCKET)print("Failed to accept!"),exit(1);
	print("Succeeded to receive a connection: ",inet_ntoa(client_addr.sin_addr));
}
void response(){
	send(client_socket,echo,SEND_SIZE,0);
}
void auto_send(){
	char send_data[SEND_SIZE+1];
	while(1){
		printf("Input command:~# ");
		memset(send_data,0,sizeof send_data);
		cin.getline(send_data,SEND_SIZE);
		int flag=3;
		while(flag!=-1&&flag!=0){
			if(send(client_socket,send_data,SEND_SIZE,0)<0)print("Failed to send command! Retrying..."),flag--,sleep(100);
			else flag=-1,locked=1;
		}
		if(flag==0)print("Send error!");
		while(locked)sleep(1);//replace with mutex ?
		sleep(100);
	}
}
int main(){
	init();
	thread cmd_send(auto_send);
	cmd_send.detach();
	
	char recv_data[RECV_SIZE+1];
	while(1){
		memset(recv_data,0,sizeof(recv_data));
		int len=recv(client_socket,recv_data,RECV_SIZE,0);
		if(len<0){
			fflush(stdout);
			sleep(100);
			printf("\n\nFailed to get response! Client may close the connection!\n\n");
			break;
		}else{
			if(strcmp(recv_data,ques)==0){
				response();
			}else if(strcmp(recv_data,done)==0){
				fflush(stdout);
				locked=0;
			}else{
				for(int i=0;i<len;i++){
					if(strcmp(recv_data+i,stop)==0)break;
					putchar(recv_data[i]);
				}
			}
		}
	}
	
	closesocket(client_socket);
	closesocket(server_socket);
	WSACleanup();
	system("Pause");
	return 0;
}
