#ifndef _BLK_H
#define _BLK_H

#include <linux/kernel.h>
#include <linux/sched.h>

#define NR_BLK_DEV	7	// ���豸��������.
/*
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 *
 * 32 seems to be a reasonable number: enough to get some benefit
 * from the elevator-mechanism, but not so much as to lock a lot of
 * buffers when they are in the queue. 64 seems to be too many (easily
 * long pauses in reading when heavy writing/syncing is going on)
 */
/*
 * ���涨���NR_REQUEST�����������������������.
 * ע��,д������ʹ����Щ���еͶ˵�2/3��;���������ȴ���.
 *
 * 32�������һ�����������:�����Ѿ��㹻�ӵ����㷨�л�úô�,�����������ڶ����ж���סʱ�ֲ��Ե��Ǻܴ����.64�Ϳ���ȥ̫����(
 * ��������д/ͬ����������ʱ����������ʱ�����ͣ).
 */
#define NR_REQUEST	32

/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
/*
 * OK,������request�ṹ��һ����չ��ʽ,�����ʵ���Ժ����ǾͿ����ڷ�ҳ������ʹ��ͬ����request�ṹ.�ڷ�ҳ������'bh'��NULL,
 * ��'waiting'�����ڵȴ���/д�����.
 */
struct request {
	int dev;							/* -1 if no request */	// ��������豸��
	int cmd;							/* READ or WRITE */	// READ��WRITE����.
	int errors;             			// ����ʱ�����Ĵ������.
	unsigned long sector;   			// ��ʼ����.(1��=2����)
	unsigned long nr_sectors;			// ��/д������.
	char * buffer;                  	// ���ݻ�����.
	struct task_struct * waiting;   	// ����ȴ�������ɲ����ĵط�(����).
	struct buffer_head * bh;        	// ������ͷָ��(include/linux/fs.h).
	struct request * next;          	// ָ����һ������.
};

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 */
/*
 * ����Ķ������ڵ����㷨:ע�������������д����֮ǰ����.���Ǻ���Ȼ��:��������ʱ���Ҫ��Ҫ��д�����ϸ�ö�.
 */
// ������в���s1��s2��ȡֵ�����涨�������ṹrequest��ָ��.�ú궨�����ڸ�����������ָ����������ṹ�е���Ϣ(����cmd(READ��WRITE),
// �豸��dev�Լ���������������sector)���жϳ�����������ṹ��ǰ������˳��.���˳���������ʿ��豸ʱ��������ִ��˳��.�������ڳ���
// blk_drv/ll_rw_blk.c�к���add_request()�б�����.�ò��ֵ�ʵ����I/O���ȹ���,��ʵ���˶��������������(��һ����������ϲ�����).
#define IN_ORDER(s1, s2) \
((s1)->cmd < (s2)->cmd || (s1)->cmd == (s2)->cmd && \
((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
(s1)->sector < (s2)->sector)))

// ���豸����ṹ.
struct blk_dev_struct {
	void (*request_fn)(void);							// ��������ָ��
	struct request * current_request;					// ��ǰ���������ṹ.
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];       // ���豸��(����).ÿ�ֿ��豸ռ��һ��,��7��.
extern struct request request[NR_REQUEST];              // �����������,��32��.
extern struct task_struct * wait_for_request;           // �ȴ�����������Ľ��̶���ͷָ��.

// �豸���ݿ�����ָ������.ÿ��ָ����ָ��ָ�����豸�ŵ��ܿ�����hd_sizes[].���ܿ�������ÿһ���Ӧ���豸��ȷ����һ�����豸����ӵ�е�
// ���ݿ�����(1���С=1KB).
extern int * blk_size[NR_BLK_DEV];

// �ڿ��豸��������(��hd.c)������ͷ�ļ�ʱ,�����ȶ��������������豸�����豸��.����,�������о���Ϊ�������ļ����������������ȷ�ĺ궨��.
#ifdef MAJOR_NR		// ���豸��.

/*
 * Add entries as needed. Currently the only block devices
 * supported are hard-disks and floppies.
 */
/*
 * ��Ҫʱ������Ŀ.Ŀǰ���豸��֧��Ӳ�̺�����(����������).
 */

// ���������MAJOR_NR=1(RAM�����豸��),���������·��ų����ͺ�.
#if (MAJOR_NR == 1)
/* ram disk */
#define DEVICE_NAME "ramdisk"									// �豸����("�ڴ�������")
#define DEVICE_REQUEST do_rd_request							// �豸���������.
#define DEVICE_NR(device) ((device) & 7)						// �豸��(0 - 7)
#define DEVICE_ON(device)  										// �����豸(���������뿪���͹ر�).
#define DEVICE_OFF(device)										// �ر��豸.

// ����,���������MAJOR_NR = 2(�������豸��),���������·��ų����ͺ�.
#elif (MAJOR_NR == 2)
/* floppy */
#define DEVICE_NAME "floppy"									// �豸����("����������")
#define DEVICE_INTR do_floppy									// �豸�жϴ�����
#define DEVICE_REQUEST do_fd_request							// �豸���������
#define DEVICE_NR(device) ((device) & 3)						// �豸��(0 - 3)
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))			// �����豸��
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))		// �ر��豸��.

// ����,���������MAJOR_NR = 3(Ӳ�����豸��),���������·��ų����ͺ�
#elif (MAJOR_NR == 3)
/* harddisk */
#define DEVICE_NAME "harddisk"									// �豸����("Ӳ��")
#define DEVICE_INTR do_hd										// �豸�жϴ�����
#define DEVICE_TIMEOUT hd_timeout								// �豸��ʱֵ
#define DEVICE_REQUEST do_hd_request							// �豸���������
#define DEVICE_NR(device) (MINOR(device) / 5)					// �豸��
#define DEVICE_ON(device)										// �����豸
#define DEVICE_OFF(device)										// �ر��豸

// �����ڱ���Ԥ����׶���ʾ������Ϣ:"δ֪���豸".
#else
/* unknown blk device */
#error "unknown blk device"

#endif

// Ϊ�˱��ڱ�̱�ʾ,���ﶨ����������:CURENT��ָ�����豸�ŵĵ�ǰ����ṹ��ָ��,CURRENT_DEV�ǵ�ǰ������CURRENT���豸��.
#define CURRENT (blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)

// ����������豸�жϴ�����ų���,���������Ϊһ������ָ��,Ĭ��ΪNULL.
#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif
// ����������豸��ʱ���ų���,������ֵ����0,������SET_INTR()��.����ֻ�����.
#ifdef DEVICE_TIMEOUT
int DEVICE_TIMEOUT = 0;
#define SET_INTR(x) (DEVICE_INTR = (x),DEVICE_TIMEOUT = 200)
#else
#define SET_INTR(x) (DEVICE_INTR = (x))
#endif
// �����豸������ų���DEVICE_REGUEST��һ�������������޷��صľ�̬����ָ��.
static void (DEVICE_REQUEST)(void);

// ����ָ���Ļ����.
// ���ָ�������bh��û�б�����,����ʾ������Ϣ.���򽫸û�������,�����ѵȴ��û����Ľ���.��Ϊ��Ƕ����.�����ǻ����ͷָ��.
static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)
		printk(DEVICE_NAME ": free buffer being unlocked\n");
	bh->b_lock = 0;
	wake_up(&bh->b_wait);
}

// ����������.
// ����uptodate�Ǹ��±�־.
// ���ȹر�ָ�����豸,Ȼ����˴ζ�д�������Ƿ���Ч.�����Ч����ݲ���ֵ���û��������ݸ��±�־,�������û�����.������±�־����ֵ��0,
// ��ʾ�˴�������Ĳ���ʧ��,�����ʾ��ؿ��豸IO������Ϣ.���,���ѵȴ���������Ľ����Լ��ȴ�������������ֵĽ���,�ͷŲ�����������
// ��ɾ����������,���ѵ�ǰ������ָ��ָ����һ������.
static inline void end_request(int uptodate)
{
	DEVICE_OFF(CURRENT->dev);							// �ر��豸
	if (CURRENT->bh) {									// CURRENTΪ��ǰ����ṹ��ָ��
		CURRENT->bh->b_uptodate = uptodate;				// �ø��±�־.
		unlock_buffer(CURRENT->bh);						// ����������.
	}
	if (!uptodate) {									// �����±�־Ϊ0����ʾ������Ϣ.
		printk(DEVICE_NAME " I/O error\n\r");
		printk("dev %04x, block %d\n\r",CURRENT->dev,
			CURRENT->bh->b_blocknr);
	}
	wake_up(&CURRENT->waiting);							// ���ѵȴ���������Ľ���.
	wake_up(&wait_for_request);							// ���ѵȴ�����������Ľ���.
	CURRENT->dev = -1;									// �ͷŸ�������.
	CURRENT = CURRENT->next;							// ָ����һ������.
}

// ����������豸��ʱ���ų���DEVICE_TIMEOUT,����CLEAR_DEVICE_TIMEOUT���ų���Ϊ"DEVICE_TIMEOUT =0".������CLEAR_DEVICE_TIMEOUTΪ��.
#ifdef DEVICE_TIMEOUT
#define CLEAR_DEVICE_TIMEOUT DEVICE_TIMEOUT = 0;
#else
#define CLEAR_DEVICE_TIMEOUT
#endif

// ����������豸�жϷ��ų���DEVICE_INTR,����CLEAR_DEVICE_INTR���ų���Ϊ"DEVICE_INTR = 0",��������Ϊ��.
#ifdef DEVICE_INTR
#define CLEAR_DEVICE_INTR DEVICE_INTR = 0;
#else
#define CLEAR_DEVICE_INTR
#endif

// �����ʼ���������.
// ���ڼ������豸��������ʼ����������ĳ�ʼ����������,�������Ϊ���Ƕ�����һ��ͳһ�ĳ�ʼ����.�ú����ڶԵ�ǰ���������һЩ��Ч���ж�.������������:����豸
// ��ǰ������Ϊ��(NULL),��ʾ���豸Ŀǰ������Ҫ�����������.��������ɨβ���˳���Ӧ����.����,�����ǰ���������豸�����豸�Ų�������������������豸
// ��,˵������������ҵ���,�����ں���ʾ������Ϣ��ͣ��.�������������õĻ����û�б�����,Ҳ˵���ں˳����������,������ʾ������Ϣ��ͣ��.
#define INIT_REQUEST \
repeat: \
	if (!CURRENT) {											/* �����ǰ������ָ��ΪNULL�򷵻� */\
		CLEAR_DEVICE_INTR \
		CLEAR_DEVICE_TIMEOUT \
		return; \
	} \
	if (MAJOR(CURRENT->dev) != MAJOR_NR)  					/* �����ǰ�豸���豸�Ų�����ͣ�� */\
		panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT->bh) { \
		if (!CURRENT->bh->b_lock)  							/* ���������Ļ�����û������ͣ�� */\
			panic(DEVICE_NAME ": block not locked"); \
	}

#endif

#endif

