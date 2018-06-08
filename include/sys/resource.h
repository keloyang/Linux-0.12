/*
 * Resource control/accounting header file for linux
 */
/*
 * Linux��Դ����/���ͷ�ļ���
 */

#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H

// ���·��ų����ͽṹ����getrusage()���μ�kernel/sys.c��
/*
 * Definition of struct rusage taken from BSD 4.3 Reno
 * 
 * We don't support all of these yet, but we might as well have them....
 * Otherwise, each time we add new items, programs which depend on this
 * structure will lose.  This reduces the chances of that happening.
 */
/*
 * rusage�ṹ�Ķ���ȡ��BSD 4.3 Renoϵͳ��
 * 
 * �������ڻ�û��֧�ָýṹ�е�������Щ�ֶΣ������ǿ��ܻ�֧�����ǵ�....����Ļ���ÿ�����������µ��ֶΣ���Щ���������
 * �ṹ�ĳ���ͻ�����⡣���ڰ������ֶζ����������Ϳ��Ա����������鷢����
 */
// ������getrusage()�Ĳ���who��ʹ�õķ��ų�����
#define	RUSAGE_SELF	0       // ���ص�ǰ���̵���Դ������Ϣ��
#define	RUSAGE_CHILDREN	-1      // ���ص�ǰ��������ֹ�͵ȴ��ŵ��ӽ��̵���Դ������Ϣ��

// rusage�ǽ��̵���Դ����ͳ�ƽṹ������getrusage()����ָ�����̶���Դ���õ�ͳ��ֵ��Linux0.12�ں˽�ʹ����ǰ�����ֶΣ�
// ������timeval�ṹ��include/sys/time.h����
// ru_utime - �������û�̬����ʱ��ͳ��ֵ��ru_stime - �������ں�̬����ʱ��ͳ��ֵ��
struct	rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	long	ru_maxrss;		/* maximum resident set size */
	long	ru_ixrss;		/* integral shared memory size */
	long	ru_idrss;		/* integral unshared data size */
	long	ru_isrss;		/* integral unshared stack size */
	long	ru_minflt;		/* page reclaims */
	long	ru_majflt;		/* page faults */
	long	ru_nswap;		/* swaps */
	long	ru_inblock;		/* block input operations */
	long	ru_oublock;		/* block output operations */
	long	ru_msgsnd;		/* messages sent */
	long	ru_msgrcv;		/* messages received */
	long	ru_nsignals;		/* signals received */
	long	ru_nvcsw;		/* voluntary context switches */
	long	ru_nivcsw;		/* involuntary " */
};

// ������getrlimit()��setrlimit()ʹ�õķ��ų����ͽṹ��
/*
 * Resource limits
 */
/*
 * ��Դ���ơ�
 */

// ������Linux0.12�ں������������Դ���࣬��getrlimit()��setrlimit()�е�1������resource��ȡֵ��Χ����ʵ��Щ���ų���
// ���ǽ�������ṹ��rlim[]�����������ֵ��rlim[]����ÿһ���һ��rlimit�ṹ���ýṹ�����档
#define RLIMIT_CPU	0		/* CPU time in ms */            /* ʹ�õ�CPUʱ�� */
#define RLIMIT_FSIZE	1		/* Maximum filesize */          /* ����ļ����� */
#define RLIMIT_DATA	2		/* max data size */             /* ������ݳ��� */
#define RLIMIT_STACK	3		/* max stack size */            /* ���ջ���� */
#define RLIMIT_CORE	4		/* max core file size */        /* ���core�ļ����� */
#define RLIMIT_RSS	5		/* max resident set size */     /* ���פ������С */

#ifdef notdef
#define RLIMIT_MEMLOCK	6		/* max locked-in-memory address space*/ /* ������ */
#define RLIMIT_NPROC	7		/* max number of processes */           /* ����ӽ����� */
#define RLIMIT_OFILE	8		/* max number of open files */          /* �����ļ��� */
#endif

// ������ų���������Linux�����Ƶ���Դ���ࡣRLIM_NLIMITS=6����˽�ǰ��6����Ч��
#define RLIM_NLIMITS	6

// ��ʾ��Դ���ޣ������޸ġ�
#define RLIM_INFINITY	0x7fffffff

struct rlimit {
	int	rlim_cur;       // ��ǰ��Դ���ƣ���������ƣ�soft_limit)��
	int	rlim_max;       // Ӳ����(hard_limit)��
};

#endif /* _SYS_RESOURCE_H */

