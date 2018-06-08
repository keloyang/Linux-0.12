/*
 *  linux/fs/pipe.c
 *
 *  (C) 1991  Linus Torvalds
 */
//#include <signal.h>
#include <errno.h>
#include <termios.h>

#include <linux/sched.h>
//#include <linux/mm.h>	/* for get_free_page */
#include <asm/segment.h>
#include <linux/kernel.h>

// ���ܵ�����������
// ����inode�ǹܵ���Ӧ��i�ڵ㣬buf���û����ݻ�����ָ�룬count�Ƕ�ȡ���ֽ�����
int read_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, read = 0;

	// �����Ҫ��ȡ���ֽ���count����0,���Ǿ�ѭ��ִ�����²�������ѭ�������������У�����ǰ�ܵ���û�����ݣ�size=0����
	// ���ѵȴ��ýڵ�Ľ��̣���ͨ����д�ܵ����̡������û��д�ܵ��ߣ���i�ڵ����ü���С��2,�򷵻��Ѷ��ֽ����˳���
	// ���Ŀǰ�յ��������źţ������̷����Ѷ�ȡ�ֽ����˳�������û���յ��κ����ݣ��򷵻���������ϵͳ���ú��˳���
	// ������ý����ڸùܵ���˯�ߣ����Եȴ���Ϣ�ĵ�������PIPE_SIZE������include/linux/fs.h�С����ڡ���������
	// ϵͳ���úš�����μ�kernel/signal.c����
	while (count > 0) {
		while (!(size = PIPE_SIZE(*inode))) {     						// ȡ�ܵ������ݳ���ֵ��
			// ���ѵȴ���ùܵ�д���ݵĽ���
			wake_up(& PIPE_WRITE_WAIT(*inode));
			if (inode->i_count != 2) 									/* are there any writers? */
				return read;
			if (current->signal & ~current->blocked)
				return read ? read : -ERESTARTSYS;
			// ��ǰ����û�����ݿɶ������˯�ߵȴ�
			interruptible_sleep_on(& PIPE_READ_WAIT(*inode));
		}
		// ��ʱ˵���ܵ������������������ݡ���������ȡ�ܵ�βָ�뵽������ĩ�˵��ֽ���chars���������ڻ���Ҫ��ȡ���ֽ���
		// count�����������count�����chars���ڵ�ǰ�ܵ��к������ݵĳ���size�����������size��Ȼ�������ֽ���count
		// ��ȥ�ɶ����ֽ���chars�����ۼ��Ѷ��ֽ���read��
		chars = PAGE_SIZE - PIPE_TAIL(*inode);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;
		read += chars;
		// ����sizeָ��ܵ�βָ�봦����������ǰ�ܵ�βָ�루ǰ��chars�ֽڣ�����βָ�볬���ܵ�ĩ�����ƻء�Ȼ�󽫹ܵ��е�
		// ���ݸ��Ƶ��û��������С����ڹܵ�i�ڵ㣬��i_size�ֶ����ǹܵ������ָ�롣
		size = PIPE_TAIL(*inode);
		PIPE_TAIL(*inode) += chars;
		PIPE_TAIL(*inode) &= (PAGE_SIZE - 1);
		while (chars-- > 0)
			put_fs_byte(((char *)inode->i_size)[size++], buf++);
	}
	// ���˴ζ��ܵ��������������ѵȴ��ùܵ��Ľ��̣������ض�ȡ���ֽ�����
	wake_up(& PIPE_WRITE_WAIT(*inode));
	return read;
}

// �ܵ�д����������
// ����inode�ǹܵ���Ӧ��i�ڵ㣬buf�����ݻ�����ָ�룬count�ǽ�д��ܵ����ֽ�����
int write_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, written = 0;

	// ���Ҫд����ֽ���count������0,��ô���Ǿ�ѭ��ִ�����²�������ѭ�����������У������ǰ�ܵ����Ѿ����ˣ����пռ�
	// size = 0�������ѵȴ��ùܵ��Ľ��̣�ͨ�����ѵ��Ƕ��ܵ����̡�����Ѿ�û�ж��ܵ��ߣ���i�ڵ����ü���ֵС��2,����
	// ��ǰ���̷���SIGPIPE�źţ���������д����ֽ����˳�����д��0�ֽڣ��򷵻�-1�������õ�ǰ�����ڸùܵ���˯�ߣ��Եȴ�
	// ���ܵ���������ȡ���ݣ��Ӷ��ùܵ��ڳ��ռ䡣��PIPE_SIZE()��PIPE_HEAD()�ȶ������ļ�include/linux/fs.h�С�
	while (count > 0) {
		while (!(size = (PAGE_SIZE - 1) - PIPE_SIZE(*inode))) {
			wake_up(& PIPE_READ_WAIT(*inode));
			if (inode->i_count != 2) { 								/* no readers */
				current->signal |= (1 << (SIGPIPE - 1));
				return written ? written : -1;
			}
			sleep_on(& PIPE_WRITE_WAIT(*inode));
		}
		// ����ִ�е������ʾ�ܵ����������п�д�ռ�size����������ȡ�ܵ�ͷָ�뵽������ĩ�˿ռ��ֽ���chars��д�ܵ������Ǵӹܵ�
		// ͷָ�봦��ʼд�ġ����chars���ڻ���Ҫд����ֽ���count�����������count�����chars���ڵ�ǰ�ܵ��п��пռ䳤��size
		// ���������size��Ȼ�����Ҫд���ֽ���count��ȥ�˴ο�д����ֽ���chars������д���ֽ����ۼӵ�written�С�
		chars = PAGE_SIZE - PIPE_HEAD(*inode);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;
		written += chars;
		// ����sizeָ��ܵ�����ͷָ�봦����������ǰ�ܵ�����ͷ��ָ�루ǰ��chars�ֽڣ�����ͷָ�볬���ܵ�ĩ�����ƻء�Ȼ����û�����
		// ������chars���ֽڵ��ܵ�ͷָ�뿪ʼ�������ڹܵ�i�ڵ㣬��i_size�ֶ����ǹܵ������ָ�롣
		size = PIPE_HEAD(*inode);
		PIPE_HEAD(*inode) += chars;
		PIPE_HEAD(*inode) &= (PAGE_SIZE - 1);
		while (chars-- > 0)
			((char *)inode->i_size)[size++] = get_fs_byte(buf++);
	}
	// ���˴�д�ܵ��������������ѵȴ��ܵ��Ľ��̣�������д����ֽ������˳���
	wake_up(& PIPE_READ_WAIT(*inode));
	return written;
}

// �����ܵ�ϵͳ���á�
// ��fildes��ָ�������д���һ�Ծ������������������ļ����ָ��һ�ܵ�i�ڵ㡣
// ������fildes - �ļ�������顣fildes[0]���ڶ��ܵ����ݣ�fildes[1]��ܵ�д�����ݡ�
// �ɹ�ʱ����0,����ʱ����-1.
int sys_pipe(unsigned long * fildes)
{
	struct m_inode * inode;
	struct file * f[2];             						// �ļ��ṹ���顣
	int fd[2];                      						// �ļ�������顣
	int i, j;

	// ���ȴ�ϵͳ�ļ�����ȡ������������ü����ֶ�Ϊ0��������ֱ��������ü���Ϊ1.
	// ��ֻ��1����������ͷŸ�����ü�����λ������û���ҵ�����������򷵻�-1��
	j = 0;
	for(i = 0; j < 2 && i < NR_FILE; i++)
		if (!file_table[i].f_count)
			(f[j++] = i + file_table)->f_count++;
	if (j == 1)
		f[0]->f_count = 0;
	if (j < 2)
		return -1;
	// �������ȡ�õ������ļ���ṹ��ֱ����һ�ļ�����ţ���ʹ�����ļ��ṹָ�����������ֱ�ָ���������ļ�
	// �ṹ�����ļ�������Ǹ�����������š����Ƶأ����ֻ��һ�������ļ���������ͷŸþ�����ÿ���Ӧ���������
	// ��û���ҵ��������о�������ͷ������ȡ�������ļ��ṹ���λ���ü���ֵ����������-1��
	j = 0;
	for(i = 0; j < 2 && i < NR_OPEN; i++)
		if (!current->filp[i]) {
			current->filp[ fd[j] = i ] = f[j];
			j++;
		}
	if (j == 1)
		current->filp[fd[0]] = NULL;
	if (j < 2) {
		f[0]->f_count = f[1]->f_count = 0;
		return -1;
	}
	// Ȼ�����ú���get_pipe_inode()����һ���ܵ�ʹ�õ�i�ڵ㣬��Ϊ�ܵ�����һҳ�ڴ���Ϊ��������������ɹ�����
	// ��Ӧ�ͷ������ļ�������ļ��ṹ�������-1.
	if (!(inode = get_pipe_inode())) {                		// fs/inode.c��
		current->filp[fd[0]] =
			current->filp[fd[1]] = NULL;
		f[0]->f_count = f[1]->f_count = 0;
		return -1;
	}
	// ����ܵ�i�ڵ�����ɹ�����������ļ��ṹ���г�ʼ�������������Ƕ�ָ��ͬһ���ܵ�i�ڵ㣬���Ѷ�дָ�붼���㡣
	// ��1���ļ��ṹ���ļ�ģʽΪ������2���ļ��ṹ���ļ�ģʽ��Ϊд������ļ�������鸴�Ƶ���Ӧ���û��ռ������У�
	// �ɹ�����0,�˳���
	f[0]->f_inode = f[1]->f_inode = inode;
	f[0]->f_pos = f[1]->f_pos = 0;
	f[0]->f_mode = 1;										/* read */
	f[1]->f_mode = 2;										/* write */
	put_fs_long(fd[0], 0 + fildes);
	put_fs_long(fd[1], 1 + fildes);
	return 0;
}

// �ܵ�io���ƺ�����
// ������pino - �ܵ�i�ڵ�ָ�룻cmd - �������arg - ������
// ��������0��ʾִ�гɹ������򷵻س����롣
int pipe_ioctl(struct m_inode *pino, int cmd, int arg)
{
	// ���������ȡ�ܵ��е�ǰ�ɶ����ݳ��ȣ���ѹܵ����ݳ���ֵ�����û�����ָ����λ�ô���������0�����򷵻���Ч����
	// �����롣
	switch (cmd) {
		case FIONREAD:
			verify_area((void *) arg, 4);
			put_fs_long(PIPE_SIZE(*pino), (unsigned long *) arg);
			return 0;
		default:
			return -EINVAL;
	}
}

