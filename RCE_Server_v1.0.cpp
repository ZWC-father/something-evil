#include <bits/stdc++.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h> 
#include <ctime>
#include <cstdlib>
#include <windows.h>
#define RECV_SIZE 16//禁止修改
#define SEND_SIZE 256//根据情况修改
#define PORT 41862//根据情况修改
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))
using namespace std;

int client_addr_len;
WORD winsock_version;
WSADATA wsa_data;
SOCKET server_socket,client_socket;
struct sockaddr_in server_addr,client_addr;
bool locked;

void print(const char *str){printf("[% *.3f] %s\n",10,(double)clock()/1000,str);}
void print(const char *str1,const char *str2){printf("[% *.3f] %s%s\n",10,(double)clock()/1000,str1,str2);}
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
	if(send(client_socket,"Keep connection!",16,0)<0)print("Failed to respond client!");
}
void auto_recv(){
	char recv_data[RECV_SIZE*64+1];
	while(1){
		int len=recv(client_socket,recv_data,RECV_SIZE*64,0);
		if(len<0){
			print("Failed to get response! Client may close the connection.");
			break;
		}else if(len==0){
			print("Received data damaged! Skip!");
		}else{
			recv_data[len]=0;
			if(recv_data[0]=='!')print("Command process return value: ",recv_data+1),fflush(stdout),locked=0;
			else response();
		}
	}
}
int main(){
	init();
	thread cmd_recv(auto_recv);
	cmd_recv.detach();

	char send_data[SEND_SIZE+2];
	while(1){
		printf("Input command:\n");
		memset(send_data,0,sizeof send_data);
		cin.getline(send_data+1,SEND_SIZE);
		send_data[0]='!';
		int flag=3;
		while(flag!=-1&&flag!=0){
			if(send(client_socket,send_data,SEND_SIZE+1,0)<0)print("Failed to send command! Retrying..."),flag--;
			else flag=-1,locked=1;
		}
		if(flag==0)print("Send error!");
		while(locked)sleep(1);
		sleep(200);
	}

	closesocket(client_socket);
	closesocket(server_socket);
	WSACleanup();
	system("Pause");
	return 0;
}
