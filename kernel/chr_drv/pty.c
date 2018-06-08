/*
 *  linux/kernel/chr_drv/pty.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	pty.c
 *
 * This module implements the pty functions
 *	void mpty_write(struct tty_struct * queue);
 *	void spty_write(struct tty_struct * queue);
 */
/*
 *      pty.c
 * ����ļ�ʵ����α�ն�ͨ�ź�����
 *      void mpty_write(struct tty_struct * queue);
 *      void spty_write(struct tty_struct * queue);
 */
#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>

// α�ն�д������
// ������from - Դα�ն˽ṹ��to - Ŀ��α�ն˽ṹ��
static inline void pty_copy(struct tty_struct * from, struct tty_struct * to)
{
	char c;

	// �ж�Դ�ն��Ƿ�ֹͣ��Դ�ն�д�����Ƿ�Ϊ�ա����Դ�ն�δֹͣ������Դ�ն�д���в�Ϊ�գ���ѭ������֮��
	while (!from->stopped && !EMPTY(from->write_q)) {
		// �ж�Ŀ���ն˶������Ƿ�����������������ȵ���copy_to_cooked��������Ŀ���ն˶����У�Ȼ������ѭ������
		if (FULL(to->read_q)) {
			// �ж�Ŀ���ն˸��������Ƿ����������������ֱ���˳�ѭ�������ٴ���Դ�ն�д�����е����ݡ�
			if (FULL(to->secondary))
				break;
			copy_to_cooked(to);     						// �Ѷ������е��ַ�����ɳɹ淶ģʽ�ַ����з��븨�����С�
			continue;
		}
		GETCH(from->write_q, c);         					// ��Դ�ն�д������ȡһ���ַ�������c��
		PUTCH(c, to->read_q);            					// Ȼ���c�е��ַ�����Ŀ���ն˶������С�
		// �жϵ�ǰ�����Ƿ����ź���Ҫ��������У����˳�ѭ����
		if (current->signal & ~current->blocked)
			break;
	}
	copy_to_cooked(to);     								// �Ѷ������е��ַ�����ɳɹ淶ģʽ�ַ����з��븨�����С�
	wake_up(&from->write_q->proc_list);     				// ���ѵȴ�Դ�ն�д���еĽ��̣�����С�
}

/*
 * This routine gets called when tty_write has put something into
 * the write_queue. It copies the input to the output-queue of it's
 * slave.
 */
/*
 * �������������ʱ��tty_write�����Ѿ���һЩ�ַ��ŵ�д����write_queue�С�����������Щ���뵽���Ĵ�α�ն˵�
 * ��������С�
 */
// ��α�ն�д������
void mpty_write(struct tty_struct * tty)
{
	int nr = tty - tty_table;       						// ��ȡ�ն˺�

	// �ն˺ų���64ȡ�����Ϊ2���ǡ���α�նˡ���������ʾ������Ϣ��
	if ((nr >> 6) != 2)
		printk("bad mpty\n\r");
	else
		pty_copy(tty, tty + 64);   							// ����α�ն�д������
}

// ��α�ն�д������
void spty_write(struct tty_struct * tty)
{
	int nr = tty - tty_table;       						// ��ȡ�ն˺�

	// �ն˺ų���64ȡ�����Ϊ3���ǡ���α�նˡ���������ʾ������Ϣ��
	if ((nr >> 6) != 3)
		printk("bad spty\n\r");
	else
		pty_copy(tty, tty - 64);   							// ����α�ն�д������
}

