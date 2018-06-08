/*
 *  linux/init/main.c
 *
 *  (C) 1991  Linus Torvalds
 */
// ��������Э������
#define EM
// �����"__LIBRARY__"��Ϊ�˰���������unistd.h�е���Ƕ���������Ϣ.
#define __LIBRARY__
// *.hͷ�ļ����ڵ�Ĭ��Ŀ¼��include/,���ڴ����оͲ�����ȷָ����λ��.�������UNIX�ı�׼ͷ�ļ�,����Ҫָ�����ڵ�Ŀ¼,����˫�º�
// ��ס.unitd.h�Ǳ�׼���ų����������ļ�.���ж����˸��ַ��ų���������,�������˸��ֺ���.����������˷���__LIBRARY__,�򻹻����
// ϵͳ���úź���Ƕ������syscall0()��.
#include <unistd.h>
#include <time.h>			//��ʱ������ͷ�ļ�.������Ҫ������tm�ṹ��һЩ�й�ʱ��ĺ���ԭ��.

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
/*
 * ������Ҫ������Щ��Ƕ��� - ���ں˿ռ䴴�����̽�����û��дʱ����(COPY ON WRITE)!!!ֱ��ִ��һ��execve���á�
 * ��Զ�ջ���ܴ������⡣����������fork()���ú���main()ʹ���κζ�ջ����˾Ͳ����к������� - ����ζ��fork
 * ҲҪʹ����Ƕ���룬���������ڴ�fork()�˳�ʱ��Ҫʹ�ö�ջ�ˡ�
 *
 * ʵ����ֻ��pause��fork��Ҫʹ����Ƕ��ʽ���Ա�֤��main()�в���Ū�Ҷ�ջ����������ͬʱ������������һЩ������
 */
// Linux���ں˿ռ䴴������ʱ��ʹ��дʱ���Ƽ���(Copy on write)��main()���ƶ����û�ģʽ��������0����ִ����Ƕ
// ��ʽ��fork()��pause(),��˿ɱ�֤��ʹ������0���û�ջ����ִ��move_to_user_mode()֮�󣬱�����main()��������
// 0������������ˡ�������0�����н������ӽ��̵ĸ����̡����ִ���һ���ӽ���ʱ��init���̣�����������1���������ں˿ռ䣬
// ���û��ʹ��дʱ���ƹ��ܡ���ʱ����0���û�ջ��������1���û�ջ�������ǹ�ͬʹ��һ��ջ�ռ䡣���ϣ����main.c������
// ����0�Ļ�����ʱ��Ҫ�жԶ�ջ���κβ���������Ū�Ҷ�ջ�������ٴ�ִ��fork()��ִ�й�execve()�����󣬱����س����Ѳ�����
// �ں˿ռ䣬��˿���ʹ��дʱ���Ƽ����ˡ�

// static inline���εĺ�������������󲿷ֱ��ֺ���ͨ��static����һ����ֻ�����ڵ������ֺ�����ʱ��gcc������
// ���ô���������չ���������Ϊ����������ɶ����Ļ����

// �����_syscall0()��unistd.h�е���Ƕ�����.��Ƕ�������ʽ����Linux��ϵͳ�����ж�0x80.���ж�������ϵͳ���õ����.
// �������ʵ������int fork()��������ϵͳ����.��չ����֮�ͻ���������.syscall0����������0��ʾ�޲���,1��ʾ1������.
// __attribute__�������ú�������,����������β��������֮ǰ
// �������Կ��԰��������߰�һЩ������ӵ����������У��Ӷ�����ʹ�������ڴ����鷽��Ĺ��ܸ�ǿ��
// __attribute__((always_inline))��ʾ������ǿ������Ϊ��������
// int fork(void) __attribute__((always_inline));
//  int pause()ϵͳ���ã���ͣ���̵�ִ�У�ֱ���յ�һ���źš�
// int pause(void) __attribute__((always_inline));
// fork()ϵͳ���ú����Ķ���
_syscall0(int, fork)
// pause()ϵͳ���ú����Ķ���
_syscall0(int, pause)
// int setup(void * BIOS)ϵͳ����,������linux��ʼ��(������������б�����).
_syscall1(int, setup, void *, BIOS)
// int sync()ϵͳ���ã������ļ�ϵͳ��
_syscall0(int, sync)

#include <linux/tty.h>                  			// ttyͷ�ļ����������й�tty_io������ͨ�ŷ���Ĳ���������
#include <linux/sched.h>							// ���ȳ���ͷ�ļ�,����������ṹtask_struct,��1����ʼ���������.����һЩ�Ժ��
													// ��ʽ������й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������.
//#include <linux/head.h>
#include <asm/system.h>								// ϵͳͷ�ļ�.���������û��޸�������/�ж��ŵȵ�Ƕ��ʽ����.
#include <asm/io.h>									//��ioͷ�ļ�.�Ժ��Ƕ���������ʽ�����io�˿ڲ����ĺ���.

#include <stddef.h>                     			// ��׼����ͷ�ļ���������NULL��offsetof(TYPE,MEMBER)��
#include <stdarg.h>									// ��׼����ͷ�ļ�.�Ժ����ʽ������������б�.��Ҫ˵����һ������(va_list)������
													// ��(va_start,va_arg��va_end),vsprintf,vprintf,vfprintf.
#include <unistd.h>
#include <fcntl.h>                      			// �ļ�����ͷ�ļ�.�����ļ������������Ĳ������Ƴ������ŵĶ���
//#include <sys/types.h>

#include <linux/fs.h>								// �ļ�ϵͳͷ�ļ�.�����ļ���ṹ(file,buffer_head,m_inode��).
													// �����ж���:extern int ROOT_DEV.

#include <linux/kernel.h>							// �ں�ͷ�ļ�

#include <string.h>									// �ַ���ͷ�ļ�.��Ҫ������һЩ�й��ڴ���ַ���������Ƕ�뺯��.

static char printbuf[1024];							// ��̬�ַ�������,�����ں���ʾ��Ϣ�Ļ���.

extern char *strcpy();
extern int vsprintf();								// �͸�ʽ�������һ�ַ�����(vsprintf.c)
extern void init(void);								// ����ԭ��,��ʼ��
extern void blk_dev_init(void);						// ���豸��ʼ���ӳ���(blk_drv/ll_rw_blk.c)
extern void chr_dev_init(void);						// �ַ��豸��ʼ��(chr_drv/tty_io.c)
extern void hd_init(void);							// Ӳ�̳�ʼ������(blk_drv/hd.c)
extern void floppy_init(void);						// ������ʼ������(blk_drv/floppy.c)
extern void mem_init(long start, long end);			// �ڴ�����ʼ��(mm/memory.c)
extern long rd_init(long mem_start, int length);	// �����̳�ʼ��(blk_drv/ramdisk.c)
extern long kernel_mktime(struct tm * tm);			// ����ϵͳ��������ʱ��(��)

// forkϵͳ���ú���,�ú�����Ϊstatic inline��ʾ������������Ҫ�����ڽ���0���洴������1��ʱ��������ʹ����0�����ɽ���1��ʱ��
// ��ʹ���Լ����û���ջ
static inline long fork_for_process0() {
	long __res;
	__asm__ volatile (
		"int $0x80\n\t"  														/* ����ϵͳ�ж�0x80 */
		: "=a" (__res)  														/* ����ֵ->eax(__res) */
		: "0" (2));  															/* ����Ϊϵͳ�жϵ��ú�__NR_name */
	if (__res >= 0)  															/* �������ֵ>=0,��ֱ�ӷ��ظ�ֵ */
		return __res;
	errno = -__res;  															/* �����ó����,������-1 */
	return -1;
}

// �ں�ר��sprintf()����.�ú������ڲ�����ʽ����Ϣ�������ָ��������str��.����'*fmt'ָ����������ø�ʽ.
static int sprintf(char * str, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(str, fmt, args);
	va_end(args);
	return i;
}

/*
 * This is set up by the setup-routine at boot-time
 */
/*
 * ������Щ���������ں������ڼ���setup.s�������õ�.
 */
 // �������зֱ�ָ�������Ե�ַǿ��ת��Ϊ�����������͵�ָ��,����ȡָ����ָ����.�����ں˴���α�ӳ�䵽�������ַ�㿪ʼ�ĵط�,���
 // ��Щ������ַ����Ҳ�Ƕ�Ӧ�������ַ.
#define EXT_MEM_K (*(unsigned short *)0x90002)							// 1MB�Ժ����չ�ڴ��С(KB).
#define CON_ROWS ((*(unsigned short *)0x9000e) & 0xff)					// ѡ���Ŀ���̨��Ļ��,����
#define CON_COLS (((*(unsigned short *)0x9000e) & 0xff00) >> 8)
#define DRIVE_INFO (*((struct drive_info *)0x90080))					// Ӳ�̲�����32�ֽ�����.
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)						// ���ļ�ϵͳ�����豸��.
#define ORIG_SWAP_DEV (*(unsigned short *)0x901FA)						// �����ļ������豸��.

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */
// ��κ��ȡCMOSʵʱʱ����Ϣ.outb_p��inb_p��include/asm/io.h�ж���Ķ˿��������
#define CMOS_READ(addr) ({ \
	outb_p(0x80 | addr, 0x70); 					/* 0x70��д��ַ�˿ں�,0x80|addr��Ҫ��ȡ��CMOS�ڴ��ַ.*/\
	inb_p(0x71); 								/* 0x71�Ƕ����ݶ˿ں�.*/\
})

// �����.��BCD��ת���ɶ�����ֵ.BCD�����ð���ֽ�(4λ)��ʾһ��10������,���һ���ֽڱ�ʾ2��10������.(val)&15ȡBCD
// ��ʾ��10���Ƹ�λ��,��(val)>>4ȡBCD��ʾ��10����ʮλ��,�ٳ���10.������������Ӿ���һ���ֽ�BCD���ʵ�ʶ�������ֵ.
#define BCD_TO_BIN(val) ((val) = ((val)&15) + ((val) >> 4) * 10)

// �ú���ȡCMOSʵʱ����Ϣ��Ϊ����ʱ��,�����浽ȫ�ֱ���startup_time(��)��.���е��õĺ���kernel_mktime()���ڼ����
// 1970��1��1��0ʱ�𵽿������վ���������,��Ϊ����ʱ��.
static void time_init(void)
{
	struct tm time;								// ʱ��ṹtm������include/time.h��
	// CMOS�ķ����ٶȺ���.Ϊ�˼�Сʱ�����,�ڶ�ȡ������ѭ����������ֵ��,����ʱCMOS����ֵ�˱仯,��ô�����¶�ȡ����ֵ.�����ں�
	// ���ܰ���CMOSʱ����������1��֮��.
	do {
		time.tm_sec = CMOS_READ(0);				// ��ǰʱ����ֵ(����BCD��ֵ)
		time.tm_min = CMOS_READ(2);				// ��ǰ����ֵ.
		time.tm_hour = CMOS_READ(4);			// ��ǰСʱֵ.
		time.tm_mday = CMOS_READ(7);			// һ���еĵ�������.
		time.tm_mon = CMOS_READ(8);				// ��ǰ�·�(1-12)
		time.tm_year = CMOS_READ(9);			// ��ǰ���.
	} while (time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(time.tm_sec);					// ת���ɽ�������ֵ.
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
	time.tm_mon--;								// tm_mon���·ݷ�Χ��0~11.
	startup_time = kernel_mktime(&time);		// ���㿪��ʱ��.kernel/mktime.c
}

 // ���涨��һЩ�ֲ�����.
static long memory_end = 0;						// �������е������ڴ�����(�ֽ���).
static long buffer_memory_end = 0;				// ���ٻ�����ĩ�˵�ַ.
static long main_memory_start = 0;				// ���ڴ�(�����ڷ�ҳ)��ʼ��λ��.
static char term[32];							// �ն������ַ���(��������).

// ��ȡ��ִ��/etc/rc�ļ�ʱ��ʹ�õ������в����ͻ�������.
static char * argv_rc[] = { "/bin/sh", NULL };		// ����ִ�г���ʱ�������ַ�������.
static char * envp_rc[] = { "HOME=/", NULL ,NULL };	// ����ִ�г���ʱ�Ļ����ַ�������.

// ���е�¼shellʱ��ʹ�õ������в����ͻ�������.
static char * argv[] = { "-/bin/sh",NULL };			// �ַ�"-"�Ǵ��ݸ�shell����sh��һ����־.ͨ��ʶ��ñ�־,sh�������Ϊ��¼
													// shellִ��.��ִ�й�����shell��ʾ����ִ��sh��һ��.
static char * envp[] = { "HOME=/usr/root", NULL, NULL };

struct drive_info { char dummy[32]; } drive_info;	// ���ڴ��Ӳ�̲�������Ϣ.

// �ں˳�ʼ��������.��ʼ��������������0(idle���񼴿�������)���������.
// Ӣ��ע�ͺ�����"����ȷʵ��void,û��.��startup����(head.s)�о������������".�μ�head.h�������.
int main(void)										/* This really IS void, no error here. */
{													/* The startup routine assumes (well, ...) this */
#ifdef EM
	// ��������Э������
	__asm__("movl %cr0,%eax \n\t" \
	        "xorl $6,%eax \n\t" \
	        "movl %eax,%cr0");
#endif
	/*
	 * Interrupts are still disabled. Do necessary setups, then
	 * enable them
	 */
	/*
	 * ��ʱ�ж��Ա���ֹ��,�����Ҫ�����ú�ͽ��俪��.
	 */
	// ���ȱ�����ļ�ϵͳ�豸�ͽ����ļ��豸��,������setup.s�����л�ȡ����Ϣ���ÿ���̨�ն���Ļ��,������������TERM,���������ó�ʼinit����
	// ��ִ��etc/rc�ļ���shell����ʹ�õĻ�������,�Լ������ڴ�0x90080����Ӳ�̱�.
	// ����ROOT_DEV����ǰ���������include/linux/fs.h�ļ��ϱ�����Ϊextern_int
	// ��SWAP_DEV��include/linux/mm.h�ļ���Ҳ������ͬ����.����mm.h�ļ���û����ʽ�����ڱ�����ǰ��,��Ϊǰ���������include/linux/sched.h
	// �ļ����Ѿ�������.
 	ROOT_DEV = ORIG_ROOT_DEV;										// ROOT_DEV������fs/super.c
 	SWAP_DEV = ORIG_SWAP_DEV;										// SWAP_DEV������mm/swap.c
   	sprintf(term, "TERM=con%dx%d", CON_COLS, CON_ROWS);
	envp[1] = term;
	envp_rc[1] = term;
    drive_info = DRIVE_INFO;										// �����ڴ�0x90080����Ӳ�̲�����.

	// ���Ÿ��ݻ��������ڴ��������ø��ٻ����������ڴ��λ�úͷ�Χ.
	// ���ٻ���ĩ�˵�ַ->buffer_memory_end;�����ڴ�����->memory_end;���ڴ濪ʼ��ַ->main_memory_start.
	// ���������ڴ��С
	memory_end = (1 << 20) + (EXT_MEM_K << 10);						// �ڴ��С=1MB + ��չ�ڴ�(k)*1024�ֽ�.
	memory_end &= 0xfffff000;										// ���Բ���4KB(1ҳ)���ڴ���.
	if (memory_end > 16 * 1024 * 1024)								// ����ڴ�������16MB,��16MB��.
		memory_end = 16 * 1024 * 1024;
	// ���������ڴ�Ĵ�С���ø��ٻ���ȥ��ĩ�˴�С
	if (memory_end > 12 * 1024 * 1024) 								// ����ڴ�>12MB,�����û�����ĩ��=4MB
		buffer_memory_end = 4 * 1024 * 1024;
	else if (memory_end > 6 * 1024 * 1024)							// �������ڴ�>6MB,�����û�����ĩ��=2MB
		buffer_memory_end = 2 * 1024 * 1024;
	else
		buffer_memory_end = 1 * 1024 * 1024;						// ���������û�����ĩ��=1MB
	// ���ݸ��ٻ�������ĩ�˴�С�������ڴ�������ʼ��ַ
	main_memory_start = buffer_memory_end;							// ���ڴ���ʼλ�� = ������ĩ��
	// �����Makefile�ļ��ж������ڴ������̷���RAMDISK,���ʼ��������.��ʱ���ŵ㽫����.
	// �μ�kernel/blk_drv/ramdisk.c.
#ifdef RAMDISK
	main_memory_start += rd_init(main_memory_start, RAMDISK * 1024);
#endif
	// �������ں˽������з���ĳ�ʼ������.
	mem_init(main_memory_start, memory_end);						// ���ڴ�����ʼ��.(mm/memory.c)
	trap_init();                                    				// ������(Ӳ���ж�����)��ʼ��.(kernel/traps.c)
	blk_dev_init();													// ���豸��ʼ��.(blk_drv/ll_rw_blk.c)
	chr_dev_init();													// �ַ��豸��ʼ��.(chr_drv/tty_io.c)
 	tty_init();														// tty��ʼ��(chr_drv/tty_io.c)
	time_init();													// ���ÿ�������ʱ��.
 	sched_init();													// ���ȳ����ʼ��(��������0��tr,ldtr)(kernel/sched.c)
	buffer_init(buffer_memory_end);									// ��������ʼ��,���ڴ������.(fs/buffer.c)
	hd_init();														// Ӳ�̳�ʼ��.	(blk_drv/hd.c)
	floppy_init();													// ������ʼ��.	(blk_drv/floppy.c)
	sti();															// ���г�ʼ������������,���ǿ����ж�.
	// ��ӡ�ں˳�ʼ�����
	//Log(LOG_INFO_TYPE, "<<<<< Linux0.12 Kernel Init Finished, Ready Start Process0 >>>>>\n");
	// �������ͨ���ڶ�ջ�����õĲ���,�����жϷ���ָ����������0ִ��.
	move_to_user_mode();											// �Ƶ��û�ģʽ��ִ��.(include/asm/system.h)
	if (!fork_for_process0()) {										/* we count on this going ok */
		init();														// ���½����ӽ���(����1��init����)��ִ��.
	}
	/*
	 *   NOTE!!   For any other task 'pause()' would mean we have to get a
	 * signal to awaken, but task0 is the sole exception (see 'schedule()')
	 * as task 0 gets activated at every idle moment (when no other tasks
	 * can run). For task0 'pause()' just means we go check if some other
	 * task can run, and if not we return here.
	 */
	/*
	 * ע��!!�����κ���������,'pause()'����ζ�����Ǳ���ȴ��յ�һ���źŲŻ᷵�ؾ���̬,������0(task0)��Ψһ�������(�μ�'schedule()'),��Ϊ
	 * ����0���κο���ʱ���ﶼ�ᱻ����(��û����������������ʱ),��˶�������0'pause()'����ζ�����Ƿ������鿴�Ƿ������������������,���û�еĻ�
	 * ���Ǿͻص�����,һֱѭ��ִ��'pause()'.
	 */
	// pause()ϵͳ����(kernel/sched.c)�������0ת���ɿ��жϵȴ�״̬,��ִ�е��Ⱥ���.���ǵ��Ⱥ���ֻҪ����ϵͳ��û�����������������ʱ�ͻ��л�
	// ������0,�ǲ�����������0��״̬.
	for(;;)
		__asm__("int $0x80"::"a" (__NR_pause):);					// ��ִ��ϵͳ����pause().
}

// ���溯��������ʽ����Ϣ���������׼����豸stdout(1),������ָ��Ļ����ʾ.����'*fmt'ָ����������õĸ�ʽ,�μ���׼C�����鼮.
// ���ӳ���������vsprintf���ʹ�õ�һ��������.�ó���ʹ��vsprintf()����ʽ�����ַ�������printbuf������,Ȼ����write()��
// �������������������׼�豸(1--stdout).vsprintf()������ʵ�ּ�kernel/vsprintf.c.
int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1, printbuf, i = vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

// ��main()���Ѿ�������ϵͳ��ʼ��,�����ڴ����,����Ӳ���豸����������.init()����������0��1�δ������ӽ���(����1)��.�����ȶԵ�һ����Ҫִ��
// �ĳ���(shell)�Ļ������г�ʼ��,Ȼ���Ե�¼shell��ʽ���س���ִ��֮.
void init(void)
{
	int pid, i, fd;
	// setup()��һ��ϵͳ����.���ڶ�ȡӲ�̲����ͷ�������Ϣ������������(�����ڵĻ�)�Ͱ�װ���ļ�ϵͳ�豸.�ú�����25���ϵĺ궨��,�Ծͺ�����sys_setup(),
	// �ڿ��豸��Ŀ¼kernel/blk_drv/hd.c.
	setup((void *) &drive_info);
	// �����Զ�д���ʷ�ʽ���豸"/dev/tty0",����Ӧ�ն˿���̨.�������ǵ�һ�δ��ļ�����,��˲������ļ������(�ļ�������)�϶���0.�þ����UNIX�����
	// ϵͳĬ�ϵĿ���̨��׼������stdin.�����ٰ����Զ���д�ķ�ʽ�ֱ����Ϊ�˸��Ʋ�����׼���(д)���stdout�ͱ�׼����������stderr.����ǰ���"(void)"
	// ǰ׺���ڱ�ʾǿ�ƺ������践��ֵ.
	(void) open("/dev/tty1", O_RDWR, 0);
	(void) dup(0);													// ���ƾ��,�������1��--stdout��׼����豸.
	(void) dup(0);													// ���ƾ��,�������2��--stderr��׼��������豸.
	// ����1ִ�е��û�������Ŀ�ʼ
	//printf("<<<<< Process 1 console fd = %d >>>>>\n", fd);
	// �����ӡ���������������ֽ���,ÿ��1024�ֽ�,�Լ����ڴ��������ڴ��ֽ���.
	//printf("<<<<< %d buffers = %d bytes buffer space >>>>>\n\r", NR_BUFFERS,
	//		NR_BUFFERS * BLOCK_SIZE);
	//printf("<<<<< Free mem: %d bytes >>>>>\n\r", memory_end - main_memory_start);
	// ����fork()���ڴ���һ���ӽ���(����2).���ڱ��������ӽ���,fork()������0ֵ,����ԭ����(������)�򷵻��ӽ��̵Ľ��̺�pid.���Ե�202--206�����ӽ���ִ�е�����.
	// ���ӽ��̹ر��˾��0(stdin),��ֻ����ʽ��/etc/rc�ļ�,��ʹ��execve()���������������滻��/bin/sh����(��shell����),Ȼ��ִ��/bin/sh����.��Я���Ĳ���
	// �ͻ��������ֱ���argv_rc��envp_rc�������.�رվ��0�����̴�/etc/rc�ļ��������ǰѱ�׼����stdin�ض���/etc/rc/�ļ�.����shell����/bin/sh�Ϳ�������
	// rc�ļ������õ�����.��������sh�����з�ʽ�Ƿǽ���ʽ��,�����ִ����rc�ļ��е������ͻ������˳�,����2Ҳ��֮����.����execve()����˵����μ�fs/exec.c����.
	// ����_exit()�˳�ʱ�ĳ�����1 - ����δ���;2 -- �ļ���Ŀ¼������.
	if (!(pid = fork())) {
		close(0);
		if (open("/etc/rc", O_RDONLY, 0))
			_exit(1);												// �����ļ�ʧ��,���˳�(lib/_exit.c).
		execve("/bin/sh", argv_rc, envp_rc);						// �滻��/bin/sh����ִ��.
		_exit(2);													// ��execve()ִ��ʧ�����˳�.
    }
	// ���滹�Ǹ����̣�1��ִ�е���䡣wait()�ȴ��ӽ���ֹͣ����ֹ������ֵӦ���ӽ��̵Ľ��̺ţ�pid)��������������Ǹ����̵ȴ��ӽ���
	// �Ľ�����&i�Ǵ�ŷ���״̬��Ϣ��λ�á����wait()����ֵ�������ӽ��̺ţ�������ȴ���
  	if (pid > 0)
		while (pid != wait(&i));
	// ���ִ�е����˵���մ������ӽ��̵�ִ����ֹͣ����ֹ�ˡ�����ѭ���������ٴ���һ���ӽ��̣������������ʾ����ʼ�����򴴽��ӽ���
	// ʧ�ܡ���Ϣ������ִ�С��������������ӽ��̽��ر�������ǰ�������ľ����stdin��stdout��stderr�����´���һ���Ự�����ý�����ţ�
	// Ȼ�����´�/dev/tty0��Ϊstdin�������Ƴ�stdout��stderr���ٴ�ִ��ϵͳ���ͳ���/bin/sh�������ִ����ѡ�õĲ����ͻ���������
	// ѡ��һ�ס�Ȼ�󸸽����ٴ�����wait()�ȵȡ�����ӽ�����ֹͣ��ִ�У����ڱ�׼�������ʾ������Ϣ���ӽ���pidֹͣ�����У���������i����
	// Ȼ�����������ȥ...���γɡ�����ѭ����
	while (1) {
		if ((pid = fork()) < 0) {
			printf("Fork failed in init %c\r\n", ' ');
			continue;
		}
		if (!pid) {                             					// �µ��ӽ��̡�
			close(0); close(1); close(2);
			setsid();                       						// ����һ�µĻỰ�ڣ�������˵����
			(void) open("/dev/tty1", O_RDWR, 0);
			(void) dup(0);
			(void) dup(0);
			_exit(execve("/bin/sh", argv, envp));
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r", pid, i);
		sync();
	}
	_exit(0);														/* NOTE! _exit, not exit() */
}





