/*
 * linux/kernel/math/error.c
 *
 * (C) 1991 Linus Torvalds
 */

#include <signal.h>

#include <linux/sched.h>

// Э�����������ж�int 16���õĴ�������
// ��Э��������⵽�Լ���������ʱ���ͻ�ͨ��ERROR����֪ͨCPU������������ڴ���Э�����������ĳ����źš�����תȥִ��math_error()
// ���غ���ת�����ret_from_sys_call������ִ�С�
void math_error(void)
{
	__asm__("fnclex");              // ��80387���״̬���������쳣��־λ��æλ��
	if (last_task_used_math)        // ��ʹ����Э��������������Э�����������źš�
		last_task_used_math->signal |= 1<<(SIGFPE-1);
}

