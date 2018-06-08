/*
 *  linux/kernel/serial.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	serial.c
 *
 * This module implements the rs232 io functions
 *	void rs_write(struct tty_struct * queue);
 *	void rs_init(void);
 * and all interrupts pertaining to serial IO.
 */
/*
 *      serial.c
 * �ó�������ʵ��rs232�������������
 *      void rs_write(struct tty_struct *queue);
 *      void rs_init(void);
 * �Լ��봮��IO�й�ϵ�������жϴ������
 */

#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>

#define WAKEUP_CHARS (TTY_BUF_SIZE / 4)   						// ��д�����к���WAKEUP_CHARS���ַ�ʱ�Ϳ�ʼ���͡�

extern void rs1_interrupt(void);        						// ���п�1���жϴ������rs_io.s����
extern void rs2_interrupt(void);        						// ���п�2���жϴ������rs_io.s����

// ��ʼ�����ж˿ڡ�
// ����ָ�����ж˿ڵĴ��䲨���ʣ�2400bps�����������д���ּĴ��������������ж�Դ�����⣬�����2�ֽڵĲ���������ʱ����
// ����������·���ƼĴ�����DLABλ��λ7����
// ������port�Ǵ��ж˿ڻ���ַ������1 - 0x3F8������2 - 0x2F8��
static void init(int port)
{
	outb_p(0x80, port + 3);										/* set DLAB of line control reg */
	outb_p(0x30, port);											/* LS of divisor (48 -> 2400 bps */
	outb_p(0x00, port + 1);										/* MS of divisor */
	outb_p(0x03, port + 3);										/* reset DLAB */
	outb_p(0x0b, port + 4);										/* set DTR,RTS, OUT_2 */
	outb_p(0x0d, port + 1);										/* enable all intrs but writes */
	(void)inb(port);											/* read data port to reset things (?) */
}

// ��ʼ�������жϳ���ʹ��нӿڡ�
// �ж���������IDT�е������������ú�set_intr_gate()��include/asm/system.h��ʵ�֡�
void rs_init(void)
{
	// �����������������������пڵ��ж�����������rs1_interrupt�ǿڴ�1���жϴ������ָ�롣
	// ����1ʹ�õ��ж���int 0x24������2����int 0x23��
	set_intr_gate(0x24, rs1_interrupt);      					// ���ô��п�1���ж�������IRQ4�źţ���
	set_intr_gate(0x23, rs2_interrupt);      					// ���ô��п�2���ж�������IRQ3�źţ���
	init(tty_table[64].read_q->data);       					// ��ʼ�����п�1��.data�Ƕ˿ڻ���ַ����
	init(tty_table[65].read_q->data);       					// ��ʼ�����п�2.
	outb(inb_p(0x21) & 0xE7, 0x21);            					// ������8259A��ӦIRQ3��IRQ4�ж�����
}

/*
 * This routine gets called when tty_write has put something into
 * the write_queue. It must check wheter the queue is empty, and
 * set the interrupt register accordingly
 *
 *	void _rs_write(struct tty_struct * tty);
 */
/*
 * ��tty_write()�ѽ����ݷ��������д������ʱ�����������ӳ����ڸ��ӳ����б������ȼ��д�����Ƿ�Ϊ�գ�Ȼ������
 * ��Ӧ�жϼĴ�����
 */
// �������ݷ��������
// �ú���ʵ����ֻ�ǿ������ͱ��ּĴ����ѿ��жϱ�־���˺󵱷��ͱ��ּĴ�����ʱ��UART�ͻ�����ж����󡣶��ڸô����ж�
// ��������У������ȡ��д����βָ�봦���ַ�������������ͱ��ּĴ����С�һ��UART�Ѹ��ַ����ͳ�ȥ�����ͱ��ּĴ���
// �ж������־��λ�����Ӷ��ٴν�ֹ���ͱ��ּĴ����������ж����󡣴˴Ρ�ѭ�������Ͳ���Ҳ��֮������
void rs_write(struct tty_struct * tty)
{
	// ���д���в��գ������ȴ�0x3f9����0x2f9����ȡ�ж�����Ĵ������ݣ����Ϸ��ͱ��ּĴ����ж������־��λ1������д
	// �ظüĴ����������������ͱ��ּĴ�����ʱUART���ܹ���������������͵��ַ��������жϡ�write_q.data���Ǵ��ж˿ڻ�
	// ��ַ��
	cli();
	if (!EMPTY(tty->write_q))
		outb(inb_p(tty->write_q->data + 1) | 0x02, tty->write_q->data + 1);
	sti();
}

