#ifndef _MTKWIFI_H
#define _MTKWIFI_H

//扫描当前环境的wifi信息
typedef struct{
	char ssid[64];
	char bssid[20];
	char security[23];	//WPA1PSK/WPA2PSK/AES/NONE/TKIP/OPEN
						//上一级路由为OPEN，这个加密模式获取到的值为:NONE
	char signal[9];	
	char mode[12];		//11b/g/n
	char ext_ch[7];
	char net_type[3];	//In
	char channel[4];	
}WifiResult;

#define CACHE_SIZE	64
typedef struct {
	char ssid[CACHE_SIZE];
	char passwd[CACHE_SIZE];
}WIFI;

typedef struct {
	char ssid[CACHE_SIZE];
	char passwd[CACHE_SIZE];
	void (*connetEvent)(int event);
	void (*enableGpio)(void);
}ConnetWIFI;

#ifdef DEBUG_WIFI
#define DBGWIFI(fmt,args...)	printf("" fmt, ## args)
#else
#define DBGWIFI(fmt,args...)	{}
#endif

extern void debugWifiResult(WifiResult *wifiresult);

/*
@使能中继连接
*/
extern void ApcliEnable(char *ssid,char *bssid,char *mode,char *enc,char *passwd,char *channel);
/*
@ mtk系列芯片扫描当前环境下ssid
@参数:	data传入的参数 *size 
		getWifiResult获取到当前之后回调函数,返回非0 的值退出扫描
@返回值: 
*/
extern int __mtkScanWifi(void *data,int *size,int getWifiResult(void *data,int *size,WifiResult *wifiresult));
/*
@ mtk系列芯片扫描当前环境下ssid
@参数:	wifi获取到当前网络环境的ssid信息
		len 总的长度
	数据格式:
	ssid&+security&+signal&-
@返回值: 
*/
extern void mtk76xxScanWifi(char *wifi,int *len);
/*
@恢复出厂设置
*/
extern void ResetDefaultRouter(void);
/*
@设置板子的发出的wifi名和密码
*/
extern void setBasicWifi(char *ssid,char *passwd);
/*
@读取板子的wifi
*/
extern void ReadWifi(viod);
/*
@缓存ssid和密码
*/
extern void SaveCacheSsid(char *saveSsid,char *savePasswd);
/*
@获取缓存wifi数据
*/
extern int getCacheWifi(WIFI **wifi,int *cacheSize);
/*
@释放获取缓存数据
*/
extern void freeCacheWifi(WIFI **wifi,int cacheSize);

#endif
