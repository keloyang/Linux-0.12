/*
 *  linux/kernel/traps.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'Traps.c' handles hardware traps and faults after we have saved some
 * state in 'asm.s'. Currently mostly a debugging-aid, will be extended
 * to mainly kill the offending process (probably by giving it a signal,
 * but possibly by killing it outright if necessary).
 */
/* �ڳ���asm.s�б�����һЩ״̬��,��������������Ӳ������͹���.Ŀǰ��Ҫ���ڵ���Ŀ��,�Ժ���չ����ɱ�����𻵵Ľ���(����ͨ������һ���ź�,
 * �������ҪҲ��ֱ��ɱ��.
 */
// #include <string.h>

#include <linux/head.h>
#include <linux/sched.h>				// ���ȳ���ͷ�ļ�,����������ṹtask_struct,��ʼ����0������.
#include <linux/kernel.h>
#include <asm/system.h>					// ϵͳͷ�ļ�.���������û��޸�������/�ж��ŵȵ�Ƕ��ʽ����.
#include <asm/segment.h>
#include <asm/io.h>						// ����/���ͷ�ļ�.����Ӳ���˿�����/����������.

// ȡseg�е�ַaddr����һ���ֽ�.
// ����: seg - ��ѡ���;addr - ����ָ����ַ.
// ���: %0 - eax(__res);����: %1 - eax(seg); %2 - �ڴ��ַ(*(addr))
#define get_seg_byte(seg, addr) ({ \
register char __res; \
__asm__("push %%fs;mov %%ax,%%fs;movb %%fs:%2,%%al;pop %%fs" \
	:"=a" (__res):"0" (seg),"m" (*(addr))); \
__res;})

// ȡseg�е�ַaddr����һ������(4�ֽ�).
// ����: seg - ��ѡ���;addr - ����ָ����ַ.
// ���: %0 - eax(__res);����: %1 - eax(seg); %2 - �ڴ��ַ(*(addr))
#define get_seg_long(seg, addr) ({ \
register unsigned long __res; \
__asm__("push %%fs;mov %%ax,%%fs;movl %%fs:%2,%%eax;pop %%fs" \
	:"=a" (__res):"0" (seg),"m" (*(addr))); \
__res;})

// ȡfs�μĴ�����ֵ(ѡ���).
// ���:%0 - eax(__res)
#define _fs() ({ \
register unsigned short __res; \
__asm__("mov %%fs,%%ax":"=a" (__res):); \
__res;})

// ���¶�����һЩ����ԭ��.
void page_exception(void);					// ҳ�쳣.ʵ����page_fault(mm/page.s)

void divide_error(void);					// int0(kernel/asm.s)
void debug(void);							// int1(kernel/asm.s)
void nmi(void);								// int2(kernel/asm.s)
void int3(void);							// int3(kernel/asm.s)
void overflow(void);						// int4(kernel/asm.s)
void bounds(void);							// int5(kernel/asm.s)
void invalid_op(void);						// int6(kernel/asm.s)
void device_not_available(void);			// int7(kernel/sys_call.s)
void double_fault(void);					// int8(kernel/asm.s)
void coprocessor_segment_overrun(void);		// int9(kernel/asm.s)
void invalid_TSS(void);						// int10(kernel/asm.s)
void segment_not_present(void);				// int11(kernel/asm.s)
void stack_segment(void);					// int12(kernel/asm.s)
void general_protection(void);				// int13(kernel/asm.s)
void page_fault(void);						// int14(mm/page.s)
void coprocessor_error(void);				// int16(kernel/sys_call.s)
void reserved(void);						// int15(kernel/asm.s)
void parallel_interrupt(void);				// int39(kernel/sys_call.s)
void irq13(void);							// int45Э�������жϴ���(kernel/asm.s)
void alignment_check(void);					// int46(kernel/asm.s)

// ���ӳ���������ӡ�����жϵ�����,�����,���ó����EIP,EFLAGS,ESP,fs�μĴ���ֵ,�εĻ�ַ,�εĳ���,���̺�pid,�����,10�ֽ�ָ����.���
// ��ջ���û����ݶ�,�򻹴�ӡ16�ֽڶ�ջ����.��Щ��Ϣ�����ڳ������.
static void die(char * str, long esp_ptr, long nr)
{
	long * esp = (long *) esp_ptr;
	int i;

	printk("%s: %04x\n\r",str, nr & 0xffff);
	// ���д�ӡ�����ʾ��ǰ���ý��̵�CS:EIP,EFLAGS��SS:ESP��ֵ.
	// (1) EIP:\t%04x:%p\n	-- esp[1]�Ƕ�ѡ���(cs),esp[0]��eip
	// (2) EFLAGS:\t%p	-- esp[2]��eflags
	// (2) ESP:\t%04x:%p\n	-- esp[4]��ԭss,esp[3]��ԭesp
	printk("EIP:\t%04x:%p\nEFLAGS:\t%p\nESP:\t%04x:%p\n",
		esp[1], esp[0], esp[2], esp[4], esp[3]);
	printk("fs: %04x\n", _fs());
	printk("base: %p, limit: %p\n", get_base(current->ldt[1]), get_limit(0x17));
	if (esp[4] == 0x17) {						// ��ԭssֵΪ0x17(�û�ջ),�򻹴�ӡ���û�ջ��4������ֵ(16�ֽ�).
		printk("Stack: ");
		for (i = 0; i < 4; i++)
			printk("%p ", get_seg_long(0x17, i + (long *)esp[3]));
		printk("\n");
	}
	str(i);										// ȡ��ǰ��������������(include/linux/sched.h).
	printk("Pid: %d, process nr: %d\n\r", current->pid, 0xffff & i);
                        						// ���̺�,�����.
	for(i = 0; i < 10; i++)
		printk("%02x ", 0xff & get_seg_byte(esp[1], (i+(char *)esp[0])));
	printk("\n\r");
	do_exit(11);								/* play segment exception */
}

// ������Щ��do_��ͷ�ĺ�����asm.s�ж�Ӧ�жϴ��������õ�C����.
void do_double_fault(long esp, long error_code)
{
	die("double fault", esp, error_code);
}

void do_general_protection(long esp, long error_code)
{
	die("general protection", esp, error_code);
}

void do_alignment_check(long esp, long error_code)
{
    die("alignment check", esp, error_code);
}

void do_divide_error(long esp, long error_code)
{
	die("divide error", esp, error_code);
}

// �����ǽ����жϺ�˳��ѹ���ջ�ļĴ���ֵ.�μ�asm.s����.
void do_int3(long * esp, long error_code,
		long fs, long es, long ds,
		long ebp, long esi, long edi,
		long edx, long ecx, long ebx, long eax)
{
	int tr;

	__asm__("str %%ax":"=a" (tr):"0" (0));		// ȡ����Ĵ���ֵ->tr
	printk("eax\t\tebx\t\tecx\t\tedx\n\r%8x\t%8x\t%8x\t%8x\n\r",
		eax, ebx, ecx, edx);
	printk("esi\t\tedi\t\tebp\t\tesp\n\r%8x\t%8x\t%8x\t%8x\n\r",
		esi, edi, ebp, (long) esp);
	printk("\n\rds\tes\tfs\ttr\n\r%4x\t%4x\t%4x\t%4x\n\r",
		ds, es, fs, tr);
	printk("EIP: %8x   CS: %4x  EFLAGS: %8x\n\r", esp[0], esp[1], esp[2]);
}

void do_nmi(long esp, long error_code)
{
	die("nmi", esp, error_code);
}

void do_debug(long esp, long error_code)
{
	die("debug", esp, error_code);
}

void do_overflow(long esp, long error_code)
{
	die("overflow", esp, error_code);
}

void do_bounds(long esp, long error_code)
{
	die("bounds", esp, error_code);
}

void do_invalid_op(long esp, long error_code)
{
	die("invalid operand", esp, error_code);
}

void do_device_not_available(long esp, long error_code)
{
	die("device not available", esp, error_code);
}

void do_coprocessor_segment_overrun(long esp, long error_code)
{
	die("coprocessor segment overrun", esp, error_code);
}

void do_invalid_TSS(long esp, long error_code)
{
	die("invalid TSS", esp, error_code);
}

void do_segment_not_present(long esp, long error_code)
{
	die("segment not present", esp, error_code);
}

void do_stack_segment(long esp, long error_code)
{
	die("stack segment", esp, error_code);
}

void do_coprocessor_error(long esp, long error_code)
{
	if (last_task_used_math != current)
		return;
	die("coprocessor error", esp, error_code);
}

void do_reserved(long esp, long error_code)
{
	die("reserved (15,17-47) error", esp, error_code);
}

// �������쳣(����)�жϳ����ʼ���ӳ���.�������ǵ��жϵ�����(�ж�����).
// set_trap_gate()��set_system_gate()��ʹ�����ж���������IDT�е�������(Trap Gate),����֮�����Ҫ��������ǰ�����õ���Ȩ��Ϊ0,
// ������3.��˶ϵ������ж�int3,����ж�overflow�ͱ߽�����ж�bounds�������κγ������.��������������Ƕ��ʽ�������,�μ�
// include/asm/system.h
void trap_init(void)
{
	int i;

	set_trap_gate(0, &divide_error);							// ���ó�����������ж�����ֵ.
	set_trap_gate(1, &debug);
	set_trap_gate(2, &nmi);
	set_system_gate(3, &int3);									/* int3-5 can be called from all */
	set_system_gate(4, &overflow);
	set_system_gate(5, &bounds);
	set_trap_gate(6, &invalid_op);
	set_trap_gate(7, &device_not_available);					// ����δʵ��
	set_trap_gate(8, &double_fault);
	set_trap_gate(9, &coprocessor_segment_overrun);
	set_trap_gate(10, &invalid_TSS);
	set_trap_gate(11, &segment_not_present);
	set_trap_gate(12, &stack_segment);
	set_trap_gate(13, &general_protection);
	set_trap_gate(14, &page_fault);
	set_trap_gate(15, &reserved);
	set_trap_gate(16, &coprocessor_error);						// ����δʵ��
	set_trap_gate(17, &alignment_check);
	// �����int17-47���������Ⱦ�����Ϊreserved,�Ժ��Ӳ����ʼ��ʱ�����������Լ���������.
	for (i = 18; i < 48; i++)
		set_trap_gate(i, &reserved);
	// ����Э�������ж�0x2d(45)������������,������������ж�����.���ò��п��ж�������.
	set_trap_gate(45, &irq13);
	outb_p(inb_p(0x21)&0xfb, 0x21);								// ����8259A��оƬ��IRQ2�ж�����(���Ӵ�оƬ)
	outb(inb_p(0xA1)&0xdf, 0xA1);								// ����8259A��оƬ��IRQ13�ж�����(Э�������ж�)
	set_trap_gate(39, &parallel_interrupt);						// ���ò��п�1���ж�0x27������������.
}

