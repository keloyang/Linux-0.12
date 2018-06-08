/*
 *  linux/fs/fcntl.c
 *
 *  (C) 1991  Linus Torvalds
 */

//#include <string.h>
#include <errno.h>
#include <linux/sched.h>
//#include <linux/kernel.h>
//#include <asm/segment.h>

#include <fcntl.h>
//#include <sys/stat.h>

extern int sys_close(int fd);

// �����ļ����(�ļ�������).
// ����fd�������Ƶ��ļ����,argָ�����ļ��������С��ֵ.
// �������ļ�����������.
static int dupfd(unsigned int fd, unsigned int arg)
{
	// ���ȼ�麯����������Ч��.����ļ����ֵ����һ�����������ļ���NR_OPEN,���߸þ�����ļ��ṹ������,�򷵻س����벢�˳�.���ָ������
	// ���ֵarg���������ļ���,Ҳ���س����벢�˳�.ע��,ʵ�����ļ�������ǽ����ļ��ṹָ��������������.
	if (fd >= NR_OPEN || !current->filp[fd])
		return -EBADF;
	if (arg >= NR_OPEN)
		return -EINVAL;
	// Ȼ���ڵ�ǰ���̵��ļ��ṹָ��������Ѱ�������ŵ��ڻ����arg,����û��ʹ�õ���.���ҵ����¾��ֵarg���������ļ���(��û�п�����),�򷵻�
	// �����벢�˳�.
	while (arg < NR_OPEN)
		if (current->filp[arg])
			arg++;
		else
			break;
	if (arg >= NR_OPEN)
		return -EMFILE;
	// ��������ҵ��Ŀ�����(���),��ִ��ʱ�رձ�־λͼclose_on_exec�и�λ�þ��λ.��������exec()�ຯ��ʱ,����ر���dup()�����ĵľ��.�����
	// �ļ��ṹָ�����ԭ���fd��ָ��,���ҽ��ļ���������1.��󷵻��µ��ļ����arg.
	current->close_on_exec &= ~(1 << arg);
	(current->filp[arg] = current->filp[fd])->f_count++;
	return arg;
}

// �����ļ����ϵͳ���á�
// ����ָ���ļ����oldfd�����ļ����ֵ����newfd�����newfd�Ѵ򿪣������ȹر�֮��
// ������oldfd -- ԭ�ļ������newfd - ���ļ������
// �������ļ����ֵ��
int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
	sys_close(newfd);               						// �����newfd�Ѿ��򿪣������ȹر�֮��
	return dupfd(oldfd, newfd);      						// ���Ʋ������¾����
}

// �����ļ����ϵͳ����.
// ����ָ���ļ����oldfd,�¾����ֵ�ǵ�ǰ��С��δ�þ��ֵ.
// ����:fildes -- �����Ƶ��ļ����.
// �������ļ����ֵ.
int sys_dup(unsigned int fildes)
{
	return dupfd(fildes, 0);
}

// �ļ�����ϵͳ���ú�����
// ����fd���ļ������cmd�ǿ�������μ�include/fcntl.h����arg����Բ�ͬ�������в�ͬ�ĺ��塣���ڸ��ƾ������F_DUFD
// arg�����ļ����ȡ����Сֵ�����������ļ������ͷ��ʱ�־����F_SETFL��arg���µ��ļ������ͷ���ģʽ�������ļ���������
// F_GETLK��F_SETLK��F_SETLKW��arg��ָ��flock�ṹ��ָ�롣�����ں���û��ʵ���ļ��������ܡ�
// ���أ������������в���������-1.���ɹ�����ôF_DUPFD�������ļ������F_GETFD�����ļ�����ĵ�ǰִ��ʱ�رձ�־
// close_on_exec��F_GETFL�����ļ������ͷ��ʱ�־��
int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct file * filp;

	// ���ȼ��������ļ������Ч�ԡ�Ȼ����ݲ�ͬ����cmd���зֱ�������ļ����ֵ����һ�����������ļ���NR_OPEN������
	// �þ�����ļ��ṹָ��Ϊ�գ��򷵻س����벢�˳���
	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	switch (cmd) {
		case F_DUPFD:   										// �����ļ������
			return dupfd(fd,arg);
		case F_GETFD:   										// ȡ�ļ������ִ��ʱ�رձ�־��
			return (current->close_on_exec >> fd) & 1;
		case F_SETFD:   										// ����ִ��ʱ�رձ�־��argλ0��λ�����ã�����رա�
			if (arg & 1)
				current->close_on_exec |= (1 << fd);
			else
				current->close_on_exec &= ~(1 << fd);
			return 0;
		case F_GETFL:   										// ȡ�ļ�״̬��־�ͷ���ģʽ��
			return filp->f_flags;
		case F_SETFL:   										// �����ļ�״̬�ͷ���ģʽ������arg������ӡ���������־����
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
			return 0;
		case F_GETLK:	case F_SETLK:	case F_SETLKW:  		// δʵ�֡�
			return -1;
		default:
			return -1;
	}
}

