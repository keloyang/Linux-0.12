/*
 *  linux/kernel/asm.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * asm.s contains the low-level code for most hardware faults.
 * page_exception is handled by the mm, so that isn't here. This
 * file also handles (hopefully) fpu-exceptions due to TS-bit, as
 * the fpu must be properly saved/resored. This hasn't been tested.
 */
/*
 * asm.s�����а����󲿷ֵ�Ӳ������(�����)����ĵͲ�δ���.ҳ�쳣���ڴ�������mm����,���Բ�������.
 *
 */

# �������ļ���Ҫ�漰��Intel�����ж�int0--int16�Ĵ���(int17-int31�������ʹ��).
# ������һЩȫ�ֺ�����������,��ԭ����traps.c��˵��.
.globl divide_error, debug, nmi, int3, overflow, bounds, invalid_op
.globl double_fault, coprocessor_segment_overrun
.globl invalid_TSS, segment_not_present, stack_segment
.globl general_protection, coprocessor_error, irq13, reserved
.globl alignment_check

# ������γ������޳���ŵ����.
# int0 -- ���������������. ����:����;�����:��.
# ��ִ��DIV��IDIVָ��ʱ,��������0,CPU�ͻ��������쳣.��EAX(��AX,AL)���ɲ���һ���Ϸ��������Ľ��ʱ,Ҳ���������쳣.
divide_error:
	pushl $do_divide_error						# ���Ȱѽ�Ҫ���õĺ�����ַ��ջ.
no_error_code:									# �������޳���Ŵ������ڴ�.
	xchgl %eax, (%esp)							# do_divide_error�ĵ�ַ->eax,eax��������ջ.
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds									# 16λ�ĶμĴ�����ջ��ҲҪռ��4���ֽ�.
	push %es
	push %fs
	pushl $0									# "error code"	# ����ֵ0��Ϊ��������ջ.
	lea 44(%esp), %edx							# ȡ��Ч��ַ,��ջ��ԭ���÷��ص�ַ����ջָ��λ��,��ѹ���ջ.
	pushl %edx
	movl $0x10, %edx							# ��ʼ���μĴ���ds,es��fs,�����ں����ݶ�ѡ���.
	mov %dx, %ds
	mov %dx, %es
	mov %dx, %fs
	# �����ϵ�'*'�ű�ʾ���ò�����ָ����ַ���ĺ���,��Ϊ��ӵ���.���ĺ����ǵ������𱾴��쳣��C������,����do_divide_error()��.
	call *%eax
	# ���н���ջָ���8�൱��ִ������pop����,����(����)������ջ������C��������,�ö�ջָ������ָ��Ĵ���fs��ջ��.
	addl $8, %esp
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax									# ����ԭ��eax�е�����.
	iret

# int1 -- debug�����ж���ڵ�.�������ͬ��.����:����/����(Fault/Trap);�޳����.
# ��eflags��TF��־��λʱ���������ж�.������Ӳ���ϵ�(����:����,����:����),���߿�����ָ���������򽻻�����,���ߵ��ԼĴ���������Ч(
# ����),CPU�ͻ�������쳣.
debug:
	pushl $do_int3								# _do_debug
	jmp no_error_code

# int2 -- �������жϵ�����ڵ�. ����:����;�޴����.
# ���ǽ��еı�����̶��ж�������Ӳ���ж�.ÿ�����յ�һ��NMI�ź�,CPU�ڲ��ͻ�����ж�����2,��ִ�б�׼�ж�Ӧ������,��˺ܽ�ʡʱ��.NMIͨ��
# ����Ϊ��Ϊ��Ҫ��Ӳ���¼�ʹ��.��CPU�յ�һ��NMI�źŲ���ʼִ�����жϴ��������,������е�Ӳ���ж϶���������.
nmi:
	pushl $do_nmi
	jmp no_error_code

# int3 -- �ϵ�ָ�������жϵ���ڵ�.������:������;���޴����.
# ��int 3ָ���������ж�,��Ӳ���ж��޹�.��ָ��ͨ���ɵ��������뱻���Գ���Ĵ�����.
int3:
	pushl $do_int3
	jmp no_error_code

# int4 -- ����������ж���ڵ�. ����:����; �޴����.
# EFLAGS��OF��־��λʱCPUִ��INTOָ��ͻ��������ж�.ͨ�����ڱ�����������������.
overflow:
	pushl $do_overflow
	jmp no_error_code

# int5 -- �߽�������ж���ڵ�. ����: ����; �޴����.
# ������������Ч��Χ�Դ�ʱ�������ж�.��BOUNDָ�����ʧ�ܾͻ�������ж�.BOUNDָ����3��������,�����1��������������֮��,�Ͳ����쳣5.
bounds:
	pushl $do_bounds
	jmp no_error_code

# CPUִ�л�����⵽һ����Ч�Ĳ������������ж�.
invalid_op:
	pushl $do_invalid_op
	jmp no_error_code

# int9 -- Э���������߳������ж����. ����: ����; �޴����.
# ���쳣�����ϵ�ͬ��Э������������.��Ϊ�ڸ���ָ�������̫��ʱ,���Ǿ���������������ػ򱣴泬�����ݶεĸ���ֵ.
coprocessor_segment_overrun:
	pushl $do_coprocessor_segment_overrun
	jmp no_error_code

# int15 -- ����Intel�����жϵ���ڵ�.
reserved:
	pushl $do_reserved
	jmp no_error_code

# int45 -- (0x20 + 13) Linux���õ���ѧЭ������Ӳ���ж�.
# ��Э������ִ����һ������ʱ�ͻᷢ��IRQ13�ж��ź�,��֪ͨCPU�������.80387��ִ�м���ʱ,CPU��ȴ���������.�����ϵ�0xF0��Э����˿�,������æ
# ������.ͨ��д�ö˿�,���жϽ�����CPU��BUSY�����ź�,�����¼���80387�Ĵ�������չ��������PEREQ.
# �ò�����Ҫ��Ϊ��ȷ���ڼ���ִ��80387���κ�ָ��֮ǰ,CPU��Ӧ���ж�.
irq13:
	pushl %eax
	xorb %al, %al
	outb %al, $0xF0
	movb $0x20, %al								# ��8259���жϿ���оƬ����EOI(�жϽ���)�ź�.
	outb %al, $0x20								# ��������תָ������ʱ����.
	jmp 1f
1:	jmp 1f
1:	outb %al, $0xA0								# ����8359���жϿ���оƬ����EOI(�жϽ���)��Ϣ.
	popl %eax
	jmp coprocessor_error						# �ú���ԭ�ڱ�������,���ѷŵ�system_call.s��.


# �����ж��ڵ���ʱCPU�����жϼӵ�ַ֮�󽫳����ѹ���ջ,��˷���ʱҲ��Ҫ������ŵ���.

# int8 -- ˫�������. ����: ����; �д�����.
# ͨ����CPU�ڵ���ǰһ���쳣�Ĵ��������ּ�⵽һ���µ��쳣ʱ,�������쳣�ᱻ���еؽ��д���,��Ҳ���������ٵ����,CPU���ܽ��������Ĵ��д������,��ʱ�ͻ�
# �������ж�.
double_fault:
	pushl $do_double_fault						# C������ַ��ջ.
error_code:
	xchgl %eax, 4(%esp)							# error code <-> %eax,eaxԭ����ֵ�������ڶ�ջ��.
	xchgl %ebx, (%esp)							# &function <-> %ebx,ebxԭ����ֵ�������ڶ�ջ��.
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl %eax									# error code	# �������ջ
	lea 44(%esp), %eax							# offset	# ���򷵻ص�ַ����ջָ��λ��ֵ��ջ.
	pushl %eax
	movl $0x10, %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	call *%ebx									# ��ӵ���,������Ӧ��C����,���������ջ.
	addl $8, %esp								# ������ջ��2������C�����Ĳ���.
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

# int10 -- ��Ч������״̬��(TSS). ����: ����; �г�����.
# CPU��ͼ�л���һ������,���ý��̵�TSS��Ч.����TSS����һ�����������쳣,������TSS���ȳ���104�ֽ�ʱ,����쳣�ڵ�ǰ�����в���,����л�����ֹ.��������
# ��ᵼ�����л�����������в������쳣.
invalid_TSS:
	pushl $do_invalid_TSS
	jmp error_code

# int11 -- �β�����. ����: ����; �г�����.
# �����õĶβ����ڴ���.���������б�־�Ŷβ����ڴ���.
segment_not_present:
	pushl $do_segment_not_present
	jmp error_code

# int12 -- ��ջ�δ���. ����:����; �г�����.
# ָ�������ͼ������ջ�η�Χ,���߶�ջ�β����ڴ���.�����쳣11��13������.��Щ����ϵͳ������������쳣��ȷ��ʲôʱ��Ӧ��Ϊ�����������ջ�ռ�.
stack_segment:
	pushl $do_stack_segment
	jmp error_code

# int13 -- һ�㱣���Գ���.����: ����;�г�����.
# �����ǲ������κ�������Ĵ���.��һ���쳣����ʱû�ж�Ӧ�Ĵ�������(0--16),ͨ���ͻ�鵽����.
general_protection:
	pushl $do_general_protection
	jmp error_code

# int17 -- �߽���������.
# ���������ڴ�߽���ʱ,����Ȩ��3(�û���)���ݷǱ߽����ʱ��������쳣.
alignment_check:
	pushl $do_alignment_check
	jmp error_code

# int7 -- �豸������(device_not_available)��kernel/sys_call.s
# int14 -- ҳ����(page_fault)��mm/page.s
# int16 -- Э����������(coprocessor_error)��kernel/sys_call.s
# ʱ���ж�int 0x20(timer_interrupt)��ernel/sys_call.s
# ϵͳ����int 0x80(system_call)��kernel/sys_call.s

