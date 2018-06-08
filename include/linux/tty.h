/*
 * 'tty.h' defines some structures used by tty_io.c and some defines.
 *
 * NOTE! Don't touch this without checking that nothing in rs_io.s or
 * con_io.s breaks. Some constants are hardwired into the system (mainly
 * offsets into 'tty_queue'
 */
/*
 * 'tty.h'�ж�����tty_io.c����ʹ�õ�ĳЩ�ṹ������һЩ����.
 *
 * ע��!���޸�����Ķ���ʱ,һ��Ҫ���rs_io.s��con_io.s�����в����������
 * ��ϵͳ����Щ������ֱ��д�ڳ����е�(��Ҫ��һЩtty_queue�е�ƫ��ֵ).
 */

#ifndef _TTY_H
#define _TTY_H

#define MAX_CONSOLES	8								// ����������̨����.
#define NR_SERIALS		2								// �����ն�����
#define NR_PTYS			4								// α�ն�����

extern int NR_CONSOLES;

#include <termios.h>

#define TTY_BUF_SIZE 1024								// tty������(�������)��С.

// tty�ַ�����������ݽṹ.����tty_struc�ṹ�еĶ�/д�͸���(�淶)�������.
struct tty_queue {
	unsigned long data;									// ���л������к����ַ�����ֵ(���ǵ�ǰ�ַ���).���ڴ����ն�,���Ŵ��ж˿ڵ�ַ.
	unsigned long head;									// ������������ͷָ��
	unsigned long tail;									// ������������βָ��
	struct task_struct * proc_list;						// �ȴ������еĽ����б�.
	char buf[TTY_BUF_SIZE];								// ���еĻ�����.
};

#define IS_A_CONSOLE(min)			(((min) & 0xC0) == 0x00)	// ��һ�������ն�.
#define IS_A_SERIAL(min)			(((min) & 0xC0) == 0x40)	// ��һ�����ն�.
#define IS_A_PTY(min)				((min) & 0x80)				// ��һ��α�ն�.
#define IS_A_PTY_MASTER(min)		(((min) & 0xC0) == 0x80)	// ��һ����α�ն�.
#define IS_A_PTY_SLAVE(min)			(((min) & 0xC0) == 0xC0)	// ��һ����α�ն�.
#define PTY_OTHER(min)				((min) ^ 0x40)				// ����α�ն�.

// ���¶�����tty�ȴ������л����������꺯��.(tail��ǰ,head�ں�)
#define INC(a) ((a) = ((a) + 1) & (TTY_BUF_SIZE - 1))           				// a������ָ��ǰ��1�ֽ�,���ѳ����������Ҳ�,��ָ��ѭ��
#define DEC(a) ((a) = ((a) - 1) & (TTY_BUF_SIZE - 1))           				// a������ָ�����1�ֽ�,��ѭ��
#define EMPTY(a) ((a)->head == (a)->tail)                       				// �������Ƿ�Ϊ��
#define LEFT(a) (((a)->tail - (a)->head - 1) & (TTY_BUF_SIZE - 1))      		// ���������ɴ���ַ��ĳ���(����������)
#define LAST(a) ((a)->buf[(TTY_BUF_SIZE - 1) & ((a)->head - 1)])      			// �����������һ��λ��
#define FULL(a) (!LEFT(a))                                      				// ��������(���Ϊ1�Ļ�)
#define CHARS(a) (((a)->head - (a)->tail) & (TTY_BUF_SIZE - 1))       			// ���������Ѵ���ַ��ĳ���(�ַ���)
// ��queue�����������ȡһ�ַ�(��tail��,����tail+=1)
#define GETCH(queue, c) \
(void)({c = (queue)->buf[(queue)->tail]; INC((queue)->tail);})
// ��queue����������з���һ�ַ�(��head��,����head+=1)
#define PUTCH(c, queue) \
(void)({(queue)->buf[(queue)->head] = (c); INC((queue)->head);})

// �ж��ն˼����ַ�����
#define INTR_CHAR(tty) ((tty)->termios.c_cc[VINTR])             			// �жϷ�.���ж��ź�SIGINT
#define QUIT_CHAR(tty) ((tty)->termios.c_cc[VQUIT])     					// �˳���.���˳��ź�SIGQUIT
#define ERASE_CHAR(tty) ((tty)->termios.c_cc[VERASE])   					// ɾ����.����һ���ַ�
#define KILL_CHAR(tty) ((tty)->termios.c_cc[VKILL])             			// ɾ����.ɾ��һ���ַ�
#define EOF_CHAR(tty) ((tty)->termios.c_cc[VEOF])       					// �ļ�������
#define START_CHAR(tty) ((tty)->termios.c_cc[VSTART])   					// ��ʼ��.�ָ����
#define STOP_CHAR(tty) ((tty)->termios.c_cc[VSTOP])     					// ֹͣ��.ֹͣ���
#define SUSPEND_CHAR(tty) ((tty)->termios.c_cc[VSUSP])  					// �����.�������ź�SIGTSTP

// tty���ݽṹ
struct tty_struct {
	struct termios termios;						// �ն�io���ԺͿ����ַ����ݽṹ.
	int pgrp;									// ����������.
	int session;								// �Ự��.
	int stopped;								// ֹͣ��־.
	void (*write)(struct tty_struct * tty);		// ttyд����ָ��.
	struct tty_queue *read_q;					// tty������.
	struct tty_queue *write_q;					// ttyд����.
	struct tty_queue *secondary;				// tty��������(��Ź淶ģʽ�ַ�����).�ɳ�Ϊ�淶(��)ģʽ����.
	};

extern struct tty_struct tty_table[];			// tty�ṹ����.
extern int fg_console;							// ǰ̨����̨��

// �����ն�������tty_table[]��ȡ��Ӧ�ն˺�nr��tty�ṹָ��.��73�к�벿�����ڸ������豸��dev��tty_table[]����ѡ���Ӧ��tty�ṹ.
// ���dev = 0,��ʾ����ʹ��ǰ̨�ն�,���ֱ��ʹ���ն˺�fg_console��Ϊtty_table[]������ȡtty�ṹ.���dev����0,��ô��Ҫ���������
// ����:1,dev�������ն˺�;2,dev�Ǵ����ն˺Ż���α�ն˺�.���������ն���tty�ṹ��tty_table[] ��������dev-1(0 -- 63).������������
// �ն�,��������tty�ṹ���������dev.���磬���dev = 64����ʾ��һ�������նˣ�����tty�ṹ����tty_table[dev]�����dev = 1,���Ӧ
// �ն˵�tty�ṹ��tty_table[0].
#define TTY_TABLE(nr) \
(tty_table + ((nr) ? (((nr) < 64)? (nr) - 1 : (nr))	: fg_console))

// ����������ն�termios�ṹ�пɸ��ĵ������ַ�����c_cc[]�ĳ�ʼֵ.��termios�ṹ������include/termios.h��.POSIX.1������11��
// �����ַ�,����Linuxϵͳ�����ⶨ����SVR4ʹ�õ�6�������ַ�.���������_POSIX_VDISABLE(\0),��ô��ĳһ��ֵ����_POSIX_VDISABLE
// ��ֵʱ,��ʾ��ֹʹ����Ӧ�������ַ�.[8����ֵ]
/*	intr=^C		quit=^|		erase=del	kill=^U
	eof=^D		vtime=\0	vmin=\1		sxtc=\0
	start=^Q	stop=^S		susp=^Z		eol=\0
	reprint=^R	discard=^U	werase=^W	lnext=^V
	eol2=\0
*/
/*	�ж�intr=^C	�˳�quit=^|	ɾ��erase=del	��ֹkill=^U
	�ļ�����eof=^D	vtime=\0	vmin=\1		sxtc=\0
	��ʼstart=^Q	ֹͣstop=^S	����susp=^Z	�н���eol=\0
	����reprint=^R	����discard=^U	werase=^W	lnext=^V
	�н���eol2=\0
*/
#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

void rs_init(void);     // �첽����ͨ�ų�ʼ������kernel/chr_drv/serial.c��
void con_init(void);	// �����ն˳�ʼ��.(kernel/chr_drv/console.c)
void tty_init(void);	// tty��ʼ��.(kernel/chr_drv/tty_io.c)

int tty_read(unsigned c, char * buf, int n);    // ��kernel/chr_drv/tty_io.c��
int tty_write(unsigned c, char * buf, int n);   // ��kernel/chr_drv/tty_io.c��

void con_write(struct tty_struct * tty);	// (kernel/chr_drv/console.c)
void rs_write(struct tty_struct * tty);         //��kernel/chr_drv/serial.c��
void mpty_write(struct tty_struct * tty);       //��kernel/chr_drv/pty.c��
void spty_write(struct tty_struct * tty);       //��kernel/chr_drv/pty.c��

void copy_to_cooked(struct tty_struct * tty);   //��kernel/chr_drv/tty_io.c��

void update_screen(void);	// (kernel/chr_drv/console.c)

#endif

