/*
 *  linux/kernel/rs_io.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	rs_io.s
 *
 * This module implements the rs232 io interrupts.
 */
/*
 * ��ģ��ʵ��rs232��������жϴ������
 */

.text
.globl rs1_interrupt,rs2_interrupt

# size�Ƕ�д���л��������ֽڳ��ȡ���ֵ������2�Ĵη������ұ�����tty_io.c�е�ƥ�䡣
size	= 1024				/* must be power of two !
							   and must match the value
							   in tty_io.c!!! */

/* these are the offsets into the read/write buffer structures */
/* ������Щ�Ƕ�д������нṹ�е�ƫ���� */
rs_addr = 0             										# ���ж˿ں��ֶ�ƫ�ƣ��˿���0x3f8��0x2f8����
head = 4                										# ��������ͷָ���ֶ�ƫ�ơ�
tail = 8                										# ��������βָ���ֶ�ƫ�ơ�
proc_list = 12          										# �ȴ��û���Ľ����ֶ�ƫ�ơ�
buf = 16                										# �������ֶ�ƫ�ơ�

# ��һ��д������������ں˾ͻ��Ҫ��д�������ַ��Ľ�������Ϊ�ȴ�״̬����д��������л�ʣ�����256���ַ�ʱ���жϴ������
# �Ϳ��Ի�����Щ�ȴ����̼�����д�����з��ַ���
startup	= 256													/* chars left in write queue when we restart it */
                        										/* ���������¿�ʼдʱ����������໹ʣ���ַ������� */
/*
 * These are the actual interrupt routines. They look where
 * the interrupt is coming from, and take appropriate action.
 */
/*
 * ��Щ��ʵ�ʵ��жϴ�����򡣳������ȼ���жϵ���Դ��Ȼ��ִ����Ӧ�Ĵ���
 */
## ���ж˿�1�жϴ��������ڵ㡣
# ��ʼ��ʱrs1_interrupt��ַ�������ж�������0x24�У���Ӧ8259A���ж�����IRQ4���š�
# �������Ȱ�tty���д����ն�1������1����д�������ָ��ĵ�ַ��ջ��tty_io.c����Ȼ����ת��rs_int������������������
# �ô���1�ʹ���2�Ĵ�����빫�á��ַ�������нṹtty_queue��ʽ��μ�include/linux/tty.h��
.align 4
rs1_interrupt:
	pushl $table_list + 8         								# tty���д���1��д�������ָ���ַ��ջ��
	jmp rs_int
.align 4
rs2_interrupt:
	pushl $table_list + 16        								# tty���д���2��д�������ָ���ַ��ջ��

# ��δ��������öμĴ���ds��esָ���ں����ݶΣ�Ȼ��Ӷ�Ӧ��д�������data�ֶ�ȡ�����ж˿ڻ���ַ���õ�ַ��2�����жϱ�ʶ
# �Ĵ���IIR�Ķ˿ڵ�ַ����λ0 = 0����ʾ����Ҫ������жϡ����Ǹ���λ2��λ1ʹ��ָ����ת�������Ӧ�ж�Դ���ʹ����ӳ�����
# ÿ���ӳ����л��ڴ������λUART����Ӧ�ж�Դ�����ӳ��򷵻غ���δ����ѭ���ж��Ƿ��������ж�Դ��λ0 = 0���������
# �����жϻ��������ж�Դ����IIR��λ0��Ȼ��0�������жϴ��������ٵ�����Ӧ�ж�Դ�ӳ����������ֱ�����𱾴��жϵ�������
# ��Դ����������λ����ʱUART���Զ�������IIR��λ0 = 1,��ʾ���޴�������жϣ������жϴ�����򼴿��˳���
rs_int:
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	push %es
	push %ds													/* as this is an interrupt, we cannot */
	pushl $0x10													/* know that bs is ok. Load it */
	pop %ds                 									/* ��������һ���жϳ������ǲ�֪��ds�Ƿ���ȷ�� */
	pushl $0x10             									/* ���Լ������ǣ���ds��esָ���ں����ݶΣ� */
	pop %es
	movl 24(%esp), %edx      									# ȡ����35��39����ջ����Ӧ���ڻ������ָ���ַ��
	movl (%edx), %edx        									# ȡ��������нṹָ�루��ַ��->edx��
	movl rs_addr(%edx), %edx 									# ȡ����1���򴮿�2���˿ڻ���ַ->edx��
	addl $2, %edx												/* interrupt ident. reg */  /* ָ���жϱ�ʶ�Ĵ��� */
                                								# �жϱ�ʶ�Ĵ����˿ڵ�ַ��0x3fa��0x2fa����
rep_int:
	xorl %eax, %eax
	inb %dx, %al             									# ȡ�жϱ�ʶ�ֽڣ����ж��ж���Դ����4���ж��������
	testb $1, %al            									# �����ж����޴������жϣ�λ0 = 0���жϣ���
	jne end                 									# ���޴������жϣ�����ת���˳�����end��
	cmpb $6, %al												/* this shouldn't happen, but ... */    /* �ⲻ�ᷢ������... */
	ja end                  									# alֵ����6������ת��end��û������״̬����
	movl 24(%esp), %ecx      									# �����ӳ���֮ǰ���������ָ���ַ����ecx��
	pushl %edx              									# ��ʱ�����жϱ�ʶ�Ĵ����˿ڵ�ַ��
	subl $2, %edx            									# edx�лָ����ڻ���ֵַ0x3f8��0x2f8����
	call *jmp_table(, %eax, 2)									/* NOTE! not *4, bit0 is 0 already */
	# ���������ָ�����д������ж�ʱ��al��λ0=0,λ2��λ1���ж����ͣ�����൱���Ѿ����ж����ͳ���2�������ٳ�2�������ת���Ӧ��
	# �ж����͵�ַ������ת������ȥ����Ӧ�����ж���Դ��4�֣�modem״̬�����仯��Ҫд�����ͣ��ַ���Ҫ�������գ��ַ�����·״̬����
	# �仯���������ַ��ж�ͨ�����÷��ͱ��ּĴ�����־ʵ�֡���serial.c�����У���д���������������ʱ��rs_write()�����ͻ��޸�
	# �ж�����Ĵ������ݣ�����Ϸ��ͱ��ּĴ����ж������־���Ӷ���ϵͳ��Ҫ�����ַ�ʱ�������жϷ�����
	popl %edx               									# �ָ��жϱ�ʶ�Ĵ����˿ڵ�ַ0x3fa��0x2fa����
	jmp rep_int             									# ��ת�������ж����޴������жϲ�����Ӧ����
end:
	movb $0x20, %al
	outb %al, $0x20												/* EOI */
	pop %ds
	pop %es
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	addl $4, %esp												# jump over _table_list entry   # ��������ָ���ַ��
	iret

# ���ж����ʹ����ӳ����ַ��ת������4���ж���Դ��
# modem״̬�仯�жϣ�д�ַ��жϣ����ַ��жϣ���·״̬�������жϡ�
jmp_table:
	.long modem_status, write_char, read_char, line_status

# ����mode״̬�����仯�������˴��жϡ�ͨ����modem״̬�Ĵ���MSR������и�λ������
.align 4
modem_status:
	addl $6, %edx												/* clear intr by reading modem status reg */
	inb %dx, %al             									/* ͨ����modem״̬�Ĵ������и�λ��0x3fe�� */
	ret

# ������·״̬�����仯��������δ����жϡ�ͨ������·״̬�Ĵ���LSR������и�λ������
.align 4
line_status:
	addl $5, %edx												/* clear intr by reading line status reg. */
	inb %dx, %al             									/* ͨ������·״̬�Ĵ������и�λ��0x3fd�� */
	ret

# ����UARTоƬ���յ��ַ�����������жϡ��Խ��ջ���Ĵ���ִ�ж������ɸ�λ���ж�Դ��
# ����ӳ��򽫽��յ����ַ��ŵ����������read_qͷָ�루head�����������ø�ָ��ǰ��һ���ַ�λ�á���headָ���Ѿ����ﻺ����ĩ�ˣ�
# �������۷�����������ʼ����������C����do_tty_interrupt()����copy_to_cooked()�����Ѷ�����ַ������������淶ģʽ����
# ���У������������secondary���С�
.align 4
read_char:
	inb %dx, %al                     							# ��ȡ���ջ���Ĵ���RBR���ַ�->al��
	movl %ecx, %edx                  							# ��ǰ���ڻ������ָ���ַ->edx��
	subl $table_list, %edx           							# ��ǰ���ڶ���ָ���ַ - �������ָ�����ַ ->edx��
	shrl $3, %edx                    							# ��ֵ/8���ô��ںš����ڴ���1��1,���ڴ���2��2��
	movl (%ecx), %ecx											# read-queue    # ȡ��������нṹ��ַ ->ecx��
	movl head(%ecx), %ebx            							# ȡ�������л���ͷָ�� ->ebx��
	movb %al, buf(%ecx, %ebx)         							# ���ַ����ڻ�������ͷָ����ָλ�ô���
	incl %ebx                       							# ��ͷָ��ǰ�ƣ����ƣ�һ�ֽڡ�
	andl $size - 1, %ebx               							# �û��������ȶ�ͷָ��ȡģ������
	cmpl tail(%ecx), %ebx            							# ������ͷָ����βָ��Ƚϡ�
	je 1f                           							# ��ָ���ƶ�����ȣ���ʾ����������������ͷָ�룬��ת��
	movl %ebx, head(%ecx)            							# �����޸Ĺ���ͷָ�롣
1:	addl $63, %edx                   							# ���ں�ת����tty�ţ�63��64������Ϊ������ջ��
	pushl %edx
	call do_tty_interrupt           							# ����tty�жϴ���C������tty_io.c����
	addl $4, %esp                    							# ������ջ�����������ء�
	ret

# �������˷��ͱ��ּĴ��������жϱ�־������˴��жϡ�˵����Ӧ�����ն˵�д�ַ�������������ַ���Ҫ���͡����Ǽ����д�����е�ǰ
# �����ַ��������ַ�����С��256�������ѵȴ�д�������̡�Ȼ���д�������β��ȡ��һ���ַ����ͣ��������ͱ���βָ�롣���д��
# ������ѿգ�����ת��write_buffer_empty������д������пյ������
.align 4
write_char:
	movl 4(%ecx), %ecx											# write-queue       # ȡд������нṹ��ַ->ecx��
	movl head(%ecx), %ebx            							# ��д����ͷָ��->ebx��
	subl tail(%ecx), %ebx            							# ͷָ�� - βָ�� = �������ַ�����
	andl $size - 1, %ebx										# nr chars in queue
	je write_buffer_empty           							# ��ͷָ�� = βָ�룬˵��д���пգ���ת����
	cmpl $startup, %ebx              							# �������ַ���������256����
	ja 1f                           							# ��������ת����
	movl proc_list(%ecx), %ebx									# wake up sleeping process  # ���ѵȴ��Ľ��̡�
	testl %ebx, %ebx											# is there any?     # �еȴ�д�Ľ�����
	je 1f                           							# �ǿյģ�����ǰ��ת�����1����
	movl $0, (%ebx)                  							# ���򽫽�����Ϊ������״̬�����ѽ��̣���
1:	movl tail(%ecx), %ebx            							# ȡβָ�롣
	movb buf(%ecx, %ebx), %al         							# �ӻ�����βָ�봦ȡһ�ַ�->al��
	outb %al, %dx                    							# ��˿�0x3f8��0x2f8��д�����ͱ��ּĴ����С�
	incl %ebx                       							# βָ��ǰ�ơ�
	andl $size - 1, %ebx               							# βָ������������ĩ�ˣ����ۻء�
	movl %ebx, tail(%ecx)            							# �������޸ĵ�βָ�롣
	cmpl head(%ecx), %ebx            							# βָ����ͷָ����Ƚϣ�
	je write_buffer_empty           							# ����ȣ���ʾ�����ѿգ�����ת��
	ret

# ����д�������write_q�ѿյ���������еȴ�д�ô����ն˵Ľ�������֮��Ȼ�����η��ͱ��ּĴ����жϣ����÷��ͱ��ּĴ�����ʱ��
# ���жϡ�
# �����ʱд�������write_q�ѿգ���ʾ��ǰ���ַ���Ҫ���͡���������Ӧ�����������顣
# ���ȿ�����û�н������ȴ�д���пճ���������оͻ���֮�����⣬��Ϊ����ϵͳ�����ַ���Ҫ���ͣ����Դ�ʱ����Ҫ��ʱ��ֹ���ͱ��ּĴ���
# THR��ʱ�����жϣ����UART���ֻᡰ�Զ�������ȡд��������е��ַ��������ͳ�ȥ��
.align 4
write_buffer_empty:
	movl proc_list(%ecx), %ebx									# wake up sleeping process  # ���ѵȴ��Ľ��̡�
                                        						# ȡ�ȴ��ö��еĽ��̵�ָ�룬���ж��Ƿ�Ϊ�ա�
	testl %ebx, %ebx											# is there any?     # �еȴ��Ľ����룿
	je 1f                           							# �ޣ�����ǰ��ת�����1����
	movl $0, (%ebx)                  							# ���򽫽�����Ϊ������״̬�����ѽ��̣���
1:	incl %edx                       							# ָ��˿�0x3f9��0x2f9����
	inb %dx, %al                     							# ��ȡ�ж�����Ĵ���IER��
	jmp 1f                          							# �����ӳ١�
1:	jmp 1f                  									/* ���η��ͱ��ּĴ������жϣ�λ1�� */
1:	andb $0xd, %al												/* disable transmit interrupt */
	outb %al, %dx                    							# д��0x3f9��0x2f9����
	ret

	ret
