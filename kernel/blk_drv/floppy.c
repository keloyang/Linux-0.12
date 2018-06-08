/*
 *  linux/kernel/floppy.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 02.12.91 - Changed to static variables to indicate need for reset
 * and recalibrate. This makes some things easier (output_byte reset
 * checking etc), and means less interrupt jumping in case of errors,
 * so the code is hopefully easier to understand.
 */
/*
 * 02.12.91 - �޸ĳɾ�̬����,����Ӧ��λ������У������.��ʹ��ĳЩ������������Ϊ����(output_byte��λ����),������ζ���ڳ���
 * ʱ�ж���תҪ��һЩ,����Ҳϣ�������ܸ����ױ����.
 */

/*
 * This file is certainly a mess. I've tried my best to get it working,
 * but I don't like programming floppies, and I have only one anyway.
 * Urgel. I should check for more errors, and do more graceful error
 * recovery. Seems there are problems with several drives. I've tried to
 * correct them. No promises.
 */
/*
 * ����ļ���Ȼ�Ƚϻ���.���Ѿ���������ʹ���ܹ�����,���Ҳ�ϲ���������,������Ҳֻ��һ������.����,��Ӧ��������Ĳ����,�Լ�����
 * ����Ĵ���.����ĳЩ����������,��������񻹴���һЩ����.���Ѿ������Ž��о�����,�����ܱ�֤��������ʧ.
 */

/*
 * As with hd.c, all routines within this file can (and will) be called
 * by interrupts, so extreme caution is needed. A hardware interrupt
 * handler may not sleep, or a kernel panic will happen. Thus I cannot
 * call "floppy-on" directly, but have to set a special timer interrupt
 * etc.
 *
 * Also, I'm not certain this works on more than 1 floppy. Bugs may
 * abund.
 */
/*
 * ��ͬhd.c�ļ�һ��,���ļ��е������ӳ����ܹ����жϵ���,������Ҫ�ر��С��.Ӳ���жϴ�������ǲ���˯�ߵ�,�����ں˾ͻ�ɵ��(����).
 * ��˲���ֱ�ӵ���"floppy-on",��ֻ������һ������Ķ�ʱ�жϵ�.
 * ����,�Ҳ��ܱ�֤�ó������ڶ���1������ϵͳ�Ϲ���,�п��ܴ��ڴ���.
 */

#include <linux/sched.h>								// ���Գ���ͷ�ļ�,����������ṹtask_struct,����0���ݵ�.
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

// �����������豸�ŷ��ų���.������������,���豸�ű����ڰ���blk.h�ļ�֮ǰ������.
// ��Ϊblk.h�ļ���Ҫ�����������ֵ��ȷ��һЩ��������ط��ų����ͺ�.
#define MAJOR_NR 2										// ���������豸����2.
#include "blk.h"										// ���豸ͷ�ļ�.��������ṹ,���豸�ṹ�ͺ꺯������Ϣ.

static int recalibrate = 0;								// ��־:1��ʾ��Ҫ����У����ͷλ��(��ͷ�����).
static int reset = 0;									// ��־:1��ʾ��Ҫ���и�λ����.
static int seek = 0;									// ��־:1��ʾִ��Ѱ������.

// ��ǰ��������Ĵ���DOR(Digital Output Register),������kernel/sched.c.�ñ����������������е���Ҫ��־,����ѡ������,���Ƶ������,
// ������λ���̿������Լ�����/��ֹDMA���ж�����.
extern unsigned char current_DOR;

// �ֽ�ֱ�����(Ƕ�����).��ֵval�����port�˿�.
#define immoutb_p(val, port) \
__asm__("outb %0,%1\n\tjmp 1f\n1:\tjmp 1f\n1:"::"a" ((char) (val)), "i" (port))

// �������궨�����ڼ����������豸��.
#define TYPE(x) ((x) >> 2)								// ��������(2--1.2Mb,7--1.44Mb).
#define DRIVE(x) ((x) & 0x03)							// �������(0--3��ӦA--D)

/*
 * Note that MAX_ERRORS=8 doesn't imply that we retry every bad read
 * max 8 times - some types of errors increase the errorcount by 2,
 * so we might actually retry only 5-6 times before giving up.
 */
/*
 * ע��,���涨��MAX_ERRORS=8������ʾ��ÿ�ζ����������8�� - ��Щ���͵Ĵ����ѳ������ֵ��2,��������ʵ�����ڷ�������֮ǰֻ��
 * ����5-6�鼴��.
 */
#define MAX_ERRORS 8

/*
 * globals used by 'result()'
 */
/* �����Ǻ���'result()'�õ�ȫ�ֱ��� */
// ��Щ״̬�ֽ��и�λ�ĺ���μ�include/linux/fdreg.hͷ�ļ�.
#define MAX_REPLIES 7								// FDC��෵��7�ֽڵĽ����Ϣ.
static unsigned char reply_buffer[MAX_REPLIES];		// ���FDC���ص�Ӧ������Ϣ.
#define ST0 (reply_buffer[0])						// ���״̬�ֽ�0.
#define ST1 (reply_buffer[1])						// ���״̬�ֽ�1.
#define ST2 (reply_buffer[2])						// ���״̬�ֽ�2.
#define ST3 (reply_buffer[3])						// ���״̬�ֽ�3.

/*
 * This struct defines the different floppy types. Unlike minix
 * linux doesn't have a "search for right type"-type, as the code
 * for that is convoluted and weird. I've got enough problems with
 * this driver as it is.
 *
 * The 'stretch' tells if the tracks need to be boubled for some
 * types (ie 360kB diskette in 1.2MB drive etc). Others should
 * be self-explanatory.
 */
/*
 * ��������̽ṹ�����˲�ͬ����������.��minix��ͬ����,Linuxû��"������ȷ������"-����,��Ϊ���䴦��Ĵ������˷ѽ�
 * �ҹֵֹ�.�������Ѿ���������̫���������.
 *
 * ��ĳЩ���͵�����(������1.2MB�������е�360KB���̵�),"stretch"���ڼ��ŵ��Ƿ���Ҫ���⴦��.
 */
// �������̽ṹ.���̲�����:
// size		��С(������);
// sect		ÿ�ŵ�������;
// head		��ͷ��;
// track	�ŵ���;
// stretch	�Դŵ��Ƿ�Ҫ���⴦��(��־);
// gap		������϶����(�ֽ���);
// rate		���ݴ�������;
// spec1	����(��4λ��������,����λ��ͷж��ʱ��).
static struct floppy_struct {
	unsigned int size;						// ��С(������)
	unsigned int sect;						// ÿ�ŵ�������
	unsigned int head;						// ��ͷ��
	unsigned int track;						// �ŵ���
	unsigned int stretch;					// �Դŵ��Ƿ�Ҫ���⴦��(��־)
	unsigned char gap;						// ������϶����(�ֽ���)
	unsigned char rate;						// ���ݴ�������
	unsigned char spec1;					// ����(��4λ��������,����λ��ͷж��ʱ��)
} floppy_type[] = {
	{    0, 0,0, 0,0,0x00,0x00,0x00 },	/* no testing */
	{  720, 9,2,40,0,0x2A,0x02,0xDF },	/* 360kB PC diskettes */
	{ 2400,15,2,80,0,0x1B,0x00,0xDF },	/* 1.2 MB AT-diskettes */
	{  720, 9,2,40,1,0x2A,0x02,0xDF },	/* 360kB in 720kB drive */
	{ 1440, 9,2,80,0,0x2A,0x02,0xDF },	/* 3.5" 720kB diskette */
	{  720, 9,2,40,1,0x23,0x01,0xDF },	/* 360kB in 1.2MB drive */
	{ 1440, 9,2,80,0,0x23,0x01,0xDF },	/* 720kB in 1.2MB drive */
	{ 2880,18,2,80,0,0x1B,0x00,0xCF },	/* 1.44MB diskette */
};

/*
 * Rate is 0 for 500kb/s, 2 for 300kbps, 1 for 250kbps
 * Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc),
 * H is head unload time (1=16ms, 2=32ms, etc)
 *
 * Spec2 is (HLD<<1 | ND), where HLD is head load time (1=2ms, 2=4 ms etc)
 * and ND is set means no DMA. Hardcoded to 6 (HLD=6ms, use DMA).
 */
/*
 * ��������rate: 0��ʾ500kbps,1��ʾ300kbps,2��ʾ250kbps.
 * ����spec1��0xSH,����S�ǲ�������(F-1ms,E-2ms,D=3ms��),H�Ǵ�ͷж��ʱ��(1=16ms,2=32ms��)
 *
 * spec2��(HLD<<1 | ND),����HLD�Ǵ�ͷ����ʱ��(1=2ms,2=4ms��)
 * ND��λ��ʾ��ʹ��DMA(No DMA),�ڳ�����Ӳ�����6(HLD=6ms,ʹ��DMA).
 */
// ע��,������ͷ����ʱ�����дHLD���д�ɱ�׼��HLT(Head Load Time).

// floppy_interrupt()��sys_call.s�����������жϴ�����̱��.���ｫ�����̳�ʼ������floppy_init()ʹ������ʼ���ж�������������.
extern void floppy_interrupt(void);
// ��ʱboot/head.s���������ʱ���̻�����.���������Ļ����������ڴ�1MB����ĳ���ط�,����Ҫ��DMA������������ʱ��������.��Ϊ
// 8237AоƬֻ����1MB��ַ��Χ��Ѱַ.
extern char tmp_floppy_area[1024];

/*
 * These are global variables, as that's the easiest way to give
 * information to interrupts. They are the data used for the current
 * request.
 */
/*
 * ������һЩȫ�ֱ���,��Ϊ���ǽ���Ϣ�����жϳ����˼򵥵ķ�ʽ.�������ڵ�ǰ�����������.
 */
// ��Щ��ν��"ȫ�ֱ���"��ָ�������жϴ�������е��õ�C����ʹ�õı���.��Ȼ��ЩC�������ڱ�������.
static int cur_spec1 = -1;									// ��ǰ���̲���spec1.
static int cur_rate = -1;									// ��ǰ����ת��rate.
static struct floppy_struct * floppy = floppy_type;			// �������ͽṹ����ָ��.
static unsigned char current_drive = 0;						// ��ǰ��������.
static unsigned char sector = 0;							// ��ǰ������.
static unsigned char head = 0;								// ��ǰ��ͷ��.
static unsigned char track = 0;								// ��ǰ�ŵ���.
static unsigned char seek_track = 0;						// Ѱ���ŵ���.
static unsigned char current_track = 255;					// ��ǰ��ͷ���ڴŵ���.
static unsigned char command = 0;							// ��/д����.
unsigned char selected = 0;									// ������ѡ����־.�ڴ���������֮ǰҪ����ѡ������.
struct task_struct * wait_on_floppy_select = NULL;			// �ȴ�ѡ���������������.

// ȡ��ѡ������.
// �����������ָ��������nr��ǰ��û�б�ѡ��,����ʾ������Ϣ.Ȼ��λ������ѡ����־selected,�����ѵȴ�ѡ�������������.�������
// �Ĵ���(DOR)�ĵ�2λ����ָ��ѡ�������(0-3��ӦA-D).
void floppy_deselect(unsigned int nr)
{
	if (nr != (current_DOR & 3))
		printk("floppy_deselect: drive not selected\n\r");
	selected = 0;											// ��λ������ѡ����־.
	wake_up(&wait_on_floppy_select);						// ���ѵȴ�������.
}

/*
 * floppy-change is never called from an interrupt, so we can relax a bit
 * here, sleep etc. Note that floppy-on tries to set current_DOR to point
 * to the desired drive, but it will probably not survive the sleep if
 * several floppies are used at the same time: thus the loop.
 */
/*
 * floppy-change()���Ǵ��жϳ����е��õ�,�����������ǿ�������һ��,˯�ߵ�.
 * ע��floppy-on()�᳢������current_DORָ�������������,����ͬʱʹ�ü�������ʱ����˯��:��˴�ʱֻ��ʹ��ѭ����ʽ.
 */
// ���ָ�����������̸������.
// ����nr��������.������̸������򷵻�1,���򷵻�0.
// �ú�������ѡ������ָ��������nr,Ȼ��������̿���������������Ĵ���DIR��ֵ,���ж��������е������Ƿ񱻸�����.�ú����ɳ���
// fs/buffer.c�е�check_disk_change()��������.
int floppy_change(unsigned int nr)
{
	// ����Ҫ��������������ת�������ﵽ��������ת��.����Ҫ����һ��ʱ��.���õķ���������kernel/sched.c�����̶�ʱ����do_floppy_timer()
	// ����һ������ʱ����.floppy_on()�����������ж���ʱ�Ƿ�(mon_timer[nr]==0?),��û�е����õ�ǰ���̼���˯�ߵȴ�.����ʱ����
	// do_floppy_timer()�ỽ�ѵ�ǰ����.
repeat:
	floppy_on(nr);										// �������ȴ�ָ������nr(kernel/sched.c)
	// ����������(��ת)֮��,�������鿴һ�µ�ǰѡ��������ǲ��Ǻ�������ָ��������nr.
	// �����ǰѡ�����������ָ��������nr,�����Ѿ�ѡ������������,���õ�ǰ���������жϵȴ�״̬,�Եȴ�����������ȡ��ѡ��.�μ�����
	// floppy_deselect().�����ǰû��ѡ������������������������ȡ��ѡ����ʹ��ǰ���񱻻���ʱ,��ǰ������Ȼ����ָ��������nr,����ת��
	// ������ʼ������ѭ���ȴ�.
	while ((current_DOR & 3) != nr && selected)
		sleep_on(&wait_on_floppy_select);
	if ((current_DOR & 3) != nr)
		goto repeat;
	// �������̿������Ѿ�ѡ������ָ��������nr.����ȡ��������Ĵ���DIR��ֵ,��������λ(λ7)��λ,���ʾ�����Ѹ���,��ʱ���ɹر���ﲢ
	// ����1�˳�.����ر���ﷵ��0�˳�.��ʾ����û�б�����.
	if (inb(FD_DIR) & 0x80) {
		floppy_off(nr);
		return 1;
	}
	floppy_off(nr);
	return 0;
}

// �����ڴ滺���,��1024�ֽ�.
// ���ڴ��ַfrom������1024�ֽ����ݵ���ַto��.
#define copy_buffer(from, to) \
__asm__("cld ; rep ; movsl" \
	::"c" (BLOCK_SIZE/4), "S" ((long)(from)), "D" ((long)(to)) \
	:)

// ����(��ʼ��)����DMAͨ��.
// ���������ݶ�д������ʹ��DMA���е�.�����ÿ�ν������ݴ���֮ǰ��Ҫ����DMAоƬר��������������ͨ��2.
static void setup_DMA(void)
{
	long addr = (long) CURRENT->buffer;				// ��ǰ��������������ڴ��ַ.

	// ���ȼ��������Ļ���������λ��.��������������ڴ�1MB���ϵ�ĳ���ط�,����Ҫ��DMA������������ʱ��������(tmp_floppy_area)��.��Ϊ
	// 8237AоƬֻ����1MB��ַ��Χ��Ѱַ.�����д������,����Ҫ�����ݴ�������������Ƶ�����ʱ����.
	cli();
	if (addr >= 0x100000) {
		addr = (long) tmp_floppy_area;
		if (command == FD_WRITE)
			copy_buffer(CURRENT->buffer,tmp_floppy_area);
	}
	// ���������ǿ�ʼ����DMAͨ��2.�ڿ�ʼ����֮ǰ��Ҫ�����θ�ͨ��.��ͨ�����μĴ����˿�Ϊ0x0A.λ0-1ָ��DMAͨ��(0--3),λ2:1��ʾ����,0
	// ��ʾ��������.Ȼ����DMA�������˿�D12��11д�뷽ʽ��(������0x46,д����0x4A).��д�봫��ʹ�û�������ַaddr����Ҫ������ֽ���0x3ff
	// (0--1023).���λ��DMAͨ��2������,����DMA2����DREQ�ź�.
	/* mask DMA 2 */	/* ����DMAͨ��2 */
	immoutb_p(4 | 2,10);
	/* output command byte. I don't know why, but everyone (minix, */
	/* sanches & canton) output this twice, first to 12 then to 11 */
	/* ��������ֽ�.���ǲ�֪��Ϊʲô,����ÿ��(minix,*/
	/* sanches��canton)���������,������12��,Ȼ����11�� */
	// ����Ƕ���������DMA��������"����Ⱥ󴥷���"�˿�12�ͷ�ʽ�Ĵ����˿�11д�뷽ʽ��(����ʱ��0x46,д����0x4A).
	// ���ڸ�ͨ���ĵ�ַ�ͼ����Ĵ�������16λ��,�������������ʱ����Ҫ��2�ν��в���.һ�η��ʵ��ֽ�,��һ�η��ʸ��ֽ�.��ʵ����д�ĸ��ֽ���
	// ���Ⱥ󴥷�����״̬һ.��������Ϊ0ʱ,����ʵ��ֽ�;���ֽڴ�����Ϊ1ʱ,����ʸ��ֽ�.ÿ����һ��,�ô�������״̬�ͱ仯һ��.��д�˿�12�Ϳ�
	// �Խ�����˳�ó�0״̬,�Ӷ���16λ�Ĵ��������ôӵ��ֽڿ�ʼ.
 	__asm__("outb %%al,$12\n\tjmp 1f\n1:\tjmp 1f\n1:\t"
	"outb %%al,$11\n\tjmp 1f\n1:\tjmp 1f\n1:"::
	"a" ((char) ((command == FD_READ)?DMA_READ:DMA_WRITE)));
	/* 8 low bits of addr */	/* ��ַ��0-7λ */
	// ��DMAͨ��2д���/��ǰ��ַ�Ĵ���(�˿�4).
	immoutb_p(addr, 4);
	addr >>= 8;
	/* bits 8-15 of addr */		/* ��ַ��8-15λ */
	immoutb_p(addr, 4);
	addr >>= 8;
	/* bits 16-19 of addr */	/* ��ַ16-19λ */
	// DMAֻ������1MB�ڴ�ռ���Ѱַ,����16-19λ��ַ�����ҳ��Ĵ���(�˿�0x81).
	immoutb_p(addr, 0x81);
	/* low 8 bits of count-1 (1024-1=0x3ff) */	/* ��������8λ(1024-1 = 0x3ff) */
	// ��DMAͨ��2д���/��ǰ�ֽڼ���ֵ(�˿�5).
	immoutb_p(0xff, 5);
	/* high 8 bits of count-1 */	/* ��������8λ */
	// һ�ι�����1024�ֽ�(��������).
	immoutb_p(3, 5);
	/* activate DMA 2 */	/* ����DMAͨ��2������ */
	immoutb_p(0 | 2, 10);
	sti();
}

// ���������������һ���ֽ���������.
// �������������һ���ֽ�֮ǰ,��������Ҫ����׼����״̬,�������ݴ��䷽��������óɴ�CPU��FDC,��˺�����Ҫ���ȶ�ȡ������״̬��Ϣ.
// ����ʹ����ѭ����ѯ��ʽ,�����ʵ���ʱ.������,������ø�λ��־reset.
static void output_byte(char byte)
{
	int counter;
	unsigned char status;

	// ѭ����ȡ��״̬������FD_STATUS(0x3f4)��״̬.�������״̬��STATUS_READY���ҷ���λSTATUS_DIR = 0(CPU ->FDC),�������ݶ˿����
	// ָ���ֽ�.
	if (reset)
		return;
	for(counter = 0 ; counter < 10000 ; counter++) {
		status = inb_p(FD_STATUS) & (STATUS_READY | STATUS_DIR);
		if (status == STATUS_READY) {
			outb(byte,FD_DATA);
			return;
		}
	}
	// �����ѭ��1��ν��������ܷ���,���ø�λ��־,����ӡ������Ϣ.
	reset = 1;
	printk("Unable to send byte to FDC\n\r");
}

// ��ȡFDCִ�еĽ����Ϣ.
// �����Ϣ���7���ֽ�,���������reply_buffer[]��.���ض���Ľ���ֽ���,������ֵ=-1,���ʾ����.������ʽ�����溯������.
static int result(void)
{
	int i = 0, counter, status;

	// ����λ��־����λ,�������˳�.ȥִ�к��������еĸ�λ����.����ѭ����ȡ��״̬������FD_STATUS(0x3f4)��״̬.�����ȡ�Ŀ�����״̬
	// ��READY,��ʾ�Ѿ�û�����ݿ�ȡ,�򷵻��Ѷ�ȡ���ֽ���i.���������״̬�Ƿ����־��λ(CPU <-FDC),��׼����,æ,��ʾ�����ݿɶ�ȡ.
	// ���ǰѿ������еĽ�����ݶ��뵽Ӧ����������.����ȡMAX_REPLIES(7)���ֽ�.
	if (reset)
		return -1;
	for (counter = 0 ; counter < 10000 ; counter++) {
		status = inb_p(FD_STATUS)&(STATUS_DIR|STATUS_READY|STATUS_BUSY);
		if (status == STATUS_READY)
			return i;
		if (status == (STATUS_DIR|STATUS_READY|STATUS_BUSY)) {
			if (i >= MAX_REPLIES)
				break;
			reply_buffer[i++] = inb_p(FD_DATA);
		}
	}
	// �����ѭ��1��ν��������ܷ���,���ø�λ��־,����ӡ������Ϣ.
	reset = 1;
	printk("Getstatus times out\n\r");
	return -1;
}

// ���̶�д��������
// �ú����������̶�д���������ȷ����Ҫ��ȡ�Ľ�һ���ж�.�����ǰ��������������������ڹ涨�����������MAX_ERRORS(8��),��
// ���ٶԵ�ǰ����������һ���Ĳ�������.�����/д��������Ѿ�����MAX_ERRORS/2,����Ҫ����������λ����,�������ø�λ��־reset.����
// ������������������ֵ��һ��,��ֻ������У��һ�´�ͷλ��,������������У����־recalibrate.�����ĸ�λ������У��������ں�����
// �����н���.
static void bad_flp_intr(void)
{
	// ���Ȱѵ�ǰ��������������1.�����ǰ�����������������������������,��ȡ��ѡ����ǰ����,��������������(����������û�б�����).
	CURRENT->errors++;
	if (CURRENT->errors > MAX_ERRORS) {
		floppy_deselect(current_drive);
		end_request(0);
	}
	// �����ǰ������������������������������һ��,���ø�λ��־,����������и�λ����,Ȼ������.��������������У��һ������.
	if (CURRENT->errors > MAX_ERRORS / 2)
		reset = 1;
	else
		recalibrate = 1;
}

/*
 * Ok, this interrupt is called after a DMA read/write has succeeded,
 * so we check the results, and copy any buffers.
 */
/*
 * OK,������жϴ���������DMA��/д�ɹ�����õ�,�������ǾͿ��Լ��ִ�н��,�����ƻ������е�����.
 */
// ���̶�д�жϵ��ú���.
// �ú��������������������������������жϴ�������б�����.�������ȶ�ȡ�������״̬��Ϣ,�ݴ��жϲ����Ƿ�������Ⲣ����Ӧ����.���
// ��/д�����ɹ�,��ô���������Ƕ����������仺�������ڴ�1MB����λ��,����Ҫ�����ݴ�������ʱ���������Ƶ�������Ļ�����.
static void rw_interrupt(void)
{
	// ��ȡFDCִ�еĽ����Ϣ.��������ֽ���������7,����״̬�ֽ�0,1��2�д��ڳ����־,��ô����д��������ʾ������Ϣ,�ͷŵ�ǰ������,��
	// ������ǰ������.�����ִ�г����������.Ȼ�����ִ���������������.����״̬�ĺ���μ�fdreg.h�ļ�.
	if (result() != 7 || (ST0 & 0xd8) || (ST1 & 0xbf) || (ST2 & 0x73)) {    // ��0xf8�޸�0xd8
		if (ST1 & 0x02) {
			printk("Drive %d is write protected\n\r",current_drive);
			floppy_deselect(current_drive);
			end_request(0);
		} else
			bad_flp_intr();
		do_fd_request();
		return;
	}
	// �����ǰ������Ļ�����λ��1MB��ַ����,��˵���˴����̶����������ݻ�������ʱ��������,��Ҫ���Ƶ���ǰ������Ļ�������(��ΪDMAֻ����
	// 1MB��ַ��ΧѰַ).����ͷŵ�ǰ����(ȡ��ѡ��),ִ�е�ǰ�������������:���ѵȴ���������Ľ���,���ѵȴ�����������Ľ���(���еĻ�),������
	// �豸������������ɾ����������.�ټ���ִ�������������������.
	if (command == FD_READ && (unsigned long)(CURRENT->buffer) >= 0x100000)
		copy_buffer(tmp_floppy_area,CURRENT->buffer);
	floppy_deselect(current_drive);
	end_request(1);
	do_fd_request();
}

// ����DMAͨ��2�������̿������������Ͳ���(���1�ֽ�����+0~7�ֽڲ���).
// ��reset��־û����λ,��ô�ڸú����˳��������̿�����ִ������Ӧ��/д������ͻ����һ�������ж�����,����ʼִ�������жϴ������.
void setup_rw_floppy(void)
{
	setup_DMA();										// ��ʼ������DMAͨ��.
	do_floppy = rw_interrupt;							// �������жϵ��ú���ָ��.
	output_byte(command);								// ���������ֽ�.
	output_byte(head<<2 | current_drive);				// ����:��ͷ�� + ��������.
	output_byte(track);									// ����:�ŵ���.
	output_byte(head);									// ����:��ͷ��.
	output_byte(sector);								// ����:��ʼ������.
	output_byte(2);										/* sector size = 512 */	// ����:(N=2)512�ֽ�.
	output_byte(floppy->sect);							// ����:ÿ�ŵ�������.
	output_byte(floppy->gap);							// ����:�����������.
	output_byte(0xFF);									/* sector size (0xff when n!=0 ?) */ // ����:��N=0ʱ,����������ֽڳ���,��������.
	// �������κ�һ��output_byte()��������,������ø�λ��־reset.��ʱ��������ȥִ��do_fd_request()�еĸ�λ�������.
	if (reset)
		do_fd_request();
}

/*
 * This is the routine called after every seek (or recalibrate) interrupt
 * from the floppy controller. Note that the "unexpected interrupt" routine
 * also does a recalibrate, but doesn't come here.
 */
/*
 * ���ӳ�������ÿ�����̿�����Ѱ��(������У��)�ж��б����õ�.ע��"unexpected interrupt"(�����ж�)�ӳ���Ҳ��ִ������У��
 * ����,�����ڴ˵�.
 */
// Ѱ������������жϹ����е��õ�C����.
// ���ȷ��ͼ���ж�״̬����,���״̬��ϢST0�ʹ�ͷ���ڴŵ���Ϣ.��������ִ�д��������⴦���ȡ���������̲���������.�������
// ״̬��Ϣ���õ�ǰ�ŵ�����,Ȼ����ú���setup_rw_floppy()����DMA��������̶�д����Ͳ���.
static void seek_interrupt(void)
{
	// ���ȷ��ͼ���ж�״̬����,�Ի�ȡѰ������ִ�еĽ��.�����������.���ؽ����Ϣ�������ֽ�:ST0�ʹ�ͷ��ǰ�ŵ���.һ��ȡFDCִ�е�
	// �����Ϣ.������ؽ���ֽ���������2,����ST0��ΪѰ������,���ߴ�ͷ���ڴŵ�(ST1)�������趨�ŵ�,��˵�������˴���.����ִ�м�����
	// ��������,Ȼ�����ִ�������������ִ�и�λ����.
	/* sense drive status */	/* ���������״̬ */
	output_byte(FD_SENSEI);
	if (result() != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track) {
		bad_flp_intr();
		do_fd_request();
		return;
	}
	// ��Ѱ�������ɹ�,�����ִ�е�ǰ����������̲���,�������̿�������������Ͳ���.
	current_track = ST1;							// ���õ�ǰ�ŵ�.
	setup_rw_floppy();								// ����DMA��������̲�������Ͳ���.
}

/*
 * This routine is called when everything should be correctly set up
 * for the transfer (ie floppy motor is on and the correct floppy is
 * selected).
 */
/*
 * �ú������ڴ��������������Ϣ����ȷ���úú󱻵��õ�(����������ѿ���������ѡ������ȷ������(����).
 */
// ��д���ݴ��亯��.
static void transfer(void)
{
	// ���ȼ�鵱ǰ�����������Ƿ����ָ���������Ĳ���.�����Ǿͷ����������������������Ӧ����(����1:��4λ��������,��4λ��ͷж��ʱ��;
	// ����2:��ͷ����ʱ��).Ȼ���жϵ�ǰ���ݴ��������Ƿ���ָ����������һ��,�����Ǿͷ���ָ������������ֵ�����ݴ������ʿ��ƼĴ���(FD_DCR).
	if (cur_spec1 != floppy->spec1) {				// ��⵱ǰ����.
		cur_spec1 = floppy->spec1;
		output_byte(FD_SPECIFY);					// �������ô��̲�������.
		output_byte(cur_spec1);						/* hut etc */	// ���Ͳ���.
		output_byte(6);								/* Head load time =6ms, DMA */
	}
	if (cur_rate != floppy->rate)					// ��⵱ǰ����.
		outb_p(cur_rate = floppy->rate,FD_DCR);
	// �������κ�һ��output_byte()����ִ�г���,��λ��־reset�ͻᱻ��λ.�������������Ҫ���һ��reset��־.��reset��ı���λ��,������
	// ȥִ��do_fd_requst()�еĸ�λ�������.
	if (reset) {
		do_fd_request();
		return;
	}
	// �����ʱѰ����־Ϊ��(������ҪѰ��),������DMA�������̿�����������Ӧ��������Ͳ����󷵻�.�����ִ��Ѱ������,���������������жϴ���
	// ���ú���ΪѰ���жϺ���.�����ʼ�ŵ��Ų����������ʹ�ͷѰ������Ͳ���.��ʹ�õĲ������ǵ�112-121�������õ�ȫ�ֱ���ֵ.�����ʼ�ŵ�
	// ��seek_trackΪ0,��ִ������У�������ô�ͷ����λ.
	if (!seek) {
		setup_rw_floppy();							// �������������.
		return;
	}
	do_floppy = seek_interrupt;						// Ѱ���жϵ��õ�C����.
	if (seek_track) {								// ��ʼ�ŵ���.
		output_byte(FD_SEEK);						// ���ʹ�ͷѰ������.
		output_byte(head<<2 | current_drive);		// ���Ͳ���:��ͷ��+��ǰ������.
		output_byte(seek_track);					// ���Ͳ���:�ŵ���.
	} else {
		output_byte(FD_RECALIBRATE);				// ��������У������(��ͷ����).
		output_byte(head<<2 | current_drive);		// ���Ͳ���:��ͷ��+��ǰ������.
	}
	// ͬ����,�������κ�һ��output_byte()����ִ�г���,��λ��־reset�ͻᱻ��λ.��reset��ı���λ��,������ȥִ��do_fd_requet()�и�λ
	// �������.
	if (reset)
		do_fd_request();
}

/*
 * Special case - used after a unexpected interrupt (or reset)
 */
/*
 * ������� - ���������ж�(��λ)�����.
 */
// ��������У���жϵ��ú���.
// ���ȷ��ͼ���ж�״̬����(�޲���),������ؽ����������,���ø�λ��־.��������У����־����.Ȼ���ٴ�ִ���������������
// ����Ӧ����.
static void recal_interrupt(void)
{
	output_byte(FD_SENSEI);							// ���ͼ���ж�״̬����.
	if (result() != 2 || (ST0 & 0xE0) == 0x60)		// ������ؽ���ֽ���������2�������쳣����,���ø�λ��־.
		reset = 1;
	else
		recalibrate = 0;							// ����λ����У����־
	do_fd_request();								// ����Ӧ����.
}

// ���������ж����������������жϴ�������е��õĺ���.
// ���ȷ��ͼ���ж�״̬����(�޲���),������ؽ����������,���ø�λ��־,��������У����־.
void unexpected_floppy_interrupt(void)
{
	output_byte(FD_SENSEI);							// ���ͼ���ж�״̬����.
	if (result()!=2 || (ST0 & 0xE0) == 0x60)		// ������ؽ���ֽ���������2�������쳣����,���ø�λ��־.
		reset = 1;
	else
		recalibrate = 1;							// ����������У����־.
}

// ��������У��������.
// �����̿�����FDC��������У������Ͳ���,����λ����У����־.�����̿�����ִ��������У������ͻ��ٴ������������жϵ���
// recal_interrupt()����.
static void recalibrate_floppy(void)
{
	recalibrate = 0;								// ��λ����У����־.
	current_track = 0;								// ��ǰ�ŵ��Ź���.
	do_floppy = recal_interrupt;					// ָ������У���жϵ��õ�C����.
	output_byte(FD_RECALIBRATE);					// ����:����У��.
	output_byte(head<<2 | current_drive);			// ����:��ͷ�� + ��ǰ��������.
	// �����κ�һ��output_byte()����ִ�г���,��λ��־reset�ͻᱻ��λ.�������������Ҫ���һ��reset��־.��reset��ı�
	// ��λ��,������ȥִ��do_fd_requeset()�еĸ�λ�������.
	if (reset)
		do_fd_request();
}

// ���̿�����FDC��λ�жϵ��ú���.
// �ú�������������������˸�λ��������������������жϴ�������б�����.
// ���ȷ��ͼ���ж�״̬����(�޲���),Ȼ��������صĽ���ֽ�.���ŷ����趨���������������ز���,����ٴε������������
// do_fd_request()ȥִ������У������.������ִ��output_byte()������������ʱ��λ��־�ֻᱻ��λ,���Ҳ�����ٴ�ȥִ�и�λ����.
static void reset_interrupt(void)
{
	output_byte(FD_SENSEI);							// ���ͼ���ж�״̬����.
	(void) result();								// ��ȡ����ִ�н���ֽ�.
	output_byte(FD_SPECIFY);						// �����趨������������.
	output_byte(cur_spec1);							/* hut etc */	// ���Ͳ���
	output_byte(6);									/* Head load time =6ms, DMA */
	do_fd_request();                				// ����ִ����������.
}

/*
 * reset is done by pulling bit 2 of DOR low for a while.
 */
/* FDC��λ��ͨ������������Ĵ���(DOR)λ2��0һ���ʵ�ֵ� */
// ��λ���̿�����.
// �ú����������ò����ͱ�־,�Ѹ�λ��־��0,Ȼ�����������cur_spec1��cur_rate��Ϊ��Ч.��Ϊ��λ������,��������������Ҫ��������.����
// ��Ҫ����У����־,������FDCִ�и�λ�����������������ж��е��õ�C����reset_interrupt().����DOR�Ĵ���λ2��0һ����Զ�����ִ��
// ��λ����.��ǰ��������Ĵ���DOR��λ2������/��λ����λ.
static void reset_floppy(void)
{
	int i;

	reset = 0;										// ��λ��־��0.
	cur_spec1 = -1;									// ʹ��Ч.
	cur_rate = -1;
	recalibrate = 1;								// ����У����־��λ.
	printk("Reset-floppy called\n\r");				// ��ʾִ�����̸�λ������Ϣ.
	cli();											// ���ж�.
	do_floppy = reset_interrupt;					// �������жϴ�������е��õĺ���.
	outb_p(current_DOR & ~0x04,FD_DOR);				// �����̿�����FDCִ�и�λ����.
	for (i = 0 ; i < 100 ; i++)						// �ղ���,�ӳ�.
		__asm__("nop");
	outb(current_DOR,FD_DOR);						// ���������̿�����.
	sti();											// ���ж�.
}

// ����������ʱ�жϵ��ú���.
// ��ִ��һ��������Ҫ��Ĳ���֮ǰ,Ϊ�˵ȴ�ָ�����������ת�������������Ĺ���ת��,do_fd_request()����Ϊ׼���õĵ�ǰ�����������һ����ʱ
// ��ʱ��.���������Ǹö�ʱ������ʱ���õĺ���.�����ȼ����������Ĵ���(DOR),ʹ��ѡ��ǰָ��������.Ȼ�����ִ�����̶�д���亯��transfer().
static void floppy_on_interrupt(void)
{
	/* We cannot do a floppy-select, as that might sleep. We just force it */
	/* ���ǲ�����������ѡ�������,��Ϊ����ܻ��������˯��.����ֻ����ʹ���Լ�ѡ�� */
	// �����ǰ������������������Ĵ���DOR�еĲ�ͬ,����Ҫ��������DORΪ��ǰ������.������������Ĵ��������ǰDOR�Ժ�,ʹ�ö�ʱ���ӳ�2���δ�
	// ʱ��,��������õ�ִ��.Ȼ��������̶�д���亯��transfer().����ǰ��������DOR�е����,��ô�Ϳ���ֱ�ӵ������̶�д���亯��.
	selected = 1;									// ����ѡ����ǰ��������־.
	if (current_drive != (current_DOR & 3)) {
		current_DOR &= 0xFC;
		current_DOR |= current_drive;
		outb(current_DOR,FD_DOR);					// ����������Ĵ��������ǰDOR.
		add_timer(2,&transfer);						// ��Ӷ�ʱ����ִ�д��亯��.
	} else
		transfer();									// ִ�����̶�д���亯��.
}

// ���̶�д���������
// �ú�����������������������Ҫ�ĺ���.��Ҫ������:1�����и�λ��־������У����־��λ���;2�����������е��豸�ż���ȡ��������ָ��������
// ������;3�����ں˶�ʱ���������̶�/д����.
void do_fd_request(void)
{
	unsigned int block;

	// ���ȼ���Ƿ��и�λ��־����У����־��λ,�����򱾺�����ִ����ر�־�Ĵ����ܺ�ͷ���.�����λ��־����λ,��ִ�����̸�λ����������.
	// �������У����־����λ,��ִ����������У������������.
	seek = 0;										// ��Ѱ����־.
	if (reset) {									// ��λ��־����λ.
		reset_floppy();
		return;
	}
	if (recalibrate) {								// ����У����־����λ.
		recalibrate_floppy();
		return;
	}
	// ���������������ܴ����￪ʼ.��������blk.h�ļ��е�INIT_REQUEST�������������ĺϷ���,�����û�����������˳�.Ȼ�������������е��豸
	// ��ȡ��������ָ�������Ĳ�����.��������齫�����������������̲���ʹ�õ�ȫ�ֱ���������.�������豸���е���������(MINOR(CURRENT->dev)>>2)
	// ������������������floppy_type[]������ֵ��ȡ��ָ�������Ĳ�����.
	INIT_REQUEST;
	floppy = (MINOR(CURRENT->dev) >> 2) + floppy_type;
	// ���濪ʼ����ȫ�ֱ���ֵ.�����ǰ��������current_drive������������ָ������������,���ñ�־seek,��ʾ��ִ�ж�/д����֮ǰ��Ҫ��������
	// ��ִ��Ѱ������.Ȼ��ѵ�ǰ������������Ϊ��������ָ������������.
	if (current_drive != CURRENT_DEV)				// CURRENT_DEV����������ָ����������.
		seek = 1;
	current_drive = CURRENT_DEV;
	// ���ö�д��ʼ����block.��Ϊÿ�ζ�д���Կ�Ϊ��λ(1��Ϊ2������),������ʼ������Ҫ����ȴ�����������С2������.����˵��������� ������Ч,
	// �����ô�����������ȥִ����һ��������.
	block = CURRENT->sector;						// ȡ��ǰ��������������ʼ������.
	if (block + 2 > floppy->size) {					// ���block + 2���ڴ�����������,�������������������.
		end_request(0);
		goto repeat;
	}
	// �����Ӧ�ڴŵ��ϵ�������,��ͷ��,�ŵ���,��Ѱ�ŵ���(������������ͬ��ʽ����).
	sector = block % floppy->sect;					// ��ʼ������ÿ�ŵ�������ȡģ,�ôŵ���������.
	block /= floppy->sect;							// ��ʼ������ÿ�ŵ�������ȡ��,����ʼ�ŵ���.
	head = block % floppy->head;					// ��ʼ�ŵ����Դ�ͷ��ȡģ,�ò����Ĵ�ͷ��.
	track = block / floppy->head;					// ��ʼ�ŵ����Դ�ͷ��ȡ��,�ò����Ĵŵ���.
	seek_track = track << floppy->stretch;			// ��Ӧ�������������ͽ��е���,��Ѱ����.
	// �ٿ����Ƿ���Ҫ����ִ��Ѱ������.���Ѱ�����뵱ǰ��ͷ���ڴŵ��Ų�ͬ,����Ҫ����Ѱ������,��������ҪѰ����־seek.�����������ִ�е�����
	// ����command.
	if (seek_track != current_track)
		seek = 1;
	sector++;										// ������ʵ�����������Ǵ�1����.
	if (CURRENT->cmd == READ)						// ����������Ƕ�����,���ö�������.
		command = FD_READ;
	else if (CURRENT->cmd == WRITE)					// �����������д����,����д������.
		command = FD_WRITE;
	else
		panic("do_fd_request: unknown command");
	// ���������ú�����ȫ�ֱ���ֵ֮��,���ǿ��Կ�ʼִ�������������.�ò������ö�ʱ��������.��ΪΪ���ܶ��������ж�д����,��Ҫ�����������������
	// ���ﵽ������ת�ٶ�.������Ҫһ����ʱ��.�����������ticks_to_floppy_on()������������ʱʱ��,Ȼ��ʹ�ø���ʱ�趨һ����ʱ��.��ʱ�䵽ʱ�͵���
	// ����floppy_on_interrupt().
	add_timer(ticks_to_floppy_on(current_drive), &floppy_on_interrupt);
}

// �����������������е����ݿ�����.
static int floppy_sizes[] ={
	   0,   0,   0,   0,
	 360, 360, 360, 360,
	1200, 1200, 1200, 1200,
	 360, 360, 360, 360,
	 720, 720, 720, 720,
	 360, 360, 360, 360,
	 720, 720, 720, 720,
	1440, 1440, 1440, 1440
};

// ����ϵͳ��ʼ��.
// �������̿��豸������Ĵ�����do_fd_request(),�����������ж���(int 0x26,��ӦӲ���ж������ź�IRQ6).Ȼ��ȡ���Ը��ж��źŵ�����,��
// �������̿�����FDC�����ж������ź�.�ж���������IDT�����������������ú�set_trap_gate()������ͷ�ļ�include/asm/system.h��.
void floppy_init(void)
{
	// ���������ж�����������floppy_interrup(kernel/sys_call.s)�����жϴ�����̡�
	blk_size[MAJOR_NR] = floppy_sizes;
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;  						// = do_fd_request()��
	set_trap_gate(0x26, &floppy_interrupt);          						// ������������������
	outb(inb_p(0x21) & ~0x40, 0x21);                   						// ��λ�����ж���������λ��
}



