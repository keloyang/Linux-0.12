/*
 *  linux/kernel/panic.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This function is used through-out the kernel (includeinh mm and fs)
 * to indicate a major problem.
 */
/*
 * �ú����������ں���ʹ��(������ͷ�ļ�*.h,�ڴ�������mm���ļ�ϵͳfs��),����ָ����Ҫ�ĳ�������.
 */
#include <linux/kernel.h>                           // �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�Ͷ��塣
#include <linux/sched.h>	                        // ���ȳ���ͷ�ļ�,����������ṹtask_struct,��ʼ����0������,����һЩ�й�������
				                                    // �������úͻ�ȡ��Ƕ��ʽ��ຯ�������.

void sys_sync(void);	                           /* it's really int */	/* ʵ����������int(fs/buffer.c) */

// �ú���������ʾ�ں��г������ش������Ϣ,�������ļ�ϵͳͬ������,Ȼ�������ѭ��--����.
// �����ǰ����������0�Ļ�,��˵���ǽ����������,���һ�û�������ļ�ϵͳͬ������.
// ������ǰ�Ĺؼ���volatile���ڸ��߱�����gcc�ú������᷵��.��������gcc��������һЩ�Ĵ���,����Ҫ����ʹ������ؼ���
// �Ա������ĳЩ(δ��ʼ��������)�پ�����Ϣ.
// ��ͬ������gcc�ĺ�������˵��:void panic(const char *s) __attribute__((noreturn));
void panic(const char * s)
{
	printk("Kernel panic: %s\n\r", s);
	if (current == task[0])
		printk("In swapper task - not syncing\n\r");
	else
		sys_sync();
	for(;;);
}


