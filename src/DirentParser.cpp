#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <time.h>
#include "DirentParser.h"

// スペースで区切られた文字列を item 配列に分割
void strparse(char *str, char *item[], uint32 *itemCount)
{
	uint32 i = 0, cnt = *itemCount;
	char *p = str;
	
	while (true) {
		if (i >= cnt) break;
		for(; *p == ' '; p++);
		if (*p == 0) break;
		item[i] = p;
		i++;
		if (i >= cnt) break;
		for(; (*p != ' ') && (*p != 0); p++);
		if (*p == 0) break;
		*p = 0;
		p++;
	}
	*itemCount = i;
	for(; i < cnt; i++) item[i] = 0;
}

status_t TGenericDirentParser::AddEntries(const char *strDirList, const char *option)
{
	status_t status = B_OK;
	uint32 st = 1;
	BList list;
	BString strdir(strDirList), curdir;
	strdir.ReplaceAll("\r\n", "\n");
	strdir.ReplaceAll("\r", "\n");
	
	// ftpd から得た NLST 文字列 strDirList を各行に分割して list にポインタを記憶する
	char *p = (char *)strdir.String();
	while (*p != 0) {
		list.AddItem(p);
		p = strchr(p, '\n');
	    if (p == NULL) break;
		*p = 0;
		p++;
	}
	
	// 解析
	for(int32 i = 0; i < list.CountItems(); i++) {
		p = (char *)list.ItemAt(i);
		size_t length = strlen(p);
		
		// 空白行は無視。但し、次行はディレクトリ名と仮定(再帰モード時)
		if (length == 0) {
			st = 0;
			continue;
		}
		if (st == 0) {
			st = 1;
			if (strcmp(option, "R") == 0) {
				if (p[length - 1] == ':')
					--length;
				curdir.SetTo(p, length);
				i++;		// ディレクトリ名の次は total または空行なので無視
				continue;
			}
		}
		
		// 先頭文字(即ち permission のファイル属性) が "-", "d", "l", "c" でなければ無視する。
		if (strchr("-dlc", *p) == NULL) continue;
		
		uint32 itemCount;
		char* dlist[10];
		char* permission;
		char* owner;
		char* group;
		char* size;
		char* month;
		char* day;
		char* houryear;
		char* name;

		memset(dlist, 0, sizeof(dlist));
		if (*p == 'c') {
			itemCount = 10;
			strparse(p, dlist, &itemCount);
			if (itemCount != 10) continue;
			permission = dlist[0];
			owner      = dlist[2];
			group      = dlist[3];
			size       = dlist[5];
			month      = dlist[6];
			day        = dlist[7];
			houryear   = dlist[8];
			name       = dlist[9];
		} else {
			itemCount = 9;
			strparse(p, dlist, &itemCount);
			if (itemCount != 9) continue;
			permission = dlist[0];
			owner      = dlist[2];
			group      = dlist[3];
			size       = dlist[4];
			month      = dlist[5];
			day        = dlist[6];
			houryear   = dlist[7];
			name       = dlist[8];
		}
		
		
		// 日付・時間を変換
		BString strdate, strtime;
		if (atoi(houryear) < 1900) {
			char stryear[5];
			time_t timer;
			struct tm *date;
			time(&timer);
			date = localtime(&timer);
			strftime(stryear, sizeof(stryear), "%Y", date);
			strdate << stryear;
			strtime << " " << houryear;
		} else {
			strdate << houryear;
		}
		strdate << "/";
		if (strcasecmp("Jan", month) == 0) strdate << "01"; else
		if (strcasecmp("Feb", month) == 0) strdate << "02"; else
		if (strcasecmp("Mar", month) == 0) strdate << "03"; else
		if (strcasecmp("Apr", month) == 0) strdate << "04"; else
		if (strcasecmp("May", month) == 0) strdate << "05"; else
		if (strcasecmp("Jun", month) == 0) strdate << "06"; else
		if (strcasecmp("Jul", month) == 0) strdate << "07"; else
		if (strcasecmp("Aug", month) == 0) strdate << "08"; else
		if (strcasecmp("Sep", month) == 0) strdate << "09"; else
		if (strcasecmp("Oct", month) == 0) strdate << "10"; else
		if (strcasecmp("Nov", month) == 0) strdate << "11"; else
		if (strcasecmp("Dec", month) == 0) strdate << "12"; else strdate << month;
		char strday[3];
		sprintf(strday, "%2.2d", atoi(day));
		strdate << "/" << strday << strtime;

		status = this->AddEntry(curdir.String(), name, atoll(size), strdate.String(), permission, owner, group);
		if (status != B_OK) break;
	}
	return status;
}
