/*
 *  linux/fs/stat.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/sched.h>
//#include <linux/kernel.h>
#include <asm/segment.h>

// �����ļ�״̬��Ϣ��
// ����inode���ļ�i�ڵ㣬statbuf���û����ݿռ���stat�ļ�״̬�ṹָ�룬���ڴ��ȡ�õ�״̬��Ϣ��
static void cp_stat(struct m_inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	int i;

	// ������֤������䣩������ݵ��ڴ�ռ䡣Ȼ����ʱ������Ӧ�ڵ��ϵ���Ϣ��
	verify_area(statbuf, sizeof (struct stat));
	tmp.st_dev = inode->i_dev;              						// �ļ������豸�š�
	tmp.st_ino = inode->i_num;              						// �ļ�i�ڵ�š�
	tmp.st_mode = inode->i_mode;            						// �ļ����ԡ�
	tmp.st_nlink = inode->i_nlinks;         						// �ļ���������
	tmp.st_uid = inode->i_uid;              						// �ļ����û�ID��
	tmp.st_gid = inode->i_gid;              						// �ļ�����ID��
	tmp.st_rdev = inode->i_zone[0];         						// �豸�ţ����������ַ��ļ�����豸�ļ�)��
	tmp.st_size = inode->i_size;            						// �ļ��ֽڳ��ȣ�����ļ��ǳ����ļ�����
	tmp.st_atime = inode->i_atime;          						// ������ʱ�䡣
	tmp.st_mtime = inode->i_mtime;          						// ����޸�ʱ�䡣
	tmp.st_ctime = inode->i_ctime;          						// ���i�ڵ��޸�ʱ�䡣
	// �����Щ״̬��Ϣ���Ƶ��û��������С�
	for (i = 0 ; i<sizeof (tmp) ; i++)
		put_fs_byte(((char *) &tmp)[i], i + (char *) statbuf);
}

// �ļ�״̬ϵͳ���á�
// ���ݸ������ļ�����ȡ����ļ�״̬��Ϣ��
// ����filename��ָ�����ļ�����statbuf�Ǵ��״̬��Ϣ�Ļ�����ָ�롣
// ���أ��ɹ�����0,�������򷵻س����롣
int sys_stat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	// ���ȸ����ļ����ҳ���Ӧ��i�ڵ㡣Ȼ��i�ڵ��ϵ��ļ�״̬��Ϣ���Ƶ��û��������У����Ż�i�ڵ㡣
	if (!(inode = namei(filename)))
		return -ENOENT;
	cp_stat(inode, statbuf);
	iput(inode);
	return 0;
}

// �ļ�״̬ϵͳ���á�
// ���ݸ������ļ�����ȡ����ļ�״̬��Ϣ���ļ�·�������з��������ļ�������ȡ�����ļ���״̬��
// ������filename��ָ�����ļ�����statbuf�Ǵ��״̬��Ϣ�Ļ�����ָ�롣
int sys_lstat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	// ���ȸ����ļ����ҳ���Ӧ��i�ڵ㡣Ȼ��i�ڵ��ϵ��ļ�״̬��Ϣ���Ƶ��û��������У����Żظ�i�ڵ㡣
	if (!(inode = lnamei(filename)))        					// ȡָ��·����i�ڵ㣬������������ӡ�
		return -ENOENT;
	cp_stat(inode, statbuf);
	iput(inode);
	return 0;
}

// �ļ�״̬ϵͳ���á�
// ���ݸ������ļ������ȡ����ļ�״̬��Ϣ��
// ����fd��ָ���ļ��ľ��������������statbuf�Ǵ��״̬��Ϣ�Ļ�����ָ�롣
// ���أ��ɹ�����0,�������򷵻س����롣
int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	// ����ȡ�ļ������Ӧ���ļ��ṹ��Ȼ����еõ��ļ���i�ڵ㡣Ȼ��i�ڵ��ϵ��ļ�״̬��Ϣ���Ƶ��û��������С����
	// �ļ����ֵ����һ�����������ļ���NR_OPEN�����߸þ�����ļ��ṹָ��Ϊ�գ����߶�Ӧ�ļ��ṹ��i�ڵ��ֶ�Ϊ�գ�
	// ��������س����벢�˳���
	if (fd >= NR_OPEN || !(f = current->filp[fd]) || !(inode = f->f_inode))
		return -EBADF;
	cp_stat(inode, statbuf);
	return 0;
}

// ���������ļ�ϵͳ���á�
// �õ��ö�ȡ���������ļ������ݣ����÷���������ָ���ļ���·�����ַ����������ŵ�ָ�����ȵ��û��������С���������
// ̫С���ͻ�ضϷ������ӵ����ݡ�
// ������path -- ���������ļ�·������buf -- �û���������bufsiz -- ���������ȡ�
// ���أ��ɹ��򷵻ط��뻺�����е��ַ�������ʧ���򷵻س����롣
int sys_readlink(const char * path, char * buf, int bufsiz)
{
	struct m_inode * inode;
	struct buffer_head * bh;
	int i;
	char c;

	// ���ȼ�����֤������������Ч�ԣ���������е������û��������ֽڳ���bufsiz������1--1023֮�䡣Ȼ��ȡ�÷�������
	// �ļ�����i�ڵ㣬����ȡ���ļ��ĵ�1���������ݡ�֮��Ż�i�ڵ㡣
	if (bufsiz <= 0)
		return -EBADF;
	if (bufsiz > 1023)
		bufsiz = 1023;
	verify_area(buf, bufsiz);
	if (!(inode = lnamei(path)))
		return -ENOENT;
	if (inode->i_zone[0])
		bh = bread(inode->i_dev, inode->i_zone[0]);
	else
		bh = NULL;
	iput(inode);
	// �����ȡ�ļ��������ݳɹ�����������и������bufsiz���ַ����û��������У�������NULL�ַ�������ͷŻ���飬������
	// ���Ƶ��ֽ�����
	if (!bh)
		return 0;
	i = 0;
	while (i < bufsiz && (c = bh->b_data[i])) {
		i++;
		put_fs_byte(c, buf++);
	}
	brelse(bh);
	return i;
}

