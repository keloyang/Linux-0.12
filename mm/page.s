/*
 *  linux/mm/page.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * page.s contains the low-level page-exception code.
 * the real work is done in mm.c
 */
/*
 * page.s��������ײ�ҳ�쳣�������.ʵ�ʹ�����memory.c�����.
 */
.globl page_fault					# ����Ϊȫ�ֱ���.����traps.c����������ҳ�쳣������.

page_fault:
	xchgl %eax, (%esp)				# ȡ�����뵽eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10, %edx				# ���ں����ݶ�ѡ���.
	mov %dx, %ds
	mov %dx, %es
	mov %dx, %fs
	movl %cr2, %edx					# ȡ����ҳ���쳣�����Ե�ַ.
	pushl %edx						# �������Ե�ַ�ͳ�����ѹ��ջ��,��Ϊ�����ú����Ĳ���.
	pushl %eax
	testl $1, %eax					# ����ҳ���ڱ�־P(λ0),�������ȱҳ������쳣����ת.
	jne 1f
	call do_no_page					# ����ȱҳ������(mm/memory.c)
	jmp 2f
1:	call do_wp_page					# ����д����������(mm/memory.c)
2:	addl $8, %esp					# ����ѹ��ջ����������,����ջ�мĴ������˳��ж�.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

