#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 

#include "ap_sta.h"
#include "mtkwifi.h"
#include "cJSON.h"
#include "pool.h"
#include "systools.h"
#include "nvram.h"

#define NET_SERVER_FILE_LOCK			"/var/server.lock"		//开机联网进程文件锁

#define INTEN_NETWORK_FILE_LOCK			"/var/internet.lock"	//连接文件锁，和联网进程当中，扫描和连接过程当中起到一个互斥文件锁作用，防止这边进程在配网状态，联网进程进行扫描和连接

#define SMART_CONFIG_FILE_LOCK			"/var/SmartConfig.lock"	//配网文件锁,防止第开机过程自动连接网络冲突

#define LOCAL_SERVER_FILE_LOCK			"/var/localserver.lock"	//本地服务器文件锁

#define ENABLE_RECV_NETWORK_FILE_LOCK	"/var/startNet.lock"	//使能接收文件锁

#define CLOSE_SYSTEM_LOCK_FILE			"/var/close_system.lock"


static unsigned char connetState=UNLOCK_SMART_CONFIG_WIFI;
static int sockfd=-1;
static struct sockaddr_in wifiAddr,localserverAddr;
int checkconnetState(void){
	if(connetState==UNLOCK_SMART_CONFIG_WIFI)
		return 0;
	else
		return -1;
}
static void createSmartConfigLock(void){
	FILE *fp = fopen(SMART_CONFIG_FILE_LOCK,"w+");
	if(fp){
		fclose(fp);
	}
}
static void delSmartConfigLock(void){
	remove(SMART_CONFIG_FILE_LOCK);
}
static void createInternetLock(void){
	FILE *fp =fopen(INTEN_NETWORK_FILE_LOCK,"w+");
	if(fp){
		fclose(fp);
	}
}
static void delInternetLock(void){
	remove(INTEN_NETWORK_FILE_LOCK);
}
int SendtoServicesWifi(const void *msg,int size){
	return sendto(sockfd,msg,size,0,(const struct sockaddr *)&wifiAddr,sizeof(struct sockaddr_in));
}
int SendtoLocalserver(const void *msg,int size){
	return sendto(sockfd,msg,size,0,(const struct sockaddr *)&localserverAddr,sizeof(struct sockaddr_in));
}

int SendSsidPasswd_toNetServer(const char *ssid,const char *passwd,int random){
	int ret=0;
	char* szJSON = NULL;
	cJSON* pItem = NULL;
	pItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pItem, "handler", "ServerWifi");
	cJSON_AddNumberToObject(pItem, "event",SMART_CONFIG_OK);
	cJSON_AddStringToObject(pItem, "ssid",ssid);
	cJSON_AddStringToObject(pItem, "passwd",passwd);
	cJSON_AddNumberToObject(pItem, "random",random);
	szJSON = cJSON_Print(pItem);
	ret= SendtoServicesWifi(szJSON,strlen(szJSON));
	cJSON_Delete(pItem);
	free(szJSON);
	return ret ;
}
int SendSmartState(int event){
	int ret=0;
	char* szJSON = NULL;
	cJSON* pItem = NULL;
	pItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pItem, "handler", "ServerWifi");
	cJSON_AddNumberToObject(pItem, "event",event);
	szJSON = cJSON_Print(pItem);
	ret= SendtoLocalserver(szJSON,strlen(szJSON));
	cJSON_Delete(pItem);
	free(szJSON);
	return ret ;
}

/*******************************************************
鍑芥暟鍔熻兘: 妫�鏌ラ厤缃戞枃浠�
鍙傛暟: 鏃�
杩斿洖鍊�: 鏃�
********************************************************/
int checkInternetFile(void){
	if(access(INTEN_NETWORK_FILE_LOCK,0) < 0){
		return -1;
	}
	return 0;
}
static int GetSsidAndPasswd(char *smartData,char *ssid,char *passwd,int *random){
	int smartconfig_headlen=strlen(SMART_CONFIG_HEAD);	
	char wifidata[200]={0};
	snprintf(wifidata,200,"%s",smartData+smartconfig_headlen);
	cJSON *pJson = cJSON_Parse((const char *)wifidata);
	if(NULL == pJson){
		return -1;
	}
	cJSON * pSub = cJSON_GetObjectItem(pJson, "ssid");
	if(pSub!=NULL){
		if(strcmp(pSub->valuestring,"")){
			sprintf(ssid,"%s",pSub->valuestring);
			pSub = cJSON_GetObjectItem(pJson, "pwd");
			sprintf(passwd,"%s",pSub->valuestring);
			pSub = cJSON_GetObjectItem(pJson, "random");
			if(pSub){
				printf("recv random = %d\n",pSub->valueint);
				*random=pSub->valueint;
			}
			return 0;
		}
	}	
	return -1;
}
static int Compare_PIDName(char *pid_name){
	if(!strcmp(pid_name,"NetManger")){
		return 0;
	}
	return -1;
}
//閲嶆柊杩愯NetManger 杩欎釜杩涚▼
static void Restart_RunNetManger(void){
	system("NetManger -t 2 -wifi on &");
	sleep(1);
}
//妫�鏌ュ悗鍙拌仈缃戣繘绋�(NetManger)杩愯鐘舵��
void CheckNetManger_PidRunState(void){
	if(judge_pid_exist(Compare_PIDName)){
		remove(NET_SERVER_FILE_LOCK);
		remove(INTEN_NETWORK_FILE_LOCK);	
		Restart_RunNetManger();
	}
}

//妫�鏌ョ綉缁滄鏌ヨ繍琛岀姸鎬�,寮傚父鐘舵�佸惎鍔紝鑷姩鍚姩鑱旂綉杩涚▼
static void *CheckNetWork_taskRunState(void *arg){
	int timeOut=0;
	while(1){
		if(++timeOut>5){
			break;
		}
		CheckNetManger_PidRunState();
		sleep(2);
	}
	return NULL;
}

static void RunSmartConfig_Task(void){
  	FILE *fp=NULL;
   	char *buf, *ptr;
	char ssid[64]={0},pwd[64]={0};
	int ret=-1,timeout=0;
	int random=0;
	buf = (char *)calloc(512,1);
	if(buf==NULL){
		perror("calloc failed ");
		goto exit0;
	}	
	createSmartConfigLock();
	system("iwpriv apcli0 elian start");
	while (++timeout<400){	//100--->400
		//蹇呴』瀹炴椂鎵撳紑绠￠亾锛屾墠鑳借鍙栧埌鏇存柊鐨勬暟鎹�
		if((fp = popen("iwpriv apcli0 elian result", "r"))==NULL){
			fprintf(stderr, "%s: iwpriv apcli0 elian result failed !\n", __func__);
			break;
		}
		memset(buf,0,512);
		fgets(buf, 512, fp);
		ptr =buf; 

		if(!GetSsidAndPasswd(ptr,ssid,pwd,&random)){
			printf("ssid:%s   pwd:%s\n",ssid,pwd);
			ret=0;
			break;
		}
		usleep(100000);
		
		memset(ssid,0,64);
		memset(pwd,0,64);
		pclose(fp);
	}
	timeout=0;
	system("iwpriv apcli0 elian stop");
	system("iwpriv apcli0 elian clear");
	if(ret==0){
		SendSsidPasswd_toNetServer(ssid,pwd,random);//宸茬粡鎺ユ敹鍒皊sid 鍜� passwd
		delSmartConfigLock();
		sleep(5);
		while(++timeout<40){	//绛夊緟閰嶇綉鎴愬姛鍚庯紝浣胯兘鎸夐敭
			sleep(1);
			if(checkInternetFile()){
				sleep(5);
				break;
			}
		}
		if(timeout>=40){
			delInternetLock();		//闃叉鑱旂綉杩涚▼鍑虹幇闂锛岄渶瑕佹墜鍔ㄥ垹闄�  2017-03-05-22:57
		}
	}else{
		SendSmartState(22); //娌℃湁鏀跺埌app鍙戦�佽繃鏉ョ殑ssid鍜宲asswd
		delSmartConfigLock();
		delInternetLock();	//涓婂崐娈佃В鏂囦欢閿�
	}
	free(buf);
exit0:	

	printf("enable gpio \n");
}
int initAddrNet(void){
	sockfd =create_listen_udp(NULL,20023);
   	if(sockfd<=0){
	    perror("create udp socket failed ");
    	return -1;
  	}
    char IP[20]={0};
    if(GetNetworkcardIp("br0",IP)){
  		perror("get br0 ip failed");
		return -1;
  	}
  	init_addr(&wifiAddr, IP,  20003);
	init_addr(&localserverAddr, IP,  20001);
   	return 0;
}

int main(int argc,char **argv){
	if(argc<2){
		return -1;
	}
	initAddrNet();
	createInternetLock();	//涓婂崐娈典笂鏂囦欢閿�
	if(!strcmp(argv[1],"restart")){
		char wifiBuf[128]={0};
		char passwdBuf[64]={0};
		char *wifi = nvram_bufget(RT2860_NVRAM, "ApCliSsid");
		snprintf(wifiBuf,128,"%s",wifi);
		char *passwd = nvram_bufget(RT2860_NVRAM, "ApCliWPAPSK");
		snprintf(passwdBuf,64,"%s",passwd);
		SendSsidPasswd_toNetServer(wifiBuf,passwdBuf,1);//宸茬粡鎺ユ敹鍒皊sid 鍜� passwd
		CheckNetWork_taskRunState(NULL);
	}else{
		RunSmartConfig_Task();
		CheckNetWork_taskRunState(NULL);
	}
	return 0;
}
