#ifndef _SYS_TIME_H
#define _SYS_TIME_H

/* gettimofday returns this */          /*  gettimeofday()�������ظ�ʱ��ṹ */
struct timeval {
	long	tv_sec;		/* seconds */           // �롣
	long	tv_usec;	/* microseconds */      // ΢�롣
};

// ʱ�����ṹ��tzΪʱ����Time Zone������д��DST��Daylight Saving Time��������ʱ����д��
struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */ // ����������������ʱ�䡣
	int	tz_dsttime;	/* type of dst correction */    // ����ʱ������ʱ�䡣
};

#define	DST_NONE	0	/* not on dst */                // ������ʱ��
#define	DST_USA		1	/* USA style dst */             // USA��ʽ������ʱ��
#define	DST_AUST	2	/* Australian style dst */      // �Ĵ�������ʽ������ʱ��
#define	DST_WET		3	/* Western European dst */
#define	DST_MET		4	/* Middle European dst */
#define	DST_EET		5	/* Eastern European dst */
#define	DST_CAN		6	/* Canada */
#define	DST_GB		7	/* Great Britain and Eire */
#define	DST_RUM		8	/* Rumania */
#define	DST_TUR		9	/* Turkey */
#define	DST_AUSTALT	10	/* Australian style with shift in 1986 */

// �ļ��������������ú꣬����select()������
#define FD_SET(fd,fdsetp)	(*(fdsetp) |= (1 << (fd)))
#define FD_CLR(fd,fdsetp)	(*(fdsetp) &= ~(1 << (fd)))
#define FD_ISSET(fd,fdsetp)	((*(fdsetp) >> fd) & 1)
#define FD_ZERO(fdsetp)		(*(fdsetp) = 0)

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
// timevalʱ��ṹ�Ĳ���������
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)	\
	((tvp)->tv_sec cmp (uvp)->tv_sec || \
	 (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
#define	timerclear(tvp)		((tvp)->tv_sec = (tvp)->tv_usec = 0)

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
/* �ڲ���ʱ�����ƺͽṹ�����ڶ��嶨ʱ�����á�*/
#define	ITIMER_REAL	0       // ��ʵ��ʱ��ݼ���
#define	ITIMER_VIRTUAL	1       // �Խ�������ʱ��ݼ���
#define	ITIMER_PROF	2       // �Խ�������ʱ����ߵ�ϵͳ����ʱ�Խ���ʱ��ݼ���

// �ڲ�ʱ��ṹ������it��Internal Timer�����ڲ���ʱ������д��
struct	itimerval {
	struct	timeval it_interval;	/* timer interval */
	struct	timeval it_value;	/* current value */
};

#include <time.h>
#include <sys/types.h>

int gettimeofday(struct timeval * tp, struct timezone * tz);

#endif /*_SYS_TIME_H*/

