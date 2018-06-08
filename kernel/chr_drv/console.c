/*
 *  linux/kernel/console.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 * Beeping thanks to John T Kohl.
 *
 * Virtual Consoles, Screen Blanking, Screen Dumping, Color, Graphics
 *   Chars, and VT100 enhancements by Peter MacDonald.
 */
/*
 *
 *	console.c
 * ��ģ��ʵ�ֿ���̨�����������
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * ϣ������һ���ǳ�������VT102ʵ��.
 *
 * ��лJohn T. Kohlʵ���˷���ָʾ�ӳ���.
 *
 * �������̨,��Ļ��������,��Ļ����,��ɫ����,ͼ���ַ���ʾ�Լ�VT100�ն���ǿ������Peter MacDonald����.
 */

/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */
/*
 * ע��!!!������ʱ���ݵؽ�ֹ�������ж�(�����һ����(word)����ƵIO),����ʹ���ڼ����ж���Ҳ�ǿ��Թ�����.��Ϊ����ʹ����������
 * ��������֪���ڴ���һ�����̹����ڼ��ж��Ǳ���ֹ��.ϣ��һ�о�����.
 */

/*
 * Code to check for different video-cards mostly by Galen Hunt,
 * <g-hunt@ee.utah.edu>
 */
/*
 * ��ⲻͬ��ʾ���Ĵ����������Galen Hunt��д��.
 * <g-hunt@ee.utah.edu>
 */

#include <linux/sched.h>
#include <linux/tty.h>										// ttyͷ�ļ�,�����й�tty_io,����ͨ�ŷ���Ĳ���,����.
//#include <linux/config.h>
//#include <linux/kernel.h>

#include <asm/io.h>											// ioͷ�ļ�.����Ӳ���˿�����/����������.
#include <asm/system.h>
#include <asm/segment.h>

//#include <string.h>
#include <errno.h>

// �÷��ų��������ն�IO�ṹ��Ĭ������.���з��ų��������include/termios.h�ļ�.
#define DEF_TERMIOS \
(struct termios) { \
	ICRNL, \
	OPOST | ONLCR, \
	0, \
	IXON | ISIG | ICANON | ECHO | ECHOCTL | ECHOKE, \
	0, \
	INIT_C_CC \
}


/*
 * These are set up by the setup-routine at boot-time:
 */
/*
 * ��Щ��setup��������������ϵͳʱ���õĲ���:
 */

#define ORIG_X				(*(unsigned char *)0x90000)							// ��ʼ����к�
#define ORIG_Y				(*(unsigned char *)0x90001)							// ��ʼ����к�.
#define ORIG_VIDEO_PAGE		(*(unsigned short *)0x90004)						// ��ʼ��ʾҳ��.
#define ORIG_VIDEO_MODE		((*(unsigned short *)0x90006) & 0xff)				// ��ʾģʽ.
#define ORIG_VIDEO_COLS 	(((*(unsigned short *)0x90006) & 0xff00) >> 8)		// ��Ļ����.
#define ORIG_VIDEO_LINES	((*(unsigned short *)0x9000e) & 0xff)				// ��Ļ����.
#define ORIG_VIDEO_EGA_AX	(*(unsigned short *)0x90008)						//
#define ORIG_VIDEO_EGA_BX	(*(unsigned short *)0x9000a)						// ��ʾ�ڴ��С��ɫ��ģʽ.
#define ORIG_VIDEO_EGA_CX	(*(unsigned short *)0x9000c)						// ��ʾ�����Բ���.

#define VIDEO_TYPE_MDA		0x10	/* Monochrome Text Display	*/				/* ��ɫ�ı� */
#define VIDEO_TYPE_CGA		0x11	/* CGA Display 			*/					/* CGA��ʾ�� */
#define VIDEO_TYPE_EGAM		0x20	/* EGA/VGA in Monochrome Mode	*/			/* EGA/VGA��ɫ */
#define VIDEO_TYPE_EGAC		0x21	/* EGA/VGA in Color Mode	*/				/* EGA/VGA��ɫ */

#define NPAR 16																	// ת���ַ�����������������.

int NR_CONSOLES = 0;															// ϵͳʵ��֧�ֵ��������̨����.

extern void keyboard_interrupt(void);

// ����Щ��̬�����Ǳ��ļ�������ʹ�õ�һЩȫ�ֱ���.
static unsigned char	video_type;			/* Type of display being used	*/	// ʹ�õ���ʾ����.
static unsigned long	video_num_columns;	/* Number of text columns	*/		// ��Ļ�ı�����.
static unsigned long	video_mem_base;		/* Base of video memory		*/		// ������ʾ�ڴ����ַ.
static unsigned long	video_mem_term;		/* End of video memory		*/		// ������ʾ�ڴ�ĩ�˵�ַ
static unsigned long	video_size_row;		/* Bytes per row		*/			// ��Ļÿ��ʹ�õ��ֽ���.
static unsigned long	video_num_lines;	/* Number of test lines		*/		// ��Ļ�ı�����.
static unsigned char	video_page;			/* Initial video page		*/		// ������ʾҳ��.
static unsigned short	video_port_reg;		/* Video register select port	*/	// ��ʾ����ѡ��Ĵ����˿�.
static unsigned short	video_port_val;		/* Video register value port	*/	// ��ʾ�������ݼĴ����˿�.
static int can_do_colour = 0;				// ��־: ��ʹ�ò�ɫ����.

// �������̨�ṹ.���а���һ���������̨�ĵ�ǰ������Ϣ.����vc_origin��vc_scr_end�ǵ�ǰ���ڴ�����������ִ̨�п��ٹ�������ʱʹ�õ�
// ��ʼ�к�ĩ�ж�Ӧ����ʾ�ڴ�λ��.vc_video_mem_start��vc_video_meme_end�ǵ�ǰ�������̨ʹ�õ���ʾ�ڴ����򲿷�.
static struct {
	unsigned short	vc_video_erase_char;										// �����ַ����Լ��ַ�(0x0720)
	unsigned char	vc_attr;													// �ַ�����
	unsigned char	vc_def_attr;												// Ĭ���ַ�����.
	int				vc_bold_attr;												// �����ַ�����.
	unsigned long	vc_ques;													// �ʺ��ַ�.
	unsigned long	vc_state;													// ����ת���������еĵ�ǰ״̬.
	unsigned long	vc_restate;													// ����ת���������е���һ״̬.
	unsigned long	vc_checkin;
	unsigned long	vc_origin;													/* Used for EGA/VGA fast scroll	*/
	unsigned long	vc_scr_end;													/* Used for EGA/VGA fast scroll	*/
	unsigned long	vc_pos;														// ��ǰ����Ӧ����ʾ�ڴ�λ��.
	unsigned long	vc_x, vc_y;													// ��ǰ�����,��ֵ.
	unsigned long	vc_top, vc_bottom;											// ����ʱ�����к�;�����к�.
	unsigned long	vc_npar, vc_par[NPAR];										// ת�����в��������Ͳ�������.
	unsigned long	vc_video_mem_start;											/* Start of video RAM		*/
	unsigned long	vc_video_mem_end;											/* End of video RAM (sort of)	*/
	unsigned int	vc_saved_x;													// ����Ĺ���к�.
	unsigned int	vc_saved_y;													// ����Ĺ���к�.
	unsigned int	vc_iscolor;													// ��ɫ��ʾ��־.
	char *			vc_translate;												// ʹ�õ��ַ���.
} vc_cons [MAX_CONSOLES];

// Ϊ�˱�������,���¶��嵱ǰ���ڴ������̨��Ϣ�ķ���.����ͬ��.����currcons��ʹ��vc_cons[]�ṹ�ĺ��������еĵ�ǰ�����ն˺�.
#define origin					(vc_cons[currcons].vc_origin)					// ���ٹ���������ʼ�ڴ�λ��.
#define scr_end					(vc_cons[currcons].vc_scr_end)					// ���ٹ�������ĩ���ڴ�λ��.
#define pos						(vc_cons[currcons].vc_pos)
#define top						(vc_cons[currcons].vc_top)
#define bottom					(vc_cons[currcons].vc_bottom)
#define x						(vc_cons[currcons].vc_x)
#define y						(vc_cons[currcons].vc_y)
#define state					(vc_cons[currcons].vc_state)
#define restate					(vc_cons[currcons].vc_restate)
#define checkin					(vc_cons[currcons].vc_checkin)
#define npar					(vc_cons[currcons].vc_npar)
#define par						(vc_cons[currcons].vc_par)
#define ques					(vc_cons[currcons].vc_ques)
#define attr					(vc_cons[currcons].vc_attr)
#define saved_x					(vc_cons[currcons].vc_saved_x)
#define saved_y					(vc_cons[currcons].vc_saved_y)
#define translate				(vc_cons[currcons].vc_translate)
#define video_mem_start			(vc_cons[currcons].vc_video_mem_start)			// ʹ���Դ����ʼλ��.
#define video_mem_end			(vc_cons[currcons].vc_video_mem_end)			// ʹ���Դ��ĩ��λ��.
#define def_attr				(vc_cons[currcons].vc_def_attr)
#define video_erase_char  		(vc_cons[currcons].vc_video_erase_char)
#define iscolor					(vc_cons[currcons].vc_iscolor)

int blankinterval = 0;															// �趨����Ļ�������ʱ��.
int blankcount = 0;																// ����ʱ�����.

static void sysbeep(void);														// ϵͳ��������

/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 */
/*
 * �������ն˻�ӦESC-Z��csi0c�����Ӧ��(=vt100)��Ӧ
 */
// csi - ��������������(Control Sequence Introducer).
// ����ͨ�����Ͳ��������������0����������(DA)��������('ESC [c'��'ESC [0c']Ҫ���ն�Ӧ��һ���豸���Կ�������(ESC Z�����������ͬ)
// �ն�����������������Ӧ����.������(��'ESC [?1;2c')��ʾ�ն��Ǿ��и߼���Ƶ���ܵ�VT100�����ն�.
#define RESPONSE "\033[?1;2c"

// ����ʹ�õ��ַ���.�����ϰ벿��ʱ��ͨ7λASCII��,��US�ַ���.�°벿�ֶ�ӦVT100�ն��豸�е������ַ�,����ʾͼ���������ַ���.
static char * translations[] = {
/* normal 7-bit ascii */
	" !\"#$%&'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmnopqrstuvwxyz{|}~ ",
/* vt100 graphics */
	" !\"#$%&'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^ "
	"\004\261\007\007\007\007\370\361\007\007\275\267\326\323\327\304"
	"\304\304\304\304\307\266\320\322\272\363\362\343\\007\234\007 "
};

#define NORM_TRANS (translations[0])
#define GRAF_TRANS (translations[1])

// ���ٹ�굱ǰλ��.
// ����: currcons - ��ǰ�����ն˺�;new_x - ��������к�;new_y - ��������к�.
// ���µ�ǰ���λ�ñ���x,y,�������������ʾ�ڴ��еĶ�Ӧλ��pos.�ú��������ȼ���������Ч��.��������Ĺ���кų�����ʾ���������,����
// ����кŲ�������ʾ���������,���˳�.����͸��µ�ǰ���������¹��λ�ö�Ӧ����ʾ�ڴ���λ��pos.
// ע��,�����е����б���ʵ������vc_cons[currcons]�ṹ�е���Ӧ�ֶ�.���º�����ͬ.
/* NOTE! gotoxy thinks x==video_num_columns is ok */
/* ע��!gotoxy������Ϊx==video_num_columnsʱ����ȷ�� */
static inline void gotoxy(int currcons, int new_x, unsigned int new_y)
{
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;
	x = new_x;
	y = new_y;
	pos = origin + y * video_size_row + (x << 1);	// 1����2���ֽڱ�ʾ,����x<<1.
}

// ���ù�����ʼ��ʾ�ڴ��ַ.
static inline void set_origin(int currcons)
{
	// �����ж���ʾ������.����EGA/VGA,���ǿ���ָ�����ڷ�Χ(����)���й�������,��MDA��ɫ��ʾ��ֻ�ܽ���������������.���ֻ��EGA/VGA������Ҫ����
	// ������ʼ����ʾ�ڴ��ַ(��ʼ����origin��Ӧ����).����ʾ�����������EGA/VGA��ɫģʽ,Ҳ����EGA/VGA��ɫģʽ,��ô��ֱ�ӷ���.����,����ֻ��ǰ
	// ̨����̨���в���,��˵�ǰ����̨currocons������ǰ̨����̨ʱ,���ǲ���Ҫ�����������ʼ�ж�Ӧ���ڴ����λ��.
	if (video_type != VIDEO_TYPE_EGAC && video_type != VIDEO_TYPE_EGAM)
		return;
	if (currcons != fg_console)
		return;
	// Ȼ������ʾ�Ĵ���ѡ��˿�video_port_reg���12,��ѡ����ʾ�������ݼĴ���r12,����д�������ʼ��ַ���ֽ�.���������ƶ�9λ,ʵ���ϱ�ʾ�����ƶ�
	// 8λ�ٳ���2(��1���ַ���2�ֽڱ�ʾ).��ѡ����ʾ�������ݼĴ���r13,Ȼ��д�������ʼ��ַ���ֽ�.�����ƶ�1λ��ʾ����2,ͬ��������Ļ��1���ַ���2�ֽ�
	// ��ʾ.���ֵ�����Ĭ����ʾ�ڴ���ʼλ��video_mem_base���в���.
	// �������EGA/VGA��ɫģʽ,viedo_mem_base = �����ڴ��ַ0xb8000.
	cli();
	outb_p(12, video_port_reg);											// ѡ�����ݼĴ���r12,���������ʼλ�ø��ֽ�.
	outb_p(0xff & ((origin - video_mem_base) >> 9), video_port_val);
	outb_p(13, video_port_reg);											// ѡ�����ݼĴ���r13,���������ʼλ�õ��ֽ�.
	outb_p(0xff & ((origin - video_mem_base) >> 1), video_port_val);
	sti();
}

// ���Ͼ�����
// ����Ļ�������������ƶ�һ��,������Ļ��������׳��ֵ���������ӿո��ַ�.��������������1��.
static void scrup(int currcons)
{
	// �����������������2��.������������кŴ��ڵ���������к�,��������й��в���������.����,����EGA/VGA��,���ǿ���ָ�������з�Χ(����)
	// ���й�������,��MDA��ɫ��ʾ��ֻ�ܽ�����������.�ú�����EGA��MDA��ʾ���ͽ��зֱ���.�����ʾ������EGA,�򻹷�Ϊ���������ƶ��������ڴ����ƶ�
	// �������ȴ�����ʾ����EGA/VGA��ʾ���͵����.
	if (bottom <= top)
		return;
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		// ����ƶ���ʼ��top=0,�ƶ������bottom = video_num_lines = 25,���ʾ�������������ƶ�,���ǰ�������Ļ�������ϽǶ�Ӧ����ʼ�ڴ�λ��origin
		// ����Ϊ�����ƶ�һ�ж�Ӧ���ڴ�λ��,ͬʱҲ���ٵ�����ǰ����Ӧ���ڴ�λ���Լ���Ļĩ��ĩ���ַ�ָ��scr_end��λ��.��������Ļ�����ڴ���ʼλ��ֵ
		// originд����ʾ��������
		if (!top && bottom == video_num_lines) {
			origin += video_size_row;
			pos += video_size_row;
			scr_end += video_size_row;
			// �����Ļ����ĩ������Ӧ����ʾ�ڴ�ָ��scr_end������ʵ����ʾ�ڴ�ĩ��,����Ļ���ݳ���һ�����������ж�Ӧ���ڴ������ƶ�����ʾ�ڴ����ʼλ��video_mem_start
			// ��,�����������������ƶ����ֵ�����������ո��ַ�.Ȼ�������Ļ�ڴ������ƶ�������,���µ�����ǰ��Ļ��Ӧ�ڴ����ʼָ��,���λ��ָ�����Ļĩ��
			// ��Ӧ�ڴ�ָ��scr_end.
			if (scr_end > video_mem_end) {
				// ���Ƕ����������Ƚ�(��Ļ�ַ����� - 1)�ж�Ӧ���ڴ������ƶ�����ʾ�ڴ���ʼλ��video_mem_start��,Ȼ���������ڴ�λ�ô����һ�пո�(����)
				// �ַ�����.
				// %0 - eax(�����ַ�+����);%1 - ecx((��Ļ�ַ�����-1)*����Ӧ���ַ���/2,�Գ����ƶ�);
				// %2 - edi(��ʾ�ڴ���ʼλ��video_mem_start); %3 - esi(��Ļ�����ڴ���ʼλ��origin).
				// �ƶ�����:[edi] -> [esi],�ƶ�ecx������.
				__asm__("cld\n\t"											// �巽��λ
					"rep\n\t"												// �ظ�����,����ǰ��Ļ�ڴ������ƶ�����ʾ�ڴ���ʼ��
					"movsl\n\t"
					"movl video_num_columns, %1\n\t"
					"rep\n\t"												// ������������ո��ַ�
					"stosw"
					::"a" (video_erase_char),
					"c" ((video_num_lines - 1) * video_num_columns >> 1),
					"D" (video_mem_start),
					"S" (origin)
					:);
				// �������ÿ��ٹ�����ĩ��λ��
				scr_end -= origin - video_mem_start;
				// ���õ�ǰ��ʾλ��
				pos -= origin - video_mem_start;
				// �������ÿ��ٹ�������ʼλ��
				origin = video_mem_start;
			// ������������Ļĩ�˶�Ӧ���ڴ�ָ��scr_endû�г�����ʾ�ڴ��ĩ��video_mem_end,��ֻ������������������ַ�(�ո��ַ�).
			// %0 - eax(�����ַ�+����);%1 - ecx(��Ļ����);%2 - edi(���1�п�ʼ����Ӧ�ڴ�λ��);
			} else {
				__asm__("cld\n\t"
					"rep\n\t"												// �ظ�����,���³���������������ַ�(�ո��ַ�).
					"stosw"
					::"a" (video_erase_char),
					"c" (video_num_columns),
					"D" (scr_end - video_size_row)
					:);
			}
			// Ȼ�������Ļ���������ڴ���ʼλ��ֵoriginд����ʾ��������.
			set_origin(currcons);
		// �����ʾ���������ƶ�.����ʾ��ָ����top��ʼ��bottom�����е������������ƶ�1��,ָ����top��ɾ��.��ʱֱ�ӽ���Ļ��ָ����top����Ļĩ��
		// �����ж�Ӧ����ʾ�ڴ����������ƶ�1��,�����������³��ֵ�������������ַ�.
		// %0 - eax(�����ַ�+����);%1 - ecx(top����1�п�ʼ��bottom������Ӧ���ڴ泤����);
		// %2 - edi(top���������ڴ�λ��); %3 - esi(top+1���������ڴ�λ��).
		} else {
			__asm__("cld\n\t"
				"rep\n\t"													// ѭ������,��top+1��bottom������Ӧ���ڴ���Ƶ�top�п�ʼ��.
				"movsl\n\t"
				"movl video_num_columns, %%ecx\n\t"
				"rep\n\t"													// ����������������ַ�.
				"stosw"
				::"a" (video_erase_char),
				"c" ((bottom - top - 1) * video_num_columns >> 1),
				"D" (origin + video_size_row * top),
				"S" (origin + video_size_row * (top + 1))
				:);
		}
	}
	// �����ʾ���Ͳ���EGA(����MDA),��ִ�������ƶ�����.��ΪMDA��ʾ���ƿ�ֻ����������,���һ��Զ�����������ʾ��Χ�����,�����Զ�����ָ��,�������ﲻ������Ļ
	// �������Ӧ�ڴ泬����ʾ�ڴ�������������������EGA�������ƶ������ȫһ��.
	else		/* Not EGA/VGA */
	{
		__asm__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl video_num_columns, %%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom - top - 1) * video_num_columns >> 1),
			"D" (origin + video_size_row * top),
			"S" (origin + video_size_row * (top + 1))
			:);
	}
}

// ���¾�һ��
// ����Ļ�������������ƶ�һ��,��Ӧ��Ļ�����������������ƶ�1��.�����ƶ���ʼ�е��Ϸ�����һ����.��������scrup()����,ֻ��Ϊ����
// �ƶ���ʾ�ڴ�����ʱ����������ݸ��ǵ�����,���Ʋ�������������е�,���ȴ���Ļ������2�е����һ���ַ���ʼ���Ƶ����һ��,�ٽ�������3��
// ���Ƶ�������2��,�ȵ�.��Ϊ��ʱ��EGA/VGA��ʾ���ͺ�MDA���͵Ĵ��������ȫһ��,���Ըú���ʵ����û�б�Ҫд������ͬ�Ĵ���.������if��
// else�����еĲ�����ȫһ��.
static void scrdown(int currcons)
{
	// ͬ��,�����������������2��.������������кŴ��ڵ���������к�,��������й��в���������.����,����EGA/VGA��,���ǿ���ָ�������з�Χ(����)
	// ���й�������,��MDA��ɫ��ʾ��ֻ�ܽ�����������.���ڴ��������ƶ�����ƶ��Ե�ǰ����̨ʵ����ʾ�ڴ�ĩ�˵����,��������ֻ��Ҫ������ͨ���ڴ�����
	// �ƶ����.
	if (bottom <= top)
		return;
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		// %0 - eax(�����ַ�+����);%1 - ecx(top�е�bottom-1������Ӧ���ڴ泤����);
		// %2 - edi(�������½����һ���ֳ�λ��); %3 - esi(���ڵ�����2�����һ������λ��).
		__asm__("std\n\t"										// �÷���λ!!
			"rep\n\t"											// �ظ�����,�����ƶ���top�е�bottom-1�ж�Ӧ���ڴ�����
			"movsl\n\t"
			"addl $2, %%edi\n\t"								/* %edi has been decremented by 4 */ /* %edi�Ѽ�4,��Ҳ�Ƿ���������ַ� */
			"movl video_num_columns, %%ecx\n\t"
			"rep\n\t"											// �������ַ������Ϸ�������.
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom - top - 1) * video_num_columns >> 1),
			"D" (origin + video_size_row * bottom - 4),
			"S" (origin + video_size_row * (bottom - 1) - 4)
			:);
	}
	// �������EGA��ʾ����,��ִ�����²���(���������һ��).
	else														/* Not EGA/VGA */
	{
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2, %%edi\n\t"									/* %edi has been decremented by 4 */
			"movl video_num_columns, %%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom - top - 1) * video_num_columns >> 1),
			"D" (origin + video_size_row * bottom - 4),
			"S" (origin + video_size_row * (bottom - 1) - 4)
			:);
	}
}

// �����ͬ��λ������һ��.
// ������û�д������һ��,��ֱ���޸Ĺ�굱ǰ�б���y++,����������Ӧ��ʾ�ڴ�λ��pos(����һ���ַ�����Ӧ���ڴ泤��).����
// ��Ҫ����Ļ������������һ��.
// ��������lf(line feed ����)��ָ��������ַ�LF.
static void lf(int currcons)
{
	if (y + 1 < bottom) {
		y++;
		pos += video_size_row;							// ������Ļһ��ռ���ڴ���ֽ���.
		return;
	}
	scrup(currcons);									// ����Ļ������������һ��.
}

// �����ͬ������һ��
// �����겻����Ļ��һ����,��ֱ���޸Ĺ�굱ǰ����y--,����������Ӧ��ʾ�ڴ�λ��pos,��ȥ��Ļ��һ���ַ�����Ӧ���ڴ泤���ֽ���.
// ������Ҫ����Ļ������������һ��.
// ��������ri(reverse index ��������)��ָ�����ַ�RI��ת������"ESC M".
static void ri(int currcons)
{
	if (y > top) {
		y--;											// ��ȥ��Ļһ��ռ���ڴ���ֽ���
		pos -= video_size_row;
		return;
	}
	scrdown(currcons);									// ����Ļ������������һ��
}

// ���ص���1��(0��).
// ��������Ӧ�ڴ�λ��pos.��������к�*2����0�е���������ж�Ӧ���ڴ��ֽڳ���.
// ��������cr(carriage return�س�)ָ������Ŀ����ַ��Ļس�.
static void cr(int currcons)
{
	pos -= x << 1;										// ��ȥ0�е���괦ռ�õ��ڴ��ֽ���.
	x = 0;
}

// �������ǰһ�ַ�(�ÿո����)(del -delete ɾ��)
// ������û�д���0��,�򽫹���Ӧ�ڴ�λ��pos����2�ֽ�(��Ӧ��Ļ��һ���ַ�),Ȼ�󽫵�ǰ��������ֵ��1,�����������λ�ô��ַ�������
static void del(int currcons)
{
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = video_erase_char;
	}
}

// ɾ����Ļ������λ����صĲ���
// ANSI��������: 'ESC [ Ps J'(Ps = 0 - ɾ����괦����Ļ�׶�;1 - ɾ����Ļ��ʼ����괦;2 - ����ɾ��).����������ָ���Ŀ�������
// �������ֵ,ִ������λ�õ�ɾ������,�����ڲ����ַ�����ʱ���λ�ò���.
// ��������csi_J(CSI - Control Sequence Introducer,����������������)ָ���Կ�������"CSI Ps J"���д���.
// ����:vpar - ��Ӧ�������������Ps��ֵ.
static void csi_J(int currcons, int vpar)
{
	long count;
	long start;

	// ���ȸ�����������ֱ�������Ҫɾ�����ַ�����ɾ����ʼ����ʾ�ڴ�λ��.
	switch (vpar) {
		case 0:												/* erase from cursor to end of display */
			count = (scr_end - pos) >> 1;					/* ������굽��Ļ�׶������ַ� */
			start = pos;
			break;
		case 1:												/* erase from start to cursor */
			count = (pos - origin) >> 1;					/* ɾ������Ļ��ʼ����괦���ַ� */
			start = origin;
			break;
		case 2: 											/* erase whole display */
			count = video_num_columns * video_num_lines;	/* ɾ��������Ļ�ϵ������ַ� */
			start = origin;
			break;
		default:
			return;
	}
	// Ȼ��ʹ�ò����ַ���д��ɾ���ַ��ĵط�.
	// %0 - ecx(ɾ�����ַ���count);%1 - edi(ɾ��������ʼ�ĵ�ַ);%2 - eax(����Ĳ����ַ�).
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		:);
}

// ɾ����һ��������λ����صĲ���.
// ANSIת���ַ�����:'ESC [ Ps K'(Ps = 0ɾ������β;1 �ӿ�ʼɾ��;2 ���ж�ɾ��).���������ݲ���������������еĲ��ֻ������ַ�.������������Ļ��
// �����ַ�����Ӱ�������ַ�.�������ַ�������.�ڲ����ַ�����ʱ���λ�ò���.
// ����:par - ��Ӧ�������������Ps��ֵ.
static void csi_K(int currcons, int vpar)
{
	long count;
	long start;

	// ���ȸ�����������ֱ�������Ҫɾ�����ַ�����ɾ����ʼ����ʾ�ڴ�λ��.
	switch (vpar) {
		case 0:													/* erase from cursor to end of line */
			if (x >= video_num_columns)							/* ɾ����굽��β�����ַ� */
				return;
			count = video_num_columns - x;
			start = pos;
			break;
		case 1:													/* erase from start of line to cursor */
			start = pos - (x << 1);								/* ɾ�����п�ʼ����괦 */
			count = (x < video_num_columns) ? x : video_num_columns;
			break;
		case 2: 												/* erase whole line */
			start = pos - (x << 1);								/* �������ַ�ȫɾ�� */
			count = video_num_columns;
			break;
		default:
			return;
	}
	// Ȼ��ʹ�ò����ַ���дɾ���ַ��ĵط�.
	// %0 - ecx(ɾ���ַ���count);%1 - edi(ɾ��������ʼ��ַ);%2 - eax(����Ĳ����ַ�).
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		:);
}

// ������ʾ�ַ�����
// ANSIת������:'ESC [ Ps;PS m'.Ps = 0 - Ĭ������;1 - ���岢����;4 - �»���;5 - ��˸;7 - ����;22 - �Ǵ���;24 - ���»���;
// 25 - ����˸;27 - ����;30~38 - ����ǰ��ɫ��;39 - Ĭ��ǰ��ɫ(White);40~48 - ���ñ���ɫ��;49 - Ĭ�ϱ���ɫ(Black).
// �ÿ������и��ݲ��������ַ���ʾ����.�Ժ����з��͵��ն˵��ַ�����ʹ������ָ��������,ֱ���ٴ�ִ�б������������������ַ���ʾ������.
void csi_m(int currcons)
{
	int i;

	// һ�������п��Դ��ж����ͬ����.�����洢������par[]��.����͸��ݽ��յ��Ĳ�������npar,ѭ�������������Ps.
	// ���Ps = 0,��ѵ�ǰ�������̨�����ʾ���ַ���������ΪĬ������def_attr.��ʼ��ʱdef_attr�ѱ����ó�0x07(�ڵװ���).
	// ���Ps = 1,��ѵ�ǰ�������̨�����ʾ���ַ���������Ϊ�����������ʾ.����ǲ�ɫ��ʾ,����ַ����Ի���0x08���ַ���������ʾ;����ǵ�ɫ��ʾ,
	// �����˴��»�����ʾ.
	// ���ps = 4,��Բ�ɫ�͵�ɫ��ʾ���в�ͬ�Ĵ���.����ʱ���ǲ�ɫ��ʾ��ʽ,�����ַ����»�����ʾ.����ǲ�ɫ��ʾ,��ô��ԭ��vc_bold_attr������-1
	// ʱ�͸�λ�䱳��ɫ;����Ļ��Ͱѱ���ɫȡ��.��ȡ����ǰ��ɫ�뱳��ɫ��ͬ,�Ͱ�ǰ��ɫ��1��ȡ��һ����ɫ.
	for (i = 0; i <= npar; i++)
		switch (par[i]) {
			case 0:
				attr = def_attr; break;  									/* default */
			case 1:
				attr = (iscolor ? attr | 0x08 : attr | 0x0f); break;  		/* bold */
			/*case 4: attr=attr|0x01;break;*/  /* underline */
			case 4: 														/* bold */
			  if (!iscolor)
			    attr |= 0x01;												// ��ɫ����»�����ʾ.
			  else
			  { 															/* check if forground == background */
			    if (vc_cons[currcons].vc_bold_attr != -1)
			      attr = (vc_cons[currcons].vc_bold_attr & 0x0f) | (0xf0 & (attr));
			    else
			    {
			    	short newattr = (attr&0xf0)|(0xf&(~attr));
			      	attr = ((newattr&0xf)==((attr>>4)&0xf)?
			        (attr&0xf0)|(((attr&0xf)+1)%0xf):
			        newattr);
			    }
			  }
			  break;
			// ���Ps = 5,��ѵ�ǰ�������̨�����ʾ���ַ�����Ϊ��˸,���������ֽ�λ7��1.
			case 5: attr = attr | 0x80; break;  							/* blinking */
			// ���Ps = 7,��ѵ�ǰ�������̨�����ʾ���ַ�����Ϊ����,����ǰ��ɫ�ͱ���ɫ����.
			case 7: attr = (attr << 4) | (attr >> 4); break;  				/* negative */
			// ���Ps = 22,��ȡ������ַ��ĸ�������ʾ(ȡ��������ʾ).
			case 22: attr = attr & 0xf7; break; 							/* not bold */
			// ���Ps = 24,����ڵ�ɫ��ʾ��ȡ������ַ����»�����ʾ,���ڲ�ɫ��ʾȡ��ȡ����ɫ.
			case 24: attr = attr & 0xfe; break;  							/* not underline */
			// ���Ps = 25,��ȡ������ַ�����˸��ʾ.
			case 25: attr = attr & 0x7f; break;  							/* not blinking */
			// ���Ps = 27,��ȡ������ַ��ķ���.
			case 27: attr = def_attr; break; 								/* positive image */
			// ���Ps = 39,��λ����ַ���ǰ��ɫΪĬ��ǰ��ɫ(��ɫ).
			case 39: attr = (attr & 0xf0) | (def_attr & 0x0f); break;
			// ���Ps = 49,��λ����ַ��ı���ɫΪĬ�ϱ���ɫ(��ɫ).
			case 49: attr = (attr & 0x0f) | (def_attr & 0xf0); break;
			// ��Ps(par[i])Ϊ����ֵʱ,��������ָ����ǰ��ɫ�򱳾�ɫ.���Ps = 30..37,��������ǰ��ɫ;���Ps=40..47,�������ñ���ɫ.
			default:
			  if (!can_do_colour)
			    break;
			  iscolor = 1;
			  if ((par[i] >= 30) && (par[i] <= 38))		 					// ����ǰ��ɫ.
			    attr = (attr & 0xf0) | (par[i] - 30);
			  else  														/* Background color */			 // ���ñ���ɫ.
			    if ((par[i] >= 40) && (par[i] <= 48))
			      attr = (attr & 0x0f) | ((par[i] - 40) << 4);
			    else
					break;
		}
}

// ������ʾ���.
// ���ݹ���Ӧ��ʾ�ڴ�λ��pos,������ʾ������������ʾλ��.
static inline void set_cursor(int currcons)
{
	// ��Ȼ������Ҫ������ʾ���,˵���м��̲���,�����Ҫ�ָ����к�����������ʱ����ֵ.
	// ����,��ʾ���Ŀ���̨�����ǵ�ǰ����̨,�������ǰ�����̨��currocons����ǰ̨����̨�����̷���.
	blankcount = blankinterval;						// ��λ���������ļ���ֵ.
	if (currcons != fg_console)
		return;
	// Ȼ��ʹ�������Ĵ����˿�ѡ����ʾ�������ݼĴ���r14(��굱ǰ��ʾλ�ø��ֽ�),����д���굱ǰλ�ø��ֽ�(�����ƶ�9λ��ʾ���ֽ��Ƶ����ֽ��ٳ���2),
	// �����Ĭ����ʾ�ڴ������.��ʹ�������Ĵ���ѡ��r15,������굱ǰλ�õ��ֽ�д������.
	cli();
	outb_p(14, video_port_reg);
	outb_p(0xff & ((pos - video_mem_base) >> 9), video_port_val);
	outb_p(15, video_port_reg);
	outb_p(0xff & ((pos - video_mem_base) >> 1), video_port_val);
	sti();
}

// ���ع��
// �ѹ�����õ���ǰ�������̨���ڵ�ĩ�ˣ������ع������á�
static inline void hide_cursor(int currcons)
{
	// ����ʹ�������Ĵ����˿�ѡ��������ݼĴ���r14����굱ǰ��ʾλ�ø��ֽڣ���Ȼ��д���굱ǰλ�ø��ֽڣ������ƶ�9λ��ʾ���ֽ��Ƶ����ֽ��ٳ���2����
	// �������Ĭ����ʾ�ڴ�����ġ���ʹ�������Ĵ���ѡ��r15��������굱ǰλ�õ��ֽ�д�����С�
	outb_p(14, video_port_reg);
	outb_p(0xff & ((scr_end - video_mem_base) >> 9), video_port_val);
	outb_p(15, video_port_reg);
	outb_p(0xff & ((scr_end - video_mem_base) >> 1), video_port_val);
}

// ���Ͷ�VT100����Ӧ����.
// ��Ϊ��Ӧ���������ն������������豸����(DA).����ͨ�����Ͳ��������������0��DA��������('ESC [ 0c'��'ESC Z']Ҫ���ն˷���һ���豸����(DA)����
// ����,�ն����͵�85���϶����Ӧ������(��'ESC [?];2c']������������,�����и����������ն��Ǿ��и߼���Ƶ���ܵ�VT100�����ն�.��������ǽ�Ӧ������
// ��������������,��ʹ��copy_to_cooked()�����������븨��������.
static void respond(int currcons, struct tty_struct * tty)
{
	char * p = RESPONSE;

	cli();
	while (*p) {									// ��Ӧ�����з��������.
		PUTCH(*p, tty->read_q);						// ���ַ�����.include/linux/tty.h
		p++;
	}
	sti();
	copy_to_cooked(tty);							// ת���ɹ淶ģʽ(���븨��������).tty_io.c
}

// �ڹ�괦����һ�ո��ַ�.
// �ѹ�꿪ʼ���������ַ�����һ��,���������ַ������ڹ�����ڴ�.
static void insert_char(int currcons)
{
	int i = x;
	unsigned short tmp, old = video_erase_char;		// �����ַ�(������)
	unsigned short * p = (unsigned short *) pos;	// ����Ӧ�ڴ�λ��.

	while (i++ < video_num_columns) {
		tmp = *p;
		*p = old;
		old = tmp;
		p++;
	}
}

// �ڹ�괦����һ��.
// ����Ļ���ڴӹ�������е����ڵ׵��������¾�һ��.��꽫�����µĿ�����.
static void insert_line(int currcons)
{
	int oldtop, oldbottom;

	// ���ȱ�����Ļ���ھ���ʼ��top�������bottomֵ,Ȼ��ӹ������������Ļ�������¹���һ��.���ָ���Ļ���ھ���ʼ��top�������bottom
	// ��ԭ��ֵ.
	oldtop = top;
	oldbottom = bottom;
	top = y;										// ������Ļ����ʼ�кͽ�����.
	bottom = video_num_lines;
	scrdown(currcons);								// �ӹ�꿪ʼ��,��Ļ�������¹���һ��.
	top = oldtop;
	bottom = oldbottom;
}

// ɾ��һ���ַ�
// ɾ����괦��һ���ַ�,����ұߵ������ַ�����һ��.
static void delete_char(int currcons)
{
	int i;
	unsigned short * p = (unsigned short *) pos;

	// ������ĵ�ǰ��λ��x�߳���Ļ������,�򷵻�.����ӹ����һ���ַ���ʼ����ĩ�����ַ�����һ��.Ȼ�������һ���ַ�����������ַ�.
	if (x >= video_num_columns)
		return;
	i = x;
	while (++i < video_num_columns) {				// ����������ַ�����1��.
		*p = *(p + 1);
		p++;
	}
	*p = video_erase_char;							// �����������ַ�.
}

// ɾ�����������
// ɾ��������ڵ�һ��,���ӹ�������п�ʼ��Ļ�����ώ�һ��.
static void delete_line(int currcons)
{
	int oldtop, oldbottom;

	// ���ȱ�����Ļ���ھ���ʼ��top�������bottomֵ,Ȼ��ӹ������������Ļ�������Ϲ���һ��.���ָ���Ļ���ھ���ʼ��top�������bottom
	// ��ԭ��ֵ.
	oldtop = top;
	oldbottom = bottom;
	top = y;										// ������Ļ����ʼ�к������.
	bottom = video_num_lines;
	scrup(currcons);								// �ӹ�꿪ʼ��,��Ļ�������Ϲ���һ��.
	top = oldtop;
	bottom = oldbottom;
}

// �ڹ�괦����nr���ַ�
// ANSIת���ַ�����:'ESC [ Pn @'.�ڵ�ǰ��괦����1��������ȫ�ո��ַ�.Pn�ǲ�����ַ���.Ĭ����1.��꽫��Ȼ���ڵ�1������Ŀո��ַ���.�ڹ�����ұ߽�
// ���ַ�������.�����ұ߽���ַ�������ʧ.
// ���� nr = ת���ַ������еĲ���Pn.
static void csi_at(int currcons, unsigned int nr)
{
	// ���������ַ�������һ���ַ���,���Ϊһ���ַ���;�������ַ���nrΪ0,�����1���ַ�.Ȼ��ѭ������ָ�����ո��ַ�.
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
	while (nr--)
		insert_char(currcons);
}

// �ڹ��λ�ô�����nr��.
// ANSIת���ַ�����: 'ESC [ Pn L'.�ÿ��������ڹ�괦����1�л���п���.������ɺ���λ�ò���.�����б�����ʱ,������¹��������ڵ��������ƶ�.����������ʾҳ��
// �оͶ�ʧ.
// ����nr = ת���ַ������еĲ���Pn.
static void csi_L(int currcons, unsigned int nr)
{
	// �������������������������,���Ϊ��Ļ��ʾ����;����������nrΪ0,�����1��.Ȼ��ѭ������ָ������nr�Ŀ���.
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr = 1;
	while (nr--)
		insert_line(currcons);
}

// ɾ����괦��nr���ַ�.
// ANSIת������:'ESC [ Pn P'.�ÿ������дӹ�괦ɾ��Pn���ַ�.��һ���ַ���ɾ��ʱ,����������ַ�������,������ұ߽紦����һ�����ַ�.������Ӧ�������һ�������ַ�
// ��ͬ,���������˼򻯴���,��ʹ���ַ���Ĭ������(�ڵװ��ֿո�0x0720)�����ÿ��ַ�.
// ����nr = ת���ַ������еĲ���Pn.
static void csi_P(int currcons, unsigned int nr)
{
	// ���ɾ�����ַ�������һ���ַ���,���Ϊһ���ַ���;��ɾ���ַ���nrΪ0,��ɾ��1���ַ�.Ȼ��ѭ��ɾ����괦ָ���ַ���nr.
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
	while (nr--)
		delete_char(currcons);
}

// ɾ����괦��nr��.
// ANSIת������: 'ESC [ Pn M'.�ÿ��������ڹ���������,�ӹ�������п�ʼɾ��1�л����.���б�ɾ��ʱ,���������ڵı�ɾ�����µ��л������ƶ�,���һ�����������1����.��
// Pn������ʾҳ��ʣ������,�����н�ɾ����Щʣ����,���Թ������򴦲�������.
// ����nr = ת���ַ������еĲ���Pn.
static void csi_M(int currcons, unsigned int nr)
{
	// ���ɾ��������������Ļ�������,���Ϊ��Ļ��ʾ����;����ɾ��������nrΪ0,��ɾ��1��.Ȼ��ѭ��ɾ��ָ������nr.
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_line(currcons);
}

//// ���浱ǰ���λ��
static void save_cur(int currcons)
{
	saved_x = x;
	saved_y = y;
}

// �ָ�����Ĺ��λ��
static void restore_cur(int currcons)
{
	gotoxy(currcons, saved_x, saved_y);
}

// ���ö�ٶ�����������con_write()�����д���ת�����л�������еĽ���.ESnormal�ǳ�ʼ����״̬,Ҳ��ת���������д������ʱ��״̬.
// ESnormal  - ��ʾ���ڳ�ʼ����״̬.��ʱ�����յ�������ͨ��ʾ�ַ�,����ַ�ֱ����ʾ����Ļ��;�����յ����ǿ����ַ�(����س��ַ�),��Թ�
//             ��λ�ý�������.���մ�����һ��ת����������,����Ҳ�᷵�ص���״̬.
// ESesc     - ��ʾ���յ�ת�����������ַ�ESC(0x1b = 033 = 27);����ڴ�״̬�½��յ�һ��'['�ַ�,��˵��ת������������,������ת��
//             ESsquareȥ����.����Ͱѽ��յ����ַ���Ϊת������������.����ѡ���ַ���ת������'ESC ('��'ESC )',ʹ�õ�����״̬ESsetgraph
//             ������;�����豸�����ַ�������'ESC P',ʹ�õ�����״̬ESsetterm������.
// ESsquare  - ��ʾ�Ѿ����յ�һ����������������('ESC ['),��ʾ���յ�����һ����������.���Ǳ�״ִ̬�в�������par[]�����ʼ������.���
//             ��ʱ���յ�������һ��'['�ַ�,��ʾ�յ���'ESC [['����.�������Ǽ��̹��ܼ�����������,������ת��ESfunckeyȥ����.����
//             ������Ҫ׼�����տ������еĲ���,������״̬ESgetparts��ֱ�ӽ����״̬ȥ���ղ��������еĲ����ַ�.
// ESgetparts- ��״̬��ʾ��ʱҪ���տ������еĲ���ֵ.������ʮ��������ʾ,�ѽ��յ��������ַ�ת������ֵ�����浽par[]������.����յ�һ���ֺ�';',
//             ����ά���ڱ�״̬,���ѽ��յ��Ĳ���ֵ����������par[]��һ����.�����������ַ���ֺ�,˵����ȡ�����в���,��ô��ת�Ƶ�״̬
//             ESgotpartsȥ����.
// ESgotparts- ��ʾ�Ѿ����յ�һ�������Ŀ�������.��ʱ���Ը��ݱ�״̬���յ��Ľ�β�ַ�����Ӧ�������н��д���.�����ڴ���֮ǰ,�����ESsquare״̬
//             �յ���'?',˵������������ն��豸˽������.���ں˲���֧�ֶ��������еĴ���,����ֱ�ӻָ���ESnormal״̬.�����ȥִ����Ӧ��������.
//             �����д������Ͱ�״̬�ָ���ESnormal.
// ESfunckey - ��ʾ���յ��˼����Ϲ��ܼ�������һ������,������ʾ.���ǻָ�������״̬ESnormal.
// ESsetterm - ��ʾ�����豸�����ַ�������״̬(DCS).��ʱ�յ��ַ�'S',��ָ���ʼ����ʾ�ַ�����.���յ����ַ���'L'��'l',�������������ʾ��ʽ.
// ESsetgraph- ��ʾ�յ������ַ�ת������'ESC ('��'ESC )'.���Ƿֱ�����ָ��G0��G1���õ��ַ���.��ʱ���յ��ַ�'0',��ѡ��ͼ���ַ�����ΪG0��G1,
//             ���յ����ַ���'B',��ѡ����ͨASCII�ַ�����ΪG0��G1���ַ���.
enum { ESnormal, ESesc, ESsquare, ESgetpars, ESgotpars, ESfunckey,
	ESsetterm, ESsetgraph };

// ����̨д����
// ���ն˶�Ӧ��ttyд���������ȡ�ַ����ÿ���ַ����з���.���ǿ����ַ���ת����������,����й�궨λ,�ַ�ɾ���ȵĿ��ƴ���;������ͨ�ַ���ֱ���ڹ�괦
// ��ʾ.
// ����:tty�ǵ�ǰ����̨ʹ�õ�tty�ṹָ��.
void con_write(struct tty_struct * tty)
{
	int nr;
	char c;
	int currcons;

	// �ú������ȸ��ݵ�ǰ����̨ʹ�õ�tty��tty�����λ��ȡ�ö�Ӧ����̨��currcons,Ȼ������(CHARS())Ŀǰttyд�����к��е��ַ���nr,��ѭ��ȡ�����е�ÿ��
	// �ַ����д���.���������ǰ����̨���ڽ��ռ��̻򷢳�����ͣ����(�簴��Ctrl-S)������ֹͣ״̬,��ô��������ֹͣ����д�����е��ַ�,�˳�����.����,���ȡ����
	// �ǿ����ַ�CAN(24)��SUB(6),��ô������ת�����������ڼ��յ���,�����в���ִ�ж�������ֹ,ͬʱ��ʾ�����ַ�.ע��,con_write()����ֻ����ȡ�����ַ���
	// ʱд�����е�ǰ���е��ַ�.���п�����һ�����б��ŵ�д�����ڼ��ȡ�ַ���,��˱�����ǰһ���˳�ʱstate�п��ܴ��ڴ���ת���������е�����״̬��.
	currcons = tty - tty_table;
	if ((currcons >= MAX_CONSOLES) || (currcons < 0))
		panic("con_write: illegal tty");

	nr = CHARS(tty->write_q);										// ȡд�������ַ���,��tty.h�ļ���
	while (nr--) {
		if (tty->stopped)
			break;
		GETCH(tty->write_q, c);										// ȡ1�ַ���c��
		if (c == 24 || c == 26)										// �����ַ�CAN,SUB - ȡ��,�滻
			state = ESnormal;
		switch(state) {
			// ESnormal:��ʾ���ڳ�ʼ����״̬.��ʱ�����յ�������ͨ��ʾ�ַ�,����ַ�ֱ����ʾ����Ļ��;�����յ����ǿ����ַ�(����س��ַ�),��Թ�
			//          ��λ�ý�������.���մ�����һ��ת����������,����Ҳ�᷵�ص���״̬.
			// �����д������ȡ�����ַ�����ͨ��ʾ�ַ�����,��ֱ�Ӵӵ�ǰӳ���ַ�����ȡ����Ӧ����ʾ�ַ�,���ŵ���ǰ�����������ʾ�ڴ�λ�ô�,��ֱ����ʾ���ַ�.Ȼ��ѹ��
			// λ������һ���ַ�λ��.�����,����ַ����ǿ����ַ�Ҳ������չ�ַ�,��(31<c<127),��ô,����ǰ��괦����ĩ�˻�ĩ������,�򽫹���Ƶ�����ͷ��.���������λ��
			// ��Ӧ���ڴ�ָ��pos.Ȼ���ַ�cд����ʾ�ڴ���pos��,�����������1��,ͬʱҲ��pos��Ӧ���ƶ�2���ֽ�.
			case ESnormal:
				if (c > 31 && c < 127) {							// ����ͨ��ʾ�ַ�
					if (x >= video_num_columns) {					// Ҫ����?
						x -= video_num_columns;
						pos -= video_size_row;
						lf(currcons);
					}
					__asm__("movb %2, %%ah\n\t"						// д�ַ�
						"movw %%ax, %1\n\t"
						::"a" (translate[c - 32]),
						"m" (*(short *)pos),
						"m" (attr)
						:);
					pos += 2;
					x++;
				// ����ַ�c��ת���ַ�ESC,��ת��״̬state��ESesc
				} else if (c == 27)									// ESC - ת������ַ�
					state = ESesc;
				// ���c�ǻ��з�LF(10),��ֱ�Ʊ��VT(11),��ҳ��FF(12),�����ƶ�����1��.
				else if (c == 10 || c == 11 || c == 12)
					lf(currcons);
				// ���c�ǻس���CR(13),�򽫹���ƶ���ͷ��(0��)
				else if (c == 13)									// CR - �س�
					cr(currcons);
				// ���c��DEL(127),�򽫹������ַ�����(�ÿո��ַ����),��������Ƶ�������λ��.
				else if (c == ERASE_CHAR(tty))
					del(currcons);
				// ���c��BS(backspace,8),�򽫹������1��,����Ӧ��������Ӧ�ڴ�λ��ָ��pos.
				else if (c == 8) {									// BS - ����.
					if (x) {
						x--;
						pos -= 2;
					}
				// ����ַ�c��ˮƽ�Ʊ��HT(9),�򽫹���Ƶ�8��λ������.����ʱ�������������Ļ�������,�򽫹���Ƶ���һ����.
				} else if (c == 9) {								// HT - ˮƽ�Ʊ�
					c = 8 - (x & 7);
					x += c;
					pos += c << 1;
					if (x > video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf(currcons);
					}
					c = 9;
				// ����ַ�c�������BEL(7),����÷�������,ʹ����������.
				} else if (c == 7)									// BEL - ����
					sysbeep();
				// ���c�ǿ����ַ�SO(14)��SI(15),����Ӧѡ���ַ���G1��G0��Ϊ��ʾ�ַ���.
			  	else if (c == 14)									// SO - ����,ʹ��G1.
			  		translate = GRAF_TRANS;
			  	else if (c == 15)									// SI - ����,ʹ��G0.
					translate = NORM_TRANS;
				break;
			// ESesc:��ʾ���յ�ת�����������ַ�ESC(0x1b = 033 = 27);����ڴ�״̬�½��յ�һ��'['�ַ�,��˵��ת������������,������ת��
			//       ESsquareȥ����.����Ͱѽ��յ����ַ���Ϊת������������.����ѡ���ַ���ת������'ESC ('��'ESC )',ʹ�õ�����״̬ESsetgraph
			//       ������;�����豸�����ַ�������'ESC P',ʹ�õ�����״̬ESsetterm������
			// �����ESnormal״̬�յ�ת���ַ�ESC(0x1b = 033 = 27),��ת����״̬����.��״̬��C1�п����ַ���ת���ַ����д���.�������Ĭ�ϵ�
			// ״̬����ESnormal.
			case ESesc:
				state = ESnormal;
				switch (c)
				{
				  case '[':											// ESC [ - ��CSI����.
					state = ESsquare;
					break;
				  case 'E':											// ESC E - �������1�л�0��.
					gotoxy(currcons, 0, y + 1);
					break;
				  case 'M':											// ESC M - �������һ��.
					ri(currcons);
					break;
				  case 'D':											// ESC D - �������һ��
					lf(currcons);
					break;
				  case 'Z':											// ESC Z - �豸���Բ�ѯ
					respond(currcons, tty);
					break;
				  case '7':											// ESC 7 - ������λ��
					save_cur(currcons);
					break;
				  case '8':											// ESC 8 - �ָ����λ��
					restore_cur(currcons);
					break;
				  case '(':  case ')':								// ESC(,ESC) - ѡ���ַ���
				    	state = ESsetgraph;
					break;
				  case 'P':											// ESC P - �����ն˲���
				    	state = ESsetterm;
				    	break;
				  case '#':											// ESC # - �޸���������
				  	state = -1;
				  	break;
				  case 'c':											// ESC c - ��λ���ն˳�ʼ����
					tty->termios = DEF_TERMIOS;
				  	state = restate = ESnormal;
					checkin = 0;
					top = 0;
					bottom = video_num_lines;
					break;
				 /* case '>':   Numeric keypad */
				 /* case '=':   Appl. keypad */
				}
				break;
			// ESsquare:��ʾ�Ѿ����յ�һ����������������('ESC ['),��ʾ���յ�����һ����������.���Ǳ�״ִ̬�в�������par[]�����ʼ������.���
			//          ��ʱ���յ�������һ��'['�ַ�,��ʾ�յ���'ESC [['����.�������Ǽ��̹��ܼ�����������,������ת��ESfunckeyȥ����.����
			//          ������Ҫ׼�����տ������еĲ���,������״̬ESgetparts��ֱ�ӽ����״̬ȥ���ղ��������еĲ����ַ�.
			// �����״̬ESesc(��ת���ַ�ESC)ʱ�յ����ַ�'[',�������CSI��������,����ת��״̬Essequare������.���ȶ�ESCת�����б����������par[]����,
			// ��������nparָ������,�������ÿ�ʼ���ڲ���״̬ESgetpars.������յ����ַ�����'?',��ֱ��ת��״̬ESgetparsȥ����,�����յ����ַ���'?',˵����
			// ���������ն��豸˽������,�������һ�������ַ�.����ȥ����һ�ַ�,�ٵ�״̬ESgetparsȥ������봦.�����ʱ���յ��ַ�����'[',��ô�����յ��˼��̹���
			// ������������,����������һ״̬ΪESfunckey.����ֱ�ӽ���ESgetpars״̬��������.
			case ESsquare:
				for(npar = 0; npar < NPAR; npar++)					// ��ʼ����������.
					par[npar] = 0;
				npar = 0;
				state = ESgetpars;
				if (c == '[')  										/* Function key */	// 'ESC [['�ǹ��ܼ�.
				{
					state = ESfunckey;
					break;
				}
				if (ques = (c == '?'))
					break;
			// ESgetpars: ��״̬��ʾ��ʱҪ���տ������еĲ���ֵ.������ʮ��������ʾ,�ѽ��յ��������ַ�ת������ֵ�����浽par[]������.����յ�һ���ֺ�';',
			//            ����ά���ڱ�״̬,���ѽ��յ��Ĳ���ֵ����������par[]��һ����.�����������ַ���ֺ�,˵����ȡ�����в���,��ô��ת�Ƶ�״̬
			//            ESgotpartsȥ����.
			// ��״̬��ʾ��ʱҪ���տ������еĲ���ֵ.������ʮ��������ʾ,�ѽ��յ��������ַ�ת������ֵ�����浽par[]������.����յ�һ���ֺ�';',����ά���ڱ�״̬,��
			// �ѽ��յ��Ĳ���ֵ����������par[]��һ����.�����������ַ���ֺ�,˵����ȡ�����в���,��ô��ת�Ƶ�״̬ESgotparsȥ����.
			case ESgetpars:
				if (c == ';' && npar < NPAR - 1) {
					npar++;
					break;
				} else if (c >= '0' && c <= '9') {
					par[npar] = 10 * par[npar] + c - '0';
					break;
				} else state = ESgotpars;
			// ESgotpars:��ʾ�Ѿ����յ�һ�������Ŀ�������.��ʱ���Ը��ݱ�״̬���յ��Ľ�β�ַ�����Ӧ�������н��д���.�����ڴ���֮ǰ,�����ESsquare״̬
			//           �յ���'?',˵������������ն��豸˽������.���ں˲���֧�ֶ��������еĴ���,����ֱ�ӻָ���ESnormal״̬.�����ȥִ����Ӧ��������.
			//           �����д������Ͱ�״̬�ָ���ESnormal.
			// ESgotpars״̬��ʾ�����Ѿ����յ�һ�������Ŀ�������.��ʱ���Ը��ݱ�״̬���յ��Ľ�β�ַ�����Ӧ�������н��д���.�����ڴ���֮ǰ,�����ESsquare״̬�յ���'?',
			// ˵������������ն��豸˽������.���ں˲�֧�ֶ��������еĴ���,����ֱ�ӻָ���ESnormal״̬.�����ȥִ����Ӧ��������.�����д������Ͱ�״̬�ָ���ESnormal.
			case ESgotpars:
				state = ESnormal;
				if (ques)
				{ ques =0;
				  break;
				}
				switch(c) {
					// ���c���ַ�'G'��'`',��par[]�е�1�����������к�,���кŲ�Ϊ��,�򽫹������1��.
					case 'G': case '`':							// CSI Pn G - ���ˮƽ�ƶ�.
						if (par[0]) par[0]--;
						gotoxy(currcons, par[0], y);
						break;
					// ���c��'A',���1���������������Ƶ�����.������Ϊ0������1��.
					case 'A':									// CSI Pn A - �������.
						if (!par[0]) par[0]++;
						gotoxy(currcons, x, y - par[0]);
						break;
					// ���c��'B'��'e',���1�����������Ƶĸ���.������Ϊ0������һ��.
					case 'B': case 'e':							// CSI Pn B - �������.
						if (!par[0]) par[0]++;
						gotoxy(currcons, x, y + par[0]);
						break;
					// ���c��'C'��'a',���1���������������Ƶĸ���.������Ϊ0������1��.
					case 'C': case 'a':							// CSI Pn C - �������.
						if (!par[0]) par[0]++;
						gotoxy(currcons, x + par[0], y);
						break;
					// ���c��'D',���1���������������Ƶĸ���.������Ϊ0������1��.
					case 'D':									// CSI Pn D - �������.
						if (!par[0]) par[0]++;
						gotoxy(currcons, x - par[0], y);
						break;
					// ���c��'E',���1�����������������ƶ�������,���ص�0��.������Ϊ0������1��.
					case 'E':									// CSI Pn E - ������ƻ�0��
						if (!par[0]) par[0]++;
						gotoxy(currcons, 0, y + par[0]);
						break;
					// ���c��'F',���1�����������������ƶ�������,���ص�0��.������Ϊ0������1��.
					case 'F':									// CSI Pn F - ������ƻ�0��.
						if (!par[0]) par[0]++;
						gotoxy(currcons, 0, y - par[0]);
						break;
					// ���c��'d',���1�����������������ڵ��к�(��0����).
					case 'd':									// CSI Pn d - �ڵ�ǰ������λ��
						if (par[0]) par[0]--;
						gotoxy(currcons, x, par[0]);
						break;
					// ���c��'H'��'f',���1�������������Ƶ����к�,��2�������������Ƶ����к�.
					case 'H': case 'f':							// CSI Pn H - ��궨λ.
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(currcons, par[1], par[0]);
						break;
					// ����ַ�c��'J',���1�����������Թ������λ�������ķ�ʽ:
					// ����: 'ESC [ Ps J'(Ps=0ɾ����굽��Ļ�׶�;Ps=1ɾ����Ļ��ʼ����괦;Ps=2����ɾ��).
					case 'J':									// CSI Pn J - ��Ļ�����ַ�.
						csi_J(currcons, par[0]);
						break;
					// ����ַ�c��'K',���1�����������Թ������λ�ö������ַ�����ɾ������ķ�ʽ:
					// ����: 'ESC [ Ps K'(Ps=0ɾ������β;Ps=1�ӿ�ʼɾ��;Ps=2���ж�ɾ��).
					case 'K':									// CSI Pn K - ���ڲ����ַ�.
						csi_K(currcons,par[0]);
						break;
					// ����ַ�c��'L',��ʾ�ڹ��λ�ô�����n��(�������� 'ESC [ Pn L')
					case 'L':									// CSI Pn L - ������.
						csi_L(currcons, par[0]);
						break;
					// ����ַ�c��'M',��ʾ�ڹ��λ�ô�ɾ��n��(�������� 'ESC [ Pn M')
					case 'M':									// ɾ����
						csi_M(currcons, par[0]);
						break;
					// ����ַ�c��'P',��ʾ�ڹ��λ�ô�ɾ��n���ַ�(�������� 'ESC [ Pn P')
					case 'P':									// ɾ���ַ�.
						csi_P(currcons, par[0]);
						break;
					// ����ַ�c��'@',��ʾ�ڹ��λ�ô�����n���ַ�(�������� 'ESC [ Pn @')
					case '@':									// �����ַ�.
						csi_at(currcons, par[0]);
						break;
					// ����ַ�c��'m',��ʾ�ı��괦�ַ�����ʾ����,����Ӵ�,���»���,��˸,���Ե�.
					// ת������: 'ESC [ Pn m'.n=0������ʾ;1�Ӵ�;4���»���;7����;27������ʾ��.
					case 'm':									// CSI Ps m - ������ʾ�ַ�����.
						csi_m(currcons);
						break;
					// ����ַ�c��'r',���ʾ�����������ù�������ʼ�кź���ֹ�к�.
					case 'r':									// CSI Pn r - ���ù������½�.
						if (par[0]) par[0]--;
						if (!par[1]) par[1] = video_num_lines;
						if (par[0] < par[1] &&
						    par[1] <= video_num_lines) {
							top = par[0];
							bottom = par[1];
						}
						break;
					// ����ַ�c��'s',���ʾ���浱ǰ�������λ��.
					case 's':									// CSI s - ������λ��.
						save_cur(currcons);
						break;
					// ����ַ�c��'u',���ʾ�ָ���굽ԭ�����λ�ô�.
					case 'u':									// CSI u - �ָ�����Ĺ��λ��.
						restore_cur(currcons);
						break;
					// ����ַ�c��'l'��'b',��ֱ��ʾ������Ļ�������ʱ������ô����ַ���ʾ.��ʱ����������par[1]��par[2]������ֵ,���Ƿֱ����par[1]=par[0]+13;
					// par[2]=par[0]+17.�����������,���c���ַ�'l',��ôpar[0]���ǿ�ʼ����ʱ�ӳٵķ�����;���c���ַ�'b',��ôpar[0]�������õĴ����ַ�����ֵ.
					case 'l': 									/* blank interval */
					case 'b': 									/* bold attribute */
						  if (!((npar >= 2) &&
						  ((par[1] - 13) == par[0]) &&
						  ((par[2] - 17) == par[0])))
						    break;
						if ((c == 'l') && (par[0] >= 0) && (par[0] <= 60))
						{
						  blankinterval = HZ * 60 * par[0];
						  blankcount = blankinterval;
						}
						if (c == 'b')
						  vc_cons[currcons].vc_bold_attr = par[0];
				}
				break;
			// ESfunckey:��ʾ���յ��˼����Ϲ��ܼ�������һ������,������ʾ.���ǻָ�������״̬ESnormal.
			// ״̬ESfunckey��ʾ���յ��˼����Ϲ��ܼ�������һ������,������ʾ.���ǻָ�������״̬ESnormal.
			case ESfunckey:									// ���̹��ܼ���.
				state = ESnormal;
				break;
			// ESsetterm:��ʾ�����豸�����ַ�������״̬(DCS).��ʱ�յ��ַ�'S',��ָ���ʼ����ʾ�ַ�����.���յ����ַ���'L'��'l',�������������ʾ��ʽ.
			// ״̬ESsetterm��ʾ�����豸�����ַ�������״̬(DCS).��ʱ���յ��ַ�'S',��ָ���ʼ����ʾ�ַ�����.���յ����ַ���'L'��'l',������ر�������ʾ��ʽ.
			case ESsetterm:  								/* Setterm functions. */
				state = ESnormal;
				if (c == 'S') {
					def_attr = attr;
					video_erase_char = (video_erase_char & 0x0ff) | (def_attr << 8);
				} else if (c == 'L')
					; 										/*linewrap on*/
				else if (c == 'l')
					; 										/*linewrap off*/
				break;
			// ESsetgraph:��ʾ�յ������ַ�ת������'ESC ('��'ESC )'.���Ƿֱ�����ָ��G0��G1���õ��ַ���.��ʱ���յ��ַ�'0',��ѡ��ͼ���ַ�����ΪG0��G1,
			//            ���յ����ַ���'B',��ѡ����ͨASCII�ַ�����ΪG0��G1���ַ���.
			// ״̬ESsetgraph��ʾ�յ������ַ���ת������'ESC ('��'ESC )'.���Ƿֱ�����ָ��G0��G1���õ��ַ���.��ʱ���յ��ַ�'0',��ѡ��ͼ���ַ�����ΪG0��G1,����
			// �����ַ���'B',��ѡ����ͨASCII�ַ�����ΪG0��G1���ַ���.
			case ESsetgraph:								// 'CSI ( 0'��'CSI ( B' - ѡ���ַ���
				state = ESnormal;
				if (c == '0')
					translate = GRAF_TRANS;
				else if (c == 'B')
					translate = NORM_TRANS;
				break;
			default:
				state = ESnormal;
        }
    }
	set_cursor(currcons);									// �������������õĹ��λ��,������ʾ�������й��λ��.
}

/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 *
 * Reads the information preserved by setup.s to determine the current display
 * type and sets everything accordingly.
 */
/*
 * void con_init(void);
 *
 * ����ӳ����ʼ������̨�ж�,����ʲô������.�����������Ļ�ɾ��Ļ�,��ʹ���ʵ���ת���ַ����е���tty_write()����.
 * ��ȡsetup.s���򱣴����Ϣ,����ȷ����ǰ��ʾ������,��������������ز���.
 */
void con_init(void)
{
	register unsigned char a;
	char *display_desc = "????";
	char *display_ptr;
	int currcons = 0;								// ��ǰ�������̨��.
	long base, term;
	long video_memory;

	// ��ʼ����Ļ������
	video_num_columns = ORIG_VIDEO_COLS;
	// ��Ļÿ�е��ֽ���������Ļ��������2����Ϊһ����ʾ�ֽ���Ҫһ�������ֽ�
	video_size_row = video_num_columns * 2;
	// ��ʼ����Ļ������
	video_num_lines = ORIG_VIDEO_LINES;
	// ��ʼ����ʾҳ��
	video_page = ORIG_VIDEO_PAGE;
	// ���ô�ʱ��0��(currcons)��ʾ�ն˵Ĳ����ַ����Լ��ַ�
	video_erase_char = 0x0720;
	// ��ʼ������ʱ�����
	blankcount = blankinterval;

	// Ȼ�������ʾģʽ�ǵ�ɫ���ǲ�ɫ,�ֱ�������ʹ�õ���ʾ�ڴ���ʼλ���Լ���ʾ�Ĵ��������˿ںź���ʾ�Ĵ������ݶ˿ں�.�����õ�BIOS��ʾ��ʽ����7,
	// ���ʾ�ǵ�ɫ��ʾ��.
	if (ORIG_VIDEO_MODE == 7)					/* Is this a monochrome display? */
	{
		video_mem_base = 0xb0000;				// ���õ���ӳ���ڴ���ʼ��ַ.
		video_port_reg = 0x3b4;					// ���õ��������Ĵ����˿�.
		video_port_val = 0x3b5;					// ���õ������ݼĴ����˿�.

		// ���Ÿ���BIOS�ж�int 0x10����0x12��õ���ʾģʽ��Ϣ,�ж���ʾ���ǵ�ɫ��ʾ�����ǲ�ɫ��ʾ��.���������жϹ������õ���BX�Ĵ�������ֵ������
		// 0x10,��˵����EGA��.��˳�ʼ��ʾ����ΪEGA��ɫ.��ȻEGA�����н϶���ʾ�ڴ�,���ڵ�ɫ��ʽ�����ֻ�����õ�ַ��Χ��0xb0000~xb8000֮�����ʾ�ڴ�.
		// Ȼ������ʾ�������ַ���Ϊ'EGAm'.
		// ������ϵͳ��ʼ���ڼ���ʾ���������ַ�������ʾ����Ļ�����Ͻ�.
		// ע��,����ʹ����bx�ڵ����ж�int 0x10ǰ���Ƿ񱻸ı�ķ������жϿ�������.��BL���жϵ��ú�ֵ���ı�,��ʾ��ʾ��֧��ah=12h���ܵ���,��EGA�����
		// ������VGA��������ʾ��.���жϵ��÷���ֵĩ��,��ʾ��ʾ����֧���������,��˵����һ�㵥ɫ��ʾ��.
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAM;		// ������ʾ����(EGA��ɫ).
			video_mem_term = 0xb8000;			// ������ʾ�ڴ�ĩ�˵�ַ.
			display_desc = "EGAm";				// ������ʾ�����ַ���.
		}
		// ���BX�Ĵ�����ֵ����0x10,��˵���ǵ�ɫ��ʾ��MDA,����8KB��ʾ�ڴ�.
		else
		{
			video_type = VIDEO_TYPE_MDA;		// ������ʾ����(MDA��ɫ).
			video_mem_term = 0xb2000;			// ������ʾ�ڴ�ĩ�˵�ַ.
			display_desc = "*MDA";				// ������ʾ�����ַ���.
		}
	}
	// �����ʾ��ʽ��Ϊ7,˵���ǲ�ɫ��ʾ��.��ʱ�ı���ʽ��������ʾ�ڴ���ʼ��ַΪ0xb8000;��ʾ���������Ĵ����˿ڵ�ַΪ0x3d4;���ݼĴ����˿ڵ�ַΪ0x3d5.
	else										/* If not, it is color. */
	{
		can_do_colour = 1;						// ���ò�ɫ��ʾ��־.
		video_mem_base = 0xb8000;				// ��ʾ�ڴ���ʼ��ַ.
		video_port_reg	= 0x3d4;				// ���ò�ɫ��ʾ�����Ĵ����˿�.
		video_port_val	= 0x3d5;				// ���ò�ɫ��ʾ���ݼĴ����˿�.
		// ���ж���ʾ�����.���BX������0x10,��˵����EGA��ʾ��,��ʱ����32KB��ʾ�ڴ����(0xb8000~0xc0000).����˵����CGA��ʾ��,ֻ��ʹ��8KB��ʾ�ڴ�(
		// 0xb8000~0xba000).
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAC;		// ������ʾ����(EGA��ɫ).
			video_mem_term = 0xc0000;			// ������ʾ�ڴ�ĩ�˵�ַ.
			display_desc = "EGAc";				// ������ʾ�����ַ���.
		}
		else
		{
			video_type = VIDEO_TYPE_CGA;		// ������ʾ����(CGA).
			video_mem_term = 0xba000;			// ������ʾ�ڴ�ĩ�˵�ַ.
			display_desc = "*CGA";				// ������ʾ�����ַ���.
		}
	}
	// ���������㵱ǰ��ʾ���ڴ��Ͽ��Կ�����������̨����.Ӳ��������������̨������������ʾ�ڴ���video_memory����ÿ���������̨ռ�õ�
	// �ֽ���.ÿ���������̨ռ�õ���ʾ�ڴ���������Ļ��ʾ��video_num_lines����ÿ���ַ�ռ�е��ֽ���video_size_row.
	// ���Ӳ����������������̨��������ϵͳ�ȶ�������MAX_CONSOLES,�Ͱ��������̨��������ΪMAX_CONSOLES.��������������������̨
	// ����Ϊ0,������Ϊ1.
	// �������ʾ�ڴ��������жϳ����������̨�����õ�ÿ���������̨ռ����ʾ�ڴ��ֽ���.
	video_memory = video_mem_term - video_mem_base;
	// ����ʵ�ʵ���ʾ�ڴ�Ĵ�С������ʾ�����ն˵�ʵ������
	NR_CONSOLES = video_memory / (video_num_lines * video_size_row);
	// ��ʾ�ն˵����������MAX_CONSOLES,������tty.hͷ�ļ���
	if (NR_CONSOLES > MAX_CONSOLES)
		NR_CONSOLES = MAX_CONSOLES;
	// ��������������ʾ�ն�����Ϊ0������ʾ�ն���������Ϊ1
	if (!NR_CONSOLES)
		NR_CONSOLES = 1;
	video_memory /= NR_CONSOLES;				// ÿ���������̨ռ����ʾ�ڴ��ֽ���.

	/* Let the user known what kind of display driver we are using */

	// Ȼ����������Ļ�����Ͻ���ʾ�����ַ���.���õķ�����ֱ�ӽ��ַ���д����ʾ�ڴ����Ӧλ�ô�.���Ƚ���ʾָ��display_ptrָ����Ļ��1���Ҷ˲�
	// 4���ַ���(ÿ���ַ���2���ֽ�,��˼�8),Ȼ��ѭ�������ַ������ַ�,����ÿ����1���ַ����տ�1�������ֽ�.
	display_ptr = ((char *)video_mem_base) + video_size_row - 8;
	while (*display_desc)
	{
		*display_ptr++ = *display_desc++;
		display_ptr++;
	}

	/* Initialize the variables used for scrolling (mostly EGA/VGA)	*/
	/* ��ʼ�����ڹ����ı���(��Ҫ����EGA/VGA) */

	// ע��,��ʱ��ǰ�������̨��curcons�Ѿ�����ʼ��0.�������ʵ�����ǳ�ʼ��0���������̨�Ľṹvc_cons[0]�е������ֶ�ֵ.������������0�ſ���̨
	// ��Ĭ�Ϲ�����ʼλ��video_mem_start��Ĭ�Ϲ���ĩ���ڴ�λ��,ʵ��������Ҳ����0�ſ���̨ռ�õĲ�����ʾ�ڴ�����.Ȼ���ʼ������0���������̨��
	// �������Ժͱ�־ֵ.
	base = origin = video_mem_start = video_mem_base;						// Ĭ�Ϲ�����ʼ�ڴ�λ��.
	term = video_mem_end = base + video_memory;								// 0����Ļ�ڴ�ĩ��λ��.
	scr_end	= video_mem_start + video_num_lines * video_size_row;			// ����ĩ��λ��.
	top	= 0;																// ��ʼ���ù���ʱ�����к�.
	bottom	= video_num_lines;												// ��ʼ���ù���ʱ�����к�.
	attr = 0x07;															// ��ʼ������ʾ�ַ�����(�ڵװ���).
	def_attr = 0x07;														// ����Ĭ����ʾ�ַ�����.
	restate = state = ESnormal;												// ��ʼ��ת�����в�����ǰ����һ״̬.
	checkin = 0;
	ques = 0;																// �յ��ʺ��ַ���־.
	iscolor = 0;															// ��ɫ��ʾ��־.
	translate = NORM_TRANS;													// ʹ�õ��ַ���(��ͨASCII���).
	vc_cons[0].vc_bold_attr = -1;											// �����ַ����Ա�־(-1��ʾ����).

	// ��������0�ſ���̨��ǰ�������λ�ú͹���Ӧ���ڴ�λ��pos��,ѭ����������ļ����������̨�ṹ�Ĳ���ֵ.���˸���ռ�õ���ʾ�ڴ濪ʼ�ͽ���λ�ò�ͬ,
	// ���ǵĳ�ʼֵ�����϶���0�ſ���̨��ͬ.
	gotoxy(currcons, ORIG_X, ORIG_Y);
  	for (currcons = 1; currcons < NR_CONSOLES; currcons++) {
		vc_cons[currcons] = vc_cons[0];         							// ����0�Žṹ�Ĳ���.
		origin = video_mem_start = (base += video_memory);
		scr_end = origin + video_num_lines * video_size_row;
		video_mem_end = (term += video_memory);
		gotoxy(currcons, 0, 0);                           					// ��궼��ʼ������Ļ���Ͻ�λ��.
	}
	// ������õ�ǰǰ̨����̨����Ļԭ��(���Ͻ�)λ�ú���ʾ�������й����ʾλ��,�����ü����ж�0x21������������(&keyboard_inierrupt�Ǽ����жϴ������
	// ��ַ).Ȼ��ȡ���жϿ���оƬ8259A�жԼ����жϵ�����,������Ӧ���̷�����IRQ1�����ź�.���λ���̿�������������̿�ʼ��������.
	update_screen();														// ����ǰ̨ԭ�������ù��λ��.
	set_trap_gate(0x21, &keyboard_interrupt);								// �μ�system.h,���ü��̵�ϵͳ�ж���
	outb_p(inb_p(0x21) & 0xfd, 0x21);										// ȡ���Լ����жϵ�����,����IRQ1.
	a = inb_p(0x61);														// ��ȡ���̶˿�0x61(8255A�˿�PB).
	outb_p(a | 0x80, 0x61);													// ���ý�ֹ���̹���(λ7��λ).
	outb_p(a, 0x61);														// ��������̹���,���Ը�λ����.
}

// ���µ�ǰ����̨.
// ��ǰ̨����̨ת��Ϊfg_consoleָ�����������̨.fg_console�����õ�ǰ̨�������̨��.
// fg_console������tty.hͷ�ļ��ж��壬����������Ĭ��ʹ�õ���ʾ�ն�
void update_screen(void)
{
	set_origin(fg_console);													// ���ù�����ʼ��ʾ�ڴ��ַ.
	set_cursor(fg_console);													// ������ʾ�������й����ʾ�ڴ�λ��.
}

/* from bsd-net-2: */

// ֹͣ����
// ��λ8255A PB�˿ڵ�λ1��λ0.
void sysbeepstop(void)
{
	/* disable counter 2 */		/* ��ֹ��ʱ��2 */
	outb(inb_p(0x61)&0xFC, 0x61);
}

int beepcount = 0;		// ����ʱ��δ����.

// ��ͨ����
// 8255AоƬPB�˿ڵ�λ1�����������Ŀ����ź�;λ0����8253��ʱ��2���ź�,�ö�ʱ���������������������,��Ϊ����������Ƶ��.���Ҫʹ������
// ����,��Ҫ����:���ȿ���PB�˿�(0x61)λ1��λ0(��λ),Ȼ�����ö�ʱ��2ͨ������һ���Ķ�ʱƵ�ʼ���.
static void sysbeep(void)
{
	/* enable counter 2 */		/* ������ʱ��2 */
	outb_p(inb_p(0x61)|3, 0x61);
	/* set command for counter 2, 2 byte write */	/* �����ö�ʱ��2���� */
	outb_p(0xB6, 0x43);		// ��ʱ��оƬ�����ּĴ����˿�.
	/* send 0x637 for 750 HZ */	/* ����Ƶ��Ϊ720Hz,����Ͷ�ʱֵ0x637 */
	outb_p(0x37, 0x42);		// ͨ��2���ݶ˿ڷֱ��ͼ����ߵ��ֽ�
	outb(0x06, 0x42);
	/* 1/8 second */		/* ����ʱ��Ϊ1/8s */
	beepcount = HZ / 8;
}

// ������Ļ
// ����Ļ���ݸ��Ƶ�����ָ�����û�������arg�С�
// ����arg��������;��һ�����ڴ��ݿ���̨�ţ�������Ϊ�û�������ָ�롣
int do_screendump(int arg)
{
	char *sptr, *buf = (char *)arg;
	int currcons, l;

	// ����������֤�û��ṩ�Ļ�����������������������ʵ���չ��Ȼ����俪ʼ��ȡ������̨��currcons.
	// ���жϿ���̨����Ч�󣬾ͰѸÿ���̨��Ļ�������ڴ����ݸ��Ƶ��û��������С�
	verify_area(buf, video_num_columns * video_num_lines);
	currcons = get_fs_byte(buf);
	if ((currcons < 1) || (currcons > NR_CONSOLES))
		return -EIO;
	currcons--;
	sptr = (char *) origin;
	for (l = video_num_lines * video_num_columns; l > 0 ; l--)
		put_fs_byte(*sptr++, buf++);
	return(0);
}

// ��������
// ���û���blankIntervalʱ������û�а��κΰ���ʱ������Ļ����,�Ա�����Ļ.
void blank_screen()
{
	if (video_type != VIDEO_TYPE_EGAC && video_type != VIDEO_TYPE_EGAM)
		return;
	/* blank here. I can't find out how to do it, though */
}

// �ָ���������Ļ
// ���û������κΰ���ʱ,�ͻָ����ں���״̬����Ļ��ʾ����.
void unblank_screen()
{
	if (video_type != VIDEO_TYPE_EGAC && video_type != VIDEO_TYPE_EGAM)
		return;
	/* unblank here */
}

// ����̨��ʾ����
// �ú����������ں���ʾ����printk()(kernel/printk.c),�����ڵ�ǰǰ̨����̨����ʾ�ں���Ϣ.
// ��������ѭ��ȡ���������е��ַ�,�������ַ������Կ��ƹ���ƶ���ֱ����ʾ����Ļ��.
// ����b��null��β���ַ���������ָ�롣
void console_print(const char * b)
{
	int currcons = fg_console;
	char c;

	// ѭ����ȡ������b�е��ַ���
	while (c = *(b++)) {
		// �����ǰ�ַ�c�ǻ��з�����Թ��ִ�лس����в���
		if (c == 10) {
			// ���ص���ǰ�еĵ�0��
			cr(currcons);
			// �����ӵ�ǰ���ƶ�����һ��
			lf(currcons);
			continue;
		}
		// ����ǻس�������ֱ��ִ�лس�������Ȼ��ȥ������һ���ַ���
		if (c == 13) {
			cr(currcons);
			continue;
		}
		// �ڶ�ȡ��һ�����ǻس������ַ���������ֵ�ǰ�����λ��x�Ѿ�������Ļ��ĩ�ˣ����ù���۷�����һ�п�ʼ����
		// Ȼ����ַ��ŵ����������ʾ�ڴ�λ�ô���������Ļ����ʾ�������ٰѹ������һ��λ�ã�Ϊ��ʾ��һ���ַ���׼����
		if (x >= video_num_columns) {
			x -= video_num_columns;
			pos -= video_size_row;
			lf(currcons);
		}
		// �Ĵ���al������Ҫ��ʾ���ַ�������������ֽڷŵ�ah�У�Ȼ���ax���ݴ洢������ڴ�λ��pos�������ڹ�괦��ʾ�ַ���
		__asm__("movb %2, %%ah\n\t"              				// �����ֽڷŵ�ah�С�
			"movw %%ax, %1\n\t"              					// ax���ݷŵ�pos����
			::"a" (c),
			"m" (*(short *)pos),
			"m" (attr)
			:);
		pos += 2;
		x++;
	}
	set_cursor(currcons);           							// ������õĹ���ڴ�λ�ã�������ʾ�������й��λ�á�
}


void del_col(int i){
	int currcons = fg_console;
	gotoxy(currcons, x - i, y);
	csi_P(currcons, i);
}

