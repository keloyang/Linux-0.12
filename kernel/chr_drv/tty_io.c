/*
 *  linux/kernel/tty_io.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'tty_io.c' gives an odo_tty_interruptrthogonal feeling to tty's, be they consoles
 * or rs-channels. It also implements echoing, cooked mode etc.
 *
 * Kill-line thanks to John T Kohl, who also corrected VMIN = VTIME = 0.
 */
/*
 * 'tty_io.c'��tty�ն�һ�ַ���صĸо�,���������ǿ���̨����Ʒ�����ն�.
 * �ó���ͬ��ʵ���˻���,�淶(��)ģʽ��.
 */

#include <ctype.h>										// �ַ�����ͷ�ļ�.������һЩ�й��ַ������жϵ�ת���ĺ�.
#include <errno.h>
#include <signal.h>
#include <unistd.h>										// unistd.h�Ǳ�׼���ų����������ļ�,�������˸��ֺ���.

// ������ʱ���棨alarm���ź����ź�λͼ�ж�Ӧ��λ����λ��
#define ALRMMASK (1<<(SIGALRM-1))

#include <linux/sched.h>
#include <linux/tty.h>									// ttyͷ�ļ�,�������й�tty_io,����ͨ�ŷ���Ĳ���,����.
#include <asm/segment.h>								// �β���ͷ�ļ�.�������йضμĴ���������Ƕ��ʽ��ຯ��.
#include <asm/system.h>									// ϵͳͷ�ļ�.�������û��޸�������/�ж��ŵ�Ƕ��ʽ����.

int kill_pg(int pgrp, int sig, int priv);
int is_orphaned_pgrp(int pgrp);                 		// �ж��Ƿ�¶�����

// ��ȡtermios�ṹ������ģʽ��־��֮һ,���������ж�һ����־���Ƿ�����λ��־.
#define _L_FLAG(tty, f)	((tty)->termios.c_lflag & f)	// ����ģʽ��־
#define _I_FLAG(tty, f)	((tty)->termios.c_iflag & f)	// ����ģʽ��־
#define _O_FLAG(tty, f)	((tty)->termios.c_oflag & f)	// ���ģʽ��־

// ȡtermios�ṹ�ն�����(����)ģʽ��־���е�һ����־.
#define L_CANON(tty)	_L_FLAG((tty), ICANON)			// ȡ�淶ģʽ��־.
#define L_ISIG(tty)		_L_FLAG((tty), ISIG)			// ȡ�źű�־
#define L_ECHO(tty)		_L_FLAG((tty), ECHO)			// ȡ�����ַ���־
#define L_ECHOE(tty)	_L_FLAG((tty), ECHOE)			// �淶ģʽʱȡ���Բ�����־.
#define L_ECHOK(tty)	_L_FLAG((tty), ECHOK)			// �淶ģʽʱȡKILL������ǰ�б�־
#define L_ECHOCTL(tty)	_L_FLAG((tty), ECHOCTL)			// ȡ���Կ����ַ���־
#define L_ECHOKE(tty)	_L_FLAG((tty), ECHOKE)			// �淶ģʽʱȡKILL�����в����Ա�־
#define L_TOSTOP(tty)	_L_FLAG((tty), TOSTOP)			// ���ں�̨�������SIGTTOU�ź�

// ȡtermios�ṹ����ģʽ��־���е�һ����־
#define I_UCLC(tty)		_I_FLAG((tty), IUCLC)			// ȡ��д��Сдת����־
#define I_NLCR(tty)		_I_FLAG((tty), INLCR)			// ȡ���з�NLת�س���CR��־
#define I_CRNL(tty)		_I_FLAG((tty), ICRNL)			// ȡ�س���CRת���з�NL��־
#define I_NOCR(tty)		_I_FLAG((tty), IGNCR)			// ȡ���Իس���CR��־
#define I_IXON(tty)		_I_FLAG((tty), IXON)			// ȡ�����������־XON

// ȡtermios�ṹ���ģʽ��־���е�һ����־.
#define O_POST(tty)		_O_FLAG((tty), OPOST)			// ȡִ����������־
#define O_NLCR(tty)		_O_FLAG((tty), ONLCR)			// ȡ���з�NLת�س����з�CR-NL��־
#define O_CRNL(tty)		_O_FLAG((tty), OCRNL)			// ȡ�س���CRת���з�NL��־
#define O_NLRET(tty)	_O_FLAG((tty), ONLRET)			// ȡ���з�NLִ�лس����ܵı�־
#define O_LCUC(tty)		_O_FLAG((tty), OLCUC)			// ȡСдת��д�ַ���־

// ȡtermios�ṹ���Ʊ�־���в����ʡ�CBAUD�ǲ����������루0000017����
#define C_SPEED(tty)	((tty)->termios.c_cflag & CBAUD)
// �ж�tty�ն��Ƿ��ѹ��ߣ�hang up�������䴫�䲨�����Ƿ�ΪB0��0����
#define C_HUP(tty)	(C_SPEED((tty)) == B0)

// ȡ��Сֵ��
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// ���涨��tty�ն�ʹ�õĻ�����нṹ����tty_queues��tty�ն˱�ṹ����tty_table.
// QUEUES��tty�ն�ʹ�õĻ�������������.α�ն˷���������(master��slave).ÿ��tty�ն�ʹ��3��tty�������,���Ƿֱ������ڻ���
// ���̻�������Ķ�����read_queue,���ڻ�����Ļ���������д����write_queue,�Լ����ڱ���淶ģʽ�ַ��ĸ����������secondary.
#define QUEUES	(3 * (MAX_CONSOLES + NR_SERIALS + 2 * NR_PTYS))					// ��54��.
static struct tty_queue tty_queues[QUEUES];										// tty�����������.
struct tty_struct tty_table[256];												// tty��ṹ����.

// �����趨�������͵�tty�ն���ʹ�û�����нṹ��tty_queues[]�����е���ʼ��λ��.
// 8���������̨�ն�ռ��tty_queues[]���鿪ͷ24��(3*MAX_CONSOLES)(0 -- 23)
// ���������ն�ռ������6��(3*NR_SERIALS)(24 -- 29)
// 4����α�ն�ռ������12��(3*NR_PTYS)(30 -- 41)
// 4����α�ն�ռ������12��(3*NR_PTYS)(42 -- 53)
#define con_queues tty_queues
#define rs_queues ((3 * MAX_CONSOLES) + tty_queues)
#define mpty_queues ((3 * (MAX_CONSOLES + NR_SERIALS)) + tty_queues)
#define spty_queues ((3 * (MAX_CONSOLES + NR_SERIALS + NR_PTYS)) + tty_queues)

// �����趨��������tty�ն���ʹ�õ�tty�ṹ��tty_table[]�����е���ʼ��λ��.
// 8���������̨�ն˿���tty_table[]���鿪ͷ64��(0 -- 63);
// 2�������ն�ʹ������2��(64 -- 65);
// 4����α�ն�ʹ�ô�128��ʼ����,���64��(128 -- 191).
// 4����α�ն�ʹ�ô�192��ʼ����,���64��(192 -- 255).
#define con_table tty_table							// �������̨�ն�tty����ų���.
#define rs_table (64 + tty_table)					// �����ն�tty��.
#define mpty_table (128 + tty_table)				// ��α�ն�tty��.
#define spty_table (192 + tty_table)				// ��α�ն�tty��.

int fg_console = 0;									// ��ǰǰ̨����̨��(��Χ0--7).

/*
 * these are the tables used by the machine code handlers.
 * you can implement virtual consoles.
 */
/*
 * �����ǻ�������ʹ�õĻ�����нṹ��ַ��.ͨ���޸������,�����ʵ���������̨
 */
// tty��д������нṹ��ַ��.��rs_io.s����ʹ��,����ȡ�ö�д������нṹ�ĵ�ַ.
struct tty_queue * table_list[] = {
	con_queues + 0, con_queues + 1,					// ǰ̨����̨��,д���нṹ��ַ
	rs_queues + 0, rs_queues + 1,					// �����ն�1��,д���нṹ��ַ
	rs_queues + 3, rs_queues + 4					// �����ն�2��,д���нṹ��ַ
};

// �ı�ǰ̨����̨.
// ��ǰ̨����̨�趨Ϊָ�����������̨
// ����:new_console - ָ�����¿���̨��
void change_console(unsigned int new_console)
{
	// �������ָ���Ŀ���̨�Ѿ���ǰ̨���߲�����Ч,���˳�.�������õ�ǰǰ̨����̨��,ͬʱ����table_list[]�е�ǰ̨����̨��/д���нṹ��ַ.
	// �����µ�ǰǰ̨����̨��Ļ.
	if (new_console == fg_console || new_console >= NR_CONSOLES)
		return;
	fg_console = new_console;
	table_list[0] = con_queues + 0 + fg_console * 3;
	table_list[1] = con_queues + 1 + fg_console * 3;
	update_screen();
}

// ������л����������ý��̽�����ж�˯��״̬.
// ����:queue - ָ�����е�ָ��.
// ������ȡ���л��������ַ�֮ǰ��Ҫ���ô˺���������֤.�����ǰ����û���ź�Ҫ����,����ָ���Ķ��л�������,���ý��̽�����ж�˯��״̬,��
// �ö��еĽ��̵ȴ�ָ��ָ��ý���.
static void sleep_if_empty(struct tty_queue * queue)
{
	cli();
	while (!(current->signal & ~current->blocked) && EMPTY(queue))
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

// �����л����������ý��̽�����жϵ�˯��״̬.
// ����:queue - ָ�����е�ָ��.
// �����������л�������д���ַ�֮ǰ��Ҫ���ô˺����ж϶��е����.
static void sleep_if_full(struct tty_queue * queue)
{
	// ������л����������򷵻��˳�.����������û���ź���Ҫ����,���Ҷ��л������п���ʣ��������<128,���ý��̽�����ж�Ϣ״̬,���øö��еĽ���
	// �ȴ�ָ��ָ��ý���.
	if (!FULL(queue))
		return;
	cli();
	while (!(current->signal & ~current->blocked) && LEFT(queue) < 128)
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

// �ȴ�����.
// ���ǰ̨����̨�����л�������,���ý��̽�����ж�˯��״̬.
void wait_for_keypress(void)
{
	sleep_if_empty(tty_table[fg_console].secondary);
}

// ���Ƴɹ淶ģʽ�ַ�����
// �����ն�termios�ṹ�����õĸ��ֱ�־,��ָ��ttyͬ�����л������е��ַ�����ת���ɹ淶ģʽ(��ģʽ)�ַ�������ڸ�������(�淶ģʽ����)��.
// ����:tty - ָ���ն˵�tty�ṹָ��.
void copy_to_cooked(struct tty_struct * tty)
{
	signed char c;

	// ���ȼ�鵱ǰ�ն�tty�ṹ�л������ָ���Ƿ���Ч.�����������ָ�붼��NULL,��˵���ں�tty��ʼ������������.
	if (!(tty->read_q || tty->write_q || tty->secondary)) {
		printk("copy_to_cooked: missing queues\n\r");
		return;
	}
	// �������Ǹ����ն�termios�ṹ�е�����ͱ��ر�־,�Դ�tty�����л�������ȡ����ÿ���ַ������ʵ��Ĵ���,Ȼ����븨������secondary��.������
	// ѭ������,�����ʱ�����л������Ѿ�ȡ�ջ򶼸������л������Ѿ������ַ�,���˳�ѭ����.�������ʹӶ����л�����βָ�봦ȡһ�ַ�,����βָ��ǰ��
	// һ���ַ�λ��.Ȼ����ݸ��ַ�����ֵ���д���.
	// ����,���������_POSIX_VDISABLE(\0),��ô�ڶ��ַ����������,���ַ�����ֵ����_POSIX_VDISABLE��ֵʱ,��ʾ��ֹʹ����Ӧ��������ַ��Ĺ���.
	while (1) {
		// ���tty��Ӧ�Ķ�����Ϊ����ֱ�������ж�ѭ��
		if (EMPTY(tty->read_q))
			break;
		// ���tty��Ӧ�ĵ���������Ϊ����ֱ�������ж�ѭ��
		if (FULL(tty->secondary))
			break;
		GETCH(tty->read_q, c);								// ȡһ�ַ���c,��ǰ��βָ��
		// ������ַ��ǻس���CR(13),��ô���س�����ת����CRNL��λ,���ַ�ת��Ϊ���з�NL(10).����������Իس���־NOCR��λ,����Ը��ַ�,�������������ַ�
		if (c == 13) {
			if (I_CRNL(tty))
				c = 10;
			else if (I_NOCR(tty))
				continue;
		// ����ַ��ǻ��з�NL(10),����ת�س���־NLCR��λ,����ת��Ϊ�س���CR(13).
		} else if (c == 10 && I_NLCR(tty))
			c = 13;
		// �����дתСд�����־UCLC��λ,�򽫸��ַ�ת��ΪСд�ַ�.
		if (I_UCLC(tty))
			c = tolower(c);
		// �������ģʽ��־���й淶ģʽ��־CANON����λ,��Զ�ȡ���ַ��������´���.����,������ַ��� ��ֹ�����ַ�KILL(^U),���������ĵ�ǰ
		// ��ִ��ɾ������.ɾ��һ���ַ���ѭ����������:���tty�������в���,����ȡ���ĸ������������һ���ַ����ǻ��з�NL(10),���Ҹ��ַ������ļ�����
		// �ַ�(^D),��ѭ��ִ�����д���:
		// ������˻��Ա�־ECHO��λ,��ô:���ַ��ǿ����ַ�(ֵ < 32),����ttyд���з�����������ַ�ERASE(^H).Ȼ���ٷ���һ�������ַ�ERASE,���ҵ���
		// ��ttyд����,��д�����е������ַ�������ն���Ļ��.����,��Ϊ�����ַ��ڷ���д����ʱ��Ҫ��2���ֽڱ�ʾ(����^V),���Ҫ���ر�Կ����ַ������
		// һ��ERASE.���tty��������ͷָ�����1�ֽ�.����,�����_POSIZ_VDISABLE(\0),��ô�ڶ��ַ����������,���ַ�����ֵ����_POSIX_VDISABLE
		// ��ֵʱ,��ʾ��ֹʹ����Ӧ��������ַ��Ĺ���.
		if (L_CANON(tty)) {
			if ((KILL_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c == KILL_CHAR(tty))) {
				/* deal with killing the input line */
				while(!(EMPTY(tty->secondary) ||
				        (c = LAST(tty->secondary)) == 10 ||
				        ((EOF_CHAR(tty) != _POSIX_VDISABLE) && (c == EOF_CHAR(tty))))) {
					if (L_ECHO(tty)) {						// �����ػ��Ա�־��λ
						if (c < 32)							// �����ַ�Ҫɾ2�ֽ�
							PUTCH(127, tty->write_q);
						PUTCH(127, tty->write_q);
						tty->write(tty);
					}
					DEC(tty->secondary->head);
				}
				continue;									// ������ȡ���������ַ����д���.
			}
			// ������ַ���ɾ�������ַ�ERASE(^H),��ô:���tty�ĸ�������Ϊ��,���������һ���ַ��ǻ��з�NL(10),�������ļ�������,��������������ַ�.������ػ���
			// ��־ECHO��λ,��ô:���ַ��ǿ����ַ�(ֵ < 32),����tty��д�����з�������ַ�ERASE.�ٷ���һ�������ַ�ERASE,���ҵ��ø�tty��д����.���tty����
			// ����ͷָ�����1�ֽ�,�������������ַ�.ͬ����,���������_POSIX_VDISABLE(\0),��ô�ڶ��ַ����������,���ַ�����ֵ����_POSIX_VDISABLE��ֵʱ,
			// ��ʾ��ֹʹ����Ӧ��������ַ��Ĺ���.
			if ((ERASE_CHAR(tty) != _POSIX_VDISABLE) && (c == ERASE_CHAR(tty))) {
				if (EMPTY(tty->secondary) ||
				   (c = LAST(tty->secondary)) == 10 ||
				   ((EOF_CHAR(tty) != _POSIX_VDISABLE) &&
				    (c == EOF_CHAR(tty))))
					continue;
				if (L_ECHO(tty)) {							// �����ػ��Ա�־��λ.
					if (c < 32)
						PUTCH(127, tty->write_q);
					PUTCH(127, tty->write_q);
					tty->write(tty);
				}
				DEC(tty->secondary->head);
				continue;
			}
        }
		// ���������IXON��־,��ʹ�ն�ֹͣ/��ʼ��������ַ�������.���û�����ô˱�־,��ôֹͣ�Ϳ�ʼ�ַ�������Ϊһ���ַ������̶�ȡ.����δ�����,�����ȡ���ַ���ֹͣ
		// �ַ�STOP(^S),����ttyֹͣ��־,��tty��ͣ���.ͬʱ��������������ַ�(�����븨��������),���������������ַ�.����ַ��ǿ�ʼ�ַ�START(^Q),��λttyֹͣ
		// ��־,�ָ�tty���.ͬʱ�����ÿ����ַ�,���������������ַ�.���ڿ���̨��˵,�����tty->write()��console.c�е�con_write()����.��˶���α�ն�Ҳ����������
		// ���ն�stopped��־������ͣд����(chr_drv/console.c).����α�ն�Ҳ�������������ն�stopped��־������ͣд����(chr_drv/pty.c).
		// ���ڴ����ն�,ҲӦ���ڷ����ն˹����и����ն�stopped��־��ͣ����,������δʵ��.
		if (I_IXON(tty)) {
			if ((STOP_CHAR(tty) != _POSIX_VDISABLE) && (c==STOP_CHAR(tty))) {
				tty->stopped = 1;
				tty->write(tty);
				continue;
			}
			if ((START_CHAR(tty) != _POSIX_VDISABLE) && (c==START_CHAR(tty))) {
				tty->stopped = 0;
				tty->write(tty);
				continue;
			}
        }
		// ������ģʽ��־����ISIG��־��λ,��ʾ�ն˼��̿��Բ����ź�,�����յ������ַ�INTR,QUIT,SUSP��DSUSPʱ,��ҪΪ���̲�����Ӧ���ź�.������ַ��Ǽ����жϷ�(^C),����
		// ��ǰ����֮�����������н��̷��ͼ����ж��ź�SIGINT,������������һ�ַ�.������ַ����˳���(^\),����ǰ����֮�����������н��̷��ͼ����˳��ź�SIGQUIT,����������
		// ��һ�ַ�.����ַ�����ͣ��(^Z),����ǰ���̷�����ͣ��ϢSIGTSTP.ͬ��,��������_POSIX_VDISABLE(\0),��ô�ڶ��ַ����������,���ַ�����ֵ����_POSIX_VDISABLE
		// ��ֵʱ,��ʾ��ֹʹ����Ӧ��������ַ��Ĺ���.
		if (L_ISIG(tty)) {
			if ((INTR_CHAR(tty) != _POSIX_VDISABLE) && (c==INTR_CHAR(tty))) {
				kill_pg(tty->pgrp, SIGINT, 1);
				continue;
			}
			if ((QUIT_CHAR(tty) != _POSIX_VDISABLE) && (c==QUIT_CHAR(tty))) {
				kill_pg(tty->pgrp, SIGQUIT, 1);
				continue;
			}
			if ((SUSPEND_CHAR(tty) != _POSIX_VDISABLE) && (c == SUSPEND_CHAR(tty))) {
				if (!is_orphaned_pgrp(tty->pgrp))				// �ж�һ���������Ƿ�¶�����
					kill_pg(tty->pgrp, SIGTSTP, 1);
				continue;
			}
		}
		// ������ַ��ǻ��з�NL(10),�������ļ�������EOF(4,^D),��ʾһ���ַ��Ѵ�����,��Ѹ�����������е�ǰ�����ַ�����ֵsecondar.data��1.����ں���tty_read()��ȡ��һ��
		// �ַ�,��ֵ�����1.
		if (c == 10 || (EOF_CHAR(tty) != _POSIX_VDISABLE && c == EOF_CHAR(tty)))
			tty->secondary->data++;
		// �������ģʽ��־�л��Ա�־ECHO����λ״̬,��ô,����ַ��ǻ��з�NL(10),�򽫻��з�NL(10)�ͻس���(13)����ttyд���л�������;����ַ��ǿ����ַ�(ֵ<32)���һ��Կ���
		// �ַ���־ECHOCTL��λ,���ַ�'^'���ַ�c+64����ttyд������(Ҳ������ʾ^C,^H��);���򽫸��ַ�ֱ�ӷ���ttyд���������.�����ø�ttyд��������.
		if (L_ECHO(tty)) {
			if (c == 10) {
				PUTCH(10, tty->write_q);
				PUTCH(13, tty->write_q);
			} else if (c < 32) {
				if (L_ECHOCTL(tty)) {
					PUTCH('^', tty->write_q);
					PUTCH(c + 64, tty->write_q);
				}
			} else
				PUTCH(c, tty->write_q);
			tty->write(tty);
		}
		// ÿһ��ѭ��ĩ����������ַ����븨��������
		PUTCH(c, tty->secondary);
    }
	// ���˳�ѭ������ѵȴ��ø���������еĽ���(����еĻ�).
	wake_up(&tty->secondary->proc_list);
}

/*
 * Called when we need to send a SIGTTIN or SIGTTOU to our process
 * group
 *
 * We only request that a system call be restarted if there was if the
 * default signal handler is being used.  The reason for this is that if
 * a job is catching SIGTTIN or SIGTTOU, the signal handler may not want
 * the system call to be restarted blindly.  If there is no way to reset the
 * terminal pgrp back to the current pgrp (perhaps because the controlling
 * tty has been released on logout), we don't want to be in an infinite loop
 * while restarting the system call, and have it always generate a SIGTTIN
 * or SIGTTOU.  The default signal handler will cause the process to stop
 * thus avoiding the infinite loop problem.  Presumably the job-control
 * cognizant parent will fix things up before continuging its child process.
 */
/*
 * ����Ҫ�����ź�SIGTTIN��SIGTTOU�����ǽ����������н���ʱ�ͻ���øú�����
 *
 * �ڽ���ʹ��Ĭ���źŴ�����������£����ǽ�Ҫ��һ��ϵͳ���ñ����������������ϵͳ�������źŶ����жϡ��������������ǣ�
 * ���һ����ҵ���ڲ���SIGTTIN��SIGTTOU�źţ���ô��Ӧ�źž��������ϣ��ϵͳ���ñ�äĿ���������������û������������
 * �ն˵�pgrp��λ����ǰpgrp���������������logoutʱ�����ն��ѱ��ͷţ�����ô���ǲ���ϣ������������ϵͳ����ʱ����һ����
 * ��ѭ���У��������ǲ���SIGTTIN��SIGTTOU�źš�Ĭ�ϵ��źž����ʹ�ý���ֹͣ��������Ա�������ѭ�����⡣��������ʶ��
 * ��ҵ���Ƶĸ����̻��ڼ���ִ�����ӽ���֮ǰ������㶨��
 */
// ��ʹ���ն˵Ľ����������н��̷����źš�
// �ں�̨�������е�һ�����̷��ʿ����ն�ʱ���ú����������̨�������е����н��̷���SIGTTIN��SIGTTOU�źš����ۺ�̨������
// �еĽ����Ƿ��Ѿ���������Ե����������źţ���ǰ���̶��������˳���д���������ء�
int tty_signal(int sig, struct tty_struct *tty)
{
	// ���ǲ�ϣ��ֹͣһ���¶�������Ľ��̣��μ��ļ�kernel/exit.c��˵������
	// ��������ǰ�������ǹ¶������飬�ͳ����ء��������ǰ���������н��̷���ָ���źš�
	if (is_orphaned_pgrp(current->pgrp))
		return -EIO;									/* don't stop an orphaned pgrp */
	(void) kill_pg(current->pgrp, sig, 1);            	// �����ź�sig��
	// �������źű���ǰ�������������Σ������߱���ǰ���̺��Ե���������ء����������ǰ���̵Ķ��ź�sig�������µĴ�����
	// ��ô�ͷ������ǿɱ��жϵ���Ϣ������ͷ�����ϵͳ����������������Լ���ִ�е���Ϣ��
	if ((current->blocked & (1 << (sig - 1))) ||
	    ((int) current->sigaction[sig - 1].sa_handler == 1))
		return -EIO;		/* Our signal will be ignored */
	else if (current->sigaction[sig-1].sa_handler)
		return -EINTR;		/* We _will_ be interrupted :-) */
	else
		return -ERESTARTSYS;	/* We _will_ be interrupted :-) */
					/* (but restart after we continue) */
}

// tty��������
// ���ն˸���������ж�ȡָ���������ַ����ŵ��û�ָ���Ļ������С�
// ������channel - ���豸�ţ�buf - �û�������ָ�룻nr - �����ֽ�����
int tty_read(unsigned channel, char * buf, int nr)
{
	struct tty_struct * tty;
	struct tty_struct * other_tty = NULL;
	char c, *b = buf;
	int minimum, time;

	// �����жϲ�����Ч�Բ�ȡ�ն˵�tty�ṹָ�롣���tty�ն˵������������ָ�붼��NULL���򷵻�EIO������Ϣ�����tty
	// �ն���һ��α�նˣ�����ȡ����һ����Ӧα�ն˵�tty�ṹother_tty��
	if (channel > 255)
		return -EIO;
	tty = TTY_TABLE(channel);
	if (!(tty->write_q || tty->read_q || tty->secondary))
		return -EIO;
	// �����ǰ����ʹ�õ����������ڴ����tty�նˣ������ն˵Ľ������ȴ�뵱ǰ������Ų�ͬ����ʾ��ǰ�����Ǻ�̨��������
	// ��һ�����̣������̲���ǰ̨����������Ҫֹͣ��ǰ����������н��̡�����������Ҫ��ǰ�����鷢��SIGTTIN�źţ�
	// �����صȴ���Ϊǰ̨���������ִ�ж�������
	if ((current->tty == channel) && (tty->pgrp != current->pgrp))
		return(tty_signal(SIGTTIN, tty));
	// �����ǰ�ն���α�նˣ���ô��Ӧ����һ��α�ն˾���other_tty��������tty����α�նˣ���ôother_tty���Ƕ�Ӧ�Ĵ�α
	// �նˣ���֮��Ȼ��
	if (channel & 0x80)
		other_tty = tty_table + (channel ^ 0x40);
	// Ȼ�����VTIME��VMIN��Ӧ�Ŀ����ַ�����ֵ���ö��ַ�������ʱ��ʱֵtime��������Ҫ��ȡ���ַ�����minimum���ڷǹ淶
	// ģʽ�£��������ǳ�ʱ��ʱֵ��VMIN��ʾΪ���������������Ҫ��ȡ�������ַ�������VTIME��һ��1/10�������ʱֵ��
	time = 10L * tty->termios.c_cc[VTIME];            				// ���ö�������ʱ��ʱֵ��
	minimum = tty->termios.c_cc[VMIN];              				// ������Ҫ��ȡ���ַ�������
	// ���tty�ն˴��ڹ淶ģʽ����������СҪ��ȡ�ַ���minimum���ڽ��������ַ���nr��ͬʱ�ѽ��̶�ȡnr�ַ��ĳ�ʱʱ��ֵ
	// ����Ϊ����ֵ�����ᳬʱ��������˵���ն˴��ڷǹ淶ģʽ�£�����ʱ���������ٶ�ȡ�ַ���minimum��������ʱ���ý��̶���ʱ
	// ��ʱֵΪ���޴����ý����ȶ�ȡ���������������ַ�������������ַ�������minimum�Ļ��������������ָ���ĳ�ʱֵtime
	// �����ý��̵Ķ���ʱֵtimeout������ȴ���ȡ�����ַ�������ʱû���������ٶ�ȡ�ַ���minimum��Ϊ0������������Ϊ����
	// �����ַ���nr��������������˳�ʱ��ʱֵtime�Ļ����Ͱѽ��̶��ַ���ʱ��ʱֵtimeout����Ϊϵͳ��ǰʱ��ֵ+ָ���ĳ�ʱ
	// ֵtime��ͬʱ��λtime�����⣬����������õ����ٶ�ȡ�ַ���minimum���ڽ�������ȡ���ַ���nr������minimum=nr������
	// �ڹ淶ģʽ�µĶ�ȡ������������VTIME��VMIN��Ӧ�����ַ�ֵ��Լ���Ϳ��ƣ����ǽ��ڷǹ淶ģʽ����ģʽ�������������á�
	if (L_CANON(tty)) {
		minimum = nr;
		current->timeout = 0xffffffff;
		time = 0;
	} else if (minimum)
		current->timeout = 0xffffffff;
	else {
		minimum = nr;
		if (time)
			current->timeout = time + jiffies;
		time = 0;
	}
	if (minimum > nr)
		minimum = nr;           									// ����ȡҪ����ַ�����
	// �������ǿ�ʼ�Ӹ���������ѭ��ȡ���ַ����ŵ��û�������buf�С����������ֽ�������0,��ִ������ѭ����������ѭ��������
	// �����ǰ�ն���α�նˣ���ô���Ǿ�ִ�����Ӧ����һ��α�ն˵�д��������������һ��α�ն˰��ַ�д�뵱ǰα�ն˸�������
	// �������С�������һ�ն˰�д���л��������ַ����Ƶ���ǰα�ն˶����л������У������й�����ת������뵱ǰα�ն˸���
	// �����С�
	while (nr > 0) {
		if (other_tty)
			other_tty->write(other_tty);
		// ���tty�����������Ϊ�գ����������˹淶ģʽ��־����tty�����л�����δ�������Ҹ����������ַ�����Ϊ0,��ô�����û
		// �����ù����̶��ַ���ʱֵ��Ϊ0�������ߵ�ǰ����Ŀǰ�յ��źţ������˳�ѭ���塣����������ն���һ����α�նˣ�������
		// ��Ӧ����α�ն��Ѿ��Ҷϣ���ô����Ҳ�˳�ѭ���塣�������������������������Ǿ��õ�ǰ���̽�����ж�˯��״̬�����غ�
		// �����������ڹ淶ģʽʱ�ں�����Ϊ��λΪ�û��ṩ���ݣ�����ڸ�ģʽ�¸��������б���������һ���ַ��ɹ�ȡʤ����
		// secondary.data������1���С�
		cli();
		if (EMPTY(tty->secondary) || (L_CANON(tty) &&
		    !FULL(tty->read_q) && !tty->secondary->data)) {
			if (!current->timeout ||
			  (current->signal & ~current->blocked)) {
			  	sti();
				break;
			}
			if (IS_A_PTY_SLAVE(channel) && C_HUP(other_tty))
				break;
			interruptible_sleep_on(&tty->secondary->proc_list);
			sti();
			continue;
		}
		sti();
		// ���濪ʼ��ʽִ��ȡ�ַ�����������ַ���nr���εݼ���ֱ��nr=0���߸����������Ϊ�ա������ѭ�������У�����ȡ��������
		// �����ַ�c�����Ұѻ������βָ��tail�����ƶ�һ���ַ�λ�á������ȡ�ַ����ļ���������^D�������ǻ��з�NL��10������
		// �Ѹ�����������к����ַ�����ֵ��1.������ַ����ļ���������^D�����ҹ淶ģʽ��־����λ״̬�����жϱ�ѭ��������˵��
		// ���ڻ�û�������ļ�����������������ԭʼ���ǹ淶��ģʽ��������ģʽ���û����ַ�����Ϊ��ȡ����Ҳ��ʶ�����еĿ����ַ�
		// �����ļ��������������ǽ��ַ�ֱ�ӷ����û����ݻ�����buf�У����������ַ�����1.��ʱ��������ַ�����Ϊ0���ж�ѭ��������
		// ����ն˴��ڹ淶ģʽ���Ҷ�ȡ���ַ��ǻ��з�NL��10������Ҳ�˳�ѭ��������֮�⣬ֻҪ��û��ȡ�������ַ���nr���Ҹ�������
		// ���գ��ͼ���ȡ�����е��ַ���
		do {
			GETCH(tty->secondary, c);
			if ((EOF_CHAR(tty) != _POSIX_VDISABLE && c == EOF_CHAR(tty)) || c == 10)
				tty->secondary->data--;
			if ((EOF_CHAR(tty) != _POSIX_VDISABLE && c == EOF_CHAR(tty)) && L_CANON(tty))
				break;
			else {
				put_fs_byte(c, b++);
				if (!--nr)
					break;
			}
			if (c == 10 && L_CANON(tty))
				break;
		} while (nr > 0 && !EMPTY(tty->secondary));
		// ִ�е��ˣ���ô���tty�ն˴��ڹ淶ģʽ�£�˵�����ǿ��ܶ����˻��з������������ļ�������������Ǵ��ڷǹ淶ģʽ�£���ô
		// ˵�������Ѿ���ȡ��nr���ַ������߸��������Ѿ���ȡ���ˡ������������Ȼ��ѵȴ����еĽ��̣�Ȼ�󿴿��Ƿ����ù���ʱ��ʱֵ
		// time�������ʱ��ʱֵtime��Ϊ0,���Ǿ�Ҫ��ȴ�һ����ʱ�����������̿��԰��ַ�д��������С��������ý��̶���ʱ��ʱֵ
		// Ϊϵͳ��ǰʱ��jiffies + ����ʱֵtime����Ȼ������ն˴��ڹ淶ģʽ�������Ѿ���ȡnr���ַ������ǾͿ���ֱ���˳������
		// ѭ���ˡ�
		wake_up(&tty->read_q->proc_list);
		if (time)
			current->timeout = time + jiffies;
		if (L_CANON(tty) || b - buf >= minimum)
			break;
    }
	// ��ʱ��ȡtty�ַ�ѭ��������������˸�λ���̵Ķ�ȡ��ʱ��ʱֵtimeout�������ʱ��ǰ�������յ��źŲ��һ�û�ж�ȡ���κ��ַ�
	// ������������ϵͳ���úš�-ERESTARTSYS�����ء�����ͷ����Ѷ�ȡ���ַ�����b-buf����
	current->timeout = 0;
	if ((current->signal & ~current->blocked) && !(b - buf))
		return -ERESTARTSYS;
	return (b - buf);
}

// ttyд����.
// ���û��������е��ַ�����ttyд���л�������.
// ����:channel - ���豸��;buf - ������ָ��;nr - д�ֽ���.
// ������д�ֽ���.
int tty_write(unsigned channel, char * buf, int nr)
{
	static int cr_flag=0;
	struct tty_struct * tty;
	char c, *b = buf;

	// �����жϲ�����Ч�Բ�ȡ�ն˵�tty�ṹָ��.���tty�ն˵������������ָ�붼��NULL,�򷵻�EIO������Ϣ.
	if (channel > 255)
		return -EIO;
	tty = TTY_TABLE(channel);
	if (!(tty->write_q || tty->read_q || tty->secondary))
		return -EIO;
	// ������ն˱���ģʽ��־����������TOSTOP,��ʾ��̨�������ʱ��Ҫ�����ź�SIGTTOU.�����ǰ����ʹ�õ����������ڴ����tty�ն�,�����ն˵Ľ������ȴ�뵱ǰ
	// ������Ų�ͬ,����ʾ��ǰ�����Ǻ�̨�������е�һ������,�����̲���ǰ̨.��������Ҫֹͣ��ǰ����������н���.����������Ҫ��ǰ�����鷢��SIGTTOU�ź�,������
	// �ȴ���Ϊǰ̨���������ִ��д����.
	if (L_TOSTOP(tty) &&
	    (current->tty == channel) && (tty->pgrp != current->pgrp))
		return(tty_signal(SIGTTOU, tty));
	// �������ǿ�ʼ���û�������buf��ѭ��ȡ���ַ����ŵ�д���л�������.����д�ֽ�������0,��ִ������ѭ������.��ѭ��������,�����ʱttyд��������,��ǰ���̽�����ж�
	// ˯��״̬.�����ǰ�������ź�Ҫ����,���˳�ѭ����.
	while (nr > 0) {
		sleep_if_full(tty->write_q);
		if (current->signal & ~current->blocked)
			break;
		// ��Ҫд���ַ���nr������0����ttyд���л���������,��ѭ��ִ�����²���.���ȴ��û���������ȡ1�ֽ�.
		while (nr > 0 && !FULL(tty->write_q)) {
			c = get_fs_byte(b);
			// ����ն����ģʽ��־���е�ִ����������־OPOST��λ,��ִ�ж��ַ��ĺ������.
			if (O_POST(tty)) {
				// ������ַ��ǻس���'\r'(CR,13)���һس���ת���б�־OCRNL��λ,�򽫸��ַ������з�'\n'(NL,10);
				if (c == '\r' && O_CRNL(tty))
					c = '\n';
				// ������ַ��ǻ��з�'\n'(NL,10)���һ���ת�س����ܱ�־ONLRET��λ�Ļ�,�򽫸��ַ����ɻس���'\r'(CR,13).
				else if (c == '\n' && O_NLRET(tty))
					c = '\r';
				// ������ַ��ǻ��з�'\n'���һس���־cr_flagû����λ,������ת�س�-���б�־ONLCR��λ�Ļ�,��cr_flag��־��λ,����һ�س�������д������.Ȼ�����������һ���ַ�.
				if (c == '\n' && !cr_flag && O_NLCR(tty)) {
					cr_flag = 1;
					PUTCH(13, tty->write_q);
					continue;
				}
				// ���Сдת��д��־OLCUC��λ�Ļ�,�ͽ����ַ�ת�ɴ�д�ַ�.
				if (O_LCUC(tty))
					c = toupper(c);									// Сдת�ɴ�д�ַ�.
			}
			// ���Ű��û����ݻ���ָ��bǰ��1�ֽ�;��д�ֽ�����1�ֽ�;��λcr_flag��־,�������ֽڷ���ttyд������.
			b++; nr--;
			cr_flag = 0;
			PUTCH(c, tty->write_q);
		}
		// ��Ҫ����ַ�ȫ��д��,����д��������,������˳�ѭ��.��ʱ����ö�Ӧttyд����,��д���л������е��ַ���ʾ�ڿ���̨��Ļ��,����ͨ�����ж˿ڷ��ͳ�ȥ.�����ǰ�����tty
		// �ǿ���̨�ն�,��ôtty->write()���õ���con_write();���tty�Ǵ����ն�,��tty->write()���õ���rs_write()����.�������ֽ�Ҫд,��ȴ�д�������ַ�ȡ��.
		// ����������õ��ȳ���,��ȥִ����������.
		tty->write(tty);
		if (nr > 0)
			schedule();
        }
	return (b - buf);												// ��󷵻�д����ֽ���.
}

/*
 * Jeh, sometimes I really like the 386.
 * This routine is called from an interrupt,
 * and there should be absolutely no problem
 * with sleeping even in an interrupt (I hope).
 * Of course, if somebody proves me wrong, I'll
 * hate intel for all time :-). We'll have to
 * be careful and see to reinstating the interrupt
 * chips before calling this, though.
 *
 * I don't think we sleep here under normal circumstances
 * anyway, which is good, as the task sleeping might be
 * totally innocent.
 */
/*
 * ��,��ʱ������Ǻ�ϲ��386.���ӳ��򱻴�һ���жϴ�������е���,���Ҽ�ʹ���жϴ��������˯��ҲӦ�þ���û������(��ϣ�����).��Ȼ,�����
 * ��֤�����Ǵ��,��ô�ҽ�����Intelһ����.
 *
 * �Ҳ���Ϊ��ͨ�������ᴦ������˯��,�����ܺ�,��Ϊ����˯������ȫ�����.
 */
// tty�жϴ�����ú��� - �ַ��淶ģʽ����.
// ����:tty - ָ����tty�ն˺�.
// ��ָ��tty�ն˶��л������е��ַ����ƻ�ת���ɹ淶(��)ģʽ�ַ�������ڸ���������.�ú������ڴ��ڶ��ַ��ж�(rs_io.s)�ͼ����ж�(
// kerboard.S)�б�����.
void do_tty_interrupt(int tty)
{
	copy_to_cooked(TTY_TABLE(tty));
}

//�ַ��豸��ʼ������.��,Ϊ�Ժ���չ��׼��.
void chr_dev_init(void)
{
}

// tty�ն˳�ʼ������
// ��ʼ�������ն˻������,��ʼ�������ն˺Ϳ���̨�ն�.
void tty_init(void)
{
	int i;

	// ���ȳ�ʼ�������ն˵Ļ�����нṹ,���ó�ֵ.���ڴ����ն˵Ķ�/д�������,�����ǵ�data�ֶ�����Ϊ���ж˿ڻ���ֵַ.����1��0x3f8,
	// ����2��0x2f8.Ȼ���ȳ������������ն˵�tty�ṹ.
	// ���������ַ�����c_cc[]���õĳ�ֵ������include/linux/tty.h�ļ���.
	for (i = 0 ; i < QUEUES ; i++)
		tty_queues[i] = (struct tty_queue) {0, 0, 0, 0, ""};
	rs_queues[0] = (struct tty_queue) {0x3f8, 0, 0, 0, ""};
	rs_queues[1] = (struct tty_queue) {0x3f8, 0, 0, 0, ""};
	rs_queues[3] = (struct tty_queue) {0x2f8, 0, 0, 0, ""};
	rs_queues[4] = (struct tty_queue) {0x2f8, 0, 0, 0, ""};
	for (i = 0 ; i < 256 ; i++) {
		tty_table[i] =  (struct tty_struct) {
		 	{0, 0, 0, 0, 0, INIT_C_CC},
			0, 0, 0, NULL, NULL, NULL, NULL
		};
	}
	// ���ų�ʼ������̨�ն�(console.c).��con_init()��������,����Ϊ������Ҫ������ʾ�����ͺ���ʾ�ڴ�������ȷ��ϵͳ�������̨������
	// NR_CONSOLES.��ֵ���������Ŀ���tty�ṹ��ʼ��ѭ����.���ڿ���̨��tty�ṹ,425--430����tty�ṹ�а�����termios�ṹ�ֶ�.����
	// ����ģʽ��־������ʼ��ΪICRNL��־;���ģʽ��־����ʼ�����к����־OPOST�Ͱ�NLת����CRNL�ı�־ONLCR;����ģʽ��־������ʼ��
	// ����IXON,ICAON,ECHO,ECHOCTL��ECHOKE��־;�����ַ�����c_cc[]�����ú��г�ʼֵINIT_C_CC.
	// 435���ϳ�ʼ������̨�ն�tty�ṹ�еĶ�����,д����͸���������нṹ,���Ƿֱ�ָ��tty������нṹ����tty_table[]�е���Ӧ�ṹ��.
	con_init();
	for (i = 0 ; i < NR_CONSOLES ; i++) {
		con_table[i] = (struct tty_struct) {
		 	{ICRNL,													/* change incoming CR to NL */	/* CRתNL */
			OPOST | ONLCR,											/* change outgoing NL to CRNL */	/* NLתCRNL */
			0,														// ����ģʽ��־��
			IXON | ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,			// ���ر�־��
			0,														/* console termio */	// ��·���,0 -- TTY
			INIT_C_CC},												// �����ַ�����c_cc[]
			0,														/* initial pgrp */	// ������ʼ������pgrp
			0,														/* initial session */	// ��ʼ�Ự��session
			0,														/* initial stopped */	// ��ʼֹͣ��־stopped
			con_write,
			con_queues + 0 + i * 3, con_queues + 1 + i * 3, con_queues + 2 + i * 3
		};
	}
	// Ȼ���ʼ�������ն˵�tty�ṹ���ֶΡ�450�г�ʼ�������ն�tty�ṹ�еĶ�/д�͸���������нṹ�����Ƿֱ�ָ��tty�������
	// �ṹ����tty_table[]����Ӧ�ṹ�
	for (i = 0 ; i < NR_SERIALS ; i++) {
		rs_table[i] = (struct tty_struct) {
			{0, 													/* no translation */        // ����ģʽ��־����0,����ת����
			0,  													/* no translation */        // ���ģʽ��־����0,����ת����
			B2400 | CS8,                    						// ����ģʽ��־����2400bpx��8λ����λ��
			0,                              						// ����ģʽ��־����
			0,                              						// ��·��̣�0 -- TTY��
			INIT_C_CC},                     						// �����ַ����顣
			0,                              						// ������ʼ�����顣
			0,                              						// ��ʼ�Ự�顣
			0,                              						// ��ʼֹͣ��־��
			rs_write,                       						// �����ն�д������
			rs_queues + 0 + i * 3, rs_queues + 1 + i * 3, rs_queues + 2 + i * 3
		};
	}
	// Ȼ���ٳ�ʼ��α�ն�ʹ�õ�tty�ṹ��α�ն������ʹ�õģ���һ������master��α�ն�����һ���ӣ�slave��α�նˡ���˶�����
	// ��Ҫ���г�ʼ�����á���ѭ���У��������ȳ�ʼ��ÿ����α�ն˵�tty�ṹ��Ȼ���ٳ�ʼ�����Ӧ�Ĵ�α�ն˵�tty�ṹ��
	for (i = 0 ; i < NR_PTYS ; i++) {
		mpty_table[i] = (struct tty_struct) {
			{0, 													/* no translation */        // ����ģʽ��־����0,����ת����
			0,  													/* no translation */        // ���ģʽ��־����0,����ת����
			B9600 | CS8,                    						// ����ģʽ��־����9600bpx��8λ����λ��
			0,                              						// ����ģʽ��־����
			0,                              						// ��·��̣�0 -- TTY��
			INIT_C_CC},                    							// �����ַ����顣
			0,                              						// ������ʼ�����顣
			0,                              						// ��ʼ�Ự�顣
			0,                              						// ��ʼֹͣ��־��
			mpty_write,                     						// ��α�ն�д������
			mpty_queues + 0 + i * 3, mpty_queues + 1 + i * 3, mpty_queues + 2 + i * 3
		};
		spty_table[i] = (struct tty_struct) {
			{0, 													/* no translation */        // ����ģʽ��־����0,����ת����
			0,  													/* no translation */        // ���ģʽ��־����0,����ת����
			B9600 | CS8,                    						// ����ģʽ��־����9600bpx��8λ����λ��
			IXON | ISIG | ICANON,           						// ����ģʽ��־����
			0,                              						// ��·��̣�0 -- TTY��
			INIT_C_CC},                    							// �����ַ����顣
			0,                              						// ������ʼ�����顣
			0,                              						// ��ʼ�Ự�顣
			0,                              						// ��ʼֹͣ��־��
			spty_write,                     						// ��α�ն�д������
			spty_queues + 0 + i * 3, spty_queues + 1 + i * 3, spty_queues + 2 + i * 3
		};
	}
	// ����ʼ�������жϴ������ʹ��нӿ�1��2��serial.c��������ʾϵͳ���е��������̨��NR_CONSOLES��α�ն���NR_PTYS��
	rs_init();
	printk("%d virtual consoles\n\r", NR_CONSOLES);
	printk("%d pty's\n\r", NR_PTYS);
}

