#ifndef _SYSTOOLS_H
#define _SYSTOOLS_H

// 头文件接口所在库函数为:　libsystools463.so

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

enum{ALL,USR,FREE};

extern int __safe_write(FILE *fp,const void *data,int input_size);
extern int GetStorageInfo(char * MountPoint,  //SD卡随便一个分区
                                 int *Capacity,  //  想要获取的空间大小
                                 int type); //获取什么类型的空间

//检查系统当中是否存在某个进程运行， 存在返回-1，不存在返回0
extern int judge_pid_exist(int get_pid_name(char *pid_name));

//将字符串str进行base64编码 ,返回处理结果
extern unsigned char *base64_encode(unsigned char *str,int inSize);
//将字符串code进行base64解码，返回处理结果
extern unsigned char *base64_decode(unsigned char *code,int inSize);

/****************************************************************
函数说明	:写入Log  日志
入口参数	:bLog  表明是否为日志文件,place 代码位置
返回类型	:
修改记录	:
用途功能	:如果是日志文件，将会在str前加上当前时间(格式如：2011-04-12 12:10:20)
****************************************************************/
extern int writeLog(const char *filename,const char *str);

/********************************************
功能：搜索字符串右边起的第一个匹配字符
********************************************/
extern char* Rstrchr(char* s, char x);
/********************************************
功能：把字符串转换为全小写
********************************************/
extern void   ToLowerCase(char* s);

/********************************************
功能：读取filename 这个文件内容到内存当中
返回指向filename文件当中内存指针，需要释放这块内存
********************************************/
extern char *readFileBuf(const char *filename);

#ifdef __cplusplus
};
#endif
#endif

