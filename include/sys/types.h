#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;    // ���ڶ���Ĵ�С�����ȣ���
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;	// ����ʱ��(�����)
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef long ptrdiff_t;
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

typedef int pid_t;	// ���ڽ��̺źͽ������
typedef unsigned short uid_t;	// �����û���(�û���ʶ��)
typedef unsigned short gid_t;	// �������
typedef unsigned short dev_t;	// �����豸��
typedef unsigned short ino_t;	// �����ļ����к�
typedef unsigned short mode_t;	// ����ĳЩ�ļ�����
typedef unsigned short umode_t;
typedef unsigned char nlink_t;	// �������Ӽ���
typedef int daddr_t;
typedef long off_t;             // �����ļ�����(��С).
typedef unsigned char u_char;   // �޷����ַ����͡�
typedef unsigned short ushort;  // �޷��Ŷ��������͡�

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned long tcflag_t;

typedef unsigned long fd_set;

typedef struct { int quot,rem; } div_t;         // ����DIV������
typedef struct { long quot,rem; } ldiv_t;       // ���ڳ�DIV������

// �ļ�ϵͳ�����ṹ������ustat()��������������ֶ�δʹ�ã����Ƿ���NULLָ�롣
struct ustat {
	daddr_t f_tfree;        // ϵͳ�ܿ��п�����
	ino_t f_tinode;         // �ܿ���i�ڵ�����
	char f_fname[6];        // �ļ�ϵͳ���ơ�
	char f_fpack[6];        // �ļ�ϵͳѹ�����ơ�
};

#endif

