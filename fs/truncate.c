/*
 *  linux/fs/truncate.c
 *
 *  (C) 1991  Linus Torvalds
 */
#include <linux/sched.h>        								// ���ȳ���ͷ�ļ�������������ṹtask_struct������0���ݵȡ�

#include <sys/stat.h>           								// �ļ�״̬ͷ�ļ��������ļ����ļ�ϵͳ״̬�ṹstat{}�ͳ�����

// �ͷ�����һ�μ�ӿ顣���ڲ�������
// ����dev���ļ�ϵͳ�����豸���豸�ţ�block���߼���š��ɹ��򷵻�1�����򷵻�0��
static int free_ind(int dev, int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;
	int block_busy;

	// �����жϲ�������Ч�ԡ�����߼����Ϊ0,�򷵻ء�Ȼ���ȡһ�μ�ӿ飬���ͷ����ϱ���ʹ�õ������߼��飬Ȼ��
	// �ͷŸ�һ�μ�ӿ�Ļ���顣����free_block()�����ͷ��豸��ָ���߼���ŵĴ��̿飨fs/bitmap.c����
	if (!block)
		return 1;
	block_busy = 0;
	if (bh = bread(dev, block)) {
		p = (unsigned short *) bh->b_data;              		// ָ�򻺳����������
		for (i = 0; i < 512; i++, p++)                         	// ÿ���߼����Ͽ���512����š�
			if (*p)
				if (free_block(dev, *p)) {       				// �ͷ�ָ�����豸�߼��顣
					*p = 0;                 					// ���㡣
					bh->b_dirt = 1;         					// �������޸ı�־��
				} else
					block_busy = 1;         					// �����߼���û���ͷű�־��
		brelse(bh);                                     		// Ȼ���ͷż�ӿ�ռ�õĻ���顣
	}
	// ����ͷ��豸�ϵ�һ�μ�ӿ顣������������߼���û�б��ͷţ��򷵻�0��ʧ�ܣ���
	if (block_busy)
		return 0;
	else
		return free_block(dev, block);                   		// �ɹ��򷵻�1,���򷵻�0.
}

// �ͷ����ж��μ�ӿ顣
// ����dev���ļ�ϵͳ�����豸���豸�ţ�block���߼���š�
static int free_dind(int dev, int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;
	int block_busy;                                         	// ���߼���û�б��ͷŵı�־��

	// �����жϲ�������Ч�ԡ�����߼����Ϊ0,�򷵻ء�Ȼ���ȡ���μ�ӿ��һ���飬���ͷ����ϱ���ʹ�õ������߼��飬
	// Ȼ���ͷŸ�һ����Ļ���顣
	if (!block)
		return 1;
	block_busy = 0;
	if (bh = bread(dev, block)) {
		p = (unsigned short *) bh->b_data;              		// ָ�򻺳����������
		for (i = 0; i < 512; i++, p++)                         	// ÿ���߼����Ͽ�����512�������顣
			if (*p)
				if (free_ind(dev, *p)) {         				// �ͷ�����һ�μ�ӿ顣
					*p = 0;                 					// ���㡣
					bh->b_dirt = 1;         					// �������޸ı�־��
				} else
					block_busy = 1;         					// �����߼���û���ͷű�־��
		brelse(bh);                                     		// �ͷŶ��μ�ӿ�ռ�õĻ���顣
	}
	// ����ͷ��豸�ϵĶ��μ�ӿ顣������������߼���û�б��ͷţ��򷵻�0��ʧ�ܣ���
	if (block_busy)
		return 0;
	else
		return free_block(dev, block);							// ����ͷŴ�ŵ�һ��ӿ���߼���
}

// �ض��ļ����ݺ�����
// ���ڵ��Ӧ���ļ����ȼ�0,���ͷ�սʤ���豸�ռ䡣
void truncate(struct m_inode * inode)
{
	int i;
	int block_busy;                 							// ���߼���û�б��ͷŵı�־��

	// �����ж�ָ��i�ڵ���Ч�ԡ�������ǳ����ļ���Ŀ¼�ļ���������򷵻ء�
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
	     S_ISLNK(inode->i_mode)))
		return;
	// Ȼ���ͷ�i�ڵ��7��ֱ���߼��飬������7���߼�����ȫ���㡣����free_block()�����ͷ��豸��ָ���߼���Ĵ��̿�
	// ��fs/bitmap.c���������߼���æ��û�б��ͷ����ÿ�æ��־block_busy��
repeat:
	block_busy = 0;
	for (i = 0; i < 7; i++)
		if (inode->i_zone[i]) {                 				// �����Ų�Ϊ0�����ͷ�֮��
			if (free_block(inode->i_dev, inode->i_zone[i]))
				inode->i_zone[i] = 0;     						// ��ָ����0��
			else
				block_busy = 1;         						// ��û���ͷŵ����ñ�־��
		}
	if (free_ind(inode->i_dev, inode->i_zone[7]))    			// �ͷ�����һ�μ�ӿ顣
		inode->i_zone[7] = 0;                   				// ��ָ����0��
	else
		block_busy = 1;                         				// ��û���ͷŵ����ñ�־��
	if (free_dind(inode->i_dev, inode->i_zone[8]))   			// �ͷ����ж��μ�ӿ顣
		inode->i_zone[8] = 0;                   				// ��ָ����0��
	else
		block_busy = 1;                         				// ��û���ͷŵ����ñ�־��
	// �˺�����i�ڵ����޸ı�־��������������߼������� ��æ����û�б��ͷţ���ѵ�ǰ��������ʱ��Ƭ��0,���õ�ǰ�����ȱ�
	// �л�ȥ�����������̣��Ե�һ��������ִ���ͷŲ�����
	inode->i_dirt = 1;
	if (block_busy) {
		current->counter = 0;           						// ��ǰ����ʱ��Ƭ��0��
		schedule();
		goto repeat;
	}
	inode->i_size = 0;                      					// �ļ���С���㡣
	// ����������ļ��޸�ʱ���i�ڵ�ı�ʱ��Ϊ��ǰʱ�䡣��CURRENT_TIME������ͷ�ļ�include/linux/sched.h�У�����
	// Ϊ��startup_time+jiffies/HZ��������ȡ�ô�1970:0:0:0��ʼ������Ϊֹ������������
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;
}



