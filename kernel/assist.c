/**********************************************************
*	assist.c       //added by mingxuan 2019-5-17
***********************************************************/

#include "fat32.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"

extern CHAR cur_path[256];
extern u8* buf;

void MakeFullPath(PCHAR parent,PCHAR name,PCHAR fullpath)
{
	int i=0,j=0,len=0;
	len=strlen(parent);
	for(i=0;i<len;i++)
	{
		fullpath[i]=parent[i];
	}
	if(fullpath[i-1]!='\\')
	{
		fullpath[i++]='\\';
	}
	len=strlen(name);
	for(j=0;j<len;j++)
	{
		fullpath[i++]=name[j];
	}
}

void ChangeCurrentPath(PCHAR addpath)
{
	int i=0,len1=0,len2=0;

	len1=strlen(cur_path);
	if(strcmp(addpath,"..")==0)
	{
		for(i=len1-1;i>=0;i--)
		{
			if(cur_path[i]!='\\')
			{
				cur_path[i]=0;
			}else{
				cur_path[i]=0;
				break;
			}
		}
		len2=strlen(cur_path);
		if(len2<=2)//根目录
		{
			cur_path[len2]='\\';
		}
	}else if(strcmp(addpath,"\\")==0){
		for(i=len1;i>=0;i--)
		{
			if(cur_path[i]!=':')
			{
				cur_path[i]=0;
			}else{
				break;
			}
		}
		cur_path[i+1]='\\';
	}else{
		if(cur_path[len1-1]!='\\')
		{
			cur_path[len1]='\\';
			len1++;
		}
		len2=strlen(addpath);
		for(i=0;i<len2;i++)
		{
			cur_path[i+len1]=addpath[i];
		}
	}
}

void GetNameFromPath(PCHAR path,PCHAR name)
{
	int i=0,j=0,len=0;
	len=strlen(path);
	for(i=len-1;i>=0;i--)
	{
		if(path[i]=='\\')
		{
			break;
		}
	}
	for(i=i+1;i<len;i++)
	{
		name[j++]=path[i];
	}
}

void GetParentFromPath(PCHAR fullpath,PCHAR parent)
{
	int i,len;
	len=strlen(fullpath);
	for(i=len-1;;i--)
	{
		if(fullpath[i]=='\\')
		{
			break;
		}
	}
	for(i=i-1;i>=0;i--)
	{
		parent[i]=fullpath[i];
	}
}

STATE IsFullPath(PCHAR path)
{
	int i=0;
	for(i=0;i<2;i++)//只看是不是以盘符和:开头的
	{
		if(cur_path[i]!=path[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}

void ToFullPath(PCHAR path,PCHAR fullpath)
{
	int i=0,j=0,len=0;
	if(IsFullPath(path))
	{
		strcpy(fullpath,path);
	}else
	{
		len=strlen(cur_path);
		for(i=0;i<len;i++)
		{
			fullpath[i]=cur_path[i];
		}
		if(cur_path[i-1]!='\\')
		{
			fullpath[i++]='\\';
		}
		len=strlen(path);
		for(j=0;j<len;j++)
		{
			fullpath[i++]=path[j];
		}
	}
	len=strlen(fullpath);
	if(fullpath[len-1]=='\\')
	{
		fullpath[len-1]=0;
	}
}


void TimeToBytes(WORD result[])//t是一个WORD类型的数组，长度为2，第0个元素为日期，第1个元素为时间
{
	WORD year,month,day,hour,minute,second;
/*
	time_t nowtime;
	struct tm *timeinfo;
	time(&nowtime);
	timeinfo=localtime(&nowtime);
	year=timeinfo->tm_year + 1900;
	month=timeinfo->tm_mon + 1;
	day=timeinfo->tm_mday;
	hour=timeinfo->tm_hour;
	minute=timeinfo->tm_min;
	second=timeinfo->tm_sec;
*/
    year = 2018;
    month = 12;
    day = 27;
    hour = 14;
    minute = 30;
    second = 24;
	result[1]=hour*2048+minute*32+second/2;
	result[0]=(year-1980)*512+month*32+day;
}


void FormatFileNameAndExt(PCHAR filename,PCHAR name,PCHAR ext)
{
	UINT i=0,j=0,len=0;

	len=strlen(filename);
	for(i=0;i<len&&i<8;i++)
	{
		if(filename[i]=='.')
		{
			break;
		}else{
			if(filename[i]>='a'&&filename[i]<='z')
			{
				name[i]=filename[i]-32;
			}else{
				name[i]=filename[i];
			}
		}
	}
	for(j=i;j<8;j++)
	{
		name[j]=' ';
	}
	if(i<len)
	{
		j=0;
		for(i=i+1;i<len;i++)
		{
			if(filename[i]>='a'&&filename[i]<='z')
			{
				ext[j++]=filename[i]-32;
			}else{
				ext[j++]=filename[i];
			}
		}
		for(;j<3;j++)
		{
			ext[j]=' ';
		}
	}
}


void FormatDirNameAndExt(PCHAR dirname,PCHAR name,PCHAR ext)
{
	UINT i=0,len=0;
	len=strlen(dirname);
	for(i=0;i<len&&i<8;i++)
	{
		if(dirname[i]>='a'&&dirname[i]<='z')
		{
			name[i]=dirname[i]-32;
		}else{
			name[i]=dirname[i];
		}
	}
	for(;i<8;i++)
	{
		name[i]=' ';
	}
	for(i=0;i<3;i++)
	{
		ext[i]=' ';
	}
}