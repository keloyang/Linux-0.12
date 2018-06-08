/*
 *  linux/kernel/printk.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * When in kernel-mode, we cannot use printf, as fs is liable to
 * point to 'interesting' things. Make a printf with fs-saving, and
 * all is well.
 */
/*
 * �������ں�ģʽʱ,���ǲ���ʹ��printf,��Ϊ�Ĵ���fsָ������������Ȥ�ĵط�.�Լ�����һ��printf����ʹ��ǰ����fs,һ�оͽ����.
 */
// ��׼����ͷ�ļ�.�Ժ����ʽ������������б�.��Ҫ˵����һ������(va_list)��������va_start��va_arg��va_end,
// ����vsprintf,vprintf,vfprintf������
#include <stdarg.h>
#include <stddef.h>             			// ��׼����ͷ�ļ���������NULL��offsetof(TYPE,MEMBER)��

#include <linux/kernel.h>       			// �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�ζ��塣

static char buf[1024];          			// ��ʾ����ʱ��������

// ����vsprintf()������linux/kernel/vsprintf.c��
extern int vsprintf(char * buf, const char * fmt, va_list args);

// �ں�ʹ�õ���ʾ����.
int printk(const char *fmt, ...)
{
	va_list args;							// va_listʵ������һ���ַ�ָ������.
	int i;

	// ���в�������ʼ����.Ȼ��ʹ�ø�ʽ��fmt�������б�args�����buf��.����ֵi��������ַ����ĳ���.�����в��������������.�����ÿ���̨��ʾ
	// ������������ʾ�ַ���.
	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);
	console_print(buf);						// chr_drv/console.c
	return i;
}

inline void check_data32(int value, int pos)
{
	asm __volatile__(
		"shl	$4, %%ebx\n\t"
		"addl	$0xb8000,%%ebx\n\t"
		"movl	$0xf0000000,%%eax\n\t"
		"movb	$28,%%cl\n\t"
		"1:\n\t"
		"movl	%0,%%edx\n\t"
		"andl	%%eax,%%edx\n\t"
		"shr	%%cl,%%edx\n\t"
		"add	$0x30,%%dx\n\t"
		"cmp	$0x3a,%%dx\n\t"
		"jb	2f\n\t"
		"add	$0x07,%%dx\n\t"
		"2:\n\t"
		"add	$0x0c00,%%dx\n\t"
		"movw	%%dx,(%%ebx)\n\t"
		"sub	$0x04,%%cl\n\t"
		"shr	$0x04,%%eax\n\t"
		"add	$0x02,%%ebx\n\t"
		"cmpl	$0x0,%%eax\n\t"
		"jnz	1b\n"
		::"m"(value), "b"(pos));
}


