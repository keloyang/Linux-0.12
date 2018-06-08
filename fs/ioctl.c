/*
 *  linux/fs/ioctl.c
 *
 *  (C) 1991  Linus Torvalds
 */

//#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <linux/sched.h>

// chr_drv/tty_ioctl.c
extern int tty_ioctl(int dev, int cmd, int arg);
// fs/pipe.c
extern int pipe_ioctl(struct m_inode *pino, int cmd, int arg);

// ��������������ƣ�ioctl������ָ�����͡�
typedef int (*ioctl_ptr)(int dev,int cmd,int arg);

// ȡϵͳ���豸�����ĺꡣ
#define NRDEVS ((sizeof (ioctl_table))/(sizeof (ioctl_ptr)))

// ioctl��������ָ���
static ioctl_ptr ioctl_table[] = {
	NULL,		/* nodev */
	NULL,		/* /dev/mem */
	NULL,		/* /dev/fd */
	NULL,		/* /dev/hd */
	tty_ioctl,	/* /dev/ttyx */
	tty_ioctl,	/* /dev/tty */
	NULL,		/* /dev/lp */
	NULL};		/* named pipes */


// ϵͳ���ú��� - ����������ƺ�����
// �ú��������жϲ����������ļ��������Ƿ���Ч��Ȼ����ݶ�Ӧi�ڵ����ļ������ж��ļ����ͣ������ݾ����ļ����͵������
// �Ĵ�������
// ������fd - �ļ��������� cmd - �����룻 arg - ������
// ���أ��ɹ��򷵻�0,���򷵻س����롣
int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct file * filp;
	int dev, mode;

	// �����жϸ������ļ�����������Ч�ԡ�����ļ������������ɴ򿪵��ļ��������߶�Ӧ���������ļ��ṹָ��Ϊ�գ��򷵻س���
	// ���˳���
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	// ����ļ��ṹ��Ӧ���ǹܵ�i�ڵ㣬����ݽ����Ƿ���Ȩ�����ùܵ�ȷ���Ƿ�ִ�йܵ�IO���Ʋ���������Ȩִ�������pipe_ioctl()��
	// ���򷵻���Ч�ļ������롣
	if (filp->f_inode->i_pipe)
		return (filp->f_mode & 1) ? pipe_ioctl(filp->f_inode, cmd, arg) : -EBADF;
	// �������������ļ���ȡ��Ӧ�ļ������ԣ����ݴ��ж��ļ������͡�������ļ��������ַ��豸�ļ���Ҳ���ǿ��豸�ļ����򷵻�
	// �������˳��������ַ�����豸�ļ�������ļ���i�ڵ���ȡ�豸�š�����豸�Ŵ���ϵͳ���е��豸�����򷵻س���š�
	mode = filp->f_inode->i_mode;
	if (!S_ISCHR(mode) && !S_ISBLK(mode))
		return -EINVAL;
	dev = filp->f_inode->i_zone[0];
	if (MAJOR(dev) >= NRDEVS)
		return -ENODEV;
	// Ȼ�����IO���Ʊ�ioctl_table��ö�Ӧ�豸��ioctl����ָ�룬�����øú�����������豸��ioctl����ָ�����û�ж�Ӧ������
	// �򷵻س����롣
	if (!ioctl_table[MAJOR(dev)])
		return -ENOTTY;
	return ioctl_table[MAJOR(dev)](dev, cmd, arg);
}

