#ifndef _TIMES_H
#define _TIMES_H

#include <sys/types.h>          // ����ͷ�ļ��������˻�����ϵͳ�������͡�

struct tms {
	time_t tms_utime;       // �û�ʹ�õ�CPUʱ�䡣
	time_t tms_stime;       // ϵͳ���ںˣ�CPUʱ�䡣
	time_t tms_cutime;      // ����ֹ���ӽ���ʹ�õ��û�CPUʱ�䡣
	time_t tms_cstime;      // ����ֹ���ӽ���ʹ�õ�ϵͳCPUʱ�䡣
};

extern time_t times(struct tms * tp);

#endif

