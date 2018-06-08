/*
 *  linux/kernel/chr_drv/tty_ioctl.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <termios.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/tty.h>

#include <asm/io.h>
#include <asm/segment.h>
#include <asm/system.h>

// ���ݽ������pgrpȡ�ý����������ĻỰ�š�������kernel/exit.c��
extern int session_of_pgrp(int pgrp);
// ��ʹ��ָ��tty�ն˵Ľ����������н��̷����źš�������chr_drv/tty_io.c��
extern int tty_signal(int sig, struct tty_struct *tty);

// ���ǲ������������飨���Ϊ�������飩���������벨�������ӵĶ�Ӧ��ϵ�μ��б��˵����
// ���粨������2400bit/sʱ����Ӧ��������48��0x300����9600bit/s��������12��0x1c����
static unsigned short quotient[] = {
	0, 2304, 1536, 1047, 857,
	768, 576, 384, 192, 96,
	64, 48, 24, 12, 6, 3
};

// �޸Ĵ��䲨���ʡ�
// ������tty - �ն˶�Ӧ��tty���ݽṹ��
// �ڳ��������־DLAB��λ����£�ͨ���˿�0x3f8��0x3f9��UART�ֱ�д�벨�������ӵ��ֽں͸��ֽڡ�д����ٸ�λDLAB
// λ�����ڴ���2,�������˿ڷֱ���0x2f8��0x2f9��
static void change_speed(struct tty_struct * tty)
{
	unsigned short port,quot;

	// �������ȼ�����ttyָ�����ն��Ƿ��Ǵ����նˣ����������˳������ڴ����ն˵�tty�ṹ������������data�ֶδ����
	// ���ж˿ڻ�ַ��0x3f8��0x2f8������һ�����̨�ն˵�tty�ṹ��read_q.data�ֶ�ֵΪ0��Ȼ����ն�termios�ṹ�Ŀ���
	// ģʽ��־����ȡ�������õĲ����������ţ����ݴ˴Ӳ�������������quotient[]��ȡ�ö�Ӧ�Ĳ���������ֵquot��CBAUD��
	// ����ģʽ��־���в�����λ�����롣
	if (!(port = tty->read_q->data))
		return;
	quot = quotient[tty->termios.c_cflag & CBAUD];
	// ���ŰѲ���������quotд�봮�ж˿ڶ�ӦUARTоƬ�Ĳ����������������С���д֮ǰ������Ҫ����·���ƼĴ���LCR�ĳ�������
	// ����λDLAB��λ7����1��Ȼ���16λ�Ĳ��������ӵ͡����ֽڷֱ�д��˿�0x3f8��0x3f9���ֱ��Ӧ���������ӵ͡����ֽ�
	// ��������������ٸ�λLCR��DLAB��־λ��
	cli();
	outb_p(0x80, port + 3);									/* set DLAB */          // �������ó���������־DLAB��
	outb_p(quot & 0xff, port);								/* LS of divisor */     // ������ӵ��ֽڡ�
	outb_p(quot >> 8, port + 1);							/* MS of divisor */     // ������Ӹ��ֽڡ�
	outb(0x03, port + 3);									/* reset DLAB */        // ��λDLAB��
	sti();
}

// ˢ��tty������С�
// ������queue - ָ���Ļ������ָ�롣
// �����е�ͷָ�����βָ�룬�Ӷ��ﵽ��ջ�������Ŀ�ġ�
static void flush(struct tty_queue * queue)
{
	cli();
	queue->head = queue->tail;
	sti();
}

// �ȴ��ַ����ͳ�ȥ��
static void wait_until_sent(struct tty_struct * tty)
{
	/* do nothing - not implemented */      /* ʲô��û�� - ��δʵ�� */
}

// ����BREAK���Ʒ���
static void send_break(struct tty_struct * tty)
{
	/* do nothing - not implemented */      /* ʲô��û�� - ��δʵ�� */
}

// ȡ�ն�termios�ṹ��Ϣ��
// ������tty - ָ���ն˵�tty�ṹָ�룻termios - ���termios�ṹ���û���������
static int get_termios(struct tty_struct * tty, struct termios * termios)
{
	int i;

	// ������֤�û�������ָ����ָ�ڴ��������Ƿ��㹻���粻��������ڴ档Ȼ����ָ���ն˵�termios�ṹ��Ϣ���û��������С�
	// ��󷵻�0.
	verify_area(termios, sizeof (*termios));
	for (i = 0 ; i < (sizeof (*termios)) ; i++)
		put_fs_byte( ((char *) & tty->termios)[i] , i + (char *)termios );
	return 0;
}

// �����ն�termios�ṹ��Ϣ��
// ������tty - ָ���ն˵�tty�ṹָ�룻termios - �û�������termios�ṹָ�롣
static int set_termios(struct tty_struct * tty, struct termios * termios,
			int channel)
{
	int i, retsig;

	/* If we try to set the state of terminal and we're not in the
	   foreground, send a SIGTTOU.  If the signal is blocked or
	   ignored, go ahead and perform the operation.  POSIX 7.2) */
    /*
     * �����ͼ�����ն˵�״̬����ʱ�ն˲���ǰ̨����ô���Ǿ���Ҫ����һ��SIGTTOU
     * �źš�������źű��������λ��ߺ����ˣ���ֱ��ִ�б��β�����POSIX 7.2 */
	// �����ǰ����ʹ�õ�tty�ն˵Ľ����������̵Ľ�����Ų�ͬ������ǰ�����ն˲���ǰ̨����ʾ��ǰ������ͼ�޸Ĳ��ܿ��Ƶ��ն�
	// ��termios�ṹ����˸���POSIX��׼��Ҫ��������Ҫ����SIGTTOU�ź���ʹ������ն˵Ľ�����ʱִֹͣ�У����������޸�termios
	// �ṹ����������źź���tty_signal()����ֵ��ERESTARTSYS��EINTR�����һ�����ִ�б��β�����
	if ((current->tty == channel) && (tty->pgrp != current->pgrp)) {
		retsig = tty_signal(SIGTTOU, tty);
		if (retsig == -ERESTARTSYS || retsig == -EINTR)
			return retsig;
	}
	// ���Ű��û���������termios�ṹ��Ϣ���Ƶ�ָ���ն�tty�ṹ��termios�ṹ�С���Ϊ�û��п������޸����ն˴��пڴ��䲨���ʣ�
	// ���������ٸ���termios�ṹ�еĿ���ģʽ��־c_cflag�еĲ�������Ϣ�޸Ĵ���UARTоƬ�ڵĴ��䲨���ʡ���󷵻�0��
	for (i = 0 ; i < (sizeof (*termios)) ; i++)
		((char *) & tty->termios)[i] = get_fs_byte(i + (char *)termios);
	change_speed(tty);
	return 0;
}

// ��ȡtermio�ṹ�е���Ϣ��
// ������tty - ָ���ն˵�tty�ṹָ�룻termio - ����termio�ṹ��Ϣ���û���������
static int get_termio(struct tty_struct * tty, struct termio * termio)
{
	int i;
	struct termio tmp_termio;

	// ������֤�û��Ļ�����ָ����ָ�ڴ��������Ƿ��㹻���粻��������ڴ档Ȼ��termios�ṹ����Ϣ���Ƶ���ʱtermio�ṹ�У�
	// �������ṹ������ͬ�����롢��������ƺͱ��ر�־���������Ͳ�ͬ��ǰ�ߵ���long�������ߵ���short������ȸ��Ƶ���ʱ
	// termio�ṹ��Ŀ����Ϊ�˽�����������ת����
	verify_area(termio, sizeof (*termio));
	tmp_termio.c_iflag = tty->termios.c_iflag;
	tmp_termio.c_oflag = tty->termios.c_oflag;
	tmp_termio.c_cflag = tty->termios.c_cflag;
	tmp_termio.c_lflag = tty->termios.c_lflag;
	tmp_termio.c_line = tty->termios.c_line;
	for(i = 0 ; i < NCC ; i++)
		tmp_termio.c_cc[i] = tty->termios.c_cc[i];
	// Ȼ�����ֽڵذ���ʱtermio�ṹ�е���Ϣ���Ƶ��û�termio�ṹ�������С�������0��
	for (i = 0 ; i < (sizeof (*termio)) ; i++)
		put_fs_byte( ((char *) & tmp_termio)[i] , i + (char *)termio );
	return 0;
}

/*
 * This only works as the 386 is low-byt-first
 */
/*
 * ����termio���ú����������ڵ��ֽ���ǰ��386CPU��
 */
// �����ն�termio�ṹ��Ϣ��
// ������tty - ָ���ն˵�tty�ṹָ�룻termio - �û���������termio�ṹ��
// ���û�������termio����Ϣ���Ƶ��ն˵�termios�ṹ�С�����0��
static int set_termio(struct tty_struct * tty, struct termio * termio,
			int channel)
{
	int i, retsig;
	struct termio tmp_termio;

	// ��set_termios()һ�����������ʹ�õ��ն˵Ľ�����ŵĽ����������̵Ľ�����Ų�ͬ������ǰ�����ն˲���ǰ̨����ʾ��ǰ
	// ������ͼ�޸Ĳ��ܿ��Ƶ��ն˵�termios�ṹ����˸���POSIX��׼��Ҫ��������Ҫ����SIGTTOU�ź���ʹ������ն˵Ľ�������
	// ʱִֹͣ�У������������޸�termios�ṹ����������źź���tty_signal()����ֵ��ERESTARTSYS��EINTR�����һ����ִ��
	// ���β�����
	if ((current->tty == channel) && (tty->pgrp != current->pgrp)) {
		retsig = tty_signal(SIGTTOU, tty);
		if (retsig == -ERESTARTSYS || retsig == -EINTR)
			return retsig;
	}
	// ���Ÿ����û���������termio�ṹ��Ϣ����ʱtermio�ṹ�С�Ȼ���ٽ�termio�ṹ����Ϣ���Ƶ�tty��termios�ṹ�С�������
	// ��Ŀ����Ϊ�˶�����ģʽ��־�������ͽ���ת��������termio�Ķ���������ת����termios�ĳ��������͡������ֽṹ��c_line��
	// c_cc[]�ֶ�����ȫ��ͬ�ġ�
	for (i = 0 ; i< (sizeof (*termio)) ; i++)
		((char *)&tmp_termio)[i] = get_fs_byte(i + (char *)termio);
	*(unsigned short *)&tty->termios.c_iflag = tmp_termio.c_iflag;
	*(unsigned short *)&tty->termios.c_oflag = tmp_termio.c_oflag;
	*(unsigned short *)&tty->termios.c_cflag = tmp_termio.c_cflag;
	*(unsigned short *)&tty->termios.c_lflag = tmp_termio.c_lflag;
	tty->termios.c_line = tmp_termio.c_line;
	for(i = 0 ; i < NCC ; i++)
		tty->termios.c_cc[i] = tmp_termio.c_cc[i];
	// �����Ϊ�û��п������޸����ն˴��пڴ��䲨���ʣ����������ٸ���termios�ṹ�еĿ���ģʽ��־c_cflag�еĲ�������Ϣ�޸�
	// ����UARTоƬ�ڵĴ��䲨���ʣ�������0��
	change_speed(tty);
	return 0;
}

// tty�ն��豸����������ƺ�����
// ������dev - �豸�ţ�cmd - ioctl���arg - ��������ָ�롣
// �ú������ȸ��ݲ����������豸���ҳ���Ӧ�ն˵�tty�ṹ��Ȼ����ݿ�������cmd�ֱ���д���
int tty_ioctl(int dev, int cmd, int arg)
{
	struct tty_struct * tty;
	int	pgrp;

	// ���ȸ����豸��ȡ��tty���豸�ţ��Ӷ�ȡ���ն˵�tty�ṹ�������豸����5�������նˣ�������̵�tty�ֶμ���tty���豸
	// �š���ʱ������̵�tty���豸���Ǹ����������ý���û�п����նˣ������ܷ�����ioctl���ã�������ʾ������Ϣ��ͣ�������
	// ���豸�Ų���5����4,���ǾͿ��Դ��豸����ȡ�����豸�š����豸�ſ�����0������̨�նˣ���1������1�նˣ���2������2�նˣ���
	if (MAJOR(dev) == 5) {
		dev = current->tty;
		if (dev < 0)
			panic("tty_ioctl: dev<0");
	} else
		dev = MINOR(dev);
	// Ȼ��������豸�ź�tty�����ǿ���ȡ�ö�Ӧ�ն˵�tty�ṹ��������ttyָ���Ӧ���豸�ŵ�tty�ṹ��Ȼ���ٸ��ݲ����ṩ��
	// ioctl����cmd���зֱ���144�к�벿�����ڸ������豸��dev��tty_table[]����ѡ���Ӧ��tty�ṹ�����dev = 0����ʾ
	// ����ʹ��ǰ̨�նˣ����ֱ��ʹ���ն˺�fg_console��Ϊtty_table[]������ȡtty�ṹ�����dev����0,��ô��Ҫ������������ǣ�
	// 1��dev�������ն˺ţ�2��dev�Ǵ����ն˺Ż���α�ն˺š����������ն���tty�ṹ��tty_table[]����������dev-1��0--63����
	// �������������նˣ������ǵ�tty�ṹ���������dev�����磬���dev = 64����ʾ��һ�������ն�1,����tty�ṹ����tty_table[dev]
	// ���dev = 1�����Ӧ�ն˵�tty�ṹ��tty_table[0]��
	tty = tty_table + (dev ? ((dev < 64)? dev - 1 : dev) : fg_console);
	switch (cmd) {
		// ȡ��Ӧ�ն�termios�ṹ��Ϣ����ʱ����arg���û�������ָ�롣
		case TCGETS:
			return get_termios(tty, (struct termios *) arg);
		// ������termios�ṹ��Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ�����ϣ�����ˢ�£���գ�������С��ٽ���ִ������������ն�termios
		// �ṹ�Ĳ�����
		case TCSETSF:
			flush(tty->read_q); 							/* fallthrough */
		// �������ն�termios����Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ����꣨�ľ����������޸Ĳ�����Ӱ����������������Ҫʹ��������ʽ��
		case TCSETSW:
			wait_until_sent(tty); 							/* fallthrough */
		// ������Ӧ�ն�termios�ṹ��Ϣ����ʱ����arg�Ǳ���termios�ṹ���û�������ָ�롣
		case TCSETS:
			return set_termios(tty,(struct termios *) arg, dev);
		// ȡ��Ӧ�ն�termio�ṹ�е���Ϣ����ʱ����arg���û�������ָ�롣
		case TCGETA:
			return get_termio(tty,(struct termio *) arg);
		// ������termio�ṹ��Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ�����ϣ�����ˢ�£���գ�������С��ٽ���ִ������������ն�termio
		// �ṹ�Ĳ�����
		case TCSETAF:
			flush(tty->read_q); 							/* fallthrough */
		// �������ն�termios����Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ����꣨�ľ����������޸Ĳ�����Ӱ����������������Ҫʹ��������ʽ��
		case TCSETAW:
			wait_until_sent(tty); 							/* fallthrough */
		// ������Ӧ�ն�termio�ṹ��Ϣ����ʱ����arg�Ǳ���termio�ṹ���û�������ָ�롣
		case TCSETA:
			return set_termio(tty,(struct termio *) arg, dev);
		// �������argֵ��0����ȴ�������д�����ϣ��գ���������һ��break��
		case TCSBRK:
			if (!arg) {
				wait_until_sent(tty);
				send_break(tty);
			}
			return 0;
		// ��ʼ/ֹͣ�����ơ��������arg��TCOOFF��Terminal Control Output OFF�������������������TCOON����ָ������������ڹ�
		// ���ָ����ͬʱ��Ҫ��д�����е��ַ�������Լӿ��û�������Ӧ�ٶȡ����arg��TCIOFF��Terminal Control Input ON���������
		// ���룻�����TCION�������¿�����������롣
		case TCXONC:
			switch (arg) {
			case TCOOFF:
				tty->stopped = 1;       					// ֹͣ�ն������
				tty->write(tty);        					// д������������
				return 0;
			case TCOON:
				tty->stopped = 0;       					// �ָ��ն������
				tty->write(tty);
				return 0;
			// �������arg��TCIOFF����ʾҪ���ն�ֹͣ���룬�����������ն�д���з���STOP�ַ������ն��յ����ַ�ʱ�ͻ���ͣ���롣���������
			// TCION����ʾ����һ��START�ַ������ն˻ָ����䡣STOP_CHAR(tty)����Ϊ((tty)->termios.c_cc[VSTOP])����ȡ�ն�termios
			// �ṹ�����ַ������Ӧ��ֵ�����ں˶�����_POSIX_VDISABLE(\0)����ô��ĳһ�����_POSIX_VDISABLE��ֵʱ����ʾ��ֹʹ����Ӧ��
			// �����ַ����������ֱ���жϸ�ֵ�Ƿ�Ϊ0��ȷ��Ҫ��Ҫ��ֹͣ�����ַ������ն�д�����С�����ͬ��
			case TCIOFF:
				if (STOP_CHAR(tty))
					PUTCH(STOP_CHAR(tty), tty->write_q);
				return 0;
			case TCION:
				if (START_CHAR(tty))
					PUTCH(START_CHAR(tty), tty->write_q);
				return 0;
			}
			return -EINVAL; 								/* not implemented */
		// ˢ����д�������û�з��͡����ѽ��յ���û�ж������ݡ��������arg��0����ˢ�£���գ�������У������1����ˢ��������У����
		// 2����ˢ�������������С�
		case TCFLSH:
			if (arg == 0)
				flush(tty->read_q);
			else if (arg == 1)
				flush(tty->write_q);
			else if (arg == 2) {
				flush(tty->read_q);
				flush(tty->write_q);
			} else
				return -EINVAL;
			return 0;
		// �����ն˴�����·ר��ģʽ��
		case TIOCEXCL:
			return -EINVAL; 							/* not implemented */   /* δʵ�� */
		// ��λ�ն˴�����·ר��ģʽ��
		case TIOCNXCL:
			return -EINVAL; 							/* not implemented */
		// ����ttyΪ�����նˡ���TIOCNOTTY - ��Ҫ�����նˣ���
		case TIOCSCTTY:
			return -EINVAL; 							/* set controlling term NI */
		// ��ȡ�ն˽�����ţ�����ȡǰ̨������ţ���������֤�û����������ȣ�Ȼ�����ն�tty��pgrp�ֶε��û�����������ʱ����arg���û�
		// ������ָ�롣
		case TIOCGPGRP:
			verify_area((void *) arg, 4);            	// ʵ�ֿ⺯��tcgetpgrp()��
			put_fs_long(tty->pgrp, (unsigned long *) arg);
			return 0;
		// �����ն˽������pgrp��������ǰ̨������ţ�����ʱ����arg���û��������н������pgrp��ָ�롣ִ�и������ǰ�������ǽ��̱���
		// �п����նˡ������ǰ����û�п����նˣ�����dev����������նˣ����߿����ն����ڵ�ȷ�����ڴ�����ն�dev�������̵ĻỰ�����
		// �ն�dev�ĻỰ�Ų�ͬ���򷵻����ն˴�����Ϣ��
		case TIOCSPGRP:                                 // ʵ�ֿ⺯��tcsetpgrp()��
			if ((current->tty < 0) ||
			    (current->tty != dev) ||
			    (tty->session != current->session))
				return -ENOTTY;
			// Ȼ�����Ǿʹ��û���������ȡ�������õĽ��̺ţ����Ը���ŵ���Ч�Խ�����֤��������pgrpС��0,�򷵻���Ч��Ŵ�����Ϣ�����pgrp
			// �ĻỰ���뵱ǰ���̵Ĳ�ͬ���򷵻���ɴ�����Ϣ���������ǿ��������ն˽������Ϊpgrp����ʱpgrp��Ϊǰ̨�����顣
			pgrp = get_fs_long((unsigned long *) arg);
			if (pgrp < 0)
				return -EINVAL;
			if (session_of_pgrp(pgrp) != current->session)
				return -EPERM;
			tty->pgrp = pgrp;
			return 0;
		// ������������л�δ�ͳ����ַ�����������֤�û����������ȣ�Ȼ���ƶ������ַ������û�����ʱ����arg���û�������ָ�롣
		case TIOCOUTQ:
			verify_area((void *) arg, 4);
			put_fs_long(CHARS(tty->write_q), (unsigned long *) arg);
			return 0;
		// ������������л�δ��ȡ���ַ�����������֤�û����������ȣ�Ȼ���ƶ������ַ������û�����ʱ����arg���û�������ָ�롣
		case TIOCINQ:
			verify_area((void *) arg, 4);
			put_fs_long(CHARS(tty->secondary),
				(unsigned long *) arg);
			return 0;
		// ģ���ն������������������һ��ָ���ַ���ָ����Ϊ��������������ַ������ն��ϼ���ġ��û������ڸÿ����ն��Ͼ��г���
		// �û�Ȩ�޻���ж����Ȩ�ޡ�
		case TIOCSTI:
			return -EINVAL; 							/* not implemented */
		// ��ȡ�ն��豸���ڴ�С��Ϣ���μ�termios.h�е�winsize�ṹ����
		case TIOCGWINSZ:
			return -EINVAL; 							/* not implemented */
		// �����ն��豸���ڴ�С��Ϣ���μ�winsize�ṹ����
		case TIOCSWINSZ:
			return -EINVAL; 							/* not implemented */
		// ����MODEM״̬�������ߵĵ�ǰ״̬λ��־�����μ�termios.h����
		case TIOCMGET:
			return -EINVAL; 							/* not implemented */
		// ���õ���modem״̬�������ߵ�״̬��true��false����
		case TIOCMBIS:
			return -EINVAL; 							/* not implemented */
		// ��λujwhMODEM״̬�������ߵ�״̬��
		case TIOCMBIC:
			return -EINVAL; 							/* not implemented */
		// ����MODEM״̬���ߵ�״̬�����ĳһλ��λ����modem��Ӧ��״̬���߽�Ϊ��Ч��
		case TIOCMSET:
			return -EINVAL; 							/* not implemented */
		// ��ȡ����ز�����־��1 - ������0 - �رգ���
		case TIOCGSOFTCAR:
			return -EINVAL; 							/* not implemented */
		// ��������ز�����־��1 - ������0 - �رգ���
		case TIOCSSOFTCAR:
			return -EINVAL; 							/* not implemented */
		default:
			return -EINVAL;
        }
}

