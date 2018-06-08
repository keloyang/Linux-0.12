/*
 *  linux/fs/inode.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <string.h>
#include <sys/stat.h>

#include <linux/sched.h>
#include <linux/kernel.h>
//#include <linux/mm.h>
#include <asm/system.h>

// �������ݿ�����ָ������.ÿ��ָ����ָ��ָ�����豸�ŵ��ܿ�������hd_sizes[].���ܿ�������ÿһ���Ӧ���豸��ȷ����һ�����豸����ӵ�е�
// ���ݿ�����(1���С = 1KB).
extern int *blk_size[];

struct m_inode inode_table[NR_INODE]={{0, }, };   					// �ڴ���i�ڵ��(NR_INODE=64��)

static void read_inode(struct m_inode * inode);						// ��ָ��i�ڵ�ŵ�i�ڵ���Ϣ.
static void write_inode(struct m_inode * inode);					// дi�ڵ���Ϣ�����ٻ�����.

// �ȴ�ָ����i�ڵ����.
// ���i�ڵ��ѱ�����,�򽫵�ǰ������Ϊ�����жϵĵȴ�״̬,����ӵ���i�ڵ�ĵȴ�����i_wait��.ֱ����i�ڵ��������ȷ�ػ��ѱ�����.
static inline void wait_on_inode(struct m_inode * inode)
{
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);									// kernel/sched.c
	sti();
}

// ��i�ڵ�����(����ָ����i�ڵ�)
// ���i�ڵ��ѱ�����,�򽫵�ǰ������Ϊ�����жϵĵȴ�״̬,����ӵ���i�ڵ�ĵȴ�����i_wait��.ֱ����i�ڵ��������ȷ�ػ��ѱ�����.Ȼ��
// ��������.
static inline void lock_inode(struct m_inode * inode)
{
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	inode->i_lock = 1;												// ��������־.
	sti();
}

// ��ָ����i�ڵ����.
// ��λi�ڵ��������־,����ȷ�ػ��ѵȴ��ڴ�i�ڵ�ȴ�����i_wait�ϵ����н���.
static inline void unlock_inode(struct m_inode * inode)
{
	inode->i_lock = 0;
	wake_up(&inode->i_wait);										// kernel/sched.c
}

// �ͷ��豸dev���ڴ�i�ڵ���е�����i�ڵ㡣
// ɨ���ڴ��е�i�ڵ�����飬�����ָ���豸ʹ�õ�i�ڵ���ͷ�֮��
void invalidate_inodes(int dev)
{
	int i;
	struct m_inode * inode;

	// ������ָ��ָ���ڴ�i�ڵ���������Ȼ��ɨ��i�ڵ��ָ�������е�����i�ڵ㡣�������ÿ��i�ڵ㣬�ȵȴ���i�ڵ�
	// �������ã���Ŀǰ���������Ļ��������ж��Ƿ�����ָ���豸��i�ڵ㡣�����ָ���豸��i�ڵ㣬�򿴿����Ƿ񻹱�ʹ��
	// �ţ��������ü����Ƿ�Ϊ0����������ʾ������Ϣ��Ȼ���ͷ�֮������i�ڵ���豸���ֶ�i_dev�á���50���ϵ�ָ��
	// ��ֵ"0+inode_table"��ͬ��"inode_table"��"&inode_table[0]"����������д���ܸ�����һЩ��
	inode = 0 + inode_table;                  						// ָ��i�ڵ��ָ���������
	for(i = 0 ; i < NR_INODE ; i++, inode++) {
		wait_on_inode(inode);           							// �ȴ���i�ڵ���ã���������
		if (inode->i_dev == dev) {
			if (inode->i_count)     								// ������������Ϊ0,����ʾ�����档
				printk("inode in use on removed disk\n\r");
			inode->i_dev = inode->i_dirt = 0;       				// �ͷ�i�ڵ㣨���豸��Ϊ0����
		}
	}
}

// ͬ������i�ڵ㡣
// ���ڴ�i�ڵ��������i�ڵ����豸��i�ڵ���ͬ��������
void sync_inodes(void)
{
	int i;
	struct m_inode * inode;

	// �������ڴ�i�ڵ����͵�ָ��ָ��i�ڵ�����Ȼ��ɨ������i�ڵ���еĽڵ㡣�������ÿ��i�ڵ㣬�ȵȴ���i�ڵ�������ã���
	// Ŀǰ���������Ļ�����Ȼ���жϸ�i�ڵ��Ƿ��ѱ��޸Ĳ��Ҳ��ǹܵ��ڵ㡣������������򽫸�i�ڵ�д����ٻ������У�����������
	// ����buffer.c�����ʵ�ʱ��������д�����С�
	inode = 0 + inode_table;                          				// ��ָ������ָ��i�ڵ��ָ���������
	for(i = 0 ; i < NR_INODE ; i++, inode++) {           			// ɨ��i�ڵ��ָ�����顣
		wait_on_inode(inode);                   					// �ȴ���i�ڵ���ã���������
		if (inode->i_dirt && !inode->i_pipe)    					// ��i�ڵ����޸��Ҳ��ǹܵ��ڵ㣬
			write_inode(inode);             						// ��д�̣�ʵ����д�뻺�����У���
	}
}

// �ļ����ݿ�ӳ�䵽�̿�Ĵ������.(blockλͼ������,bmap - block map)
// ����:inode - �ļ���i�ڵ�ָ��;block - �ļ��е����ݿ��;create - �������־.�ú�����ָ�����ļ����ݿ�block��Ӧ���豸���߼�����,�������߼����.
// ���������־��λ,�����豸�϶�Ӧ�߼��鲻����ʱ�������´��̿�,�����ļ����ݿ�block��Ӧ���豸�ϵ��߼����(�̿��).
static int _bmap(struct m_inode * inode, int block, int create)
{
	struct buffer_head * bh;
	int i;

	// �����жϲ����ļ����ݿ��block����Ч��.������С��0,��ͣ��.�����Ŵ���ֱ�ӿ��� + ��ӿ��� + ���μ�ӿ���,�����ļ�ϵͳ��ʾ��Χ,��ͣ��.
	if (block < 0)
		panic("_bmap: block<0");
	if (block >= 7 + 512 + 512 * 512)
		panic("_bmap: block>big");
	// Ȼ������ļ���ŵĴ�Сֵ���Ƿ������˴�����־�ֱ���д���.����ÿ��С��7,��ʹ��ֱ�ӿ��ʾ.���������־��λ,����i�ڵ��ж�Ӧ�ÿ���߼���(����)�ֶ�Ϊ0,
	// ������Ӧ�豸����һ���̿�(�߼���),���ҽ������߼����(�̿��)�����߼����ֶ���.Ȼ������i�ڵ�ı�ʱ��,��i�ڵ����޸ı�־.��󷵻��߼����.����new_block()
	// ������bitmap.c������.
	if (block < 7) {
		if (create && !inode->i_zone[block])
			if (inode->i_zone[block] = new_block(inode->i_dev)) {
				inode->i_ctime = CURRENT_TIME;
				inode->i_dirt = 1;
			}
		return inode->i_zone[block];
	}
	// ����ÿ��>=7,��С��7+512,��˵��ʹ�õ���һ�μ�ӿ�.�����һ�μ�ӿ���д���.����Ǵ���,���Ҹ�i�ڵ��ж�Ӧ��ӿ��ֶ�i_zone[7]��0,�����ļ����״�ʹ�ü�ӿ�,
	// ��������һ���̿����ڴ�ż�ӿ���Ϣ,������ʵ�ʴ��̿�������ӿ��ֶ���.Ȼ������i�ڵ����޸ı�־���޸�ʱ��.�������ʱ������̿�ʧ��,���ʱi�ڵ��ӿ��ֶ�i_zone[7]
	// Ϊ0,�򷵻�0.���߲��Ǵ���,��i_zone[7]ԭ����Ϊ0,����i�ڵ���û�м�ӿ�,����ӳ����̿�ʧ��,����0�˳�.
	block -= 7;
	if (block < 512) {
		// ���������־��λ��ͬʱ����7���λ��û�а󶨵���Ӧ���߼���,������һ���߼���
		if (create && !inode->i_zone[7])
			if (inode->i_zone[7] = new_block(inode->i_dev)) {
				inode->i_dirt = 1;
				inode->i_ctime = CURRENT_TIME;
			}
		if (!inode->i_zone[7])
			return 0;
		// ���ڶ�ȡ�豸�ϸ�i�ڵ��һ�μ�ӿ�.��ȡ�ü�ӿ��ϵ�block���е��߼����(�̿��)i.ÿһ��ռ2���ֽ�.����Ǵ������Ҽ�ӿ�ĵ�block���е��߼����Ϊ0�Ļ�,������һ���̿�,
		// ���ü�ӿ��еĵ�block����ڸ����߼�����.Ȼ����λ��ӿ�����޸ı�־.������Ǵ���,��i������Ҫӳ��(Ѱ��)���߼����.
		if (!(bh = bread(inode->i_dev, inode->i_zone[7])))
			return 0;
		i = ((unsigned short *) (bh->b_data))[block];
		if (create && !i)
			if (i = new_block(inode->i_dev)) {
				((unsigned short *) (bh->b_data))[block] = i;
				bh->b_dirt = 1;
			}
		// ����ͷŸü�ӿ�ռ�õĻ����,�����ش������������ԭ�еĶ�Ӧblock���߼�����.
		brelse(bh);
		return i;
	}
	// ���������е���,��������ݿ����ڶ��μ�ӿ�.�䴦�������һ�μ�ӿ�����.�����ǶԶ��μ�ӿ�Ĵ���.���Ƚ�block�ټ�ȥ��ӿ������ɵĿ���(512).Ȼ�����
	// �Ƿ������˴�����־���д�����Ѱ�Ҵ���.������´�������i�ڵ�Ķ��μ�ӿ��ֶ�Ϊ0,��������һ���̿����ڴ�Ŷ��μ�ӿ��һ������Ϣ,������ʵ�ʴ��̿������
	// ���μ�ӿ��ֶ���.֮��,��i�ڵ����޸ı��ƺ��޸�ʱ��.ͬ����,�������ʱ������̿�ʧ��,���ʱi�ڵ���μ�ӿ��ֶ�i_zone[8]Ϊ0,�򷵻�0.���߲��Ǵ���,��
	// i_zone[8]ԭ����Ϊ0,����i�ڵ���û�м�ӿ�,����ӳ����̿�ʧ��,����0�˳�.
	block -= 512;
	if (create && !inode->i_zone[8])
		if (inode->i_zone[8] = new_block(inode->i_dev)) {
			inode->i_dirt = 1;
			inode->i_ctime = CURRENT_TIME;
		}
	if (!inode->i_zone[8])
		return 0;
	// ���ڶ�ȡ�豸�ϸ�i�ڵ�Ķ��μ�ӿ�.��ȡ�ö��μ�ӿ��һ�����ϵ�(block/512)���е��߼����i.����Ǵ������Ҷ��μ�ӿ��һ�����ϵ�(block/512)���е��߼�
	// ���Ϊ0�Ļ�,��������һ���̿�(�߼���)��Ϊ���μ�ӿ�Ķ�����i,���ö��μ�ӿ��һ�����е�(block/512)����ڸö�����Ŀ��i.Ȼ����λ���μ�ӿ��һ������
	// �޸ı�־.���ͷŶ��μ�ӿ��һ����.������Ǵ���,��i������Ҫӳ��(Ѱ��)���߼����.
	if (!(bh = bread(inode->i_dev, inode->i_zone[8])))
		return 0;
	i = ((unsigned short *)bh->b_data)[block >> 9];
	if (create && !i)
		if (i = new_block(inode->i_dev)) {
			((unsigned short *) (bh->b_data))[block >> 9] = i;
			bh->b_dirt=1;
		}
	brelse(bh);
	// ������μ�ӿ�Ķ�������Ϊ0,��ʾ�������ʧ�ܻ���ԭ����Ӧ��ž�Ϊ0,�򷵻�0�˳�.����ʹ��豸�϶�ȡ���μ�ӿ�Ķ�����,��ȡ�ö������ϵ�block���е��߼���
	// ��(����511��Ϊ���޶�blockֵ������511).
	if (!i)
		return 0;
	if (!(bh = bread(inode->i_dev, i)))
		return 0;
	i = ((unsigned short *)bh->b_data)[block & 511];
	// ����Ǵ������Ҷ�����ĵ�block�����߼����Ϊ0�Ļ�,������һ���̿�(�߼���),��Ϊ���մ��������Ϣ�Ŀ�.���ö������еĵ�block����ڸ����߼�����(i).Ȼ����λ
	// ����������޸ı�־.
	if (create && !i)
		if (i = new_block(inode->i_dev)) {
			((unsigned short *) (bh->b_data))[block & 511] = i;
			bh->b_dirt = 1;
		}
	// ����ͷŸö��μ�ӿ�Ķ�����,���ش�����������Ļ�ԭ�еĶ�Ӧblock���߼�����.
	brelse(bh);
	return i;
}

// ȡ�ļ����ݿ�block���豸�϶�Ӧ���߼����.
// ����:inode - �ļ����ڴ�i�ڵ�ָ��;block - �ļ��е����ݿ��.
// �������ɹ��򷵻ض�Ӧ���߼����,���򷵻�0.
int bmap(struct m_inode * inode, int block)
{
	return _bmap(inode, block, 0);
}

// ȡ�ļ����ݿ�block���豸�϶�Ӧ���߼���š������Ӧ���߼��鲻���ھʹ���һ�顣�������豸�϶�Ӧ���߼���š�
// ������inode - �ļ�������i�ڵ�ָ�룻block - �ļ��е����ݿ�š�
// �������ɹ��򷵻ض�Ӧ���߼���ţ����򷵻�0.
int create_block(struct m_inode * inode, int block)
{
	return _bmap(inode, block, 1);
}

// �Ż�(����)һ��i�ڵ�(��д���豸).
// �ú�����Ҫ���ڰ�i�ڵ����ü���ֵ�ݼ�1,�������ǹܵ�i�ڵ�,���ѵȴ��Ľ���.���ǿ��豸�ļ�i�ڵ���ˢ���豸.������i�ڵ�����Ӽ���Ϊ0,���ͷŸ�
// i�ڵ�ռ�õ����д����߼���,���ͷŸ�i�ڵ�.
void iput(struct m_inode * inode)
{
	// �����жϲ���������i�ڵ����Ч��,���ȴ�inode�ڵ����(����Ѿ������Ļ�).���i�ڵ�����ü���Ϊ0,��ʾ��i�ڵ��Ѿ��ǿ��е�.�ں���Ҫ��������
	// �Żز���,˵���ں�����������������.������ʾ������Ϣ��ͣ��.
	if (!inode)
		return;
	wait_on_inode(inode);
	if (!inode->i_count)
		panic("iput: trying to free free inode");
	// ����ǹܵ�i�ڵ�,���ѵȴ��ùܵ��Ľ���,���ô�����1,������������򷵻�.�����ͷŹܵ�ռ�õ��ڴ�ҳ��,����λ�ýڵ�����ü���ֵ,���޸ı�־�͹ܵ�
	// ��־,������.���ڹܵ��ڵ�,inode->i_size������ڴ�ҳ��ַ.�μ�get_pipe_inode().
	if (inode->i_pipe) {
		wake_up(&inode->i_wait);
		wake_up(&inode->i_wait2);
		if (--inode->i_count)
			return;
		free_page(inode->i_size);
		inode->i_count = 0;
		inode->i_dirt = 0;
		inode->i_pipe = 0;
		return;
	}
	// ���i�ڵ��Ӧ���豸�� =0,�򽫴˽ڵ�����ü����ݼ�1,����.�������ڹܵ�������i�ڵ�,��i�ڵ���豸��Ϊ0.
	if (!inode->i_dev) {
		inode->i_count--;
		return;
	}
	// ����ǿ��豸�ļ���i�ڵ�,��ʱ�߼����ֶ�0(i_zone[0])�����豸��,��ˢ�¸��豸.���ȴ�i�ڵ����.
	if (S_ISBLK(inode->i_mode)) {
		sync_dev(inode->i_zone[0]);
		wait_on_inode(inode);
	}
	// ���i�ڵ����ü�������1,������ݼ�1���ֱ�ӷ���(��Ϊ��i�ڵ㻹��������,�����ͷ�),�����˵��i�ڵ�����ü���ֵΪ1(��Ϊ��157���Ѿ��жϹ������Ƿ�Ϊ��).
	// ���i�ڵ��������Ϊ0,��˵��i�ڵ��Ӧ�ļ���ɾ��.�����ͷŸ�i�ڵ�������߼���,���ͷŸ�i�ڵ�.����free_inode()����ʵ���ͷ�i�ڵ����,����λi�ڵ�
	// ��Ӧ��i�ڵ�λͼ,���i�ڵ�ṹ����.
repeat:
	if (inode->i_count > 1) {
		inode->i_count--;
		return;
	}
	if (!inode->i_nlinks) {
		// �ͷŸ�i�ڵ��Ӧ�������߼���
		truncate(inode);
		// �Ӹ��豸�ĳ�������ɾ����i�ڵ�
		free_inode(inode);      								// bitmap.c
		return;
	}
	// �����i�ڵ��������޸�,���д���¸�i�ڵ�,���ȴ���i�ڵ����.����������дi�ڵ�ʱ��Ҫ�ȴ�˯��,��ʱ���������п����޸ĸ�i�ڵ�,����ڽ��̱����Ѻ���Ҫ�ظ�
	// ���������жϹ���(repeat).
	if (inode->i_dirt) {
		write_inode(inode);										/* we can sleep - so do again */
		wait_on_inode(inode);									/* ��Ϊ����˯����,����Ҫ�ظ��ж� */
		goto repeat;
	}
	// ��������ִ�е���,˵����i�ڵ�����ü���ֵi_count��1,��������Ϊ��,��������û�б��޸Ĺ�.��˴�ʱֻҪ��i�ڵ����ü����ݼ�1,����.��ʱ��i�ڵ��i_count=0,
	// ��ʾ���ͷ�.
	inode->i_count--;
	return;
}

// ��i�ڵ��(inode_table)�л�ȡһ������i�ڵ���.
// Ѱ�����ü���countΪ0��i�ڵ�,������д�̺�����,������ָ��.���ü�������1.
struct m_inode * get_empty_inode(void)
{
	struct m_inode * inode;
	static struct m_inode * last_inode = inode_table;			// ָ��i�ڵ���1��.
	int i;

	// �ڳ�ʼ��last_inodeָ��ָ��i�ڵ��ͷһ���ѭ��ɨ������i�ڵ��,���last_inode�Ѿ�ָ��i�ڵ������1��֮��,����������ָ��i�ڵ��ʼ��,
	// �Լ���ѭ��Ѱ�ҿ���i�ڵ���.���last_inode��ָ���i�ڵ����ֵΪ0,��˵�������ҵ�����i�ڵ���.��inodeָ���i�ڵ�.�����i�ڵ�����޸ı�־��
	// ��������־��Ϊ0,�����ǿ���ʹ�ø�i�ڵ�,�����˳�forѭ��.
	do {
		inode = NULL;
		for (i = NR_INODE; i ; i--) {							// NR_INODE = 64.
			if (++last_inode >= inode_table + NR_INODE)
				last_inode = inode_table;
			if (!last_inode->i_count) {
				inode = last_inode;
				if (!inode->i_dirt && !inode->i_lock)
					break;
			}
		}
		// ���û���ҵ�����i�ڵ�(inode=NULL),��i�ڵ���ӡ����������ʹ��,��ͣ��.
		if (!inode) {
			for (i = 0 ; i < NR_INODE ; i++)
				printk("%04x: %6d\t", inode_table[i].i_dev,
					inode_table[i].i_num);
			panic("No free inodes in mem");
		}
		// �ȴ���i�ڵ����(����ֱ������Ļ�).�����i�ڵ����޸ı�־����λ�Ļ�,�򽫸�i�ڵ�ˢ��(ͬ��).��Ϊˢ��ʱ���ܻ�˯��,�����Ҫ�ٴ�ѭ���ȴ�i�ڵ����.
		wait_on_inode(inode);
		while (inode->i_dirt) {
			write_inode(inode);
			wait_on_inode(inode);
		}
	} while (inode->i_count);
	// ���i�ڵ��ֱ�����ռ�õĻ�(i�ڵ�ļ���ֵ��Ϊ0��),������Ѱ�ҿ���i�ڵ�.����˵�����ҵ�����Ҫ��Ŀ���i�ڵ���.�򽫸�i�ڵ�����������,�������ü���Ϊ1,���ظ�
	// i�ڵ�ָ��.
	memset(inode, 0, sizeof(*inode));
	inode->i_count = 1;
	return inode;
}

// ��ȡ�ܵ��ڵ㡣
// ����ɨ��i�ڵ��Ѱ��һ������i�ڵ��Ȼ��ȡ��һҳ�����ڴ湩�ܵ�ʹ�á�Ȼ�󽫵õ���i�ڵ�����ü�����Ϊ2������
// ��д�ߣ�����ʼ���ܵ�ͷ��β����i�ڵ�Ĺܵ����ͱ�־������i�ڵ�ָ�룬���ʧ���򷵻�NULL��
struct m_inode * get_pipe_inode(void)
{
	struct m_inode * inode;

	// ���ȴ��ڴ�i�ڵ����ȡ��һ������i�ڵ㡣����Ҳ�������i�ڵ��򷵻�NULL��Ȼ��Ϊ��i�ڵ�����һҳ�ڴ棬���ýڵ��
	// i_size�ֶ�ָ���ҳ�档�����û�п����ڴ棬���ͷŸ�i�ڵ㣬������NULL��
	if (!(inode = get_empty_inode()))
		return NULL;
	if (!(inode->i_size = get_free_page())) {         			// �ڵ��i_size�ֶ�ָ�򻺳�����
		inode->i_count = 0;
		return NULL;
	}
	// Ȼ�����ø�i�ڵ�����ü���Ϊ2,����λ�ܵ�ͷβָ�롣i�ڵ��߼��������i_zone[]��i_zone[0]��i_zone[1]�зֱ���
	// ����Źܵ�ͷ�͹ܵ�βָ�롣�������i�ڵ��ǹܵ�i�ڵ��־�����ظ�i�ڵ�š�
	inode->i_count = 2;											/* sum of readers/writers */    /* ��/д�����ܼ� */
	PIPE_HEAD(*inode) = PIPE_TAIL(*inode) = 0;      			// ��λ�ܵ�ͷβָ�롣
	inode->i_pipe = 1;                              			// �ýڵ�Ϊ�ܵ�ʹ�ñ�־��
	return inode;
}

// ȡ��һ��i�ڵ�.
// ����:dev - �豸��;nr - i�ڵ��.
// ���豸�϶�ȡָ���ڵ�ŵ�i�ڵ�ṹ���ݵ��ڴ�i�ڵ����,���ҷ��ظ�i�ڵ�ָ��.������λ�ڸ��ٻ������е�i�ڵ������Ѱ,���ҵ�ָ���ڵ�ŵ�i�ڵ����ھ���һЩ
// �жϴ���󷵻ظ�i�ڵ�ָ��.������豸dev�϶�ȡָ��i�ڵ�ŵ�i�ڵ���Ϣ����i�ڵ����,�����ظ�i�ڵ�ָ��.
struct m_inode * iget(int dev, int nr)
{
	struct m_inode * inode, * empty;

	// �����жϲ�����Ч��.���豸����0,������ں˴�������,��ʾ������Ϣ��ͣ��.Ȼ��Ԥ�ȴ�i�ڵ����ȡһ������i�ڵ㱸��.
	if (!dev)
		panic("iget with dev==0");
	empty = get_empty_inode();
	// ����ɨ��i�ڵ��.Ѱ�Ҳ���ָ���ڵ��nr��i�ڵ�.�������ýڵ�����ô���.�����ǰɨ��i�ڵ���豸�Ų�����ָ�����豸�Ż��߽ڵ�Ų�����ָ���Ľڵ��,�����ɨ��.
	inode = inode_table;
	while (inode < NR_INODE + inode_table) {
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode++;
			continue;
		}
		// ����ҵ�ָ���豸��dev�ͽڵ��nr��i�ڵ�,��ȴ��ýڵ����(����������Ļ�).�ڵȴ��ýڵ����������,i�ڵ���ܻᷢ���仯.�����ٴν���������ͬ�ж�.�������
		// �˱仯,������ɨ������i�ڵ��.
		wait_on_inode(inode);
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode = inode_table;
			continue;
		}
		// �������ʾ�ҵ���Ӧ��i�ڵ�.���ǽ���i�ڵ����ü�����1.Ȼ��������һ�����,�����Ƿ�����һ���ļ�ϵͳ�İ�װ��.������Ѱ�ұ���װ�ļ�ϵͳ���ڵ㲢����.�����i�ڵ�
		// ��ȷ�������ļ�ϵͳ�İ�װ��,���ڳ����������Ѱ��װ�ڴ�i�ڵ�ĳ�����.���û���ҵ�,����ʾ������Ϣ,���Żر�������ʼʱ��ȡ�Ŀ��нڵ�empty,���ظ�i�ڵ�ָ��.
		inode->i_count++;
		if (inode->i_mount) {
			int i;

			for (i = 0 ; i < NR_SUPER ; i++)
				if (super_block[i].s_imount == inode)
					break;
			if (i >= NR_SUPER) {
				printk("Mounted inode hasn't got sb\n");
				if (empty)
					iput(empty);
				return inode;
			}
			// ִ�е������ʾ�Ѿ��ҵ���װ��inode�ڵ���ļ�ϵͳ������.���ǽ���i�ڵ�д�̷Ż�,���Ӱ�װ�ڴ�i�ڵ��ϵ��ļ�ϵͳ��������ȡ�豸��,����i�ڵ��ΪROOT_INO.Ȼ��
			// ����ɨ������i�ڵ��,�Ի�ȡ�ñ���װ�ļ�ϵͳ�ĸ�i�ڵ���Ϣ.
			iput(inode);
			dev = super_block[i].s_dev;
			nr = ROOT_INO;
			inode = inode_table;
			continue;
		}
		// ���������ҵ�����Ӧ��i�ڵ�.��˿��Է�����������ʼ����ʱ �Ŀ���i�ڵ�,�����ҵ���i�ڵ�ָ��.
		if (empty)
			iput(empty);
		return inode;
    }
	// ���������i�ڵ����û���ҵ�ָ����i�ڵ�,������ǰ������Ŀ���i�ڵ�empty��i�ڵ���н�����i�ڵ�.������Ӧ�豸�϶�ȡ��i�ڵ���Ϣ,���ظ�i�ڵ�ָ��.
	if (!empty)
		return (NULL);
	inode = empty;
	inode->i_dev = dev;									// ����i�ڵ���豸.
	inode->i_num = nr;									// ����i�ڵ��.
	read_inode(inode);      							// ��ȡi�ڵ���Ϣ
	return inode;
}

// ��ȡָ��i�ڵ���Ϣ.
// ���豸�϶�ȡ����ָ��i�ڵ���Ϣ��i�ڵ��̿�,Ȼ���Ƶ�ָ����i�ڵ�ṹ��.Ϊ��ȷ��i�ڵ������豸�߼����(�򻺳��),�������ȶ�ȡ��Ӧ�豸�ϵĳ�����,��
// ��ȡ���ڼ����߼���ŵ�ÿ��i�ڵ�����ϢINODES_PER_BLOCK.�ڼ����i�ڵ����ڵ��߼���ź�,�ͰѸ��߼������һ�������.Ȼ��ѻ��������Ӧλ�ô���i�ڵ�
// ���ݸ��Ƶ�ָ����λ�ô�.
static void read_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	// ����������i�ڵ�,��ȡ�ýڵ������豸�ĳ�����.
	lock_inode(inode);
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to read inode without dev");
	// ��i�ڵ������豸�߼���� = (������ + ������) + i�ڵ�λͼռ�õĿ��� + �߼���λͼ�Ŀ��� + (i�ڵ��-1)/ÿ�麬�е�i�ڵ���.��Ȼi�ڵ�Ŵ�0��ʼ���,
	// ����1��0��i�ڵ㲻��,���Ҵ�����Ҳ�������Ӧ��0��i�ڵ�ṹ.��˴��i�ڵ���̿�ĵ�1���ϱ������i�ڵ����1--16��i�ڵ�ṹ������0--15��.������������
	// i�ڵ�Ŷ�Ӧ��i�ڵ�ṹ�����̿�ʱ��Ҫ��1,��:B = (i�ڵ��-1)/ÿ�麬��i�ڵ�ṹ��.����,�ڵ��16��i�ڵ�ṹӦ����B=(16-1)/16 = 0�Ŀ���.�������Ǵ�
	// �豸�϶�ȡ��i�ڵ������߼���,������ָ��i�ڵ����ݵ�inodeָ����ָλ�ô�.
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num - 1) / INODES_PER_BLOCK;
	// ��i�ڵ���Ϣ���Ǹ��߼����ȡ�����ٻ�����
	if (!(bh = bread(inode->i_dev, block)))
		panic("unable to read i-node block");
	*(struct d_inode *)inode =
		((struct d_inode *)bh->b_data)
			[(inode->i_num - 1) % INODES_PER_BLOCK];
	// ����ͷŶ���Ļ����,��������i�ڵ�.���ڿ��豸�ļ�,����Ҫ����i�ڵ���ļ���󳤶�ֵ.
	brelse(bh);
	if (S_ISBLK(inode->i_mode)) {
		int i = inode->i_zone[0];							// ���ڿ��豸�ļ�,i_zone[0]�����豸��.
		if (blk_size[MAJOR(i)])
			inode->i_size = 1024 * blk_size[MAJOR(i)][MINOR(i)];
		else
			inode->i_size = 0x7fffffff;
	}
	unlock_inode(inode);
}

// ��i�ڵ���Ϣд�뻺������.
// �ú����Ѳ���ָ����i�ڵ�д�뻺������Ӧ�Ļ������,��������ˢ��ʱ��д������.Ϊ��ȷ��i�ڵ����ڵ��豸�߼����(�򻺳��),�������ȶ�ȡ��Ӧ�豸�ϵĳ�����,
// �Ի�ȡ���ڼ����߼���ŵ�ÿ��i�ڵ�����ϢINODES_PER_BLOCK.�ڼ����i�ڵ����ڵ��߼���ź�,�ͰѸ��߼������һ�������.Ȼ���i�ڵ����ݸ��Ƶ�������
// ��Ӧλ�ô�.
static void write_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	// ����������i�ڵ�,�����i�ڵ�û�б��޸Ĺ����߸�i�ڵ���豸�ŵ�����,�������i�ڵ�,���˳�.����û�б��޸Ĺ���i�ڵ�,�������뻺�����л��豸�е���ͬ.Ȼ���ȡ
	// ��i�ڵ�ĳ�����.
	lock_inode(inode);
	if (!inode->i_dirt || !inode->i_dev) {
		unlock_inode(inode);
		return;
	}
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to write inode without device");
	// ��i�ڵ����ڵ��豸�߼��� = (������ + ������) + i�ڵ�λͼռ�õĿ��� + �߼���λͼռ�õĿ��� + (i�ڵ��-1)/ÿ�麬�е�i�ڵ���.���Ǵ��豸�϶�ȡ��i�ڵ���
	// �ڵ��߼���,������i�ڵ���Ϣ���Ƶ��߼����Ӧ��i�ڵ����λ�ô�.
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num - 1) / INODES_PER_BLOCK;
	if (!(bh = bread(inode->i_dev, block)))
		panic("unable to read i-node block");
	((struct d_inode *)bh->b_data)
		[(inode->i_num - 1) % INODES_PER_BLOCK] =
			*(struct d_inode *)inode;
	// Ȼ���û��������޸ı�־,��i�ڵ������Ѿ��뻺�����е�һ��,����޸ı�־����.Ȼ���ͷŸú���i�ڵ�Ļ�����,��������i�ڵ�.
	bh->b_dirt = 1;
	inode->i_dirt = 0;
	brelse(bh);
	unlock_inode(inode);
}

