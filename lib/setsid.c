/*
 *  linux/lib/setsid.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

// ����һ���Ự�����ý�����š�
// ����ϵͳ���ú��Ӧ�ں�����pid_t setsid()��
// ���أ����ý��̵ĻỰ��ʶ����session ID����
_syscall0(pid_t, setsid)

