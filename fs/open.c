/*
 *  linux/fs/open.c
 *
 *  (C) 1991  Linus Torvalds
 */

//#include <string.h>
#include <errno.h>									// �����ͷ�ļ�.����ϵͳ�и��ֳ����.
#include <fcntl.h>									// �ļ�����ͷ�ļ�.�����ļ������������������Ƴ������Ŷ���.
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>								// �ļ�״̬ͷ�ļ�.�����ļ�״̬�ṹstat{}�ͷ��ų�����.

#include <linux/sched.h>							// ���ȳ���ͷ�ļ�,��������ṹtask_struct,����0���ݵ�.
#include <linux/tty.h>								// ttyͷ�ļ�,�������й�tty_io,����ͨ�ŷ���Ĳ���,����.
#include <linux/kernel.h>

#include <asm/segment.h>

// ȡ�ļ�ϵͳ��Ϣ��
// ����dev�Ǻ����û��Ѱ�װ�ļ�ϵͳ���豸�š�ubuf��һ��ustat�ṹ������ָ�룬���ڴ��ϵͳ���ص��ļ�ϵͳ��Ϣ����ϵͳ
// �������ڷ����Ѱ�װ��mounted���ļ�ϵͳ��ͳ����Ϣ���ɹ�ʱ����0������ubufָ���ustate�ṹ�������ļ�ϵͳ�ܿ��п�
// �Ϳ���i�ڵ�����ustat�ṹ������include/sys/types.h�С�
int sys_ustat(int dev, struct ustat * ubuf)
{
	return -ENOSYS;         						// �����룺���ܻ�δʵ�֡�
}

// �����ļ����ʺ��޸�ʱ�䡣
// ����filename���ļ�����times�Ƿ��ʺ��޸�ʱ��ṹָ�롣
// ���timesָ�벻ΪNULL����ȡutimbuf�ṹ�е�ʱ����Ϣ�������ļ��ķ��ʺ��޸�ʱ�䡣
// ���timesָ����NULL����ȡϵͳ��ǰʱ��������ָ���ļ��ķ��ʺ��޸�ʱ����
int sys_utime(char * filename, struct utimbuf * times)
{
	struct m_inode * inode;
	long actime, modtime;

	// �ļ���ʱ����Ϣ��������i�ڵ��С�����������ȸ����ļ���ȡ�ö�Ӧi�ڵ㡣���û���ҵ����򷵻س����롣����ṩ�ķ���
	// ���޸�ʱ��ṹָ��times��ΪNULL����ӽṹ�ж�ȡ�û����õ�ʱ��ֵ���������ϵͳ��ǰʱ���������ļ��ķ��ʺ��޸�ʱ
	// �䡣
	if (!(inode = namei(filename)))
		return -ENOENT;
	if (times) {
		actime = get_fs_long((unsigned long *) &times->actime);
		modtime = get_fs_long((unsigned long *) &times->modtime);
	} else
		actime = modtime = CURRENT_TIME;
	// Ȼ���޸�i�ڵ��еķ���ʱ���ֶκ��޸�ʱ���ֶΡ�������i�ڵ����޸ı�־���Żظ�i�ڵ㣬������0��
	inode->i_atime = actime;
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

/*
 * XXX should we use the real or effective uid?  BSD uses the real uid,
 * so as to make this call useful to setuid programs.
 */
/*
 * XXX���Ǹ�����ʵ�û�id��ruid��������Ч�л�id��euid����BSDϵͳʹ������ʵ�û�id����ʹ�õ��ÿ��Թ�setuid����
 * ʹ�á�
 * ��ע��POSIX��׼����ʹ����ʵ�û�ID����
 * ��ע1��Ӣ��ע�Ϳ�ʼ�ġ�XXX����ʾ��Ҫ��ʾ����
 */
// ����ļ��ķ���Ȩ�ޡ�
// ����filename���ļ�����mode�Ǽ��ķ������ԣ�����3����Чλ��ɣ�R_OK��ֵ4����W_OK��2����X_OK��1����F_OK��0��
// ��ɣ��ֱ��ʾ����ļ��Ƿ�ɶ�����д����ִ�к��ļ��Ƿ���ڡ������������Ļ����򷵻�0,���򷵻س����롣
int sys_access(const char * filename, int mode)
{
	struct m_inode * inode;
	int res, i_mode;

	// �ļ��ķ���Ȩ����Ϣͬ���������ļ���i�ڵ�ṹ�У��������Ҫ��ȡ�ö�Ӧ�ļ�����i�ڵ㡣���ķ�������mode�ɵ�3λ��ɣ�
	// �����Ҫ���ϰ˽���0007��������и�λ������ļ�����Ӧ��i�ڵ㲻���ڣ��򷵻�û�����Ȩ�޳����롣��i�ڵ���ڣ���ȡi
	// �ڵ����ļ������룬���Żظ�i�ڵ㡣���⣬57������䡰iput(inode);����÷���61��֮��
	mode &= 0007;
	if (!(inode = namei(filename)))
		return -EACCES;                 				// �����룺�޷���Ȩ�ޡ�
	i_mode = res = inode->i_mode & 0777;
	iput(inode);
	// �����ǰ�����û��Ǹ��ļ�����������ȡ�ļ��������ԡ����������ǰ�����û�����ļ�����ͬ��һ��������ȡ�ļ������ԡ�����
	// ��ʱres��3λ�������˷��ʸ��ļ���������ԡ�
	// [??����Ӧres >> 3 ??]
	if (current->uid == inode->i_uid)
		res >>= 6;
	else if (current->gid == inode->i_gid)
		res >>= 3;
	// ��ʱres�����3λ�Ǹ��ݵ�ǰ�����û����ļ��Ĺ�ϵѡ������ķ�������λ�������������ж���3λ������ļ����Ծ��в�������ѯ
	// ������λmode���������ɣ�����0��
	if ((res & 0007 & mode) == mode)
		return 0;
	/*
	 * XXX we are doing this test last because we really should be
	 * swapping the effective with the real user id (temporarily),
	 * and then calling suser() routine.  If we do call the
	 * suser() routine, it needs to be called last.
	 */
    /*
     * XXX��������������Ĳ��ԣ���Ϊ����ʵ������Ҫ������Ч�û�ID����ʵ�û�ID����ʱ�أ���Ȼ��ŵ���suser()������
     * �������ȷʵҪ����suser()����������Ҫ���ű����á�
     */
	// �����ǰ�û�IDΪ0�������û�������������ִ��λ��0�����ļ����Ա��κ���ִ�С��������򷵻�0�����򷵻س����롣
	if ((!current->uid) &&
	    (!(mode & 1) || (i_mode & 0111)))
		return 0;
	return -EACCES;         							// �����룺�޷���Ȩ�ޡ�
}

// �ı䵱ǰ����Ŀ¼ϵͳ���á�
// ����filename��Ŀ¼����
// �����ɹ��򷵻�0,���򷵻س����롣
int sys_chdir(const char * filename)
{
	struct m_inode * inode;

	// �ı䵱ǰ����Ŀ¼����Ҫ��ѽ�������ṹ�ĵ�ǰ����Ŀ¼�ֶ�ָ�����Ŀ¼����i�ڵ㡣�����������ȡĿ¼����i�ڵ㡣���Ŀ¼����Ӧ
	// ��i�ڵ㲻���ڣ��򷵻س����롣�����i�ڵ㲻��һ��Ŀ¼i�ڵ㣬��Żظ�i�ڵ㣬�����س����롣
	if (!(inode = namei(filename)))
		return -ENOENT;                 				// �����룺�ļ���Ŀ¼�����ڡ�
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;                				// �����룺����Ŀ¼����
	}
	// Ȼ���ͷŽ���ԭ����Ŀ¼i�ڵ㣬��ʹ��ָ�������õĹ���Ŀ¼i�ڵ㡣����0.
	iput(current->pwd);
	current->pwd = inode;
	return (0);
}

// �ı��Ŀ¼ϵͳ���á�
// ��ָ����Ŀ¼�����ó�Ϊ��ǰ���̵ĸ�Ŀ¼��/����
// ��������ɹ��򷵻�0�����򷵻س����롣
int sys_chroot(const char * filename)
{
	struct m_inode * inode;

	// �õ������ڸı䵱ǰ��������ṹ�еĸ�Ŀ¼�ֶ�root������ָ���������Ŀ¼����i�ڵ㡣���Ŀ¼����Ӧi�ڵ㲻���ڣ��򷵻س����롣
	// �����i�ڵ㲻��Ŀ¼i�ڵ㣬��Żظ�i�ڵ㣬Ҳ���س����롣
	if (!(inode = namei(filename)))
		return -ENOENT;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	// Ȼ���ͷŵ�ǰ���̵ĸ�Ŀ¼������������Ϊָ��Ŀ¼����i�ڵ㣬����0��
	iput(current->root);
	current->root = inode;
	return (0);
}

// �޸��ļ�����ϵͳ���á�
// ����filename���ļ�����mode���µ��ļ����ԡ�
int sys_chmod(const char * filename, int mode)
{
	struct m_inode * inode;

	// �õ���Ϊָ���ļ������µķ�������mode���ļ��ķ����������ļ�����Ӧ��i�ڵ��У������������ȡ�ļ�����Ӧ��i�ڵ㡣���i�ڵ㲻��
	// �ڣ��򷵻س����루�ļ���Ŀ¼�����ڣ��������ǰ���̵���Ч�û���id���ļ�i�ڵ���û�id��ͬ������Ҳ���ǳ����û�����Żظ��ļ�
	// i�ڵ㣬���س����루û�з���Ȩ�ޣ���
	if (!(inode = namei(filename)))
		return -ENOENT;
	if ((current->euid != inode->i_uid) && !suser()) {
		iput(inode);
		return -EACCES;
	}
	// ������������ø�i�ڵ���ļ����ԣ����ø�i�ڵ����޸ı�־���Żظ�i�ڵ㣬����0��
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

// �޸��ļ�����ϵͳ���á�
// ����filename���ļ�����uid���û���ʶ�����û�ID����gid����ID��
// �������ɹ��򷵻�0�����򷵻س����롣
int sys_chown(const char * filename, int uid, int gid)
{
	struct m_inode * inode;

	// �õ������������ļ�i�ڵ��е��û�����ID���������Ҫȡ�ø����ļ�����i�ڵ㡣����ļ�����i�ڵ㲻���ڣ��򷵻س����루�ļ�
	// ��Ŀ¼�����ڣ��������ǰ���̲��ǳ����û�����Żظ�i�ڵ㣬�����س����루û�з���Ȩ�ޣ���
	if (!(inode = namei(filename)))
		return -ENOENT;
	if (!suser()) {
		iput(inode);
		return -EACCES;
	}
	// �������Ǿ��ò����ṩ��ֵ�������ļ�i�ڵ���û�ID����ID������i�ڵ��Ѿ��޸ı�־���Żظ�i�ڵ㣬����0��
	inode->i_uid = uid;
	inode->i_gid = gid;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

// ����ַ��豸����.
// �ú��������������ļ���ϵͳ����sys_open(),���ڼ�����򿪵��ļ���tty�ն��ַ��豸ʱ,��Ҫ�Ե�ǰ���̵����úͶ�tty�������.
// ����0��⴦��ɹ�,����-1��ʾʧ��,��Ӧ�ַ��豸���ܴ�.
static int check_char_dev(struct m_inode * inode, int dev, int flag)
{
	struct tty_struct *tty;
	int min;										// ���豸��.

	// ֻ�������豸����4(/dev/ttyxx�ļ�)��5(/dev/tty�ļ�)�����./dev/tty�����豸����0.���һ�������п����ն�,�����ǽ��̿����ն��豸
	// ��ͬ����.��/dev/tty�豸��һ�������豸,����Ӧ������ʵ��ʹ�õ�/dev/ttyxx�豸֮һ.����һ��������˵,�����п����ն�,��ô��������ṹ
	// �е�tty�ֶν���4���豸��ĳһ�����豸��.
	// ����򿪲������ļ���/dev/tty(��MAJOR(dev) = 5),��ô������min = ��������ṹ�е�tty�ֶ�,��ȡ4���豸�����豸��.��������򿪵���
	// ĳ��4���豸,��ֱ��ȡ�����豸��.����õ���4���豸���豸��С��0,��ô˵������û�п����ն�,�����豸�Ŵ���,�򷵻�-1,��ʾ���ڽ���û�п����ն�
	// ���߲��ܴ�����豸.
	if (MAJOR(dev) == 4 || MAJOR(dev) == 5) {
		if (MAJOR(dev) == 5)
			min = current->tty;
		else
			min = MINOR(dev);
		if (min < 0)
			return -1;
		// ��α�ն��豸�ļ�ֻ�ܱ����̶�ռʹ��.������豸�ű�����һ����α�ն�,���Ҹô��ļ�i�ڵ����ü�������1,��˵�����豸�ѱ���������ʹ��.��˲�����
		// �򿪸��ַ��豸�ļ�,���Ƿ���-1.����,������tty�ṹָ��ttyָ��tty���ж�Ӧ�ṹ��.�����ļ�������־flag�в�����������ն˱�־O_NOCTTY,���ҽ���
		// �ǽ���������,���ҵ�ǰ����û�п����ն�,����tty�ṹ��session�ֶ�Ϊ0(��ʾ���ն˻������κν�����Ŀ����ն�),��ô������Ϊ������������ն��豸min
		// Ϊ������ն�.�������ý�������ṹ�ն��豸���ֶ�ttyֵ����min,�������ö�Ӧtty�ṹ�ĻỰ��session�ͽ������pgrp�ֱ���ڽ��̵ĻỰ�źͽ������.
		if ((IS_A_PTY_MASTER(min)) && (inode->i_count > 1))
			return -1;
		tty = TTY_TABLE(min);
		// Log(LOG_INFO_TYPE, "<<<<< tty index = %d>>>>>\n", min);
		if (!(flag & O_NOCTTY) &&
		    current->leader &&
		    current->tty < 0 &&
		    tty->session == 0) {
			current->tty = min;
			tty->session = current->session;
			tty->pgrp = current->pgrp;
		}
		// ������ļ�������־flag�к���O_NONBLOCK(������)��־,��������Ҫ�Ը��ַ��ն��豸�����������,����Ϊ�����������Ҫ��ȡ�������ַ���Ϊ0,���ó�ʱ
		// ��ʱֵΪ0,�����ն��豸���óɷǹ淶ģʽ.��������ʽֻ�ܹ����ڷǹ淶ģʽ.�ڴ�ģʽ�µ�VMIN��VTIME������Ϊ0ʱ,�����������ж���֧���̾Ͷ�ȡ�����ַ�,
		// �����̷���.
		if (flag & O_NONBLOCK) {
			TTY_TABLE(min)->termios.c_cc[VMIN] = 0;
			TTY_TABLE(min)->termios.c_cc[VTIME] = 0;
			TTY_TABLE(min)->termios.c_lflag &= ~ICANON;
		}
	}
	return 0;
}

// ��(�򴴽�)�ļ�ϵͳ����.
// ����filename���ļ���,flag�Ǵ��ļ���־,����ȡֵ:O_RDONLY(ֻ��),O_WRONLY(ֻд)��O_RDWR(��д),�Լ�O_CREAT(����),
// O_EXCL(�������ļ����벻����),O_APPEND(���ļ�β�������)������һЩ��־�����,��������ô�����һ�����ļ�,��mode������ָ��
// �ļ����������.��Щ������S_IRWXU(�ļ��������ж�,д��ִ��Ȩ��),S_IRUSR(�û����ж��ļ�Ȩ��),S_IRWXG(���Ա�ж�,д
// ִ��)�ȵ�.�����´������ļ�,��Щ����ֻӦ���ڽ������ļ��ķ���,������ֻ���ļ��Ĵ򿪵���Ҳ������һ����д���ļ����.�������
// �����ɹ�,�򷵻��ļ����(�ļ�������),���򷵻س�����.�μ�sys/tat.h,fcntl.h.
int sys_open(const char * filename, int flag, int mode)
{
	// ���ļ���ϵͳ���õ�Log
	// Log(LOG_INFO_TYPE, "<<<<< sys_open : filename = %s, flag = %d, mode = %d>>>>>\n", filename, flag, mode);
	struct m_inode * inode;
	struct file * f;
	int i, fd;

	// ���ȶԲ������д���.���û����õ��ļ�ģʽ�ͽ���ģʽ����������,�����������ļ�ģʽ.Ϊ��Ϊ���ļ�����һ���ļ����,��Ҫ��������
	// �ṹ���ļ��ṹָ������,�Բ���һ��������.�������������fd���Ǿ��ֵ.���Ѿ�û�п�����,�򷵻س�����(������Ч).
	mode &= 0777 & ~current->umask;
	for(fd = 0 ; fd < NR_OPEN ; fd++)
		if (!current->filp[fd])
			break;          						// �ҵ�������.
	if (fd >= NR_OPEN)
		return -EINVAL;
	// Ȼ���������õ�ǰ���̵�ִ��ʱ�ر��ļ����(close_on_exec)λͼ,��λ��Ӧ��λ��close_on_exec��һ�����������ļ������λͼ��־.ÿ��
	// λ����һ�����ŵ��ļ�������,����ȷ������ϵͳ����execve()ʱ��Ҫ�رյ��ļ����.������ʹ��fork()��������һ���ӽ���ʱ,ͨ������
	// ���ӽ����е���execve()��������ִ����һ���³���.��ʱ�ӽ����п�ʼִ���³���.��һ���ļ����close_on_exec�еĶ�Ӧλ����λ,��ô��
	// ִ��execve()ʱ�ö�Ӧ�ļ���������ر�,������ļ������ʼ�մ��ڴ�״̬.����һ���ļ�ʱ,Ĭ��������ļ�������ӽ�����Ҳ���ڴ�
	// ״̬.�������Ҫ��λ��Ӧλ.Ȼ��Ϊ���ļ����ļ�����Ѱ��һ�����нṹ��.������fָ���ļ������鿪ʼ��.���������ļ��ṹ��(���ü���
	// Ϊ0����),���Ѿ�û�п����ļ���ṹ��,�򷵻س�����.����,��184���ϵ�ָ�븳ֵ"0+file_table"��ͬ��"file_table"��"&file_table[0]"
	// ��������д���ܸ�������һЩ.
	current->close_on_exec &= ~(1 << fd);           // ��λ��Ӧ�ļ���λ
	f = 0 + file_table;
	for (i = 0 ; i < NR_FILE ; i++, f++)
		if (!f->f_count) break;         			// ���ļ������ҵ����нṹ�
	if (i >= NR_FILE)
		return -EINVAL;
	// ��ʱ�����ý��̶�Ӧ�ļ����fd���ļ��ṹָ��ָ�����������ļ��ṹ,�����ļ����ü�������1.Ȼ����ú���open_namei()ִ�д򿪲���,������
	// ֵС��0,��˵������,�����ͷŸ����뵽���ļ��ṹ,���س�����i.���ļ��򿪲����ɹ�,��inode���Ѵ��ļ���i�ڵ�ָ��.
	(current->filp[fd] = f)->f_count++;
	// Log(LOG_INFO_TYPE, "<<<<< sys_open : fd = %d\n", fd);
	if ((i = open_namei(filename, flag, mode, &inode)) < 0) {
		current->filp[fd] = NULL;
		f->f_count = 0;
		return i;
	}
	// �����Ѵ��ļ�i�ڵ�������ֶ�,���ǿ���֪���ļ�������.���ڲ�ͬ���͵��ļ�,������Ҫ��һЩ�ر���.����򿪵����ַ��豸�ļ�,��ô���Ǿ�Ҫ����
	// check_char_dev()��������鵱ǰ�����Ƿ��ܴ�����ַ��豸�ļ�.�������(��������0),��ô��check_char_dev()�л���ݾ����ļ��򿪱�־Ϊ����
	// ���ÿ����ն�.����������ʹ�ø��ַ��豸�ļ�,��ô����ֻ���ͷ�����������ļ���;����Դ.���س�����.
	/* ttys are somewhat special (ttyxx major==4, tty major==5) */
	if (S_ISCHR(inode->i_mode))
		if (check_char_dev(inode, inode->i_zone[0], flag)) {
			iput(inode);
			current->filp[fd] = NULL;
			f->f_count = 0;
			return -EAGAIN;         				// �����:��Դ�ݲ�����.
		}
	// ����򿪵��ǿ��豸�ļ�,������Ƭ�Ƿ������.������������Ҫ�ø��ٻ������и��豸�����л����ʧЧ.
	/* Likewise with block-devices: check for floppy_change */
	/* ͬ�����ڿ��豸�ļ�:��Ҫ�����Ƭ�Ƿ񱻸��� */
	if (S_ISBLK(inode->i_mode))
		check_disk_change(inode->i_zone[0]);
	// �������ǳ�ʼ�����ļ����ļ��ṹ.�����ļ��ṹ���Ժͱ�־,�þ�����ü���Ϊ1,������i�ڵ��ֶ�Ϊ���ļ���i�ڵ�,��ʼ���ļ���дָ��Ϊ0.��󷵻��ļ������.
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	return (fd);
}

// �����ļ�ϵͳ���á�
// ����pathname��·������mode�������sys_open()������ͬ��
// �ɹ��򷵻��ļ���������򷵻س����롣
int sys_creat(const char * pathname, int mode)
{
	return sys_open(pathname, O_CREAT | O_TRUNC, mode);
}

// �ر��ļ�ϵͳ����.
// ����fd���ļ����.
// �ɹ��򷵻�0,���򷵻س�����.
int sys_close(unsigned int fd)
{
	struct file * filp;

	// ���ȼ�������Ч��.���������ļ����ֵ���ڳ���ͬʱ�򿪵��ļ���NR_OPEN,�򷵻س�����(������Ч).Ȼ��λ���̵�ִ��ʱ�ر��ļ����λͼ��Ӧλ.�����ļ������Ӧ��
	// �ļ��ṹָ����NULL,�򷵻س�����.
	if (fd >= NR_OPEN)
		return -EINVAL;
	current->close_on_exec &= ~(1 << fd);
	if (!(filp = current->filp[fd]))
		return -EINVAL;
	// �����ø��ļ�������ļ��ṹָ��ΪNULL.���ڹر��ļ�֮ǰ,��Ӧ�ļ��ṹ�еľ�����ü����Ѿ�Ϊ0,��˵���ں˳���,ͣ��.���򽫶�Ӧ�ļ��ṹ�����ü�����1.��ʱ���������
	// Ϊ0,��˵����������������ʹ�ø��ļ�,���Ƿ���0(�ɹ�).������ü����ѵ���0,˵�����ļ��Ѿ�û�н�������,���ļ��ṹ�ѱ�Ϊ����.���ͷŸ��ļ�i�ڵ�,����0.
	current->filp[fd] = NULL;
	if (filp->f_count == 0)
		panic("Close: file count is 0");
	if (--filp->f_count)
		return (0);
	iput(filp->f_inode);
	return (0);
}

