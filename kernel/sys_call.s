/*
 *  linux/kernel/sys_call.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  system_call.s  contains the system-call low-level handling routines.
 * This also contains the timer-interrupt handler, as some of the code is
 * the same. The hd- and flopppy-interrupts are also here.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call. Ordinary interrupts
 * don't handle signal-recognition, as that would clutter them up totally
 * unnecessarily.
 *
 * Stack layout in 'ret_from_system_call':
 *
 *	 0(%esp) - %eax
 *	 4(%esp) - %ebx
 *	 8(%esp) - %ecx
 *	 C(%esp) - %edx
 *	10(%esp) - original %eax	(-1 if not system call)
 *	14(%esp) - %fs
 *	18(%esp) - %es
 *	1C(%esp) - %ds
 *	20(%esp) - %eip
 *	24(%esp) - %cs
 *	28(%esp) - %eflags
 *	2C(%esp) - %oldesp
 *	30(%esp) - %oldss
 */
/*
 * sys_call.s�ļ�����ϵͳ����(system-call)�ײ㴦���ӳ���.������Щ����Ƚ�����,����ͬʱҲ����ʱ�Ӵ���(timer-interrupt)���.
 * Ӳ�̺����̵��жϴ������Ҳ������.
 *
 * ע��:��δ��봦���ź�(signal)ʶ��,��ÿ��ʱ���жϺ�ϵͳ����֮�󶼻����ʶ��.һ��Ϲ��̲��������ź�ʶ��,��Ϊ���ϵͳ��ɻ���.
 *
 * ��ϵͳ���÷���(ret_from_system_call)ʱ��ջ�����ݼ�����.
 */
# ����Linusԭע����һ���жϹ�����ָ����ϵͳ�����ж�(int 0x80)��ʱ���ж�(int 0x20)����������ж�.��Щ�жϻ����ں�̬���û�̬���
# ����,������Щ�жϹ�����Ҳ�����ź�ʶ��Ļ�,���п�����ϵͳ�����жϺ�ʱ���жϹ����ж��źŵ�ʶ����������ͻ,Υ�����ں˴������ռԭ��.
# ���ϵͳ���ޱ�Ҫ����Щ"����"�ж��д����ź�,Ҳ������������.

SIG_CHLD	= 17					# ����SIG_CHLD�ź�(�ӽ���ֹͣ�����).

EAX			= 0x00					# ��ջ�и����Ĵ�����ƫ��λ��.
EBX			= 0x04
ECX			= 0x08
EDX			= 0x0C
ORIG_EAX	= 0x10					# �������ϵͳ����(�������ж�)ʱ,��ֵΪ-1.
FS			= 0x14
ES			= 0x18
DS			= 0x1C
EIP			= 0x20					# ��������CPU�Զ���ջ.
CS			= 0x24
EFLAGS		= 0x28
OLDESP		= 0x2C					# ����Ȩ���仯ʱ,ԭ��ջָ��Ҳ����ջ.
OLDSS		= 0x30

# ������Щ������ṹ(task_struct)�б�����ƫ��ֵ,�μ�include/linux/sched.h
state		= 0						# these are offsets into the task-struct.	# ����״̬��.
counter		= 4						# ��������ʱ�����(�ݼ�)(�δ���),����ʱ��Ƭ.
priority 	= 8						# ����������.����ʼ����ʱcounter=priority,Խ��������ʱ��Խ��.
signal		= 12					# ���ź�λͼ,ÿ��λ����һ���ź�,�ź�ֵ=λƫ��ֵ+1.
sigaction 	= 16					# MUST be 16 (=len of sigaction)	# sigaction�ṹ���ȱ�����16�ֽ�.
blocked 	= (33*16)				# �������ź�λͼ��ƫ����.

# ���¶�����sigaction�ṹ�е�ƫ����,�μ�include/signal.h
# offsets within sigaction
sa_handler 	= 0						# �źŴ�����̵ľ��(������)
sa_mask 	= 4						# �ź�������
sa_flags 	= 8						# �źż�.
sa_restorer = 12					# �ָ�����ָ��,�μ�kernel/signal.c����˵��.

nr_system_calls = 82				# Linux 0.12���ں��е�ϵͳ��������.

ENOSYS = 38							# ϵͳ���úų�����.

/*
 * Ok, I get parallel printer interrupts while using the floppy for some
 * strange reason. Urgel. Now I just ignore them.
 */
/*
 * ����,��ʹ������ʱ���յ��˲��д�ӡ���ж�,�����.��,���ڲ�����.
 */
.globl system_call,sys_fork,timer_interrupt,sys_execve
.globl hd_interrupt,floppy_interrupt,parallel_interrupt
.globl device_not_available, coprocessor_error, sys_default

# ϵͳ���úŴ���ʱ�����س�����-ENOSYS
.align 4							# �ڴ�4�ֽڶ���.
bad_sys_call:
	pushl $-ENOSYS					# eax����-ENOSYS.
	jmp ret_from_sys_call 			# jmp��call��������jmpֱ����ת����ַ����ʼִ�У���call���Ƚ����ص�ַ��ջȻ�������Ŵ���ʼִ��

# ����ִ�е��ȳ������.���ȳ���schedule()��(kernel/sched.c)
# �����ȳ���schedule()����ʱ�ʹ�ret_from_sys_call������ִ��.
.align 4
reschedule:
	pushl $ret_from_sys_call		# ��ret_from_sys_call�ĵ�ַ��ջ.
	jmp schedule					# jmp��call��������jmpֱ����ת����ַ����ʼִ�У���call���Ƚ����ص�ַ��ջȻ�������Ŵ���ʼִ��

# int 0x80 --linuxϵͳ������ڵ�(�����ж�int 0x80,eax���ǵ��ú�).
.align 4
system_call:
	push %ds						# ����ԭ�μĴ���ֵ.
	push %es
	push %fs
	pushl %eax						# save the orig_eax	# ����eaxԭֵ.
	# һ��ϵͳ�������ɴ���3������,Ҳ���Բ�������.������ջ��ebx,ecx��edx�з���ϵͳ������ӦC���Ժ����ĵ��ò���.
	# �⼸���Ĵ�����ջ��˳������GNU gcc�涨��,ebx�пɴ�ŵ�1������,ecx�д�ŵ�2������,edx�д�ŵ�3������.
	# ϵͳ�������ɲμ�ͷ�ļ�include/unistd.h�е�ϵͳ���ú�.
	pushl %edx
	pushl %ecx						# push %ebx,%ecx,%edx as parameters
	pushl %ebx						# to the system call
	# �ڱ�����μĴ���֮��,��ds,esָ���ں����ݶ�,��fsָ��ǰ�ֲ����ݶ�,��ָ��ִ�б���ϵͳ���õ��û���������ݶ�.ע��,
	# ��Linux0.12���ں˸��������Ĵ���������ڴ�����ص���,���ǵĶλ�ַ�Ͷ��޳���ͬ.
	movl $0x10, %edx				# set up ds,es to kernel space
	mov %dx, %ds
	mov %dx, %es
	movl $0x17, %edx				# fs points to local data space
	mov %dx, %fs
	cmpl NR_syscalls, %eax			# ���ú����������Χ�Ļ�����ת.
	jae bad_sys_call

    mov sys_call_table(, %eax, 4), %ebx
    cmpl $0, %ebx
    jne sys_call
	#	pushl %eax
    call sys_default
	# �������������ĺ�����:���õ�ַ=[sys_call_table + %eax *4].
	# sys_call_table[]��һ��ָ������,������include/linux/sys.h��,���������������ں�����82��ϵͳ����C�������ĵ�ַ.
sys_call:
	call *%ebx
	# call *sys_call_table(, %eax, 4)	# ��ӵ���ָ������C����.
	pushl %eax							# ��ϵͳ���÷���ֵ��ջ.

# �����в鿴��ǰ���������״̬.������ھ���״̬(state������0)��ȥִ�е��Գ���.����������ھ���״̬,������ʱ��Ƭ�Ѿ���
# ��(counter=0),��Ҳȥִ�е��ȳ���.
# ���統��̨�������еĽ���ִ�п����ն˶�д����ʱ,��ôĬ�������¸ú�̨�������н��̻��յ�SIGTTIN��SIGTTOU�ź�,���½���
# �������н��̴���ֹͣ״̬.����ǰ����������̷���.
2:
	movl current, %eax				# ȡ��ǰ����(����)���ݽṹָ��->eax.
	cmpl $0, state(%eax)			# state,�����ǰ���̵�״̬����0����״̬�����½��е��Ƚ���ִ��
	jne reschedule
	cmpl $0, counter(%eax)			# counter,�����ǰ����ʣ���ִ��ʱ��Ϊ0�����½��е��Ƚ���ִ��
	je reschedule

# ������δ���ִ�д�ϵͳ����C�������غ�,���źŽ���ʶ����.�����жϷ�������˳�ʱҲ����ת��������д������˳��жϹ���,
# �������Ĵ����������ж�int 16.
# �����жϵ�ǰ�����ǲ��ǳ�ʼ����task0,������򲻱ض�������ź�������Ĵ���,ֱ�ӷ���.
# task��ӦC�����е�task[]����,ֱ������task�൱������task[0].
ret_from_sys_call:
	movl current, %eax
	cmpl task, %eax					# task[0] cannot have signals
	je 3f							# ��ǰ(forward)��ת�����3���˳��жϴ���.
	# ͨ����ԭ���ó������ѡ����ļ�����жϵ��ó����ǲ����û�����.���������ֱ�Ӽȳ��ж�.
	# ������Ϊ�������ں�ִ̬��ʱ������ռ.�������������ź�������.����ͨ���Ƚ�ѡ����Ƿ�Ϊ�û������ѡ���0x000f(RPL=3,�ֲ���,����
	# ��)���ж��Ƿ�Ϊ�û�����.���������˵����ĳ���жϷ������(�����ж�16)��ת����107��ִ�е���,������ת�˳��жϳ���.����,���ԭ��ջ
	# ��ѡ�����Ϊ0x17(��ԭ��ջ�����û�����),Ҳ˵������ϵͳ���õĵ����߲����û�����,��Ҳ�˳�.
	cmpw $0x0f, CS(%esp)			# was old code segment supervisor ?
	jne 3f
	cmpw $0x17, OLDSS(%esp)			# was stack segment = 0x17 ?
	jne 3f

# ������δ������ڴ���ǰ�����е��ź�.����ȡ��ǰ����ṹ�е��ź�λͼ(32λ,ÿλ����1���ź�),Ȼ��������ṹ�е��ź�����(����)��,����
# ��������ź�λ,ȡ����ֵ��С���ź�ֵ,�ٰ�ԭ�ź�λͼ�и��źŶ�Ӧ��λ��λ(��0),��󽫸��ź�ֵ��Ϊ����֮һ����do_signal().do_signal()
# ��(kernel/signal.c)��,���������13����ջ��Ϣ.��do_signal()���źŴ���������֮��,������ֵ��Ϊ0���ٿ����Ƿ���Ҫ�л����̻�
# �������������ź�.
	movl signal(%eax), %ebx			# ȡ�ź�λͼ->ebx,ÿ1λ����1���ź�,��32���ź�.
	movl blocked(%eax), %ecx		# ȡ����(����)�ź�λͼ->ecx.
	notl %ecx						# ÿλȡ��.
	andl %ebx, %ecx					# ��ȡ��ɵ��ź�λͼ.
	bsfl %ecx, %ecx					# �ӵ�λ(λ0)��ʼɨ��λͼ,���Ƿ���1��λ,����,��ecx������λ��ƫ��ֵ(����ַλ0--31).
	je 3f							# ���û���ź�����ǰ��ת�ȳ�.
	btrl %ecx, %ebx					# ��λ���ź�(ebx����ԭsignalλͼ).
	movl %ebx, signal(%eax)			# ���±���signalλͼ��Ϣ->current->signal.
	incl %ecx						# ���źŵ���Ϊ��1��ʼ����(1--32).
	pushl %ecx						# �ź�ֵ��ջ��Ϊ����do_signal�Ĳ���֮һ.
	call do_signal					# ����C�����źŴ������(kernel/signal.c)
	popl %ecx						# ������ջ�ź�ֵ.
	testl %eax, %eax				# ���Է���ֵ,����Ϊ0����ת��ǰ����2��.
	jne 2b							# see if we need to switch tasks, or do more signals
3:	popl %eax						# eax�к��е�100����ջ��ϵͳ���÷���ֵ.
	popl %ebx
	popl %ecx
	popl %edx
	addl $4, %esp					# skip orig_eax	# ����(����)ԭeaxֵ.
	pop %fs
	pop %es
	pop %ds
	iret 							# ϵͳ���ý���

#### int16 -- �����������ж�. ����: ����;�޴�����.
# ����һ���ⲿ�Ļ���Ӳ�����쳣.��Э��������⵽�Լ���������ʱ,�ͻ�ͨ��ERROR����֪ͨCPU.����������ڴ���Э�����������ĳ����ź�.����תȥִ��C����
# math_error()(kernel/math/error.c).���غ���ת�����ret_from_sys_call������ִ��.
.align 4
coprocessor_error:
	push %ds
	push %es
	push %fs
	pushl $-1						# fill in -1 for orig_eax	# ��-1,��������ϵͳ����.
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10, %eax				# ds,es��Ϊָ���ں����ݶ�.
	mov %ax, %ds
	mov %ax, %es
	movl $0x17, %eax				# fs��Ϊָ��ֲ����ݶ�(�����������ݶ�).
	mov %ax, %fs
	pushl $ret_from_sys_call		# ��������÷��صĵ�ַ��ջ.
	jmp math_error					# ִ��math_error()(kernel/math/error.c).

#### int7 -- �豸�����ڻ�Э������������. ����:����;�޴�����.
# ������ƼĴ���CR0��EM(ģ��)��־��λ,��CPUִ��һ��Э������ָ��ʱ�ͻ��������ж�,����CPU�Ϳ����л���������жϴ������ģ�⴦����ָ��.CR0�Ľ�����־TS��
# ��CPUִ������ת��ʱ���õ�.TS��������ȷ��ʲôʱ��Э�������е�������CPU����ִ�е�����ƥ����.��CPU������һ��Э������ת��ָ��ʱ����TS��λʱ,�ͻ��������ж�.
# ��ʱ�Ϳ��Ա���ǰһ�������Э����������,���ָ��������Э������ִ��״̬.�μ�kernel/sched.c.���ж����ת�Ƶ����ret_from_sys_call��ִ����ȥ(��Ⲣ����
# �ź�).
.align 4
device_not_available:
	push %ds
	push %es
	push %fs
	pushl $-1						# fill in -1 for orig_eax	# ��-1,��������ϵͳ����.
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10, %eax				# ds,es��Ϊָ���ں����ݶ�.
	mov %ax, %ds
	mov %ax, %es
	movl $0x17, %eax				# fs��Ϊָ��ֲ����ݶ�(�����������ݶ�).
	mov %ax, %fs
	# ��CR0�������ѽ�����־TS,��ȡCR0ֵ.������Э�����������־EMû����λ,˵������EM������ж�,��ָ�����Э������״̬,ִ��C����math_state_restore(),���ڷ���ʱ
	# ȥִ��ret_from_sys_call���Ĵ���.
	pushl $ret_from_sys_call		# ��������ת����õķ��ص�ַ��ջ.
	clts							# clear TS so that we can use math
	movl %cr0, %eax
	testl $0x4, %eax				# EM (math emulation bit)
	je math_state_restore			# ִ��math_state_restore()(kernel/sched.c)
	# ��EM��־��λ,��ִ֧����ѧ�������math_emulate().
	pushl %ebp
	pushl %esi
	pushl %edi
	pushl $0						# temporary storage for ORIG_EIP
	call math_emulate				# ����C����(math/math_emulate.c)
	addl $4, %esp					# ������ʱ�洢.
	popl %edi
	popl %esi
	popl %ebp
	ret								# �����ret����ת��ret_from_sys_call.

#### int32 -- (int 0x20)ʱ���жϴ������.�ж�Ƶ������Ϊ100Hz(include/linux/sched.h),��ʱоƬ8253/8254��
# ��(kernel/sched.c)����ʼ����.�������jiffiesÿ10�����1.��δ��뽫jiffies��1,���ͽ����ж�ָ���8259������,
# Ȼ���õ�ǰ��Ȩ����Ϊ��������C����do_timer(long CPL).�����÷���ʱתȥ��Ⲣ�����ź�.
.align 4
timer_interrupt:
	push %ds						# save ds,es and put kernel data space
	push %es						# into them. %fs is used by _system_call
	push %fs						# ����ds,es������ָ���ں����ݶ�.fs������system_call.
	pushl $-1						# fill in -1 for orig_eax	# ��-1,��������ϵͳ����.
	# �������Ǳ���Ĵ���eax,ecx��edx.������Ϊgcc�������ڵ��ú���ʱ���ᱣ������.����Ҳ������ebx�Ĵ���.��Ϊ�ں���ret_from_sys_call
	# �л��õ���.
	pushl %edx						# we save %eax,%ecx,%edx as gcc doesn't
	pushl %ecx						# save those across function calls. %ebx
	pushl %ebx						# is saved as we use that in ret_sys_call
	pushl %eax
	movl $0x10, %eax				# ds,es��Ϊָ���ں����ݶ�.
	mov %ax, %ds
	mov %ax, %es
	movl $0x17, %eax				# fs��Ϊָ��ֲ����� (��������ݶ�).
	mov %ax, %fs
	incl jiffies
	# ���ڳ�ʼ���жϿ���оƬʱû�в����Լ���EOI,����������Ҫ��ָ�������Ӳ���ж�.
	movb $0x20, %al					# EOI to interrupt controller #1
	outb %al, $0x20
	# ����Ӷ�ջ��ȡ��ִ��ϵͳ���ô����ѡ���(CS�μĴ���ֵ)�еĵ�ǰ��Ȩ����(0��3)��ѹ���ջ,��Ϊdo_timer�Ĳ���.do_timer()����ִ��
	# �����л�,��ʱ�ȹ���,��kernel/sched.cʵ��.
	movl CS(%esp), %eax
	andl $3, %eax					# %eax is CPL (0 or 3, 0=supervisor)
	pushl %eax
	call do_timer					# 'do_timer(long CPL)' does everything from
	addl $4, %esp					# task switching to accounting ...
	jmp ret_from_sys_call

#### ����sys_execve()ϵͳ����.ȡ�жϵ��ó���Ĵ���ָ����Ϊ��������C����do_execve().
# do_execve()��fs/exec.c.
.align 4
sys_execve:
	lea EIP(%esp), %eax				# eaxָ���ջ�б����û�����eipָ�봦.
	pushl %eax
	call do_execve
	addl $4, %esp					# ��������ʱѹ��ջ��EIPֵ.
	ret

#### sys_fork()����,���ڴ����ӽ���,��system_call����2.ԭ����include/linux/sys.h��.
# ���ȵ���C����find_empty_process(),ȡ��һ�����̺�last_pid.�����ظ�����˵��Ŀǰ������������.Ȼ�����copy_process()���ƽ���.
.align 4
sys_fork:
	call find_empty_process			# Ϊ�½���ȡ�ý��̺�last_pid(kernel/fork.c)
	testl %eax, %eax				# ��eax�з��ؽ��̺�.�����ظ������˳�.
	js 1f
	push %gs
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call copy_process				# ����C����copy_process()(kernel/fork.c)
	addl $20, %esp					# ������������ѹջ����.
1:	ret

#### int 46 -- (int 0x2E) Ӳ���жϴ������,��ӦӲ���ж�����IRQ14.
# �������Ӳ�̲�����ɻ����ͻᷢ�����ж��ź�.(�μ�kernel/blk_drv/hd.c).
# ������8259A�жϿ��ƴ�оƬ���ͽ���Ӳ���ж�ָ��(EOI),Ȼ��ȡ����do_hd�еĺ���ָ�����edx�Ĵ�����,����do_hdΪNULL,�����ж�edx����ָ��
# �Ƿ�Ϊ��.���Ϊ��,���edx��ֵָ��unexpected_hd_interrupt(),������ʾ������Ϣ.�����8259A��оƬ��EOIָ��,������edx��ָ��ָ��ĺ���:
# read_intr(),write_intr()��unexpected_hd_interrupt().
hd_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10, %eax				# dx,es��Ϊ�ں����ݶ�.
	mov %ax, %ds
	mov %ax, %es
	movl $0x17, %eax				# fs��Ϊ���ó���ľֲ����ݶ�.
	mov %ax, %fs
	# ���ڳ�ʼ���жϿ���оƬʱû�в����Լ�EOI,��������Ҫ��ָ�������Ӳ���ж�.
	movb $0x20, %al
	outb %al, $0xA0					# EOI to interrupt controller #1	# �ʹ�8259A
	jmp 1f							# give port chance to breathe	# ����jmp����ʱ����.
1:	jmp 1f
	# do_hd����Ϊһ������ָ��,������ֵread_intr()��write_intr()������ַ.�ŵ�edx�Ĵ�����ͽ�do_hdָ�������ΪNULL.Ȼ����Եõ��ĺ���ָ��,
	# ����ָ��Ϊ��,�����ָ��ָ��C����unexpected_hd_interrupt(),�Դ���δ֪Ӳ���ж�.
1:	xorl %edx, %edx
	movl %edx, hd_timeout			# hd_timeout��Ϊ0.��ʾ���������ڹ涨ʱ���ڲ������ж�.
	xchgl do_hd, %edx 				# schgl���ָ��:��һ���ֽڻ�һ���ֵ�Դ��������Ŀ�Ĳ������ཻ��
	testl %edx, %edx
	jne 1f							# ����,����ָ��ָ��C����unexpected_hd_interrupt().
	movl $unexpected_hd_interrupt, %edx
1:	outb %al, $0x20					# ��8259A��оƬEOIָ��(����Ӳ���ж�).
	call *%edx						# "interesting" way of handling intr. # ����do_hdָ���C����.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

#### int38 -- (int 0x26)�����������жϴ������,��ӦӲ���ж�����IRQ6.
# �䴦������������Ӳ�̵Ĵ������һ��.(kernel/blk_drv/floppy.c).
# ������8259A�жϿ�������оƬ����EOIָ��,Ȼ��ȡ����do_floppy�еĺ���ָ�����eax�Ĵ�����,����do_floppyΪNULL,
# �����ж�eax����ָ���Ƿ�Ϊ��.��Ϊ��,���eax��ֵ����unexpected_floppy_interrupt(),������ʾ������Ϣ.������
# eaxָ��ĺ���:rw_interrupt,seek_interrupt,recal_interrupt,reset_interrupt��unexpected_floppy_interrupt.
floppy_interrupt:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10, %eax				# ds,es��Ϊ�ں����ݶ�.
	mov %ax, %ds
	mov %ax, %es
	movl $0x17, %eax				# fs��Ϊ���ó���ľֲ����ݶ�.
	mov %ax, %fs
	movb $0x20, %al					# ����8259A�жϿ�����EOIָ��(����Ӳ���ж�).
	outb %al, $0x20					# EOI to interrupt controller #1
	xorl %eax, %eax
	# do_floppyΪһ����ָ��,������ֵʵ�ʴ���C����ָ��.��ָ���ڱ������ŵ�eax�Ĵ�����ͽ�do_floppy�����ÿ�.Ȼ�����eax
	# ��ԭָ���Ƿ�Ϊ��,������ʹָ��ָ��C����unexpected_floppy_interrupt().
	xchgl do_floppy, %eax
	testl %eax, %eax				# ���Ժ���ָ���Ƿ�=NULL?
	jne 1f							# ����,��ʹָ��C����unexpected_floppy_interrupt().
	movl $unexpected_floppy_interrupt, %eax
1:	call *%eax						# "interesting" way of handling intr.	# ��ӵ���.
	pop %fs							# �Ͼ����do_floppyָ��ĺ���.
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

#### int 39 -- (int 0x27)���п��жϴ������,��ӦӲ���ж������ź�IRQ7.
# ���汾�ں˻�δʵ��.����ֻ�Ƿ���EOIָ��.
parallel_interrupt:
	pushl %eax
	movb $0x20, %al
	outb %al, $0x20
	popl %eax
	iret
