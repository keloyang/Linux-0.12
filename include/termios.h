#ifndef _TERMIOS_H
#define _TERMIOS_H

#include <sys/types.h>

#define TTY_BUF_SIZE 1024               // tty�еĻ��������ȡ�

/* 0x54 is just a magic number to make these relatively uniqe ('T') */
/* 0x54 ֻ��һ��ħ����Ŀ����Ϊ��ʹ��Щ����Ψһ��'T'��*/

// tty�豸��ioctl���������ioctl����������ڵ�λ���С�
// ��������TC[*]�ĺ�����tty�������
// ȡ��Ӧ�ն�termios�ṹ�е���Ϣ���μ�tcgetattr()����
#define TCGETS		0x5401
// ������Ӧ�ն�termios�ṹ�е���Ϣ���μ�tcsetattr()��TCSANOW����
#define TCSETS		0x5402
// ��������Ӧ�ն�termios����Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ����꣨�ľ����������޸Ĳ�����Ӱ������������
// ����Ҫʹ��������ʽ���μ�tcsetattr()��TCSADRAINѡ���
#define TCSETSW		0x5403
// ������termios����Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ����꣬����ˢ�£���գ�������У������ã��μ�
// tcsetattr()��TCSAFLUSHѡ���
#define TCSETSF		0x5404
// ȡ��Ӧ�ն�termio�ṹ����Ϣ���μ�tcgetattr()����
#define TCGETA		0x5405
// ������Ӧ�ն�termio�ṹ�е���Ϣ���μ�tcsetattr()��TCSANOWѡ���
#define TCSETA		0x5406
// �������ն�termio����Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ����꣨�ľ����������޸Ĳ�����Ӱ����������������Ҫ
// ʹ��������ʽ���μ�tcsetattr()��TCSADRAINѡ���
#define TCSETAW		0x5407
// ������termio����Ϣ֮ǰ����Ҫ�ȵȴ�����������������ݴ����꣬����ˢ�£���գ�������У������ã��μ�tcsetattr(),
// TCSAFLUSHѡ���
#define TCSETAF		0x5408
// �ȴ�������д�����ϣ��գ���������ֵ��0,����һ��break���μ�tcsendbreak()��tcdrain()����
#define TCSBRK		0x5409
// ��ʼ/ֹͣ���ơ��������ֵ��0,���������������1,�����¿������������������2,��������룻�����3,�����¿�����
// ������루�μ�tcflow()����
#define TCXONC		0x540A
// ˢ����д�������û���ͻ����յ���û�ж����ݡ����������0,��ˢ�£���գ�������У������1,��ˢ��������У������2,
// ��ˢ�������������У��μ�tcflush()����
#define TCFLSH		0x540B
// ��������TIOC[*]�ĺ�����tty��������������
// �����ն˴�����·ר��ģʽ��
#define TIOCEXCL	0x540C
// ��λ�ն˴�����·ר��ģʽ��
#define TIOCNXCL	0x540D
// ����ttyΪ�����նˡ���TIOCNOTTY - ��ֹttyΪ�����նˣ���
#define TIOCSCTTY	0x540E
// ��ȡָ���ն��豸����id���μ�tcgetpgrp()���ó������������ǡ�Terminal IO Control Get PGRP������д����ȡǰ̨����
// ��ID��
#define TIOCGPGRP	0x540F
// ����ָ���ն��豸���̵���id���μ�tcsetpgrp()����
#define TIOCSPGRP	0x5410
// ������������л�δ�ͳ����ַ�����
#define TIOCOUTQ	0x5411
// ģ���ն����롣��������һ��ָ���ַ���ָ����Ϊ����������װ���ַ������ն��ϼ���ġ��û������ڸÿ����ն��Ͼ��г����û�Ȩ��
// ����ж����Ȩ�ޡ�
#define TIOCSTI		0x5412
// ��ȡ�ն��豸���ڴ�С��Ϣ���μ�winsize�ṹ����
#define TIOCGWINSZ	0x5413
// �����ն��豸���ڴ�С��Ϣ���μ�winsize�ṹ����
#define TIOCSWINSZ	0x5414
// ����modem״̬�������ߵĵ�ǰ״̬����λ��־����
#define TIOCMGET	0x5415
// ���õ���modem״̬�������ߵ�״̬��true��false����Individual control line Set����
#define TIOCMBIS	0x5416
// ��λ����modem״̬�������ߵ�״̬��Individual control line clear����
#define TIOCMBIC	0x5417
// ����modem״̬���ߵ�״̬�����ĳһ����λ��λ����modem��Ӧ��״̬���߽���Ϊ��Ч��
#define TIOCMSET	0x5418
// ��ȡ����ز�����־��1 - ������0 - �رգ���
// ���ڱ������ӵ��ն˻��������ã�����ز���־�ǿ����ģ�����ʹ��modem��·���ն˻��豸���ǹرյġ�Ϊ����ʹ��������ioctl���ã�
// tty��·Ӧ������O_NDELAY��ʽ�򿪵ģ�����open()�Ͳ���ȴ��ز���
#define TIOCGSOFTCAR	0x5419
// ��������ز�����־��1 - ������0 - �رգ���
#define TIOCSSOFTCAR	0x541A
// ������������л�δȡ���ַ�����Ŀ��
#define FIONREAD	0x541B
#define TIOCINQ		FIONREAD

// ���ڴ�С��Window size�����Խṹ���ڴ��ڻ����п����ڻ�����Ļ��Ӧ�ó���
// ioctls�е�TIOCGWINSZ��TIOCSWINSZ��������ȡ��������Щ��Ϣ��
struct winsize {
	unsigned short ws_row;          // �����ַ�������
	unsigned short ws_col;          // �����ַ�������
	unsigned short ws_xpixel;       // ���ڿ�ȣ�����ֵ��
	unsigned short ws_ypixel;       // ���ڸ߶ȣ�����ֵ��
};

// AT&TϵͳV��termio�ṹ��
#define NCC 8                           // termio�ṹ�п����ַ�����ĳ��ȡ�
struct termio {
	unsigned short c_iflag;		/* input mode flags */   // ����ģʽ��־��
	unsigned short c_oflag;		/* output mode flags */  // ���ģʽ��־��
	unsigned short c_cflag;		/* control mode flags */ // ����ģʽ��־��
	unsigned short c_lflag;		/* local mode flags */   // ����ģʽ��־��
	unsigned char c_line;		/* line discipline */    // ��·��̣����ʣ���
	unsigned char c_cc[NCC];	/* control characters */ // �����ַ����顣
};

// POSIX��termios�ṹ
#define NCCS 17		// termios�ṹ�п����ַ����鳤��.
struct termios {
	tcflag_t c_iflag;		/* input mode flags */	// ����ģʽ��־
	tcflag_t c_oflag;		/* output mode flags */	// ���ģʽ��־
	tcflag_t c_cflag;		/* control mode flags */	// ����ģʽ��־.
	tcflag_t c_lflag;		/* local mode flags */		// ����ģʽ��־.
	cc_t c_line;			/* line discipline */		// ��·���(����).
	cc_t c_cc[NCCS];		/* control characters */	// �����ַ�����.
};

// �����ǿ����ַ�����c_cc[]���������ֵ.�������ʼ������include/linux/tty.h��.������Ը�����������е�ֵ.���������_POSIX_VDISABLE(\0)
// ��ô������ĳһ��ֵ����_POSIX_VDISABLE��ֵʱ,��ʾ��ֹʹ����������Ӧ�������ַ�.
/* c_cc characters */	/* c_cc�����е��ַ� */
#define VINTR 0		// c_cc[VINTR] = INTR	(^C),\003,�ж��ַ�.
#define VQUIT 1		// c_cc[VQUIT] = QUIT	(^\),\034,�˳��ַ�.
#define VERASE 2	// c_cc[VERASE] = ERASE	(^H),\177,�����ַ�.
#define VKILL 3		// c_cc[VKILL] = KILL	(^U),\025,��ֹ�ַ�(ɾ����).
#define VEOF 4		// c_cc[VEOF] = EOF	(^D),\004,�ļ������ַ�.
#define VTIME 5		// c_cc[VTIME] = TIME	(\0),\0,��ʱ��ֵ.
#define VMIN 6		// c_cc[VMIN] = MIN	(\1),\1,��ʱ��ֵ.
#define VSWTC 7		// c_cc[VSWTC] = SWTC	(\0),\0,�����ַ�.
#define VSTART 8	// c_cc[VSTART] = START	(^Q),\021,��ʼ�ַ�.
#define VSTOP 9		// c_cc[VSTOP] = STOP	(^S),\023,ֹͣ�ַ�.
#define VSUSP 10	// c_cc[VSUSP] = SUSP	(^Z),\032,�����ַ�.
#define VEOL 11		// c_cc[VEOL] = EOL	(\0),\0,�н����ַ�.
#define VREPRINT 12	// c_cc[VREPRINT] = REPRINT	(^$),\022,����ʾ�ַ�.
#define VDISCARD 13	// c_cc[VDISCARD] = DISCARD	(^O),\017,�����ַ�.
#define VWERASE 14	// c_cc[VWERASE] = WERASE	(^W),\027,���ʲ����ַ�.
#define VLNEXT 15	// c_cc[VLNEXT] = LNEXT	(^V),\026,��һ���ַ�.
#define VEOL2 16	// c_cc[VEOL2] = EOL2	(\0),\0,�н����ַ�2.

// termios�ṹ����ģʽ�ֶ�c_iflag���ֱ�־�ķ��ų���.
/* c_iflag bits */	/* c_iflag����λ */
#define IGNBRK	0000001		// ����ʱ����BREAK����
#define BRKINT	0000002		// ��BREAKʱ����SIGINT�ź�
#define IGNPAR	0000004		// ������żУ�������ַ�
#define PARMRK	0000010		// �����żУ���
#define INPCK	0000020		// ����������żУ��
#define ISTRIP	0000040		// �����ַ���8λ
#define INLCR	0000100		// ����ʱ�����з�NLӳ��ɻس���CR
#define IGNCR	0000200		// ���Իس���CR
#define ICRNL	0000400		// ������ʱ���س���ӳ��ɻ��з�NL
#define IUCLC	0001000		// ������ʱ����д�ַ�ת����Сд�ַ�
#define IXON	0002000		// ����ʼ/ֹͣ(XON/XOFF)�������
#define IXANY	0004000		// �����κ��ַ��������
#define IXOFF	0010000		// ����ʼ/ֹͣ(XON/XOFF)�������
#define IMAXBEL	0020000		// ���������ʱ����

// termios�ṹ�����ģʽ�ֶ�c_olag���ֱ�־�ķ��ų���.
/* c_oflag bits */	/* c_oflag����λ */
#define OPOST	0000001		// ִ���������
#define OLCUC	0000002		// �����ʱ��Сд�ַ�ת���ɴ�д�ַ�
#define ONLCR	0000004		// �����ʱ�����з�NLӳ��ɻس�-���з�CR-NL
#define OCRNL	0000010		// �����ʱ���س���CRӳ��ɻ��з�NL
#define ONOCR	0000020		// ��0�в�����س���CR
#define ONLRET	0000040		// ���з�NLִ�лس����Ĺ���
#define OFILL	0000100		// �ӳ�ʱʹ������ַ�����ʹ��ʱ���ӳ�
#define OFDEL	0000200		// ����ַ���ASCII��DEL.���δ����,��ʹ��ASCII NULL
#define NLDLY	0000400		// ѡ�����ӳ�
#define   NL0	0000000		// �����ӳ�����0
#define   NL1	0000400		// �����ӳ�����1
#define CRDLY	0003000		// ѡ��س��ӳ�
#define   CR0	0000000		// �س��ӳ�����0
#define   CR1	0001000		// �س��ӳ�����1
#define   CR2	0002000		// �س��ӳ�����2
#define   CR3	0003000		// �س��ӳ�����3
#define TABDLY	0014000		// ѡ��ˮƽ�Ʊ��ӳ�
#define   TAB0	0000000		// ˮƽ�Ʊ��ӳ�����0
#define   TAB1	0004000		// ˮƽ�Ʊ��ӳ�����1
#define   TAB2	0010000		// ˮƽ�Ʊ��ӳ�����2
#define   TAB3	0014000		// ˮƽ�Ʊ��ӳ�����3
#define   XTABS	0014000		// ���Ʊ��TAB���ɿո�,��ֵ��ʾ�ո���
#define BSDLY	0020000		// ѡ���˸��ӳ�
#define   BS0	0000000		// �˸��ӳ�����0
#define   BS1	0020000		// �˸��ӳ�����1
#define VTDLY	0040000		// �����Ʊ��ӳ�
#define   VT0	0000000		// �����Ʊ��ӳ�����0
#define   VT1	0040000		// �����Ʊ��ӳ�����1
#define FFDLY	0040000		// ѡ��ҳ�ӳ�
#define   FF0	0000000		// ��ҳ�ӳ�����0
#define   FF1	0040000		// ��ҳ�ӳ�����1

// termios�ṹ�п���ģʽ��־�ֶ�c_cflag��־�ķ��ų���(8������).
/* c_cflag bit meaning */	/* c_cflagλ�ĺ��� */
#define CBAUD	0000017		// ��������λ������
#define  B0		0000000		/* hang up */	/* �Ҷ���· */
#define  B50	0000001		// ������ 50
#define  B75	0000002		// ������ 75
#define  B110	0000003		// ������ 110
#define  B134	0000004		// ������ 134
#define  B150	0000005		// ������ 150
#define  B200	0000006		// ������ 200
#define  B300	0000007		// ������ 300
#define  B600	0000010		// ������ 600
#define  B1200	0000011		// ������ 1200
#define  B1800	0000012		// ������ 1800
#define  B2400	0000013		// ������ 2400
#define  B4800	0000014		// ������ 4800
#define  B9600	0000015		// ������ 9600
#define  B19200	0000016		// ������ 19200
#define  B38400	0000017		// ������ 38400
#define  EXTA B19200		// ��չ������A
#define  EXTB B38400		// ��չ������B
#define CSIZE	0000060		// �ַ�λ���������
#define   CS5	0000000		// ÿ�ַ�5λ
#define   CS6	0000020		// ÿ�ַ�6λ
#define   CS7	0000040		// ÿ�ַ�7λ
#define   CS8	0000060		// ÿ�ַ�8λ
#define CSTOPB	0000100		// ��������ֹͣλ,������1��
#define CREAD	0000200		// �������
#define PARENB	0000400		// �������ʱ������żλ,����ʱ������żУ��
#define PARODD	0001000		// ����/���У������У��
#define HUPCL	0002000		// �����̹رչҶ�
#define CLOCAL	0004000		// ���Ե��ƽ����(modem)������·
#define CIBAUD	03600000		/* input baud rate (not used) */	/* ���벨����(δʹ��) */
#define CRTSCTS	020000000000		/* flow control */	/* ������ */

// termios�ṹ�б���ģʽ��־�ֶ�c_lflag�ķ��ų���.
/* c_lflag bits */	/* c_lflagλ */
#define ISIG	0000001		// ���յ��ַ�INTR,QUIT,SUSP��DSUSP,������Ӧ���ź�
#define ICANON	0000002		// �����淶ģʽ(��ģʽ)
#define XCASE	0000004		// ��������ICANON,���ն��Ǵ�д�ַ���
#define ECHO	0000010		// ���������ַ�
#define ECHOE	0000020		// ��������ICANON,��ERASE/WERASE������ǰһ�ַ�/����
#define ECHOK	0000040		// ��������ICANON,��KILL�ַ���������ǰ��
#define ECHONL	0000100		// ��������ICANON,��ʹECHOû�п���Ҳ����NL�ַ�
#define NOFLSH	0000200		// ������SIGINT��SIGOUIT�ź�ʱ��ˢ�������������,������SIGSUSP�ź�ʱ,ˢ���������
#define TOSTOP	0000400		// ����SIGTTOU�źŵ���̨���̵Ľ�����,�ú�̨������ͼд�Լ��Ŀ����ն�.
#define ECHOCTL	0001000		// ��������ECHO,���TAB,NL,START��STOP�����ASCII�����źŽ������Գ���^Xʽ��,Xֵ�ǿ��Ʒ�+0x40
#define ECHOPRT	0002000		// ��������ICANON��IECHO,���ַ��ڲ���ʱ����ʾ
#define ECHOKE	0004000		// ��������ICANON,��KILLͨ���������ϵ������ַ�������
#define FLUSHO	0010000		// �����ˢ��.ͨ������DISCARD�ַ�,�ñ�־����ת
#define PENDIN	0040000		// ����һ���ַ��Ƕ�ʱ,��������е������ַ���������
#define IEXTEN	0100000		// ����ʵ��ʱ��������봦��

/* modem lines */       /* modem��·�źŷ��ų��� */
#define TIOCM_LE	0x001           // ��·����Line Enable����
#define TIOCM_DTR	0x002           // �����ն˾�����Data erminal ready����
#define TIOCM_RTS	0x004           // �����ͣ�Request to Send����
#define TIOCM_ST	0x008           // �������ݷ��ͣ�Serial transfer����
#define TIOCM_SR	0x010           // �������ݽ��գ�Serial Receive����
#define TIOCM_CTS	0x020           // ������ͣ�Clear To Send����
#define TIOCM_CAR	0x040           // �ز���⣨Carrier Detect����
#define TIOCM_RNG	0x080           // ����ָʾ��Ring indicate����
#define TIOCM_DSR	0x100           // �����豸������Data Set Ready����
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RI	TIOCM_RNG

/* tcflow() and TCXONC use these */     /* tcflow()��TCXONCʹ����Щ���� */
#define	TCOOFF		0       // ����������ǡ�Terminal Control Output OFF������д����
#define	TCOON		1       // ����������������
#define	TCIOFF		2       // ϵͳ����һ��STOP�ַ���ʹ�豸ֹͣ��ϵͳ�������ݡ�
#define	TCION		3       // ϵͳ����һ��START�ַ���ʹ�豸��ʼ��ϵͳ�������ݡ�

/* tcflush() and TCFLSH use these */    /* tcflush()��TCFLSHʹ����Щ���ų��� */
#define	TCIFLUSH	0       // ����յ������ݵ�������
#define	TCOFLUSH	1       // ����д�����ݵ������͡�
#define	TCIOFLUSH	2       // ����յ������ݵ�����������д�����ݵ������͡�

/* tcsetattr uses these */      /* tcsetattr()ʹ����Щ���ų��� */
#define	TCSANOW		0       // �ı�����������
#define	TCSADRAIN	1       // �ı���������д�����������֮������
#define	TCSAFLUSH	2       // �ı���������д�����������֮���������н��յ�����û�ж�ȡ������
                                // ������֮������

// ������Щ�����ڱ��뻷���ĺ�����libc.a��ʵ�֣��ں���û�С��ں�����ʵ���У���Щ����ͨ������ϵͳ����ioctl()��ʵ�֡��й�
// ioctl()ϵͳ���ã���μ�fs/ioctl.c����
// ����termios_p��ָtermios�ṹ�еĽ��ղ����ʡ�
extern speed_t cfgetispeed(struct termios *termios_p);
// ����termios_p��ָtermios�ṹ�еķ��Ͳ����ʡ�
extern speed_t cfgetospeed(struct termios *termios_p);
// ��termios_p��ָtermios�ṹ�еĽ��ղ���������Ϊspeed��
extern int cfsetispeed(struct termios *termios_p, speed_t speed);
// ��termios_p��ָtermios�ṹ�еķ��Ͳ���������Ϊspeed��
extern int cfsetospeed(struct termios *termios_p, speed_t speed);
// �ȴ�fildes��ָ������д������ݱ����ͳ�ȥ��
extern int tcdrain(int fildes);
// ����/����fildes��ָ�������ݵĽ��պͷ��͡�
extern int tcflow(int fildes, int action);
// ����fildesָ������������д����û�����Լ��������յ�����û�ж�ȡ�����ݡ�
extern int tcflush(int fildes, int queue_selector);
// ��ȡ����fildes��Ӧ����Ĳ����������䱣����termios_p��ָ�صط���
extern int tcgetattr(int fildes, struct termios *termios_p);
// ����ն�ʹ���첽�������ݴ��䣬����һ��ʱ������������һϵ��0ֵ����λ��
extern int tcsendbreak(int fildes, int duration);
// ʹ��termios�ṹָ��termios_p��ָ�����ݣ��������ն���صĲ�����
extern int tcsetattr(int fildes, int optional_actions,
	struct termios *termios_p);

#endif

