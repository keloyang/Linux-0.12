/*
 *  linux/fs/buffer.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting a interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it. NOTE! As interrupts
 * can wake up a caller, some cli-sti sequences are needed to check for
 * sleep-on-calls. These should be extremely quick, though (I hope).
 */
/*
 * 'buffer.c'����ʵ�ֻ��������ٻ��湦��.ͨ�������жϴ�����̸ı仺����,�����õ�������ִ��,�����˾�������(��Ȼ���ı�������).
 * ע��!�����жϿ��Ի���һ��������,��˾���Ҫ�����ж�ָ��(cli-sti)������������ڵ��ö�˯��.����Ҫ�ǳ���(��ϣ��������).
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 */
/*
 * ע��!��һ������Ӧ����������:��������Ƿ����.�����������Ƿ��øó�����õĵط���,��Ϊ����Ҫʹ�Ѹ������̻���ʧЧ.
 */

#include <stdarg.h>

//#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
//#include <linux/kernel.h>
#include <asm/system.h>
//#include <asm/io.h>

// ����end���ɱ���ʱ�����ӳ���ld����,���ڱ����ں˴����ĩ��,��ָ���ں�ģ��ĩ��λ��.Ҳ���Դӱ���ʱ���ɵ�System.map�ļ��в��.��������
// ���������ٻ�������ʼ���ں˴���ĩ��λ��.
// buffer_wait�����ǵȴ����л�����˯�ߵ��������ͷָ��.���뻺���ͷ���ṹ��b_waitָ������ò�ͬ.����������һ����������������ϵͳ
// ȱ�����ÿ��л����ʱ,��ǰ����ͻᱻ��ӵ�buffer_wait˯�ߵȴ�������.��b_wait����ר�Ź��ȴ�ָ�������(��b_wait��Ӧ�Ļ����)������
// ʹ�õĵȴ�����ͷָ��.
extern int end;
struct buffer_head * start_buffer = (struct buffer_head *) &end;
struct buffer_head * hash_table[NR_HASH];							// NR_HASH = 307��.
static struct buffer_head * free_list;								// ���л��������ͷָ��.
static struct task_struct * buffer_wait = NULL;						// �ȴ����л�����˯�ߵ��������.
// ���涨��ϵͳ�������к��еĻ�������.����,NR_BUFFERS��һ��������linux/fs.hͷ�ļ��ĺ�,��ֵ���Ǳ�����nr_buffers,������fs.h�ļ�����
// Ϊȫ�ֱ���.
// ��д����ͨ������һ��������,Linus������д������Ϊ�����������д�����������ر�ʾnr_buffers��һ�����ں˳�ʼ��֮���ٸı��"����".������
// ��ʼ������buffer_init()�б�����.
int NR_BUFFERS = 0;													// ϵͳ���л�������.

// �ȴ�ָ����������.
// ���ָ���Ļ����bh�Ѿ��������ý��̲����жϵ�˯���ڸû����ĵȴ�����b_wait��.�ڻ�������ʱ,��ȴ������ϵ����н��̽�������.��Ȼ���ڹر�
// �ж�(cli)֮��ȥ˯�ߵ�,��������������Ӱ����������������������Ӧ�ж�.��Ϊÿ�����̶����Լ���TSS���б����˱�־�Ĵ���EFLAGS��ֵ,�����ڽ���
// �л�ʱCPU�е�ǰEFLAGS��ֵҲ��֮�ı�.ʹ��sleep_on()����˯��״̬�Ľ�����Ҫ��wake_up()��ȷ�ػ���.
static inline void wait_on_buffer(struct buffer_head * bh)
{
	cli();							// ���ж�
	while (bh->b_lock)				// ����ѱ���������̽���˯��,�ȴ������.
		sleep_on(&bh->b_wait);
	sti();							// ���ж�.
}

// �豸����ͬ����
// ͬ���豸���ڴ���ٻ��������ݡ����У�sync_inodes()������inode.c��
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;

	// ���ȵ���i�ڵ�ͬ��������������i�ڵ���������޸Ĺ���i�ڵ�д����ٻ����С�Ȼ��ɨ�����и��ٻ����������ѱ��޸ĵĻ����
	// ����д�����󣬽�����������д�����У��������ٻ����е��������豸�е�ͬ����
	sync_inodes();							/* write out inodes into buffers */
	bh = start_buffer;      				// bhָ�򻺳忪ʼ����
	for (i = 0 ; i < NR_BUFFERS ; i++, bh++) {
		wait_on_buffer(bh);             	// �ȴ�����������������������Ļ�����
		if (bh->b_dirt)
			ll_rw_block(WRITE, bh);  		// ����д�豸������
	}
	return 0;
}

// ��ָ���豸���и��ٻ����������豸�����ݵ�ͬ��������
// �ú��������������ٻ����������л���顣����ָ���豸dev�Ļ���飬���������ѱ��޸Ĺ���д�����У�ͬ����������Ȼ��
// ���ڴ���i�ڵ�����д����ٻ����С�֮����ָ���豸devִ��һ����������ͬ��д�̲�����
int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;

	// ���ȶԲ���ָ�����豸ִ������ͬ�����������豸�ϵ���������ٻ������е�����ͬ����������ɨ����ٻ����������л���飬
	// ��ָ���豸dev�Ļ���飬�ȼ�����Ƿ��ѱ����������ѱ�����˯�ߵȴ��������Ȼ�����ж�һ�θû�����Ƿ���ָ���豸��
	// ����鲢�����޸Ĺ���b_dirt��־��λ�������ǾͶ���ִ��д�̲�������Ϊ������˯���ڼ�û�����п����ѱ��ͷŻ��߱�Ų
	// �����ã������ڼ���ִ��ǰ��Ҫ�ٴ��ж�һ�¸û�����Ƿ���ָ���豸�Ļ���顣
	bh = start_buffer;                      		// bfָ�򻺳�����ʼ����
	for (i = 0 ; i < NR_BUFFERS ; i++, bh++) {
		if (bh->b_dev != dev)           			// �����豸dev�Ļ�����������
			continue;
		wait_on_buffer(bh);             			// �ȴ�����������������������Ļ�����
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE, bh);
	}
	// �ٽ�i�ڵ�����д����ٻ��塣��i�ڵ��inode_table�е�inode�뻺���е���Ϣͬ����
	sync_inodes();
	// Ȼ���ڸ��ٻ����е����ݸ���֮���ٰ��������豸�е�����ͬ���������������ͬ��������Ϊ������ں�ִ��Ч�ʡ���һ�黺
	// ����ͬ�������������ں�����ࡰ��顱��ɾ���ʹ��i�ڵ��ͬ�������ܹ���Чִ�С����λ�����ͬ�����������Щ����i�ڵ�
	// ͬ���������ֱ���Ļ�������豸������ͬ����
	bh = start_buffer;
	for (i = 0 ; i < NR_BUFFERS ; i++, bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE, bh);
	}
	return 0;
}

// ʹָ���豸�ڸ��ٻ������е�������Ч��
// ɨ����ٻ��������еĻ���顣��ָ���豸�Ļ���鸴λ����Ч�����£���־���޸ı�־��
void invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i = 0 ; i < NR_BUFFERS ; i++, bh++) {
		if (bh->b_dev != dev)           // �������ָ���豸�Ļ���飬�����ɨ����һ�顣
			continue;
		wait_on_buffer(bh);             // �ȴ��û���������������ѱ���������
		// ���ڽ���ִ�й�˯�ߵȴ���������Ҫ���ж�һ�»������Ƿ���ָ���豸�ġ�
		if (bh->b_dev == dev)
			bh->b_uptodate = bh->b_dirt = 0;
	}
}

/*
 * This routine checks whether a floppy has been changed, and
 * invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 *
 * NOTE! Although currently this is only for floppies, the idea is
 * that any additional removable block-device will use this routine,
 * and that mount/open needn't know that floppies/whatever are
 * special.
 */
/*
 * ���ӳ�����һ�������Ƿ��ѱ�����������Ѿ�������ʹ���ٻ��������������Ӧ�����л�������Ч��
 * ���ӳ��������˵��������������Ҫ������ʹ������
 * ���Խ���ִ��'mount'��'open'ʱ�ŵ��������������ǽ��̶���ʵ�������ϵ���÷��������ڲ��������и������̣�
 * �ͻᵼ�����ݵĶ�ʧ�����Ǿ�����ȡ��
 *
 * ע�⣡����Ŀǰ���ӳ�����������̣��Ժ��κο��ƶ����ʵĿ��豸���н�ʹ�øó���mount/open��������Ҫ֪��
 * �����̻�������ʲô������ʡ�
 */
// �������Ƿ����������Ѹ�����ʹ�ö�Ӧ������������Ч��
void check_disk_change(int dev)
{
	int i;

	// ���ȼ��һ���ǲ��������豸����Ϊ���ڽ�֧�����̿��ƶ����ʡ�����������˳���Ȼ����������Ƿ��Ѹ��������û��
	// ���˳���floppy_chang()��blk_drv/floppy.c��
	if (MAJOR(dev) != 2)
		return;
	if (!floppy_change(dev & 0x03))
		return;
	// �����Ѹ����������ͷŶ�Ӧ�豸��i�ڵ�λͼ���߼�λͼ��ռ�ĸ��ٻ���������ʹ���豸��i�ڵ�����ݿ���Ϣ��ռ�ݵĸ��ٻ���
	// ����Ч��
	for (i = 0 ; i < NR_SUPER ; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);         // �ͷ��豸dev���ڴ�i�ڵ���е�����i�ڵ�
	invalidate_buffers(dev);        //
}

// �������д�����hash(ɢ��)���������hash����ļ����.
// hash�����Ҫ�����Ǽ��ٲ��ұȽ�Ԫ�������ѵ�ʱ��.ͨ����Ԫ�صĴ洢λ����ؼ���֮�佨��һ����Ӧ��ϵ(hash����),���ǾͿ���ֱ��ͨ��������������
// ��ѯ��ָ����Ԫ��.����hash������ָ��������Ҫ�Ǿ���ȷ��ɢ�е��κ�������ĸ��ʻ������.���������ķ����ж���,����Linux0.12��Ҫ�����˹ؼ��ֳ�
// ������.��Ϊ����Ѱ�ҵĻ��������������,���豸��dev�ͻ�����block,�����Ƶ�hash�����϶���Ҫ�����������ؼ�ֵ.���������ؼ��ֵ�������ֻ��
// ����ؼ�ֵ��һ�ַ���.�ٶԹؼ�ֵ����MOD����Ϳ��Ա�֤��������õ���ֵ�����ں��������Χ��.
#define _hashfn(dev, block) (((unsigned)(dev ^ block)) % NR_HASH)
#define hash(dev, block) hash_table[_hashfn(dev, block)]

// ��hash���кͿ��л�����������߻����.
// hash������˫������ṹ,���л���������˫��ѭ������ṹ.
static inline void remove_from_queues(struct buffer_head * bh)
{
	/* remove from hash-queue */
	/* ��hash�������Ƴ������ */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	// ����û������Ǹö��е�ͷһ����,����hash��Ķ�Ӧ��ָ�򱾶����е���һ��������.
	if (hash(bh->b_dev, bh->b_blocknr) == bh)
		hash(bh->b_dev, bh->b_blocknr) = bh->b_next;
	/* remove from free list */
	/* �ӿ��л��������Ƴ������ */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	// �����������ͷָ�򱾻�����,������ָ����һ������.
	if (free_list == bh)
		free_list = bh->b_next_free;
}

// �����������������β��,ͬʱ����hash������.
static inline void insert_into_queues(struct buffer_head * bh)
{
	/* put at end of free list */
	/* ���ڿ�������ĩβ�� */
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
	/* put the buffer in new hash-queue if it has a device */
	/* ����û�����Ӧһ���豸,���������hash������ */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev, bh->b_blocknr);
	hash(bh->b_dev, bh->b_blocknr) = bh;
	// ��ע�⵱hash��ĳ���1�β�����ʱ,hash()����ֵ�϶�ΪNULL,��˴�ʱhash(bh->b_dev,bh->b_blocknr)�õ���bh->b_next�϶���NULL,
	// ����bh->b_next->b_prev = bhӦ����bh->b_next��ΪNULLʱ���ܸ�b_pev��bhֵ.��bh->b_next->b_prev = bhǰӦ�������ж�"
	// if(bh->b_next)".�ô���0.96���ű�����.
	if(bh->b_next)
		bh->b_next->b_prev = bh;			// �˾�ǰӦ���"if(bh->b_next)"�ж�.
}

// ����hash���ڸ��ٻ�����Ѱ�Ҹ����豸��ָ����ŵĻ�������.
// ����ҵ��򷵻ػ��������ָ��,���򷵻�NULL.
static struct buffer_head * find_buffer(int dev, int block)
{
	struct buffer_head * tmp;

	// ����hash��,Ѱ��ָ���豸��Ϳ�ŵĻ����.
	for (tmp = hash(dev, block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_dev == dev && tmp->b_blocknr == block)
			return tmp;
	return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are readint them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
/*
 * ����Ϊʲô���������ӵ�?����������......ԭ���Ǿ�������.��������û�жԻ��������(�����������ڶ�ȡ���ǵ�����),��ô������(����)˯��ʱ��������
 * ����һЩ����(����һ�������󽫵��¸û�������).Ŀǰ�������ʵ�����ǲ��ᷢ����,������Ĵ����Ѿ�׼������.
 */
// ����hash���ڸ��ٻ�������Ѱ��ָ���Ļ����.���ҵ���Ըû�������������ؿ�ͷָ��.
struct buffer_head * get_hash_table(int dev, int block)
{
	struct buffer_head * bh;

	for (;;) {
		// �ڸ��ٻ�����Ѱ�Ҹ����豸��ָ����Ļ�������,���û���ҵ��򷵻�NULL,�˳�.
		if (!(bh = find_buffer(dev, block)))
			return NULL;
		// �Ըû�����������ü���,���ȴ��û�������(����ѱ�����).���ھ�����˯��״̬,����б�Ҫ����֤�û�������ȷ��,�����ػ����ͷָ��.
		bh->b_count++;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block)
			return bh;
		// �����˯��ʱ�û�����������豸�Ż��ŷ����ĸı�,�����������ü���.����Ѱ��.
		bh->b_count--;
	}
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algoritm is changed: hopefully better, and an elusive bug removed.
 */
/*
 * OK,������getbl����,�ú������߼������Ǻ�����,ͬ��Ҳ����ΪҪ���Ǿ�����������.���д󲿷ִ�������õ�(�����ظ��������),
 * �����Ӧ�ñȿ���ȥ��������Ч�ö�.
 *
 * �㷨�Ѿ����˸ı�:ϣ���ܸ���,����һ��������ĥ�Ĵ����Ѿ�ȥ��.
 */
// ���������ͬʱ�жϻ��������޸ı�־��������־,���Ҷ����޸ı�־��Ȩ��Ҫ��������־��.
#define BADNESS(bh) (((bh)->b_dirt << 1) + (bh)->b_lock)
// ȡ���ٻ�����ָ���Ļ����.
// ���ָ��(�豸�źͿ��)�Ļ������Ƿ��Ѿ��ڸ��ٻ�����.���ָ�����Ѿ��ڸ��ٻ�����,�򷵻ض�Ӧ������ͷָ���˳�;�������,����Ҫ�ڸ�����
// ������һ����Ӧ�豸�źͿ�ŵ�����.������Ӧ������ͷָ��.
struct buffer_head * getblk(int dev, int block)
{
	struct buffer_head * tmp, * bh;

repeat:
	if (bh = get_hash_table(dev, block))
		return bh;
	// ɨ��������ݿ�����,Ѱ�ҿ��л�����.
	// ������tmpָ���������ĵ�һ�����л�����ͷ.
	tmp = free_list;
	do {
		// ����û��������ڱ�ʹ��(���ü���������0),�����ɨ����һ��.����b_count=0�Ŀ�,�����ٻ����е�ǰû�����õĿ鲻һ�����Ǹɾ���(b_dirt=0)
		// ��û��������(b_lock=0).���,���ǻ�����Ҫ����������жϺ�ѡ��.���統һ�������д��һ�����ݺ���ͷ���,���Ǹÿ�b_count=0,��b_lock������0;
		// ��һ������ִ��breada()Ԥ��������ʱ,ֻҪll_rw_block()�������,���ͻ�ݼ�b_count;����ʱʵ����Ӳ�̷��ʲ������ܻ��ڽ���,��˴�ʱ
		// b_lock=1,��b_count=0.
		if (tmp->b_count)
			continue;
		// �������ͷָ��bhΪ��,����tmp��ָ����ͷ�ı�־(�޸�,����)Ȩ��С��bhͷ��־��Ȩ��,����bhָ��tmp�����ͷ.�����tmp�����ͷ��������
		// ���û���޸�Ҳû��������־��λ,��˵����Ϊָ���豸�ϵĿ�ȡ�ö�Ӧ�ĸ��ٻ����,���˳�ѭ��.�������Ǿͼ���ִ�б�ѭ��,�����ܷ��ҵ�һ��BADNESS()
		// ��С�Ļ����.
		if (!bh || BADNESS(tmp) < BADNESS(bh)) {
			bh = tmp;
			if (!BADNESS(tmp))
				break;
		}
	/* and repeat until we find something good */	/* �ظ�����ֱ���ҵ��ʺϵĻ���� */
	} while ((tmp = tmp->b_next_free) != free_list);
	// ���ѭ����鷢�����л���鶼���ڱ�ʹ��(���л�����״�����ü�����>0)��,��˯�ߵȴ��п��л���������.���п��л�������ʱ�����ᱻ��ȷ�ػ���.Ȼ��
	// ���Ǿ���ת��������ʼ�����²��ҿ��л����.
	if (!bh) {
		sleep_on(&buffer_wait);
		goto repeat;
	}
	// ִ�е�����,˵�������Ѿ��ҵ���һ���Ƚ��ʺϵĿ��л������.�����ȵȴ��û���������(����ѱ������Ļ�).���������˯�߽׶θû������ֱ���������ʹ�õĻ�,
	// ֻ���ظ�����Ѱ�ҹ���.
	wait_on_buffer(bh);
	if (bh->b_count)	// �ֱ�ռ��??
		goto repeat;
	// ����û������ѱ��޸�,������д��,���ٴεȴ�����������.ͬ����,���û������ֱ���������ʹ�õĻ�,ֻ�����ظ�����Ѱ�ҹ���.
	while (bh->b_dirt) {
		sync_dev(bh->b_dev);
		wait_on_buffer(bh);
		if (bh->b_count)	// �ֱ�ռ��??
			goto repeat;
	}
	/* NOTE!! While we slept waiting for this block, somebody else might */
	/* already have added "this" block to the cache. check it */
	/* ע��!!������Ϊ�˵ȴ��û�����˯��ʱ,�������̿����Ѿ����û�����������ٻ�����,��������ҲҪ�Դ˽��м��. */
	// �ڸ��ٻ���hash���м��ָ���豸�Ϳ�Ļ�����Ƿ������˯��֮���Ѿ��������ȥ.����ǵĻ����ٴ��ظ�����Ѱ�ҹ���.
	if (find_buffer(dev, block))
		goto repeat;
	/* OK, FINALLY we know that this buffer is the only one of it's kind, */
	/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
	/* OK,��������֪���û������ָ��������Ψһһ��,����Ŀǰ��û�б�ռ�� */
	/* (b_count=0),Ҳδ������(b_lock=0),�����Ǹɾ���(δ���޸ĵ�) */
	// ����������ռ�ô˻����.�����ü���Ϊ1,��λ�޸ı�־����Ч(����)��־.
	bh->b_count = 1;
	bh->b_dirt = 0;
	bh->b_uptodate = 0;
	// ��hash���кͿ��п��������Ƴ��û���ͷ,�øû���������ָ���豸�����ϵ�ָ����.Ȼ����ݴ����豸�źͿ�����²�����������hash������λ�ô�.�����շ��ػ���
	// ͷָ��.
	remove_from_queues(bh);
	bh->b_dev = dev;
	bh->b_blocknr = block;
	insert_into_queues(bh);
	return bh;
}

// �ͷ�ָ�������.
// �ȴ��û�������.Ȼ�����ü����ݼ�1,����ȷ�ػ��ѵȴ����л����Ľ���.
void brelse(struct buffer_head * buf)
{
	if (!buf)						// �������ͷָ����Ч�򷵻�.
		return;
	wait_on_buffer(buf);
	if (!(buf->b_count--))
		panic("Trying to free free buffer");
	wake_up(&buffer_wait);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
/*
 * ���豸�϶�ȡָ�������ݿ鲢���غ������ݵĻ�����.���ָ���Ŀ鲻�����򷵻�NULL.
 */
// ���豸�϶�ȡ���ݿ�.
// �ú�������ָ�����豸��dev�����ݿ��block,�����ڸ��ٻ�����������һ�黺���.����û�����о���������Ч�����ݾ�ֱ�ӷ��ظû����ָ��,����ʹ��豸��
// ��ȡָ�������ݿ鵽�û�����в����ػ����ָ��.
struct buffer_head * bread(int dev, int block)
{
	struct buffer_head * bh;

	// �Ӹ��ٻ�����������һ�黺���.�������ֵ��NULL,���ʾ�ں˳���,ͣ��.���������ж������Ƿ����п�������.
	if (!(bh = getblk(dev, block)))
		panic("bread: getblk returned NULL\n");
	// ����û��������������Ч��(�Ѹ��µ�)����ֱ��ʹ��,�򷵻�.
	if (bh->b_uptodate)
		return bh;
	// �������Ǿ͵��õײ���豸��дll_rw_block()����,�������豸������.Ȼ��ȴ�ָ�����ݿ鱻����,���ȴ�����������.��˯������֮��,����û������Ѹ���,�򷵻�
	// ������ͷָ��,�˳�.����������豸����ʧ��,�����ͷŸû�����,����NULL,�˳�.
	ll_rw_block(READ, bh);
	wait_on_buffer(bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return NULL;
}

// �����ڴ��.
// ��from��ַ����һ��(1024�ֽ�)���ݵ�toλ��(��д�����Ƚ�edi,esi�Ĵ�����ջ����������������Ϻ��ٻ�ԭ������edi,esi�Ĵ�������Ⱦ)
#define COPYBLK(from, to) \
__asm__("cld\n\t" \
		"pushl %%edi\n\t" \
		"pushl %%esi\n\t" \
		"rep\n\t" \
		"movsl\n\t" \
		"popl %%esi\n\t" \
		"popl %%edi\n\t" \
	::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
	:)

//#define COPYBLK(from,to) \
	__asm__("cld\n\t" \
			"rep\n\t" \
			"movsl\n\t" \
			::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
			:)

/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc.
 */
/*
 * bread_pageһ�ζ��ĸ���������ݶ����ڴ�ָ���ĵ�ַ��.����һ�������ĺ���,��Ϊͬʱ��ȡ�Ŀ���Ի���ٶ��ϵĺô�,���õ��Ŷ�һ��,�ٶ�һ����.
 */
// ���豸��һ��ҳ��(4�������)�����ݵ�ָ���ڴ��ַ��.
// ����address�Ǳ���ҳ�����ݵĵ�ַ;dev��ָ�����豸��;b[4]�Ǻ���4���豸���ݿ�ŵ�����.
// �ú���������mm/memory.c�ļ���do_no_page()������.
void bread_page(unsigned long address, int dev, int b[4])
{
	struct buffer_head * bh[4];
	int i;

	// �ú���ѭ��ִ��4��,���ݷ�������b[]�е�4����Ŵ��豸dev�ж�ȡһҳ���ݷŵ�ָ���ڴ�λ��address��.���ڲ���b[i]������
	// ��Ч���,�������ȴӸ��ٻ�����ȡָ���豸�Ϳ�ŵĵĻ����.����������������Ч(δ����)��������豸������豸�϶�ȡ��
	// Ӧ���ݿ�.����b[i]��Ч�Ŀ�����ô�������.��˱�������ʵ���Ը���ָ����b[]�еĿ�������ȡ1-4�����ݿ�.
	for (i = 0 ; i < 4 ; i++)
		if (b[i]) {
			// �ȸ����߼��������һ�������
			if (bh[i] = getblk(dev, b[i]))
				// ����û����û�и���,��ӿ��豸�ж�ȡ����
				if (!bh[i]->b_uptodate)
					ll_rw_block(READ, bh[i]);
		} else
			bh[i] = NULL;
	// ���4��������ϵ�����˳���Ƶ�ָ����ַ��.�ڽ��и���(ʹ��)�����֮ǰ������Ҫ˯�ߵȴ���������(���������Ļ�).����,
	// ��Ϊ����˯�߹���,�������ǻ���Ҫ�ڸ���֮ǰ�ټ��һ�»�����е������Ƿ�����Ч��.����������ǻ���Ҫ�ͷŻ����.
	for (i = 0 ; i < 4 ; i++, address += BLOCK_SIZE)
		if (bh[i]) {
			wait_on_buffer(bh[i]);						// �ȴ���������(���������Ļ�).
			if (bh[i]->b_uptodate)						// ���������������Ч�Ļ�����.
				COPYBLK((unsigned long) bh[i]->b_data, address);
			brelse(bh[i]);								// �ͷŸû�����.
		}
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
/*
 * OK,breada������breadһ��ʹ��,��������Ԥ��һЩ��.�ú��������б���Ҫʹ��һ�����������������б�Ľ���.
 */
// ��ָ���豸��ȡָ����һЩ��.
// �������������ɱ�,��һϵ��ָ���Ŀ��.�ɹ�ʱ���ص�1��Ļ����ͷָ��,���򷵻�NULL.
struct buffer_head * breada(int dev, int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;

	// ����ȡ�ɱ�������е�1������(���).���Ŵӵ�����������ȡָ���豸�Ϳ�ŵĻ����.����û����������Ч(���±�־δ��λ),��
	// �������豸���ݿ�����.
	va_start(args, first);
	if (!(bh = getblk(dev, first)))
		panic("bread: getblk returned NULL\n");
	if (!bh->b_uptodate)
		ll_rw_block(READ, bh);
	// Ȼ��˳��ȡ�ɱ������������Ԥ�����,����������ͬ������,��������.ע��,336������һ��bug.���е�bhӦ����tmp.���bugֱ����0.96
	// ����ں˴����вű���������.����,��Ϊ������Ԥ���������ݿ�,ֻ������������������������Ͼ�ʹ��,���Ե�337�������Ҫ�������ü���
	// �ݼ��ͷŵ��ÿ�(��Ϊgetblk()���������ӻ�������ü���ֵ).
	while ((first = va_arg(args, int)) >= 0) {
		tmp = getblk(dev, first);
		if (tmp) {
			if (!tmp->b_uptodate)
				ll_rw_block(READA, tmp);
			tmp->b_count--;					// ��ʱ�ͷŵ���Ԥ����.
		}
	}
	// ��ʱ�ɱ�����������в����������.���ǵȴ���1������������(����ѱ�����).�ڵȴ��˳�֮�������������������Ȼ��Ч,�򷵻ػ�����ͷָ��
	// �˳�.�����ͷŸû���������NULL,�˳�.
	va_end(args);
	wait_on_buffer(bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return (NULL);
}

// ��������ʼ������
// ����buffer_end�ǻ������ڴ�ĩ��.���ھ���16M�ڴ��ϵͳ,������ĩ�˱�����Ϊ4MB.������8MB�ڴ��ϵͳ,������ĩ�˱�����2MB.�ú����ӻ�������ʼλ��
// start_buffer���ͻ�����ĩ��buffer_end���ֱ�ͬʱ����(��ʼ��)�����ͷ�ṹ�Ͷ�Ӧ�����ݿ�.ֱ���������������ڴ汻�������.
void buffer_init(long buffer_end)
{
	struct buffer_head * h = start_buffer;
	void * b;
	int i;

	// ���ȸ��ݲ����ṩ�Ļ������߶�λ��ȷ��ʵ�ʻ������߶�λ��b.����������߶˵���1MB,����Ϊ��640KB-1MB����ʾ�ڴ��BIOSռ��,����ʵ�ʿ��û������ڴ�
	// �߶�λ��Ӧ����640KB.���򻺳����ڴ�߶�һ������1MB.
	if (buffer_end == 1 << 20)
		b = (void *) (640 * 1024);
	else
		b = (void *) buffer_end;
	// ��δ������ڳ�ʼ��������,�������л����ѭ������,����ȡϵͳ�л������Ŀ.�����Ĺ����Ǵӻ������߶˿�ʼ����1KB��С�Ļ����,���ͬʱ�ڻ������Ͷ˽���
	// �����û����Ľṹbuffer_head,������Щbuffer_head���˫������.
	// h��ָ�򻺳�ͷ�ṹ��ָ��,��h+1��ָ���ڴ��ַ��������һ������ͷ��ַ,Ҳ����˵��ָ��h������ͷ��ĩ����.Ϊ�˱�֤���㹻���ȵ��ڴ����洢һ������ͷ�ṹ,
	// ��Ҫb��ָ����ڴ���ַ>=h����ͷ��ĩ��,��Ҫ��>=h+1.
	while ( (b -= BLOCK_SIZE) >= ((void *) (h + 1)) ) {
		h->b_dev = 0;								// ʹ�øû������豸��.
		h->b_dirt = 0;								// ���־,��������޸ı�־.
		h->b_count = 0;								// ��������ü���.
		h->b_lock = 0;								// �����������־.
		h->b_uptodate = 0;							// �������±�־(���������Ч��־).
		h->b_wait = NULL;							// ָ��ȴ��û��������Ľ���.
		h->b_next = NULL;							// ָ�������ͬhashֵ����һ������ͷ.
		h->b_prev = NULL;							// ָ�������ͬhashֵ��ǰһ������ͷ.
		h->b_data = (char *) b;						// ָ���Ӧ��������ݿ�(1024�ֽ�).
		h->b_prev_free = h - 1;						// ָ��������ǰһ��.
		h->b_next_free = h + 1;						// ָ����������һ��.
		h++;										// hָ����һ�»���ͷλ��.
		NR_BUFFERS++;								// �����������ۼ�.
		if (b == (void *) 0x100000)					// ��b�ݼ�������1MB,������384KB
			b = (void *) 0xA0000;					// ��bָ���ַ0xA0000(640KB)��.
	}
	h--;											// ��hָ�����һ����Ч�����ͷ.
	free_list = start_buffer;						// �ÿ�������ͷָ��ͷһ�������.
	free_list->b_prev_free = h;     				// ����ͷ��b_prev_freeָ��ǰһ������һ���
	h->b_next_free = free_list;     				// h����һ��ָ��ָ���һ��γ�һ��������
	// ����ʼ��hash��(��ϣ��ɢ�б�),�ñ�������ָ��ΪNULL��
	for (i = 0; i < NR_HASH; i++)
		hash_table[i] = NULL;
}

