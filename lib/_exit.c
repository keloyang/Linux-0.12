/*
 *  linux/lib/_exit.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__	                        // ����һ�����ų���,������˵��.
#include <unistd.h>	                        // Linux��׼ͷ�ļ�.�����˸��ַ��ų���������,�������˸��ֺ���.
			                                // ��������__LIBRARY__,�򻹺�ϵͳ���úź���Ƕ���syscall0()��.

// �ں�ʹ�õĳ���(�˳�)��ֹ����.
// ֱ�ӵ���ϵͳ�ж�int 0x80,���ܺ�__NR_exit.
// ����:exit_code - �˳���.
// ������ǰ�Ĺؼ���volatile���ڸ��߱�����gcc�ú������᷵��.��������gcc�������õĴ���,����Ҫ����ʹ������ؼ��ֿ��Ա������ĳЩ(δ��ʼ��������)
// �پ�����Ϣ.��ͬ��gcc�ĺ�������˵��:void do_exit(int error_code) __attribute__((noreturn));
void _exit(int exit_code)
{
	__asm__ __volatile__ ("int $0x80"::"a" (__NR_exit), "b" (exit_code));
}

