/*
 *  linux/kernel/blk_dev/ll_rw.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * This handles all read/write requests to block devices
 */
#include <errno.h>
#include <linux/sched.h>					// ���Գ���ͷ�ļ�,����������ṹtask_struct,����0���ݵ�.
#include <linux/kernel.h>
#include <asm/system.h>						// ϵͳͷ�ļ�.���������û��޸�������/�ж��ŵȵ�Ƕ��ʽ����.

#include "blk.h"							// ���豸ͷ�ļ�.�����������ݽṹ,���豸���ݽṹ�ͺ����Ϣ.

/*
 * The request-struct contains all necessary data
 * to load a nr of sectors into memory
 */
/*
 * ����ṹ�к��м���nr���������ݵ��ڴ���ȥ�����б������Ϣ.
 */
// �������������.����NR_REQUEST = 32��������.
struct request request[NR_REQUEST];

/*
 * used to wait on when there are no free requests
 */
/*
 * ����������������û�п�����ʱ���̵���ʱ�ȴ���.
 */
struct task_struct * wait_for_request = NULL;

/* blk_dev_struct is:
 *	do_request-address
 *	next-request
 */
/*
 * blk_dev_struct���豸�ṹ��:(�μ��ļ�kernel/blk_drv/blk.h)
 * do_request-address		// ��Ӧ���豸�ŵ����������ָ��.
 * current-request		// ���豸����һ������.
 */
// ���豸����.������ʹ�����豸����Ϊ����.ʵ�����ݽ��ڸ����豸���������ʼ��ʱ����.
// ����,Ӳ�����������ʼ��ʱ(hd.c),��һ����伴�����豸blk_dev[3]������.
struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{ NULL, NULL },		/* no_dev */		// 0 - ���豸
	{ NULL, NULL },		/* dev mem */		// 1 - �ڴ�
	{ NULL, NULL },		/* dev fd */		// 2 - �����豸
	{ NULL, NULL },		/* dev hd */		// 3 - Ӳ���豸
	{ NULL, NULL },		/* dev ttyx */		// 4 - ttyx�豸
	{ NULL, NULL },		/* dev tty */		// 5 - tty�豸
	{ NULL, NULL }		/* dev lp */		// 6 - lp��ӡ���豸
};

/*
 * blk_size contains the size of all block-devices:
 *
 * blk_size[MAJOR][MINOR]
 *
 * if (!blk_size[MAJOR]) then no minor size checking is done.
 */
/*
 * blk_size���麬�����п��豸�Ĵ�С(������):
 * blk_size[MAJOR][MINOR]
 * ���(!blk_size[MAJOR]),�򲻱ؼ�����豸�Ŀ�����.
 */
// �豸���ݿ�����ָ������.ÿ��ָ����ָ��ָ�����豸�ŵ��ܿ�����.���ܿ�������ÿһ���Ӧ���豸��ȷ����һ�����豸����ӵ��
// ����������(1���С = 1KB).
int * blk_size[NR_BLK_DEV] = { NULL, NULL, };

// ����ָ�������.
// ���ָ���Ļ�����Ѿ���������������,��ʹ�Լ�˯��(�����жϵĵȴ�),ֱ����ִ�н���������������ȷ�ػ���
static inline void lock_buffer(struct buffer_head * bh)
{
	cli();							// ���ж����.
	while (bh->b_lock)				// ����������ѱ�������˯��,ֱ������������.
		sleep_on(&bh->b_wait);
	bh->b_lock = 1;					// ��������������.
	sti();							// ���ж�.
}

// �ͷ�(����)�����Ļ�����.
// �ú�����hlk.h�ļ��е�ͬ��������ȫһ��.
static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)				// ����û�����û�б�����,���ӡ������Ϣ.
		printk("ll_rw_block.c: buffer not locked\n\r");
	bh->b_lock = 0;					// ��������־.
	wake_up(&bh->b_wait);			// ���ѵȴ��û�����������.
}

/*
 * add-request adds a request to the linked list.
 * It disables interrupts so that it can muck with the
 * request-lists in peace.
 *
 * Note that swapping requests always go before other requests,
 * and are done in the order they appear.
 */
/*
 * add-request()�������м���һ��������.����ر��ж�,�������ܰ�ȫ�ش�������������.
 *
 * ע��,����������������������֮ǰ����,���������ǳ�����˳�����.
 */
// �������м���������.
// ����dev��ָ�����豸�ṹָ��,�ýṹ���д����������ָ��͵�ǰ����������ָ��;
// req�������ú����ݵ�������ṹָ��.
// ���������Ѿ����úõ�������req��ӵ�ָ���豸��������������.������豸�ڵ�ǰ������ָ��Ϊ��,���������reqΪ��ǰ��������̵����豸����
// �����.����Ͱ�req��������뵽��������������.
static void add_request(struct blk_dev_struct * dev, struct request * req)
{
	// ���ȶԲ����ṩ���������ָ��ͱ�־����ʼ����.�ÿ��������е���һ������ָ��,���жϲ������������ػ��������־.
	struct request * tmp;

	req->next = NULL;
	cli();								// ���ж�
	if (req->bh)
		req->bh->b_dirt = 0;			// �建����"��"��־.
	// Ȼ��鿴ָ���豸�Ƿ��е�ǰ������,���鿴�豸�Ƿ���æ.���ָ���豸dev��ǰ������(current_equest)�ֶ�Ϊ��,���ʾĿǰ���豸û��������,������
	// ��1��������,Ҳ��Ψһ��һ��.��˿ɽ����豸��ǰ����ָ��ֱ��ָ���������,������ִ����Ӧ�豸��������.
	if (!(tmp = dev->current_request)) {
		dev->current_request = req;
		sti();							// ���ж�.
		(dev->request_fn)();			// ִ��������,����Ӳ����do_hd_request().
		return;
	}
	// ���Ŀǰ���豸�Ѿ��е�ǰ�������ڴ���,���������õ����㷨������Ѳ���λ��,Ȼ�󽫵�ǰ��������뵽����������.������������,����жϳ�������
	// ������Ļ����ͷָ���,��û�л����,��ô����Ҫ��һ����,���Ѿ��п��õĻ����.�������ǰ����λ��(tmp֮��)���Ŀ�������ͷָ�벻��,��ѡ�����λ��
	// �����˳�ѭ���������������˴�.����жϲ��˳�����.�����㷨���������ô��̴�ͷ���ƶ�������С,�Ӷ�����(����)Ӳ�̷���ʱ��.
	// ����forѭ����if������ڰ�req��ָ���������������(����)�����е����������Ƚ�,�ҳ�req����ö��е���ȷλ��˳��.Ȼ���ж�ѭ��,����req���뵽�ö�����ȷλ�ô�.
	for ( ; tmp->next ; tmp = tmp->next) {
		if (!req->bh)
			if (tmp->next->bh)
				break;
			else
				continue;
		if ((IN_ORDER(tmp, req) ||
		    !IN_ORDER(tmp, tmp->next)) &&
		    IN_ORDER(req, tmp->next))
			break;
	}
	req->next = tmp->next;
	tmp->next = req;
	sti();
}

// ����������������������.
// ����major�����豸��;rw��ָ������;bh�Ǵ�����ݵĻ�����ͷָ��.
static void make_request(int major, int rw, struct buffer_head * bh)
{
	struct request * req;
	int rw_ahead;

	/* WRITEA/READA is special case - it is not really needed, so if the */
	/* buffer is locked, we just forget about it, else it's a normal read */
	/* WRITEA/READA��һ��������� - ���ǲ��Ǳ�Ҫ,��������������Ѿ�����,���ǾͲ��ù���,������ֻ��һ��һ��Ķ�����. */
	// ����'READ'��'WRITE'�����'A'�ַ�����Ӣ�ĵ���Ahead,��ʾԤ��/д���ݿ����˼.
	// �ú������ȶ�����READA/WRITEA���������һЩ����.��������������,��ָ���Ļ���������ʹ�ö��ѱ�����ʱ,�ͷ���Ԥ��/д����.�������Ϊ��ͨ
	// READ/WRITE������в���.����,�����������������Ȳ���READҲ����WRITE,���ʾ�ں˳����д�,��ʾ������Ϣ��ͣ��.ע��,���޸�����֮ǰ����
	// ��Ϊ�����Ƿ�ΪԤ��/д���������˱�־rw_ahead.
	if (rw_ahead = (rw == READA || rw == WRITEA)) {
		if (bh->b_lock)
			return;
		if (rw == READA)
			rw = READ;
		else
			rw = WRITE;
	}
	if (rw != READ && rw != WRITE)
		panic("Bad block dev command, must be R/W/RA/WA");
	lock_buffer(bh);                				// ���������
	// �����WRITE�������һ����δ�޸ģ�����READ�������һ�����Ѹ��£���ֱ�ӷ��ػ������顣
	if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate)) {
		unlock_buffer(bh);
		return;
	}
repeat:
	/* we don't allow the write-requests to fill up the queue completely:
	 * we want some room for reads: they take precedence. The last third
	 * of the requests are only for reads.
	 */
	/*
	 * ���ǲ����ö�����ȫ����д������:������ҪΪ��������һЩ�ռ�:�����������ȵ�.������еĺ�����֮һ�ռ�����ڶ�������.
	 */
	// ��,�������Ǳ���Ϊ���������ɲ���Ӷ�/д��������.����������Ҫ������������Ѱ�ҵ�һ��������(��)�������������.�������̴���������ĩ�˿�ʼ.
	// ��������Ҫ��,���ڶ���������,����ֱ�ӴӶ���ĩβ��ʼ����,������д�����ֻ�ܴӶ���2/3�������ͷ��������������.�������ǿ�ʼ�Ӻ���ǰ����,
	// ������ṹrequest���豸�ֶ�devֵ=-1ʱ,��ʾ����δ��ռ��(����).���û��һ���ǿ��е�(��ʱ����������ָ���Ѿ�����Խ��ͷ��),��鿴�˴�����
	// �Ƿ�����ǰ��/д(READA��WRITEA),�����������˴��������.�����ñ������������˯��(�Եȴ���������ڳ�����),��һ������������������.
	if (rw == READ)
		req = request + NR_REQUEST;						// ���ڶ�����,��ָ��ָ�����β��.
	else
		req = request + ((NR_REQUEST * 2) / 3);			// ����д����,ָ��ָ�����2/3��.
	/* find an empty request */
	/* ����һ���������� */
	while (--req >= request)
		if (req->dev < 0)
			break;
	/* if none found, sleep on new requests: check for rw_ahead */
	/* ���û���ҵ�������,���øô��������˯��:�����Ƿ���ǰ��/д */
	if (req < request) {								// �����������ͷ(�����޿���)
		if (rw_ahead) {									// ��������ǰ��/д����,���˳�.
			unlock_buffer(bh);
			return;
		}
		sleep_on(&wait_for_request);					// �����˯��,�����ٲ鿴�������.
		goto repeat;
	}
	/* fill up the request-info, and add it to the queue */
	/* ���������������д������Ϣ,�������������� */
	// OK,����ִ�е������ʾ���ҵ�һ������������.�������������úõ����������͵���add_request()������ӵ����������,�����˳�.����ṹ��μ�blk_drv/blk.h.
	// req->sector�Ƕ�д��������ʼ������,req->buffer�������������ݵĻ�����.
	req->dev = bh->b_dev;								// �豸��.
	req->cmd = rw;										// ����(READ/WRITE).
	req->errors = 0;									// ����ʱ�����Ĵ������.
	req->sector = bh->b_blocknr << 1;					// ��ʼ����.���ת����������(1��=2����).
	req->nr_sectors = 2;								// ����������Ҫ��д��������.
	req->buffer = bh->b_data;							// ���������ָ��ָ�����д�����ݻ�����.
	req->waiting = NULL;								// ����ȴ�����ִ����ɵĵط�.
	req->bh = bh;										// �����ͷָ��.
	req->next = NULL;									// ָ����һ������.
	add_request(major + blk_dev, req);					// ����������������(blk_dev[major],reg).
}

// �ͼ�ҳ���д����(Low Level Read Write Pagk).
// ��ҳ��(4K)Ϊ��λ�����豸����,��ÿ�ζ�/д8������.�μ�����ll_rw_blk()����.
void ll_rw_page(int rw, int dev, int page, char * buffer)
{
	struct request * req;
	unsigned int major = MAJOR(dev);

	// ���ȶԺ��������ĺϷ��Խ��м��.����豸���豸�Ų����ڻ��߸��豸�������������������,����ʾ������Ϣ,������.�����������������Ȳ���
	// READҲ����WRITE,���ʾ�ں˳����д�,��ʾ������Ϣ��ͣ��.
	if (major >= NR_BLK_DEV || !(blk_dev[major].request_fn)) {
		printk("Trying to read nonexistent block-device\n\r");
		return;
	}
	if (rw != READ && rw != WRITE)
		panic("Bad block dev command, must be R/W");
	// �ڲ�����������ɺ�,����������ҪΪ���β�������������.����������Ҫ������������Ѱ�ҵ�һ��������(��)�������������.�������̴���������ĩ��
	// ��ʼ.�������ǿ�ʼ�Ӻ���ǰ����,������ṹrequest���豸�ֶ�ֵ<0ʱ,��ʾ����δ��ռ��(����).���û��һ���ǿ��е�(��ʱ����������ָ���Ѿ�����Խ��
	// ͷ��),���ñ������������˯��(�Եȴ���������ڳ�����),��һ�����������������.
repeat:
	req = request + NR_REQUEST;							// ��ָ��ָ�����β��.
	while (--req >= request)
		if (req->dev < 0)
			break;
	if (req < request) {
		sleep_on(&wait_for_request);					// ˯��,�����ٲ鿴�������.
		goto repeat;
	}
	/* fill up the request-info, and add it to the queue */
	/* ���������������д������Ϣ,�������������� */
	// OK,����ִ�е������ʾ���ҵ�һ������������.�����������ú���������,�ѵ�ǰ������Ϊ�����ж�˯���жϺ�,��ȥ����add_request()������ӵ����������,
	// Ȼ��ֱ�ӵ��õ��Ⱥ����õ�ǰ����˯�ߵȴ�ҳ��ӽ����豸�ж���.���ﲻ��make_request()��������ֱ���˳�������������schedule(),����Ϊmake_request()
	// ��������2����������.��������Ҫ�Խ����豸��/д8������,��Ҫ���ϳ���ʱ��.��˵�ǰ���̿϶���Ҫ�ȴ���˯��.�������ֱ�Ӿ��ý���ȥ˯����,ʡ���ڳ��������ط�
	// ��Ҫ������Щ�жϲ���.
	req->dev = dev;										// �豸��
	req->cmd = rw;										// ����(READ/WRITE)start_code
	req->errors = 0;									// ��д�����������
	req->sector = page << 3;							// ��ʼ��д����
	req->nr_sectors = 8;								// ��д������
	req->buffer = buffer;								// ���ݻ�����
	req->waiting = current;								// ��ǰ���̽��������ȴ�����
	req->bh = NULL;										// �޻����ͷָ��(���ø��ٻ���)
	req->next = NULL;									// ��һ��������ָ��
	current->state = TASK_UNINTERRUPTIBLE;				// ��Ϊ�����ж�״̬
	add_request(major + blk_dev, req);					// ����������������.
	// ��ǰ������Ҫ��ȡ8�����������������Ҫ˯�ߣ���˵��õ��ȳ���ѡ���������
	schedule();
}

// �ͼ����ݿ��д����(Low Level Read Write Block)
// �ú����ǿ��豸����������ϵͳ�������ֵĽӿں���.ͨ����fs/buffer.c�����б�����.
// ��Ҫ�����Ǵ������豸��д��������뵽ָ�����豸�������.ʵ�ʵĶ�д�����������豸��request_fn()�������.����Ӳ�̲���,�ú�����do_hd_request();�������̲���
// �ú�����do_fd_request();��������������do_rd_request().����,�ڵ��øú���֮ǰ,��������Ҫ���ȰѶ�/д���豸����Ϣ�����ڻ����ͷ�ṹ��,���豸��,���.
// ����:rw - READ,READA,WRITE��WRITEA������;bh - ���ݻ����ͷָ��.
void ll_rw_block(int rw, struct buffer_head * bh)
{
	unsigned int major;									// ���豸��(����Ӳ����3)

	// ����豸���豸�Ų����ڻ��߸��豸�������������������,����ʾ������Ϣ,������.���򴴽�����������������.
	if ((major = MAJOR(bh->b_dev)) >= NR_BLK_DEV ||
	!(blk_dev[major].request_fn)) {
		printk("Trying to read nonexistent block-device\n\r");
		return;
	}
	make_request(major, rw, bh);
}

// ���豸��ʼ������,�ɳ�ʼ������main.c����.
// ��ʼ����������,��������������Ϊ������(dev = -1).��32��(NR_REQUEST = 32).
void blk_dev_init(void)
{
	int i;

	for (i = 0; i < NR_REQUEST; i++) {
		request[i].dev = -1;
		request[i].next = NULL;
	}
}

