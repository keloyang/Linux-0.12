#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#include <sys/types.h>        // ����ͷ�ļ��������˻�����ϵͳ�������͡�
#include <sys/param.h>        // �ں˲����ļ���

struct utsname {
	char sysname[9];                        // ��ǰ����ϵͳ�����ơ�     
	char nodename[MAXHOSTNAMELEN+1];        // ��ʵ����ص������нڵ����ƣ��������ƣ���
	char release[9];                        // ������ϵͳʵ�ֵĵ�ǰ���м���
	char version[9];                        // ���η��еĲ���ϵͳ�汾����
	char machine[9];                        // ϵͳ���е�Ӳ���������ơ�
};

extern int uname(struct utsname * utsbuf);

#endif

