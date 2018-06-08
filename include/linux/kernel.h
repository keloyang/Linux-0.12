/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
/*
 * 'kernel.h'������һЩ�ں˳��ú�����ԭ�͵�.
 */

// ��֤������ַ��ʼ�����ڿ��Ƿ��ޡ���������׷���ڴ档��kernel/fork.c����
void verify_area(void * addr,int count);
void panic(const char * str);                   // ��ʾ�ں˳�����Ϣ,Ȼ�������ѭ��(kernel/panic.c)
void do_exit(long error_code);                  // �����˳�����(kernel/exit.c)
int printf(const char * fmt, ...);              // ��׼��ӡ��ʾ��������init/main.c��
int printk(const char * fmt, ...);              // �ں�ר�õĴ�ӡ��Ϣ������������printf()��ͬ��(kernel/printk.c)
void console_print(const char * str);           // ����̨��ʾ������(kernel/chr_drv/console.c)
int tty_write(unsigned ch,char * buf,int count);// ��tty��дָ�����ȵ��ַ�������kernel/chr_drv/tty_io.c��
void * malloc(unsigned int size);               // ͨ���ں��ڴ���亯������lib/malloc.c��
void free_s(void * obj, int size);              // �ͷ�ָ������ռ�õ����ڡ���lib/malloc.c��
extern void hd_times_out(void);                 // Ӳ�̴���ʱ����kernel/blk_drv/hd.c��
extern void sysbeepstop(void);                  // ֹͣ��������kernel/chr_drv/console.c��
extern void hd_times_out(void);                 // Ӳ�̴���ʱ(kernel/blk_drv/hd.c)
extern void sysbeepstop(void);                  // ֹͣ����(kernel/chr_drv/console.c)
extern void blank_screen(void);                 // ��������.(kernel/chr_drv/console.c)
extern void unblank_screen(void);               // �ָ�����������Ļ.(kernel/chr_drv/console.c)

extern int beepcount;		                    // ����ʱ��δ����(kernel/chr_drv/console.c)
extern int hd_timeout;		                    // Ӳ�̳�ʱ�δ�ֵ(kernel/blk_drv/blk.h)
extern int blankinterval;	                    // �趨����Ļ�������ʱ��
extern int blankcount;		                    // ����ʱ�����(kernel/chr_drv/console.c)

// ��ӡ��Ϣ����־�ȼ�
#define LOG_INFO_TYPE       0
// ��ӡdebug��־��Ϣ�ȼ�
#define LOG_DEBUG_TYPE      1
// ��ӡ������Ϣ��־�ȼ�
#define LOG_WARN_TYPE       2

extern void Log(unsigned short log_level, const char *fmt, ...);

#define free(x) free_s((x), 0)

/*
 * This is defined as a macro, but at some point this might become a
 * real subroutine that sets a flag if it returns true (to do
 * BSD-style accounting where the process is flagged if it uses root
 * privs).  The implication of this is that you should do normal
 * permissions checks first, and check suser() last.
 */
/*
 * ���溯�����Ժ����ʽ�����,������ĳ�������������Գ�Ϊһ���������ӳ���,���������trueʱ�������ñ�־(���ʹ��root�û�Ȩ��
 * �Ľ��������˱�־,������ִ��BSD��ʽ�����˴���).����ζ����Ӧ������ִ�г���Ȩ�޼��,����ټ��suser().
 */
#define suser() (current->euid == 0)		// ����Ƿ�Ϊ�����û�.

