#include "hv/TcpServer.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h> 
#include <windows.h>
#include <thread>
#include <iostream>
#include <signal.h>
#include <mutex>
#include <queue>
#define READ_SIZE 252
#define SEND_SIZE 256
#define RECV_SIZE 64
#define PORT 23769
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))
using namespace std;
using namespace hv;

TcpServer srv;
unpack_setting_t upk;//拆包策略
queue<string>send_queue;
mutex mtx;
bool connected, ctrlflag;

char ques[RECV_SIZE + 1] = { 0x01,0x01,0x07,0x07 };//心跳
char echo[SEND_SIZE + 1] = { 0x02,0x02,0x07,0x07 };//反馈
char sint[SEND_SIZE + 1] = { 0x03,0x03,0x07,0x07 };//SIGINT
char stop[SEND_SIZE + 1] = { 0x07,0x07 };//终止位

void print(const char* str) { printf("[% *.3f] %s\n", 10, (double)clock() / 1000, str); }
void print(const char* str1, const char* str2) { printf("[% *.3f] %s%s\n", 10, (double)clock() / 1000, str1, str2); }
bool cmp(const char* str1, const char* str2, int len) {
	while (len--) {
		if (*str1++ != *str2++)return 1;
	}
	return 0;
}
string strproc(char* dest, int len) {
	string str;
	str.resize(SEND_SIZE);
	for (int i = 0; i < len; i++)str[i] = *dest++;
	str[len] = stop[0], str[len + 1] = stop[1];
	return str;
}
void signal_handler(int sig) {
	string str(sint, SEND_SIZE);
	if (sig == SIGINT) {
		if (connected) {
			mtx.lock();
			send_queue.push(str);
			mtx.unlock();
		}
		ctrlflag = 1;//重要:避免ctrl+c后的命令读取失败
	}
	signal(SIGINT, signal_handler);
}
void auto_send() {
	char read_data[READ_SIZE + 5] = { 0 };
	while (1) {
		sleep(1);
		if (connected) {
			memset(read_data, 0, sizeof read_data);
			if (!scanf_s("%[^\n]s", read_data, READ_SIZE) && ctrlflag) {//重要:避免ctrl+c后的命令读取失败
				ctrlflag = 0;
				continue;
			}
			getchar();
			int len;
			for (len = READ_SIZE - 1; len; len--) {
				if (isgraph(read_data[len - 1]))break;
			}
			if (!connected)continue;
			mtx.lock();
			send_queue.push(strproc(read_data, len));
			mtx.unlock();
		}
	}
}
void send_beat() {
	string str(echo, SEND_SIZE);
	if (connected) {
		mtx.lock();
		//		cout << "OKOKOKOK" << endl;
		send_queue.push(str);
		mtx.unlock();
	}
}
void display(char* recv_data) {
	int siz = 0;
	char chBuf[RECV_SIZE + 5] = { 0 };
	memcpy(chBuf, recv_data, RECV_SIZE);
	for (int i = 0; i < RECV_SIZE; i++) {
		if (cmp(chBuf + i, stop, 2) == 0)break;
		siz++;
	}
	if (siz == RECV_SIZE) {
		puts("");
		print("Recevied data damaged! Skip!");
		return;
	}
	fwrite(chBuf, sizeof(char), siz, stdout);
	fflush(stdout);
}

void onConnection(const SocketChannelPtr& channel) {//连接事件循环
	if (channel->isConnected()) {
		string peeraddr = channel->peeraddr();
		print("Client connected: ", peeraddr.c_str());
		mtx.lock();
		while (send_queue.size())send_queue.pop();
		mtx.unlock();
		connected = 1;
		setInterval(10, [channel](TimerID timerID) {//设置定时器回调函数
			if (channel->isConnected()) {
				mtx.lock();
				while (send_queue.size()) {
					//					if(send_queue.front()!="7777")cout << send_queue.front() << endl;
					channel->write(send_queue.front());
					send_queue.pop();
				}
				mtx.unlock();
			}
			else {
				killTimer(timerID);
			}
		});
	}
	else {
		puts("");
		print("Client disconnected");
		connected = 0;
	}
}
void onMessage(const SocketChannelPtr& channel, Buffer* buf) {
	if (cmp((char*)buf->data(), ques, 4) == 0)send_beat();
	else display((char*)buf->data());
}
void onWriteComplete(const SocketChannelPtr& channel, Buffer* buf) {
	//	printf("> %.*s\n", (int)buf->size(), (char*)buf->data());
}
int main() {
	print("Initializing...");

	hlog_disable();
	if (srv.createsocket(PORT) < 0)printf("Failed to create socket"), exit(1);
	srv.onConnection = onConnection;
	srv.onMessage = onMessage;
	srv.onWriteComplete = onWriteComplete;
	upk.mode = UNPACK_BY_FIXED_LENGTH;
	upk.fixed_length = RECV_SIZE;
	srv.setUnpack(&upk);
	srv.setThreadNum(1);
	srv.setMaxConnectionNum(1);

	thread cmd_send(auto_send);

	print("Initializing done!");

	signal(SIGINT, signal_handler);
	srv.start();
	cmd_send.detach();

	while (1)sleep(1000);

	return 0;
}