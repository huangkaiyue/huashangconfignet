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

#define NET_SERVER_FILE_LOCK			"/var/server.lock"		//¿ª»úÁªÍø½ø³ÌÎÄ¼şËø

#define INTEN_NETWORK_FILE_LOCK			"/var/internet.lock"	//Á¬½ÓÎÄ¼şËø£¬ºÍÁªÍø½ø³Ìµ±ÖĞ£¬É¨ÃèºÍÁ¬½Ó¹ı³Ìµ±ÖĞÆğµ½Ò»¸ö»¥³âÎÄ¼şËø×÷ÓÃ£¬·ÀÖ¹Õâ±ß½ø³ÌÔÚÅäÍø×´Ì¬£¬ÁªÍø½ø³Ì½øĞĞÉ¨ÃèºÍÁ¬½Ó

#define SMART_CONFIG_FILE_LOCK			"/var/SmartConfig.lock"	//ÅäÍøÎÄ¼şËø,·ÀÖ¹µÚ¿ª»ú¹ı³Ì×Ô¶¯Á¬½ÓÍøÂç³åÍ»

#define LOCAL_SERVER_FILE_LOCK			"/var/localserver.lock"	//±¾µØ·şÎñÆ÷ÎÄ¼şËø

#define ENABLE_RECV_NETWORK_FILE_LOCK	"/var/startNet.lock"	//Ê¹ÄÜ½ÓÊÕÎÄ¼şËø

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
å‡½æ•°åŠŸèƒ½: æ£€æŸ¥é…ç½‘æ–‡ä»¶
å‚æ•°: æ— 
è¿”å›å€¼: æ— 
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
//é‡æ–°è¿è¡ŒNetManger è¿™ä¸ªè¿›ç¨‹
static void Restart_RunNetManger(void){
	system("NetManger -t 2 -wifi on &");
	sleep(1);
}
//æ£€æŸ¥åå°è”ç½‘è¿›ç¨‹(NetManger)è¿è¡ŒçŠ¶æ€
void CheckNetManger_PidRunState(void){
	if(judge_pid_exist(Compare_PIDName)){
		remove(NET_SERVER_FILE_LOCK);
		remove(INTEN_NETWORK_FILE_LOCK);	
		Restart_RunNetManger();
	}
}

//æ£€æŸ¥ç½‘ç»œæ£€æŸ¥è¿è¡ŒçŠ¶æ€,å¼‚å¸¸çŠ¶æ€å¯åŠ¨ï¼Œè‡ªåŠ¨å¯åŠ¨è”ç½‘è¿›ç¨‹
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
		//å¿…é¡»å®æ—¶æ‰“å¼€ç®¡é“ï¼Œæ‰èƒ½è¯»å–åˆ°æ›´æ–°çš„æ•°æ®
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
		SendSsidPasswd_toNetServer(ssid,pwd,random);//å·²ç»æ¥æ”¶åˆ°ssid å’Œ passwd
		delSmartConfigLock();
		sleep(5);
		while(++timeout<40){	//ç­‰å¾…é…ç½‘æˆåŠŸåï¼Œä½¿èƒ½æŒ‰é”®
			sleep(1);
			if(checkInternetFile()){
				sleep(5);
				break;
			}
		}
		if(timeout>=40){
			delInternetLock();		//é˜²æ­¢è”ç½‘è¿›ç¨‹å‡ºç°é—®é¢˜ï¼Œéœ€è¦æ‰‹åŠ¨åˆ é™¤  2017-03-05-22:57
		}
	}else{
		SendSmartState(22); //æ²¡æœ‰æ”¶åˆ°appå‘é€è¿‡æ¥çš„ssidå’Œpasswd
		delSmartConfigLock();
		delInternetLock();	//ä¸ŠåŠæ®µè§£æ–‡ä»¶é”
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
	initAddrNet();
	createInternetLock();	//ä¸ŠåŠæ®µä¸Šæ–‡ä»¶é”
	RunSmartConfig_Task();
	CheckNetWork_taskRunState(NULL);
	return 0;
}
