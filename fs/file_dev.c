/*
 *  linux/fs/file_dev.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>              								// �����ͷ�ļ�������ϵͳ�и��ֳ���š�
#include <fcntl.h>

#include <linux/sched.h>        								// ���ȳ���ͷ�ļ�������������ṹtask_struct������0�����ݵȡ�
#include <linux/kernel.h>       								// �ں�ͷ�ļ�������һЩ�ں˳��ú�����ԭ�Ͷ��塣
#include <asm/segment.h>        								// �β���ͷ�ļ����������йضμĴ���������Ƕ��ʽ��ຯ����

#define MIN(a, b) (((a) < (b)) ? (a) : (b))    					// ȡa��b�е���Сֵ
#define MAX(a, b) (((a) > (b)) ? (a) : (b))    					// ȡa��b�е����ֵ

// �ļ������� - ����i�ڵ���ļ��ṹ����ȡ�ļ������ݡ�
// ��i�ڵ����ǿ���֪���豸�ţ���filp�ṹ����֪���ļ��е�ǰ��дָ��λ�á�bufָ���û��ռ��л�������λ�ã�count����Ҫ��ȡ���ֽ�����
// ����ֵ��ʵ�ʶ�ȡ���ֽ����������ţ�С��0����
int file_read(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	int left, chars, nr;
	struct buffer_head * bh;

	// �����жϲ�������Ч�ԡ�����Ҫ��ȡ���ֽڼ���countС�ڵ����㣬�򷵻�0.������Ҫ��ȡ���ֽ���������0,��ѭ��ִ�����������ֱ������ȫ
	// ���������������⡣�ڶ�ѭ�����������У����Ǹ���i�ڵ���ļ���ṹ��Ϣ��������bmap()�õ������ļ���ǰ��дλ�õ����ݿ����豸�϶�Ӧ
	// ���߼����nr����nr��Ϊ0,���i�ڵ�ָ�����豸�϶�ȡ���߼��顣���������ʧ�����˳�ѭ������nrΪ0,��ʾָ�������ݿ鲻���ڣ��û���
	// ��ָ��ΪNULL��
	if ((left = count) <= 0)
		return 0;
	while (left) {
		// �����ļ��Ķ�дƫ��λ�õõ���ǰдλ�ö�Ӧ���߼����
		if (nr = bmap(inode, (filp->f_pos) / BLOCK_SIZE)) {
			// �õ����߼���Ŷ�Ӧ�ĸ��ٻ�����
			if (!(bh = bread(inode->i_dev, nr)))
				break;
		} else
			bh = NULL;
		// �������Ǽ����ļ���дָ�������ݿ��е�ƫ��ֵnr�����ڸ����ݿ�������ϣ����ȡ���ֽ���Ϊ��BLOCK_SIZE - nr����Ȼ������ڻ����ȡ��
		// �ֽ���left���Ƚϣ�����Сֵ��Ϊ���β������ȡ���ֽ���chars�������BLOCK_SIZE - nr��> left����˵���ÿ�����Ҫ��ȡ�����һ��
		// ���ݣ���֮����Ҫ��ȡ��һ�����ݡ�֮�������д�ļ�ָ�롣ָ��ǰ�ƴ˴ν���ȡ���ֽ���chars��ʣ���ֽ���left��Ӧ��ȥchars��
		nr = filp->f_pos % BLOCK_SIZE;
		chars = MIN( BLOCK_SIZE - nr , left );
		filp->f_pos += chars;
		left -= chars;
		// ��������豸�϶��������ݣ���pָ�򻺳���п�ʼ��ȡ���ݵ�λ�ã����Ҹ���chars�ֽڵ��û�������buf�С��������û�������������chars
		// ��ֵ�ֽڡ�
		if (bh) {
			char * p = nr + bh->b_data;
			while (chars-- > 0)
				put_fs_byte(*(p++), buf++);
			brelse(bh);
		} else {
			while (chars-- > 0)
				put_fs_byte(0, buf++);
		}
	}
	// �޸ĸ�i�ڵ�ķ���ʱ��Ϊ��ǰʱ�䡣���ض�ȡ���ֽ���������ȡ�ֽ���Ϊ0,�򷵻س���š�
	// CURRENT_TIME�Ƕ�����include/linux/sched.h�ϵĺ꣬���ڼ���UNIXʱ�䡣����1970��1��1��0ʱ0�뿪ʼ������ǰʱ�䡣��λ���롣
	inode->i_atime = CURRENT_TIME;
	return (count - left) ? (count-left) : -ERROR;
}

// �ļ�д���� - ����i�ڵ���ļ��ṹ��Ϣ�����û�����д���ļ��С�
// ��i�ڵ����ǿ���֪���豸�ţ�����file�ṹ����֪���ļ��е�ǰ��дָ��λ�á�bufָ���û�̬�л�������λ�ã�countΪ��Ҫд����ֽ�����
// ����ֵ��ʵ��д����ֽ����������ţ�С��0).
int file_write(struct m_inode * inode, struct file * filp, char * buf, int count)
{
	off_t pos;
	int block, c;
	struct buffer_head * bh;
	char * p;
	int i = 0;

	/*
	 * ok, append may not work when many processes are writing at the same time
	 * but so what. That way leads to madness anyway.
	 */
	/*
	 * OK����������ͬʱдʱ��append�������ܲ��У����������������������������ᵼ�»���һ�š�
	 */
	// ����ȷ������д���ļ���λ�á������Ҫ���ļ���������ݣ����ļ���дָ���Ƶ��ļ�β��������ͽ����ļ���ǰ��дָ�봦д�롣
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
	// Ȼ������д���ֽ���i���տ�ʼʱΪ0��С��ָ��д���ֽ���countʱ��ѭ��ִ�����²�������ѭ�����������У�������ȡ�ļ����ݿ�
	// �ţ�pos/BLOCK_SIZE�����豸�϶�Ӧ���߼����block�������Ӧ���߼��鲻���ھʹ���һ�顣����õ����߼���� = 0,���ʾ
	// ����ʧ�ܣ������˳�ѭ�����������Ǹ��ݸ��߼���Ŷ�ȡ�豸�ϵ���Ӧ�߼��飬������Ҳ�˳�ѭ����
	while (i < count) {
		if (!(block = create_block(inode, pos / BLOCK_SIZE)))
			break;
		if (!(bh = bread(inode->i_dev, block)))
			break;
		// ��ʱ�����ָ��bh��ָ��ն�����ļ����ݿ顣����������ļ���ǰ��дָ���ڸ����ݿ��е�ƫ��ֵc������ָ��pָ�򻺳���п�ʼд��
		// ���ݵ�λ�ã����øû�������޸ı�־�����ڿ��е�ǰָ�룬�ӿ�ʼ��дλ�õ���ĩ����д��c = (BLOCK_SIZE - c)���ֽڡ���c��
		// ��ʣ�໹��д����ֽ�����count - i������˴�ֻ���ٶ���c = (count-i)���ֽڼ��ɡ�
		c = pos % BLOCK_SIZE;
		p = c + bh->b_data;
		bh->b_dirt = 1;
		c = BLOCK_SIZE - c;
		if (c > count - i) c = count - i;
		// ��д������֮ǰ��������Ԥ�����ú���һ��ѭ������Ҫ��д�ļ��е�λ�á�������ǰ�posָ��ǰ�ƴ˴���Ҫд����ֽ����������ʱpos
		// λ��ֵ�������ļ���ǰ���ȣ����޸�i�ڵ��ļ������ֶΣ�����i�ڵ����޸ı�־��Ȼ��Ѵ˴�Ҫд����ֽ���c�ۼӵ���д���ֽڼ���ֵi�У�
		// ��ѭ���жϡ�ʹ�ý���˫�û�������buf�и���c���ֽڵ������������pָ��Ŀ�ʼλ�ô������������ͷŸû���顣
		pos += c;
		if (pos > inode->i_size) {
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		i += c;
		while (c-- > 0)
			*(p++) = get_fs_byte(buf++);
		brelse(bh);
    }
	// �������Ѿ�ȫ��д���ļ�������д���������з�������ʱ�ͻ��˳�ѭ������ʱ���Ǹ����ļ��޸�ʱ��Ϊ��ǰʱ�䣬�������ļ���дָ�롣���
	// �˴β����������ļ�β������ݣ�����ļ���дָ���������ǰ��дλ��pos�����������ļ�i�ڵ���޸�ʱ��Ϊ��ǰʱ�䡣��󷵻�д���
	// �ֽ�������д���ֽ���Ϊ0,�򷵻س����-1��
	inode->i_mtime = CURRENT_TIME;
	if (!(filp->f_flags & O_APPEND)) {
		filp->f_pos = pos;
		inode->i_ctime = CURRENT_TIME;
	}
	return (i ? i : -1);
}

