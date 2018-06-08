/*
 * This file contains the procedures for the handling of select
 *
 * Created for Linux based loosely upon Mathius Lattner's minix
 * patches by Peter MacDonald. Heavily edited by Linus.
 */
/*
 * ���ļ����д���select()ϵͳ���õĹ��̡�
 *
 * ����Peter MacDonald����Mathius Lattner�ṩ��MINIXϵͳ�Ĳ��������޸Ķ��ɡ�
 */

#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/sched.h>

#include <asm/segment.h>
#include <asm/system.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>

/*
 * Ok, Peter made a complicated, but straightforward multiple_wait() function.
 * I have rewritten this, taking some shortcuts: This code may not be easy to
 * follow, but it should be free of race-conditions, and it's practical. If you
 * understand what I'm doing here, then you understand how the linux sleep/wakeup
 * mechanism works.
 *
 * Two very simple procedures, add_wait() and free_wait() make all the work. We
 * have to have interrupts disabled throughout the select, but that's not really
 * such a loss: sleeping automatically frees interrupts when we aren't in this
 * task.
 */
/*
 * OK��Peter�����˸��ӵ���ֱ�۵Ķ��_wait()�������Ҷ���Щ���������˸�д����ʹ֮����ࣺ��Щ������ܲ����׿�������������Ӧ��
 * ������ھ����������⣬���Һ�ʵ�ʡ�����������������ƵĴ��룬��ô��˵�����Ѿ����Linux��˯��/���ѵĹ������ơ�
 *
 * �����ܼ򵥵Ĺ��̣�add_wait()��free_wait()ִ������Ҫ������������select������������ǲ��ò���ֹ�жϡ��������������������
 * ̫�����ʧ����Ϊ�����ǲ���ִ�б�����ʱ˯��״̬���Զ��ͷ��жϣ������������ʹ���Լ�EFLAGS�е��жϱ�־����
 */

typedef struct {
	struct task_struct * old_task;
	struct task_struct ** wait_address;
} wait_entry;

typedef struct {
	int nr;
	wait_entry entry[NR_OPEN * 3];
} select_table;

// ��δ׼�����������ĵȴ�����ָ�����ȴ���wait_table�С�����*wait_address������������صĵȴ�����ͷָ�롣����tty�������
// ��secondary�ĵȴ�����ͷָ����proc_list������p��do_select()�ж���ĵȴ���ṹָ�롣
static void add_wait(struct task_struct ** wait_address, select_table * p)
{
	int i;

	// �����ж��������Ƿ��ж�Ӧ�ĵȴ����У������򷵻ء�Ȼ���ڵȴ�������������ָ���ĵȴ�����ָ���Ƿ��Ѿ��ڵȴ��������ù��������ù�Ҳ
	// ���̷��ء�����ж���Ҫ����Թܵ��ļ���������������һ���ܵ��ڵȴ����Խ��ж���������ô��ض��������̽���д������
	if (!wait_address)
		return;
	for (i = 0 ; i < p->nr ; i++)
		if (p->entry[i].wait_address == wait_address)
			return;
	// Ȼ�����ǰ���������Ӧ�ȴ����е�ͷָ�뱣���ڵȴ���wait_table�У�ͬʱ�õȴ������old_task�ֶ�ָ��ȴ�����ͷָ��ָ���������
	// ����ΪNULL�������õȴ�����ͷָ��ָ��ǰ�������ѵȴ�����Ч�����ֵnr��1�����ڵ�179�г�ʼ��Ϊ0����
	p->entry[p->nr].wait_address = wait_address;
	p->entry[p->nr].old_task = * wait_address;
	*wait_address = current;
	p->nr++;
}

// ��յȴ��������ǵȴ���ṹָ�롣��������do_select()������˯�ߺ󱻻��ѷ���ʱ�����ã����ڻ��ѵȴ����д��ڸ����ȴ������ϵ�����
// ��������kernel/sched.c��sleep_on()�����ĺ�벿�ִ��뼸����ȫ��ͬ����ο���sleep_on()������˵����
static void free_wait(select_table * p)
{
	int i;
	struct task_struct ** tpp;

	// ����ȴ����и����nr����Ч���¼�ĵȴ�����ͷָ�������������������ӽ��ĵȴ����������������̵���sleep_on()������˯���ڸ�
	// �ȴ������ϣ������ʱ�ȴ�����ͷָ��ָ��Ĳ��ǵ�ǰ���̣���ô���Ǿ���Ҫ�Ȼ�����Щ���񡣲��������ǽ��ȴ�����ͷ��ָ��������Ϊ����״̬
	// ��state = 0���������Լ�����Ϊ�����жϵȴ�״̬�����Լ�Ҫ�ȴ���Щ���������е����񱻻��Ѷ�ִ��ʱ�����ѱ�����Ȼ������ִ�е��ȳ���
	for (i = 0; i < p->nr ; i++) {
		tpp = p->entry[i].wait_address;
		while (*tpp && *tpp != current) {
			(*tpp)->state = 0;
			current->state = TASK_UNINTERRUPTIBLE;
			schedule();
		}
		// ִ�е����˵���ȴ���ǰ�������еĵȴ�����ͷָ���ֶ�wait_addressָ��ǰ��������Ϊ�գ���������������⣬������ʾ������Ϣ��
		// Ȼ�������õȴ�����ͷָ��ָ��������ǰ�������е����񣨵�76�У�������ʱ��ͷָ��ȷʵָ��һ�����������NULL����˵�������л�������
		// *tpp��Ϊ�գ������ǽ����������óɾ���״̬������֮�����ѵȴ������Ч��������ֶ�nr���㡣
		if (!*tpp)
			printk("free_wait: NULL");
		if (*tpp = p->entry[i].old_task)
			(**tpp).state = 0;
	}
	p->nr = 0;
}

// �����ļ�i�ڵ��ж��ļ��ǲ����ַ��ն��豸�ļ��������򷵻���tty�ṹָ�룬���򷵻�NULL��
static struct tty_struct * get_tty(struct m_inode * inode)
{
	int major, minor;

	// ��������ַ��豸�ļ��򷵻�NULL��������豸�Ų���5�������նˣ���4���򷵻�NULL��
	if (!S_ISCHR(inode->i_mode))
		return NULL;
	if ((major = MAJOR(inode->i_zone[0])) != 5 && major != 4)
		return NULL;
	// ������豸����5����ô���ն��豸�ŵ��ڽ��̵�tty�ֶ�ֵ������͵����ַ��豸�ļ����豸�š�����ն��豸��С��0,��ʾ����û��
	// �����ն˻�û��ʹ���նˣ����Ƿ���NULL�����򷵻ض�Ӧ��tty�ṹָ�롣
	if (major == 5)
		minor = current->tty;
	else
		minor = MINOR(inode->i_zone[0]);
	if (minor < 0)
		return NULL;
	return TTY_TABLE(minor);
}

/*
 * The check_XX functions check out a file. We know it's either
 * a pipe, a character device or a fifo (fifo's not implemented)
 */
/*
 * check_XX�������ڼ��һ���ļ�������֪�����ļ�Ҫô�ǹܵ��ļ���Ҫô���ַ��豸�ļ�������Ҫô��һ��FIFO��FIFO����δʵ�֡�
 */
// �����ļ������Ƿ�׼���ã����ն˶��������secondary�Ƿ����ַ��ɶ������߹ܵ��ļ��Ƿ񲻿ա�����wait�ǵȴ���ָ�룻inode
// ���ļ�i�ڵ�ָ�롣���������ɽ��ж������򷵻�1,���򷵻�0��
static int check_in(select_table * wait, struct m_inode * inode)
{
	struct tty_struct * tty;

	// ���ȸ����ļ�i�ڵ����get_tty()����ļ��ǲ���һ��tty�նˣ��ַ����豸�ļ��������������ն˶��������secondary���Ƿ���
	// �ַ��ɹ���ȡ�������򷵻�1������ʱsecondaryΪ����ѵ�ǰ������ӵ�secondary�ĵȴ�����proc_list�ϲ�����0������ǹܵ��ļ�
	// ���ж�Ŀǰ�ܵ����Ƿ����ַ��ɶ��������򷵻�1����û�У��ܵ��գ���ѵ�ǰ������ӵ��ܵ�i�ڵ�ĵȴ������ϲ�����0��ע�⣬PIPE_-
	// EMPTY()��ʹ�ùܵ���ǰͷβָ��λ�����жϹܵ��Ƿ�Ϊ�ա��ܵ�i�ڵ��i_zone[0]��i_zone[1]�ֶηֱ����Źܵ���ǰ��ͷβָ�롣
	if (tty = get_tty(inode))
		if (!EMPTY(tty->secondary))
			return 1;
		else
			add_wait(&tty->secondary->proc_list, wait);
	else if (inode->i_pipe)
		if (!PIPE_EMPTY(*inode))
			return 1;
		else
			add_wait(&inode->i_wait, wait);
	return 0;
}

// ����ļ�д�����Ƿ�׼���ã����ն�д�������write_q���Ƿ��п���λ�ÿ�д�����ߴ�ʱ�ܵ��ļ��Ƿ���������wait�ǵȴ���ָ�룻
// inode���ļ�i�ڵ�ָ�롣���������ɽ���д�����򷵻�1�����򷵻�0��
static int check_out(select_table * wait, struct m_inode * inode)
{
	struct tty_struct * tty;

	// ���ȸ����ļ�i�ڵ����get_tty()����ļ��ǲ���һ��tty�նˣ��ַ����豸�ļ��������������ն�д�������write_q���Ƿ��пռ�
	// ��д�룬�����򷵻�1,��û�пռ���ѵ�ǰ������ӵ�write_q�ȴ�����proc_list�ϲ�����0������ǹܵ��ļ����ж�Ŀǰ�ܵ����Ƿ���
	// ���пռ��д���ַ��������򷵻�1����û�У��ܵ�������ѵ�ǰ������ӵ��ܵ�i�ڵ�ĵȴ������ϲ�����0��
	if (tty = get_tty(inode))
		if (!FULL(tty->write_q))
			return 1;
		else
			add_wait(&tty->write_q->proc_list, wait);
	else if (inode->i_pipe)
		if (!PIPE_FULL(*inode))
			return 1;
		else
			add_wait(&inode->i_wait, wait);
	return 0;
}

// ����ļ��Ƿ����쳣״̬�������ն��豸�ļ���Ŀǰ�ں����Ƿ���0�����ڹܵ��ļ��������ʱ�����ܵ�����������һ�����ѱ��رգ���
// ����1������Ͱѵ�ǰ������ӵ��ܵ�i�ڵ�ĵȴ������ϲ�����0������0������wait�ȴ���ָ�룻inode���ļ�i�ڵ�ָ�롣�������쳣����
// �򷵻�1�����򷵻�0��
static int check_ex(select_table * wait, struct m_inode * inode)
{
	struct tty_struct * tty;

	if (tty = get_tty(inode))
		if (!FULL(tty->write_q))
			return 0;
		else
			return 0;
	else if (inode->i_pipe)
		if (inode->i_count < 2)
			return 1;
		else
			add_wait(&inode->i_wait, wait);
	return 0;
}

// do_select()���ں�ִ��select()ϵͳ���õ�ʵ�ʴ��������ú������ȼ�����������и�������������Ч�ԣ�Ȼ��ֱ�������������
// ����������麯��check_XX()��ÿ�����������м�飬ͬʱͳ�����������е�ǰ�Ѿ�׼���õ������������������κ�һ���������Ѿ�׼���ã�
// �������ͻ����̷��أ�������̾ͻ��ڱ������н���˯��״̬�����ڹ��˳�ʱʱ���������ĳ�����������ڵȴ������ϵĽ��̱����Ѷ�ʹ��
// ���̼������С�
int do_select(fd_set in, fd_set out, fd_set ex,
	fd_set *inp, fd_set *outp, fd_set *exp)
{
	int count;
	select_table wait_table;
	int i;
	fd_set mask;

	// ���Ȱ�3�������������л��������mask�еõ�������������Ч��λ�����롣Ȼ��ѭ���жϵ�ǰ���̸����������Ƿ���Ч���Ұ��������������ڡ�
	// ��ѭ���У�ÿ�ж���һ���������ͻ��mask����1λ����˸���mask�������Чλ���ǾͿ����ж���Ӧ�������Ƿ����û����������������С���
	// Ч��������Ӧ����һ���ܵ��ļ���������������һ���ַ��豸�ļ���������������һ��FIFO���������������͵Ķ���Ϊ��Ч������������EBADF
	// ����
	mask = in | out | ex;
	for (i = 0 ; i < NR_OPEN ; i++, mask >>= 1) {
		if (!(mask & 1))                                        // ����������������������ж���һ����
			continue;
		if (!current->filp[i])                                  // ���ļ�δ�򿪣��򷵻�������ֵ��
			return -EBADF;
		if (!current->filp[i]->f_inode)                         // ���ļ�i�ڵ�ָ��Ϊ�գ��򷵻ش���š�
			return -EBADF;
		if (current->filp[i]->f_inode->i_pipe)                  // ���ǹܵ��ļ�������������Ч��
			continue;
		if (S_ISCHR(current->filp[i]->f_inode->i_mode))         // �ַ��豸�ļ���Ч��
			continue;
		if (S_ISFIFO(current->filp[i]->f_inode->i_mode))        // FIFOҲ��Ч��
			continue;
		return -EBADF;                  						// ���඼��Ϊ��Ч�����������ء�
	}
	// ����ѭ�����3�����������еĸ����������Ƿ�׼���ã����Բ���������ʱmask������ǰ���ڴ����������������롣ѭ���е�3������check_in()��
	// check_out()��check_ex()�ֱ������ж��������Ƿ��Ѿ�׼���á���һ���������Ѿ�׼���ã���������������������ö�Ӧλ�����Ұ���׼��
	// ����������������ֵcount��1����186��forѭ������е�mask+= mask�н���mask<<1��
repeat:
	wait_table.nr = 0;
	*inp = *outp = *exp = 0;
	count = 0;
	mask = 1;
	for (i = 0 ; i < NR_OPEN ; i++, mask += mask) {
		// �����ʱ�жϵ��������ڶ��������������У����Ҹ��������Ѿ�׼���ÿ��Խ��ж���������Ѹ�����������������in�ж�Ӧλ��Ϊ1,ͬʱ����׼��
		// ����������������ֵcount��1��
		if (mask & in)
			if (check_in(&wait_table, current->filp[i]->f_inode)) {
				*inp |= mask;   								// �������������ö�Ӧλ��
				count++;        								// ��׼��������������������
			}
		// �����ʱ�жϵ���������д�������������У����Ҹ��������Ѿ�׼���ÿ��Խ���д��������Ѹ�����������������out�ж�Ӧλ��Ϊ1,ͬʱ����׼��
		// ����������������ֵcount��1��
		if (mask & out)
			if (check_out(&wait_table, current->filp[i]->f_inode)) {
				*outp |= mask;
				count++;
			}
		// �����ʱ�жϵ����������쳣���������У����Ҹ��������Ѿ����쳣���֣���Ѹ�����������������ex�ж�Ӧλ��Ϊ1,ͬʱ����׼����������������
		// ��ֵcount��1��
		if (mask & ex)
			if (check_ex(&wait_table, current->filp[i]->f_inode)) {
				*exp |= mask;
				count++;
			}
	}
	// �ڶԽ��������������жϴ������û�з�������׼���õ���������count==0�������Ҵ�ʱ����û���յ��κη������źţ����Ҵ�ʱ�еȴ���������
	// ���ߵȴ�ʱ�仹û�г�ʱ����ô���ǾͰѵ�ǰ����״̬���óɿ��ж�˯��״̬��Ȼ��ִ�е��Ⱥ���ȥִ���������񡣵��ں���һ�ε���ִ�б�����ʱ��
	// ����free_wait()������صȴ������ϱ�����ǰ�������,Ȼ����ת��repeat��Ŵ��ٴ����¼���Ƿ������ǹ��ĵģ����������еģ���������׼��
	// �á�
	if (!(current->signal & ~current->blocked) &&
	    (wait_table.nr || current->timeout) && !count) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		free_wait(&wait_table);         						// �����񱻻��ѷ��غ�����￪ʼִ�С�
		goto repeat;
	}
	// �����ʱcount������0�����߽��յ����źţ����ߵȴ�ʱ�䵽����û����Ҫ�ȴ�������������ô���Ǿ͵���free_wait()���ѵȴ�������
	// ������Ȼ�󷵻���׼���õ�������������
	free_wait(&wait_table);
	return count;
}

/*
 * Note that we cannot return -ERESTARTSYS, as we change our input
 * parameters. Sad, but there you are. We could do some tweaking in
 * the library function ...
 */
/*
 * ע�����ǲ��ܷ���-ERESTARTSYS����Ϊ���ǻ���select���й����иı��������ֵ��*timeout�����ܲ��ң�����Ҳֻ�ܽ���
 * �����ʵ���������ǿ����ڿ⺯������Щ����...
 */
// selectϵͳ���ú������ú����еĴ�����Ҫ�������select���ܲ���ǰ��Ĳ������ƺ�ת��������select��Ҫ�Ĺ�����do_select()
// ��������ɡ�sys_select()�����ȸ��ݲ����������Ļ�����ָ����û����ݿռ��select()�������õĲ����ֽ⸴�Ƶ��ں˿ռ䣬Ȼ��
// ������Ҫ�ȴ��ĳ�ʱʱ��ֵtimeout�����ŵ���do_select()ִ��select���ܣ����غ�ͰѴ������ٸ��ƻ��û��ռ��С�
// ����bufferָ���û���������select()�����ĵ�1�����������������ֵС��0��ʾִ��ʱ���ִ����������ֵ����0,���ʾ�ڹ涨��
// ��ʱ����û��������׼���ò������������ֵ����0,���ʾ��׼���õ�������������
int sys_select( unsigned long *buffer )
{
	/* Perform the select(nd, in, out, ex, tv) system call. */
	/* ִ��select(nd, in, out, ex, tv)ϵͳ���� */
	// ���ȶ��弸���ֲ����������ڰ�ָ�������������select()���������ֽ⿪����
	int i;
	fd_set res_in, in = 0, *inp;            						// ����������������
	fd_set res_out, out = 0, *outp;         						// д��������������
	fd_set res_ex, ex = 0, *exp;            						// �쳣��������������
	fd_set mask;                            						// �������������ֵ��Χ��nd�������롣
	struct timeval *tvp;                    						// �ȴ�ʱ��ṹָ�롣
	unsigned long timeout;

	// Ȼ����û��������Ѳ����ֱ���븴�Ƶ��ֲ�ָ������У���������������ָ���Ƿ���Ч�ֱ�ȡ��3����������in��������out��д����ex
	// ���쳣��������maskҲ��һ��������������������3���������������������ֵ+1������1������nd��ֵ�����������ó��û�������ĵ�����
	// �������������롣���磬��nd = 4,��mask = 0b00001111����32λ����
	mask = ~((~0) << get_fs_long(buffer++));
	inp = (fd_set *) get_fs_long(buffer++);
	outp = (fd_set *) get_fs_long(buffer++);
	exp = (fd_set *) get_fs_long(buffer++);
	tvp = (struct timeval *) get_fs_long(buffer);

	if (inp)                                						// ��ָ����Ч����ȡ����������������
		in = mask & get_fs_long(inp);
	if (outp)                               						// ��ָ����Ч����ȡд��������������
		out = mask & get_fs_long(outp);
	if (exp)                                						// ��ָ����Ч����ȡ�쳣����������
		ex = mask & get_fs_long(exp);
	// ���������ǳ��Դ�ʱ��ṹ��ȡ���ȴ���˯�ߣ�ʱ��ֵtimeout�����Ȱ�timeout��ʼ����������ޣ�ֵ��Ȼ����û����ݿռ�ȡ�ø�ʱ��
	// �ṹ�����õ�ʱ��ֵ����ת���ͼ���ϵͳ��ǰ���ֵjiffies�����õ���Ҫ�ȴ���ʱ�������ֵtimeout�������ô�ֵ�����õ�ǰ����Ӧ��
	// �ȴ�����ʱ�����⣬��241����tv_usec�ֶ���΢��ֵ����������1000000��ɵõ���Ӧ�������ٳ���ϵͳÿ�������HZ������tv_usecת��
	// �����ֵ��
	timeout = 0xffffffff;
	if (tvp) {
		timeout = get_fs_long((unsigned long *) & tvp->tv_usec) / (1000000 / HZ);
		timeout += get_fs_long((unsigned long *) & tvp->tv_sec) * HZ;
		timeout += jiffies;
	}
	current->timeout = timeout;             						// ���õ�ǰ����Ӧ����ʱ�����ֵ��
	// select()��������Ҫ������do_select()����ɡ��ڵ��øú���֮��Ĵ������ڰѴ��������Ƶ��û��������У����ظ��û���Ϊ�˱������
	// �����������ڵ���do_select()ǰ��Ҫ��ֹ�жϣ����ڸú������غ��ٿ����жϡ�
	// �����do_select()����֮����̵ĵȴ���ʱ�ֶ�timeout�����ڵ�ǰϵͳ��ʱ���ֵjiffies��˵���ڳ�ʱ֮ǰ�Ѿ�������׼���ã���������
	// �����ȼ��µ���ʱ��ʣ���ʱ��ֵ��������ǻ�����ֵ���ظ��û���������̵ĵȴ���ʱ�ֶ�timeout�Ѿ�С�ڻ���ڵ�ǰϵͳjiffies����ʾ
	// do_select()���������ڳ�ʱ�����أ���˰�ʣ��ʱ��ֵ����Ϊ0��
	cli();                  										// ��ֹ��Ӧ�жϡ�
	i = do_select(in, out, ex, &res_in, &res_out, &res_ex);
	if (current->timeout > jiffies)
		timeout = current->timeout - jiffies;
	else
		timeout = 0;
	sti();                  										// �����ж���Ӧ��
	// ���������ǰѽ��̵ĳ�ʱ�ֶ����㡣���do_select()���ص���׼��������������С��0����ʾִ�г������Ƿ����������š�Ȼ�����ǰѴ����
	// �������������ݺ��ӳ�ʱ��ṹ����д�ص��û����ݻ���ռ䡣��ʱ��ṹ����ʱ����Ҫ�Ƚ����ʱ�䵥λ��ʾ��ʣ���ӳ�ʱ��ת�������΢��ֵ��
	current->timeout = 0;
	if (i < 0)
		return i;
	if (inp) {
		verify_area(inp, 4);
		put_fs_long(res_in, inp);        							// �ɶ�������ֵ��
	}
	if (outp) {
		verify_area(outp, 4);
		put_fs_long(res_out, outp);      							// ��д������ֵ��
	}
	if (exp) {
		verify_area(exp, 4);
		put_fs_long(res_ex, exp);        							// �����쳣��������������
	}
	if (tvp) {
		verify_area(tvp, sizeof(*tvp));
		put_fs_long(timeout / HZ, (unsigned long *) &tvp->tv_sec);  // �롣
		timeout %= HZ;
		timeout *= (1000000 / HZ);
		put_fs_long(timeout, (unsigned long *) &tvp->tv_usec);      // ΢�롣
	}
	// �����ʱ��û����׼���õ��������������յ���ĳ���������źţ��򷵻ر��жϴ���š����򷵻���׼���õ�����������ֵ��
	if (!i && (current->signal & ~current->blocked))
		return -EINTR;
	return i;
}

