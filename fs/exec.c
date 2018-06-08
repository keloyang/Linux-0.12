/*
 *  linux/fs/exec.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * #!-checking implemented by tytso.
 */
/*
 * #!��ʼ�ű�����ļ����벿������tytsoʵ�ֵ�.
 */

/*
 * Demand-loading implemented 01.12.91 - no need to read anything but
 * the header into memory. The inode of the executable is put into
 * "current->executable", and page faults do the actual loading. Clean.
 *
 * Once more I can proudly say that linux stood up to being changed: it
 * was less than 2 hours work to get demand-loading completely implemented.
 */
/*
 * ����ʱ����ʵ����1991.12.1 - ֻ�轫ִ���ļ�ͷ�������ڴ�����뽫����ִ���ļ������ؽ��ڴ�.ִ���ļ���i�ڵ㱻���ڵ�ǰ���̵Ŀ�ִ���ֶ���"current->executable",
 * ҳ�쳣�����ִ���ļ���ʵ�ʼ��ز���.�������.
 *
 * �ҿ���һ���Ժ���˵,linux�������޸�:ֻ���˲���2Сʱ�Ĺ�������ȫʵ����������ش���.
 */

//#include <signal.h>									// ��Ϣͷ�ļ�.�����źŷ��ų���,�źŽṹ���źŲ�������ԭ��.
#include <errno.h>										// �����ͷ�ļ�.����ϵͳ�и��ֳ����.
#include <string.h>
#include <sys/stat.h>									// �ļ�״̬ͷ�ļ�.�����ļ�״̬�ṹstat{}�ͷ��ų�����.
#include <a.out.h>										// a.outͷ�ļ�.������a.outִ���ļ���ʽ��һЩ��.

#include <linux/fs.h>									// �ļ�ϵͳͷ�ļ�.�����ļ���ṹ(file,m_inode)��.
#include <linux/sched.h>								// ���ȳ���ͷ�ļ�,����������ṹtask_struct,����0���ݵ�.
//#include <linux/kernel.h>								// �ں�ͷ�ļ�.����һЩ�ں˳��ú�����ԭ�Ͷ���.
//#include <linux/mm.h>									// �ڴ����ͷ�ļ�.����ҳ���С�����һЩҳ���ͷź���ԭ��.
#include <asm/segment.h>								// �β���ͷ�ļ�.�������йضμĴ���������Ƕ��ʽ��ຯ��.

extern int sys_exit(int exit_code);
extern int sys_close(int fd);

/*
 * MAX_ARG_PAGES defines the number of pages allocated for arguments
 * and envelope for the new program. 32 should suffice, this gives
 * a maximum env+arg of 128kB !
 */
/*
 * MAX_ARG_PAGES������Ϊ�³������ĸ������ͻ�������ʹ�õ�����ڴ�ҳ��.32ҳ�ڴ�Ӧ���㹻��,��ʹ�û����Ͳ���(env+arg)�ռ���ܺʹﵽ128KB!
 */
#define MAX_ARG_PAGES 32

// ʹ�ÿ��ļ�ϵͳ���á�
// ������library - ���ļ�����
// Ϊ����ѡ��һ�����ļ������滻���̵�ǰ���ļ�i�ڵ��ֶ�ֵΪ����ָ�����ļ�����i�ڵ�ָ�롣���libraryָ��Ϊ�գ���ѽ���
// ��ǰ�Ŀ��ļ��ͷŵ���
// ���أ��ɹ�����0�����򷵻س����롣
int sys_uselib(const char * library)
{
	struct m_inode * inode;
	unsigned long base;

	// �����жϵ�ǰ�����Ƿ���ͨ���̡�����ͨ���鿴��ǰ���̵Ŀռ䳤���������ġ���Ϊ��ͨ���̵Ŀռ䳤�ȱ�����ΪTASK_SIZE��64
	// MB��������������߼���ַ�ռ䳤�Ȳ�����TASK_SIZE�򷵻س����루��Ч������������ȡ���ļ�i�ڵ�inode�������ļ���ָ��
	// �գ�������inode����NULL��
	if (get_limit(0x17) != TASK_SIZE)
		return -EINVAL;
	if (library) {
		if (!(inode = namei(library)))							/* get library inode */
			return -ENOENT;                 					/* ȡ���ļ�i�ڵ� */
	} else
		inode = NULL;
	/* we should check filetypes (headers etc), but we don't */
	/* ����Ӧ�ü��һ���ļ����ͣ���ͷ����Ϣ�ȣ����������ǻ�û����������*/
	// Ȼ��Żؽ���ԭ���ļ�i�ڵ㣬��Ԥ�ý��̿�i�ڵ��ֶ�Ϊ�ա�����ȡ�ý��̵Ŀ��������λ�ã����ͷ�ԭ������ҳ����ռ�õ��ڴ�
	// ҳ�档����ý��̿�i�ڵ��ֶ�ָ���¿�i�ڵ㣬������0���ɹ�����
	iput(current->library);
	current->library = NULL;
	base = get_base(current->ldt[2]);
	base += LIBRARY_OFFSET;
	free_page_tables(base, LIBRARY_SIZE);
	current->library = inode;
	return 0;
}

/*
 * create_tables() parses the env- and arg-strings in new user
 * memory and creates the pointer tables from them, and puts their
 * addresses on the "stack", returning the new stack pointer value.
 */
/*
 * create_tables()�������������ڴ��н������������Ͳ����ַ���,�ɴ˴���ָ���,�������ǵĵ�ַ�ŵ�"ջ"��,Ȼ�󷵻���ջ��ָ��ֵ.
 */
// ���������д��������ͻ�������ָ���.
// ����:p - ���ݶ��в����ͻ�����Ϣƫ��ָ��;argc - ��������;envc - ������������.
// ����:ջָ��ֵ.
static unsigned long * create_tables(char * p, int argc, int envc)
{
	unsigned long *argv, *envp;
	unsigned long * sp;

	// ջָ������4�ֽ�(1��)Ϊ�߽����Ѱַ��,�����������spΪ4��������ֵ.��ʱspλ�ڲ����������ĩ��.Ȼ�������Ȱ�sp����
	// (�͵�ַ����)�ƶ�,��ջ�пճ���������ָ��ռ�õĿռ�,���û�������ָ��envpָ��ô�.��ճ���һ��λ�������������һ
	// ��NULLֵ.����ָ���1,sp������ָ�����ֽ�ֵ(4�ֽ�).�ٰ�sp�����ƶ�,�ճ������в���ָ��ռ�õĿռ�,����argvָ��
	// ָ��ô�.ͬ��,��մ���һ��λ�����ڴ��һ��NULLֵ.��ʱspָ�����ָ������ʼ��,���ǽ�����������ָ��envp��������
	// ������ָ���Լ������в�������ֵ�ֱ�ѹ��ջ��.
	sp = (unsigned long *) (0xfffffffc & (unsigned long) p);
	sp -= envc + 1;
	envp = sp;
	sp -= argc + 1;
	argv = sp;
	put_fs_long((unsigned long)envp, --sp);
	put_fs_long((unsigned long)argv, --sp);
	put_fs_long((unsigned long)argc, --sp);
	// �ٽ������и�����ָ��ͻ���������ָ��ֱ����ǰ��ճ�������Ӧ�ط�,���ֱ����һ��NULLָ��.
	while (argc-- > 0) {
		put_fs_long((unsigned long) p, argv++);
		while (get_fs_byte(p++)) /* nothing */ ;	// pָ��ָ����һ��������.
	}
	put_fs_long(0, argv);
	while (envc-- > 0) {
		put_fs_long((unsigned long) p, envp++);
		while (get_fs_byte(p++)) /* nothing */ ;	// pָ��ָ����һ��������.
	}
	put_fs_long(0, envp);
	return sp;										// ���ع���ĵ�ǰ��ջָ��.
}

/*
 * count() counts the number of arguments/envelopes
 */
/*
 * count()�������������в���/��������ĸ���.
 */
// �����������.
// ����:argv - ����ָ������,���һ��ָ������NULL.
// ͳ�Ʋ���ָ��������ָ��ĸ���.
// ����:��������.
static int count(char ** argv)
{
	int i = 0;
	char ** tmp;

	if (tmp = argv)
		while (get_fs_long((unsigned long *) (tmp++)))
			i++;

	return i;
}

/*
 * 'copy_string()' copies argument/envelope strings from user
 * memory to free pages in kernel mem. These are in a format ready
 * to be put directly into the top of new user memory.
 *
 * Modified by TYT, 11/24/91 to add the from_kmem argument, which specifies
 * whether the string and the string array are from user or kernel segments:
 *
 * from_kmem     argv *        argv **
 *    0          user space    user space
 *    1          kernel space  user space
 *    2          kernel space  kernel space
 *
 * We do this by playing games with the fs segment register.  Since it
 * it is expensive to load a segment register, we try to avoid calling
 * set_fs() unless we absolutely have to.
 */
/*
 * 'copy_string()'�������û��ڴ�ռ临�Ʋ���/�����ַ������ں˿���ҳ����.��Щ�Ѿ���ֱ�ӷŵ����û��ڴ��еĸ�ʽ.
 *
 * ��TYT(Tytso)��1991.11.24���޸�,������from_kmem����,�ò���ָ�����ַ������ַ��������������û��λ����ں˶�.
 *
 * from_kmem     ָ�� argv *        �ַ��� argv **
 *    0          �û��ռ�		�û��ռ�
 *    1          �ں˿ռ�		�û��ռ�
 *    2          �ں˿ռ�		�ں˿ռ�
 *
 * ������ͨ�������fs�μĴ�����������.���ڼ���һ���μĴ�������̫��,�������Ǿ����������set_fs(),����ʵ�ڱ�Ҫ.
 */
// ����ָ�������Ĳ����ַ����������ͻ����ռ���.
// ����:argc - ����ӵĲ�������;argv - ����ָ������;page - �����ͻ����ռ�ҳ��ָ������.p - ������ռ���ƫ��ָ��,ʼ��ָ��
// �Ѹ��ƴ���ͷ��;from_kmem - �ַ�����Դ��־����do_execve()������,p��ʼ��Ϊָ��
// ������(128KB)�ռ�����һ�����ִ�,�����ַ������Զ�ջ������ʽ���������и��ƴ�ŵ�.���pָ������Ÿ�����Ϣ�����Ӷ��𽥼�С,
// ��ʼ��ָ������ַ�����ͷ��.�ַ�����Դ��־from_kmemӦ����TYTΪ�˸�execve()����ִ�нű��ļ��Ĺ��ܶ��¼ӵĲ���.��û������
// �ű��ļ��Ĺ���ʱ,���в����ַ��������û����ݿռ��С�
// ����:�����ͻ����ռ䵱ǰͷ��ָ��.�������򷵻�0.
static unsigned long copy_strings(int argc, char ** argv, unsigned long *page,
		unsigned long p, int from_kmem)
{
	char *tmp, *pag;
	int len, offset = 0;
	unsigned long old_fs, new_fs;

	if (!p)
		return 0;												/* bullet-proofing */	/* ƫ��ָ����֤ */
	// ����ȡ��ǰ�μĴ���ds(ָ���ں����ݶ�)��fsֵ,�ֱ𱣴浽����new_fs��old_fs��.����ַ������ַ�������(ָ��)�����ں˿ռ�,������fs�μĴ���ָ���ں����ݶ�.
	new_fs = get_ds();
	old_fs = get_fs();
	if (from_kmem == 2)											// ����ָ�����ں˿ռ�������fsָ���ں˿ռ�.
		set_fs(new_fs);
	// Ȼ��ѭ�������������,�����һ����������ʼ����,���Ƶ�ָ��ƫ�Ƶ�ַ��.��ѭ����,����ȡ��Ҫ���Ƶĵ�ǰ�ַ���ָ��.����ַ���
	// ���û��ռ���ַ�������(�ַ���ָ��)���ں˿ռ�,������fs�μĴ���ָ���ں����ݶ�(ds).�����ں����ݿռ���ȡ���ַ���ָ��tmp֮
	// ������ָ̻�fs�μĴ���ԭֵ(fs��ָ���û��ռ�).�������޸�fsֵ��ֱ�Ӵ��û��ռ�ȡ�ַ���ָ�뵽tmp.
	while (argc-- > 0) {
		if (from_kmem == 1)										// ����ָ�����ں˿ռ�,��fsָ���ں˿ռ�.
			set_fs(new_fs);
		if (!(tmp = (char *)get_fs_long(((unsigned long *)argv) + argc)))
			panic("argc is wrong");
		if (from_kmem == 1)										// ����ָ�����ں˿ռ�,��fsָ���û��ռ�.
			set_fs(old_fs);
		// Ȼ����û��ռ�ȡ���ַ���,����������ַ�������len.�˺�tmpָ����ַ���ĩ��.������ַ������ȳ�����ʱ�����ͻ����ռ��л�ʣ��
		// �Ŀ��г���,��ռ䲻����.���ǻָ�fs�μĴ���ֵ(������ı�Ļ�)������0.������Ϊ�����ͻ����ռ���128KB,����ͨ�������ܷ�����
		// �����.
		len = 0;												/* remember zero-padding */
		do {													/* ����֪��������NULL�ֽڽ�β�� */
			len++;
		} while (get_fs_byte(tmp++));
		if (p - len < 0) {										/* this shouldn't happen - 128kB */
			set_fs(old_fs);										/* ���ᷢ��--��Ϊ��128KB�Ŀռ� */
			return 0;
		}
		// ����������������ַ��ذ��ַ������Ƶ������ͻ����ռ�ĩ�˴�.��ѭ�������ַ������ַ�������,��������Ҫ�жϲ����ͻ����ռ�����Ӧλ
		// �ô��Ƿ��Ѿ����ڴ�ҳ��.�����û�о���Ϊ������1ҳ�ڴ�ҳ��.ƫ����offset����;Ϊ��һ��ҳ���еĵ�ǰָ��ƫ��ֵ.��Ϊ�տ�ʼִ��
		// ������ʱ,ƫ�Ʊ���offset����ʼ��Ϊ0,����(offset-1 < 0)�϶�������ʹ��offset���±�����Ϊ��ǰpָ����ҳ�淶Χ�ڵ�ƫ��ֵ.
		while (len) {
			--p; --tmp; --len;
			if (--offset < 0) {
				offset = p % PAGE_SIZE;
				if (from_kmem == 2)								// �������ں˿ռ���fsָ���û��ռ�.
					set_fs(old_fs);
				// �����ǰƫ��ֵp���ڵĴ��ռ�ҳ��ָ��������page[p/PAGE_SIZE] == 0,��ʾ��ʱpָ�������Ŀռ��ڴ�ҳ�滹������,��������һ����
				// �ڴ�ҳ,������ҳ��ָ������ָ������,ͬʱҲʹҳ��ָ��pagָ�����ҳ��.�����벻������ҳ���򷵻�0.
				if (!(pag = (char *) page[p / PAGE_SIZE]) &&
				    !(pag = (char *) (page[p / PAGE_SIZE] =
				      get_free_page())))
					return 0;
				if (from_kmem == 2)								// �������ں˿ռ���fsָ���ں˿ռ�.
					set_fs(new_fs);

			}
			// Ȼ���fs���и����ַ�����1�ֽڵ������ͻ����ռ��ڴ�ҳ��pag��offset��.
			*(pag + offset) = get_fs_byte(tmp);
		}
	}
	// ����ַ������ַ����������ں˿ռ�,��ָ�fs�μĴ���ԭֵ.���,���ز����ͻ����ռ����Ѹ��Ʋ�����ͷ��ƫ��ֵ.
	if (from_kmem == 2)
		set_fs(old_fs);
	return p;
}

// �޸�����ľֲ�������������.
// �޸ľֲ���������LDT���������Ķλ�ַ�Ͷ��޳�,���������ͻ����ռ�ҳ����������ݶ�ĩ��.
// ����:text_size - ִ���ļ�ͷ����a_text�ֶθ����Ĵ���γ���ֵ;page - �����ͻ����ռ�ҳ��ָ������.
// ����:���ݶ��޳�ֵ(64MB)
static unsigned long change_ldt(unsigned long text_size, unsigned long * page)
{
	unsigned long code_limit, data_limit, code_base, data_base;
	int i;

	// ���ȰѴ�������ݶγ��Ⱦ�����Ϊ64MB.Ȼ��ȡ��ǰ���ֲ̾��������������������д���λ�ַ.����λ�ַ�����ݶλ�ַ��ͬ.
	// ��ʹ����Щ��ֵ�������þֲ����д���κ����ݶ��������еĻ�ַ�Ͷ��޳�.������ע��,���ڱ����ص��³���Ĵ�������ݶλ�ַ
	// ��ԭ�������ͬ,���û�б�Ҫ���ظ���������,��186��188���ϵ��������öλ�ַ��������,��ʡ��.
	code_limit = TASK_SIZE;
	data_limit = TASK_SIZE;
	code_base = get_base(current->ldt[1]);
	data_base = code_base;
	set_base(current->ldt[1], code_base);
	set_limit(current->ldt[1], code_limit);
	set_base(current->ldt[2], data_base);
	set_limit(current->ldt[2], data_limit);
	/* make sure fs points to the NEW data segment */
	/* Ҫȷ��fs�μĴ�����ָ���µ����ݶ� */
	// fs�μĴ����з���ֲ������ݶ���������ѡ���(0x17).��Ĭ�������fs��ָ���������ݶ�.
	__asm__("pushl $0x17\n\tpop %%fs"::);
	// Ȼ�󽫲����ͻ����ռ��Ѵ�����ݵ�ҳ��(�����MAX_ARG_PAGESҳ,128KB)�ŵ����ݶ�ĩ��.�����Ǵӽ��̿ռ�����λ�ÿ�ʼ��
	// ����һҳһҳ�ط�.���ļ�����ռ�ý��̿ռ����4MB.����put_dirty_page()���ڰ�����ҳ��ӳ�䵽�����߼��ռ���.��mm/memory.c��.
	data_base += data_limit - LIBRARY_SIZE;
	for (i = MAX_ARG_PAGES - 1 ; i >= 0 ; i--) {
		data_base -= PAGE_SIZE;
		if (page[i])									// ����ҳ�����,�ͷ��ø�ҳ��.
			put_dirty_page(page[i], data_base);
	}
	return data_limit;									// ��󷵻����ݶ��޳�(64MB).
}

/*
 * 'do_execve()' executes a new program.
 *
 * NOTE! We leave 4MB free at the top of the data-area for a loadable
 * library.
 */
/*
 * 'do_execve()'����ִ��һ���³���.
 */
// execve()ϵͳ�жϵ��ú���.���ز�ִ���ӽ���(��������).
// �ú�����ϵͳ�жϵ���(int 0x80)���ܺ�__NR_execve���õĺ���.�����Ĳ����ǽ���ϵͳ���ô�����̺�ֱ�����ñ�ϵͳ���ô�����̺͵��ñ�����֮ǰ��ѹ��ջ�е�ֵ.��Щֵ����:
// 1��86--88����ѵ�edx,ecx��ebx�Ĵ���ֵ,�ֱ��Ӧ**envp,**argv��*filename;
// 2��94�е���sys_call_table��sys_execve����(ָ��)ʱѹ��ջ�ĺ������ص�ַ(tmp);
// 3��202���ڵ��ñ�����do_execveǰ��ջ��ָ��ջ�е���ϵͳ�жϵĳ������ָ��eip.
// ����:
// eip - ����ϵͳ�жϵĳ������ָ��.
// tmp - ϵͳ�ж��ڵ���sys_execveʱ�ķ��ص�ַ,����.
// filename - ��ִ�г����ļ���ָ��;
// argv - �����в���ָ�������ָ��;
// envp - �������ָ�������ָ��.
// ����:������óɹ�,�򲻷���;�������ó����,������-1.
int do_execve(unsigned long * eip, long tmp, char * filename,
	char ** argv, char ** envp)
{
	struct m_inode * inode;
	struct buffer_head * bh;
	struct exec ex;
	unsigned long page[MAX_ARG_PAGES];							// �����ͻ������ռ�ҳ��ָ������.
	int i, argc, envc;
	int e_uid, e_gid;											// ��Ч�û�ID����Ч��ID.
	int retval;
	int sh_bang = 0;											// �����Ƿ���Ҫִ�нű�����.
	unsigned long p = PAGE_SIZE * MAX_ARG_PAGES - 4;			// pָ������ͻ����ռ�����.

	// ���ں��д�ӡҪִ�е��ļ����ļ�����
	char s, filename1[128];
	int index = 0;
	while (1) {
		s = get_fs_byte(filename + index);
		if (s) {
			*(filename1 + index) = s;
			index++;
		} else {
			break;
		}
	}
	*(filename1 + index + 1) = '\0';
	//Log(LOG_INFO_TYPE, "<<<<< process pid = %d do_execve : %s >>>>>\n", current->pid, filename1);

	// ����ʽ����ִ���ļ������л���֮ǰ,�������ȸ�Щ����.�ں�׼����128KB(32��ҳ��)�ռ��������ִ���ļ��������в����ͻ����ַ���.
	// ���а�p��ʼ���ó�λ��128KB�ռ�����1�����ִ�.�ڳ�ʼ�����ͻ����ռ�Ĳ���������,p������ָ����128KB�ռ��еĵ�ǰλ��.
	// ����,����eip[1]�ǵ��ñ���ϵͳ���õ�ԭ�û��������μĴ���CSֵ,���еĶ�ѡ�����Ȼ�����ǵ�ǰ����Ĵ����ѡ���(0x000f).
	// �����Ǹ�ֵ,��ôCSֻ�ܻ����ں˴���ε�ѡ���0x0008.�����Ǿ��Բ������,��Ϊ�ں˴����ǳ�פ�ڴ�����ܱ��滻����.����������
	// eip[1]��ֵȷ���Ƿ�����������.Ȼ���ٳ�ʼ��128KB�Ĳ����ͻ������ռ�,�������ֽ�����,��ȡ��ִ���ļ���i�ڵ�.�ٸ��ݺ�������
	// �ֱ����������в����ͻ����ַ����ĸ���argc��envc.����,ִ���ļ������ǳ����ļ�.
	if ((0xffff & eip[1]) != 0x000f)
		panic("execve called from supervisor mode");
	for (i = 0 ; i < MAX_ARG_PAGES ; i++)						/* clear page-table */
		page[i] = 0;
	if (!(inode = namei(filename)))								/* get executables inode */
		return -ENOENT;
	argc = count(argv);											// �����в�������.
	envc = count(envp);											// �����ַ�����������.

restart_interp:
	if (!S_ISREG(inode->i_mode)) {								/* must be regular file */
		retval = -EACCES;
		goto exec_error2;										// �����ǳ����ļ����ó�����,��ת��376��.
	}
	// �����鵱ǰ�����Ƿ���Ȩ����ָ����ִ���ļ�.������ִ���ļ�i�ڵ��е�����,�����������Ƿ���Ȩִ����.�ڰ�ִ���ļ�i�ڵ������
	// �ֶ�ֵȡ��i�к�,�������Ȳ鿴�������Ƿ�������"����-�û�-ID"(set-user-ID)��־��"����-��-ID)(set-group-id)��־.����
	// ����־��Ҫ����һ���û��ܹ�ִ����Ȩ�û�(�糬���û�root)�ĳ���,����ı�����ĳ���passwd��.���set-user-ID��־��λ,��
	// ����ִ�н��̵���Ч�û�ID(euid)�����ó�ִ���ļ����û�ID,�������óɵ�ǰ���̵�euid.���ִ���ļ�set-group-id����λ�Ļ�,
	// ��ִ�н��̵���Ч��ID(egid)������Ϊִ��ִ���ļ�����ID.�������óɵ�ǰ���̵�egid.�����ݰ��������жϳ�����ֵ�����ڱ���
	// e_uid��e_gid��.
	i = inode->i_mode;
	e_uid = (i & S_ISUID) ? inode->i_uid : current->euid;
	e_gid = (i & S_ISGID) ? inode->i_gid : current->egid;
	// ���ڸ��ݽ��̵�euid��egid��ִ���ļ��ķ������Խ��бȽ�.���ִ���ļ��������н��̵��û�,����ļ�����ֵi����6λ,��ʱ�����3
	// λ���ļ������ķ���Ȩ�ޱ�־.����Ļ����ִ���ļ��뵱ǰ���̵��û�����ͬ��,��ʹ�������3λ��ִ���ļ����û��ķ���Ȩ�ޱ�־.����
	// ��ʱ���������3λ���������û����ʸ�ִ���ļ���Ȩ��.Ȼ�����Ǹ���������i�����3λֵ���жϵ�ǰ�����Ƿ���Ȩ���������ִ���ļ�.
	// ���ѡ������Ӧ�û�û�����и��ļ���Ȩ��(λ0��ִ��Ȩ��),���������û�Ҳû���κ�Ȩ�޻��ߵ�ǰ�����û����ǳ����û�,�������ǰ��
	// ��û��Ȩ���������ִ���ļ�.�����ò���ִ�г�����,����ת��exec_error2��ȥ���˳�����.
	if (current->euid == inode->i_uid)
		i >>= 6;
	else if (in_group_p(inode->i_gid))
		i >>= 3;
	if (!(i & 1) &&
	    !((inode->i_mode & 0111) && suser())) {
		retval = -ENOEXEC;
		goto exec_error2;
	}
	// ����ִ�е�����,˵����ǰ����������ָ��ִ���ļ���Ȩ��.��˴����￪ʼ������Ҫȡ��ִ���ļ�ͷ�����ݲ��������е���Ϣ��������������
	// ����,����������һ��shell������ִ�нű�����.���ȶ�ȡִ���ļ���1�����ݵ����ٻ������.�����ƻ�������ݵ�ex��.���ִ���ļ���ʼ
	// �������ֽ����ַ�"#!",��˵��ִ���ļ���һ���ű��ļ�.��������нű��ļ�,���Ǿ���Ҫִ�нű��ļ��Ľ��ͳ���(����shell����).ͨ��
	// �ű��ļ��ĵ�һ���ı�Ϊ"#! /bin/bash".��ָ�������нű��ļ���Ҫ�Ľ��ͳ���.���з����Ǵӽű��ļ���1��(���ַ�"#!")��ȡ������
	// �Ľ��ͳ�����������Ĳ���(���еĻ�),Ȼ����Щ�����ͽű��ļ����Ž�ִ���ļ�(��ʱ�ǽ��ͳ���)�������в����ռ���.����֮ǰ���ǵ�Ȼ
	// ��Ҫ�ȰѺ���ָ����ԭ�������в����ͻ����ַ����ŵ�128KB�ռ���,�����ｨ�������������в�����ŵ�����ǰ��λ�ô�(��Ϊ���������).
	// ������ں�ִ�нű��ļ��Ľ��ͳ���.������������úý��ͳ���Ľű��ļ����Ȳ�����,ȡ�����ͳ����i�ڵ㲢��ת��229��ִ֧�н��ͳ���.
	// ����������Ҫ��ת��ִ�й��Ĵ���229��֧.���������ȷ�ϲ������˽ű��ļ�֮����Ҫ����һ����ֹ�ٴ�ִ������Ľű���������־sh_bang.
	// �ں���Ĵ����иñ�־Ҳ������ʾ�����Ѿ����ú�ִ���ļ��������в���,��Ҫ�ظ�����.
	if (!(bh = bread(inode->i_dev, inode->i_zone[0]))) {
		retval = -EACCES;
		goto exec_error2;
	}
	ex = *((struct exec *) bh->b_data);								/* read exec-header */
	if ((bh->b_data[0] == '#') && (bh->b_data[1] == '!') && (!sh_bang)) {
		/*
		 * This section does the #! interpretation.
		 * Sorta complicated, but hopefully it will work.  -TYT
		 */
        /*
         * �ⲿ�ִ���ԡ�#!���Ľ��ͣ���Щ���ӣ���ϣ���ܹ����� -TYT
         */

		char buf[128], *cp, *interp, *i_name, *i_arg;
		unsigned long old_fs;

		// �����￪ʼ�����Ǵӽű��ļ�����ȡ���ͳ�����������������ѽ��ͳ����������ͳ���Ĳ����ͽű��ļ�����Ϸ��뻷���������С����ȸ��ƽű�
		// �ļ�ͷ1���ַ���#!��������ַ�����buf�У����к��нű����ͳ�����������/bin/sh����Ҳ���ܻ��������ͳ���ļ���������Ȼ���buf�е�����
		// ���д���ɾ����ʼ�Ŀո��Ʊ����
		strncpy(buf, bh->b_data + 2, 127);
		brelse(bh);             									// �ͷŻ���鲢�Żؽű��ļ�i�ڵ㡣
		iput(inode);
		buf[127] = '\0';
		if (cp = strchr(buf, '\n')) {
			*cp = '\0';     										// ��1�����з�����NULL��ȥ����ͷ�ո��Ʊ����
			for (cp = buf; (*cp == ' ') || (*cp == '\t'); cp++);
		}
		if (!cp || *cp == '\0') {       							// ������û���������ݣ������
			retval = -ENOEXEC; 										/* No interpreter name found */
			goto exec_error1;       								/* û���ҵ��ű����ͳ����� */
		}
		// ��ʱ���ǵõ��˿�ͷ�ǽű����ͳ�������һ�����ݣ��ַ�����������������С�����ȡ��һ���ַ�������Ӧ���ǽ��ͳ���������ʱi_nameָ���
		// ���ơ������ͳ����������ַ���������Ӧ���ǽ��ͳ���Ĳ�������������i_argָ��ô���
		interp = i_name = cp;
		i_arg = 0;
		for ( ; *cp && (*cp != ' ') && (*cp != '\t'); cp++) {
 			if (*cp == '/')
				i_name = cp+1;
		}
		if (*cp) {
			*cp++ = '\0';           								// ���ͳ�����β���NULL�ַ���
			i_arg = cp;             								// i_argָ����ͳ��������
		}
		/*
		 * OK, we've parsed out the interpreter name and
		 * (optional) argument.
		 */
        /*
         * OK�������Ѿ����������ͳ�����ļ����Լ�����ѡ�ģ�������
         */
		// ��������Ҫ��������������Ľ��ͳ�����i_name�������i_arg�ͽű��ļ�����Ϊ���ͳ���Ĳ����Ž������Ͳ������С���������
		// ������Ҫ�Ѻ����ṩ��ԭ��һЩ�����ͻ����ַ����ȷŽ�ȥ��Ȼ���ٷ�������������ġ�������������в�����˵�����ԭ���Ĳ���
		// �ǡ�-arg1 -arg2�������ͳ������ǡ�bash����������ǡ�-iarg1 -iarg2�����ű��ļ�������ԭ����ִ���ļ������ǡ�example.sh��
		// ��ô�ڷ�������Ĳ���֮���µ�������������������
		//              ��bash -iarg1 -iarg2 example.sh -arg1 -arg2��
		// �������ǰ�sh_bang��־���ϣ�Ȼ��Ѻ��������ṩ��ԭ�в����ͻ����ַ������뵽�ռ��С������ַ����Ͳ��������ֱ���envc��
		// argc-1�����ٸ��Ƶ�һ��ԭ�в�����ԭ����ִ���ļ�����������Ľű��ļ�����[[?? ���Կ�����ʵ�������ǲ���Ҫȥ���д���ű�
		// �ļ�������������ȫ���Ը���argc������������ԭ��ִ���ļ����������ڵĽű��ļ���������Ϊ��λ��ͬһ��λ����]]��ע�⣡����
		// ָ��p���Ÿ�����Ϣ���Ӷ�����С��ַ�����ƶ���������������ƴ�����ִ����󣬻�����������Ϣ��λ�ڳ��������в�������Ϣ��
		// ���Ϸ�������pָ�����ĵ�1����������copy_strings()���һ��������0��ָ�������ַ������û��ռ䡣
		if (sh_bang++ == 0) {
			p = copy_strings(envc, envp, page, p, 0);
			p = copy_strings(--argc, argv + 1, page, p, 0);
		}
		/*
		 * Splice in (1) the interpreter's name for argv[0]
		 *           (2) (optional) argument to interpreter
		 *           (3) filename of shell script
		 *
		 * This is done in reverse order, because of how the
		 * user environment and arguments are stored.
		 */
        /*
         * ƴ�ӣ�1��argv[0]�зŽ��ͳ�������ơ�
         *    ��2������ѡ�ģ����ͳ���Ĳ�����
         *    ��3���ű���������ơ�
         *
         * ������������̴���ģ��������û������Ͳ����Ĵ�ŷ�ʽ��ɵġ�
         */
		// �������������ƽű��ļ��������ͳ���Ĳ����ͽ��ͳ����ļ����������ͻ����ռ��С����������ó�������ת��exec_error1��
		// ���⣬���ڱ����������ṩ�Ľű��ļ���filename���û��ռ䣬�����︳��copy_strings()�Ľű��ļ�����ָ�����ں˿ռ䣬���
		// ��������ַ������������һ���������ַ�����Դ��־����Ҫ�����ó�1�����ַ������ں˿ռ䣬��copy_strings()�����һ������
		// Ҫ���óɹ��������档
		p = copy_strings(1, &filename, page, p, 1);
		argc++;
		if (i_arg) {            									// ���ƽ��ͳ���Ķ��������
			p = copy_strings(1, &i_arg, page, p, 2);
			argc++;
		}
		p = copy_strings(1, &i_name, page, p, 2);
		argc++;
		if (!p) {
			retval = -ENOMEM;
			goto exec_error1;
		}
		/*
		 * OK, now restart the process with the interpreter's inode.
		 */
	    /*
	     * OK������ʹ�ý��ͳ����i�ڵ��������̡�
	     */
		// �������ȡ�ý��ͳ����i�ڵ�ָ�룬Ȼ����ת��204��ȥִ�н��ͳ���Ϊ�˻�ý��ͳ����i�ڵ㣬������Ҫʹ��namei()���������Ǹú���
		// ��ʹ�õĲ������ļ������Ǵ��û����ݿռ�õ��ģ����ӶμĴ���fs��ָ�ռ���ȡ�á�����ڵ���namei()����֮ǰ������Ҫ����ʱ��fsָ����
		// �����ݿռ䣬���ú����ܴ��ں˿ռ�õ����ͳ�����������namei()���غ�ָ�fs��Ĭ�����á����������������ʱ����ԭfs�μĴ�����ԭָ��
		// �û����ݶΣ���ֵ���������ó�ָ���ں����ݶΣ�Ȼ��ȡ���ͳ����i�ڵ㡣֮���ٻָ�fs��ԭֵ������ת��restart_interp��204�У�������
		// �����µ�ִ���ļ� -- �ű��ļ��Ľ��ͳ���
		old_fs = get_fs();
		set_fs(get_ds());
		if (!(inode = namei(interp))) { 						/* get executables inode */
			set_fs(old_fs);       								/* ȡ�ý��ͳ����i�ڵ� */
			retval = -ENOENT;
			goto exec_error1;
		}
		set_fs(old_fs);
		goto restart_interp;
    }
	// ��ʱ������е�ִ���ļ�ͷ�ṹ�Ѿ����Ƶ���ex��.�������ͷŸû����,����ʼ��ex�е�ִ��ͷ��Ϣ�����жϴ���.����Linux0.12�ں���˵,
	// ����֧��ZMAGICִ�и�ʽ,����ִ���ļ����붼���߼���ַ0��ʼִ��,��˲�֧�ֺ��д���������ض�λ��Ϣ��ִ���ļ�.��Ȼ,���ִ���ļ�
	// ʵ��̫�����ִ���ļ���ȱ��ȫ,��ô����Ҳ����������.��˶��������������ִ�г���:���ִ���ļ���������ҳ��ִ���ļ�(ZMAGIC),����
	// ����������ض�λ���ֳ��Ȳ�����0,����(�����+���ݶ�+��)���ȳ���50MB,����ִ���ļ�����С��(�����+���ݶ�+���ű���+ִ��ͷ����)
	// ���ȵ��ܺ�.
	brelse(bh);
	if (N_MAGIC(ex) != ZMAGIC || ex.a_trsize || ex.a_drsize ||
		ex.a_text + ex.a_data + ex.a_bss > 0x3000000 ||
		inode->i_size < ex.a_text + ex.a_data + ex.a_syms + N_TXTOFF(ex)) {
		retval = -ENOEXEC;
		goto exec_error2;
	}
	// ����,���ִ���ļ��д��뿪ʼ��û��λ��1��ҳ��(1024�ֽ�)�߽紦,��Ҳ����ִ��.��Ϊ����ҳ(Demand paging)����Ҫ�����ִ���ļ�����
	// ʱ��ҳ��Ϊ��λ,���Ҫ��ִ���ļ�ӳ���д�������ݶ���ҳ��߽紦��ʼ.
	if (N_TXTOFF(ex) != BLOCK_SIZE) {
		printk("%s: N_TXTOFF != BLOCK_SIZE. See a.out.h.", filename);
		retval = -ENOEXEC;
		goto exec_error2;
	}
	// ���sh_bang��־û������,����ָ�������������в����ͻ����ַ����������ͻ����ռ���.��sh_bang��־�Ѿ�����,����������нű����ͳ���,
	// ��ʱһ��������ҳ���Ѿ�����,�����ٸ���.ͬ��,��sh_bangû����λ����Ҫ���ƵĻ�,��ô��ʱָ��p���Ÿ�����Ϣ���Ӷ�����С��ַ�����ƶ�,
	// ������������ƴ�����ִ�����,������������Ϣ��λ�ڳ����������Ϣ����Ϸ�,����pָ�����ĵ�1��������.��ʵ��,p��128KB�����ͻ����ռ�
	// �е�ƫ��ֵ.������p=0,���ʾ��������������ռ�ҳ���Ѿ���ռ��,���ɲ�����.
	if (!sh_bang) {
		p = copy_strings(envc, envp, page, p, 0);
		p = copy_strings(argc, argv, page, p, 0);
		if (!p) {
			retval = -ENOMEM;
			goto exec_error2;
		}
	}
	/* OK, This is the point of no return */
	/* note that current->library stays unchanged by an exec */
	/* OK,���濪ʼ��û�з��صĵط��� */
	// ǰ��������Ժ��������ṩ����Ϣ����Ҫ����ִ���ļ��������кͻ����ռ����������,����û��Ϊִ���ļ�����ʲôʵ���ԵĹ���,����û������Ϊ
	// ִ���ļ���ʼ����������ṹ��Ϣ,����ҳ��ȹ���.�������Ǿ�������Щ����.����ִ���ļ�ֱ��ʹ�õ�ǰ���̵�"����",����ǰ���̽��������ִ��
	// �ļ��Ľ���,���������Ҫ�����ͷŵ�ǰ����ռ�õ�ĳЩϵͳ��Դ,�����ر�ָ�����Ѵ��ļ�,ռ�õ�ҳ����ڴ�ҳ���.Ȼ�����ִ���ļ�ͷ�ṹ��
	// Ϣ�޸ĵ�ǰ����ʹ�õľֲ���������LDT��������������,�������ô���κ����ݶ����������޳�,�����õ�ǰ����õ���e_uid��e_gid����Ϣ������
	// ��������ṹ����ص��ֶ�.����ִ�б���ϵͳ���ó���ķ��ص�ַeip[]ָ��ִ���ļ��д������ʼλ�ô�.������ϵͳ�����˳����غ�ͻ�ȥ��
	// ����ִ���ļ��Ĵ�����.ע��,��Ȼ��ʱ��ִ���ļ���������ݻ�û�д��ļ��м��ص��ڴ���,��������ͻ����Ѿ���copy_strings()��ʹ��
	// get_free_page()�����������ڴ�ҳ����������,����chang_ldt()������ʹ��put_page()���˽����߼��ռ��ĩ�˴�.����,��create_tables()
	// ��Ҳ���������û�ջ�ϴ�Ų����ͻ���ָ��������ȱҳ�쳣,�Ӷ��ڴ�������Ҳ��ʹ�Ϊ�û�ջ�ռ�ӳ�������ڴ�ҳ.
	//
	// �����������ȷŻؽ���ԭִ�г����i�ڵ�,�����ý���executable�ֶ�ָ����ִ���ļ���i�ڵ�.Ȼ��λԭ���̵������źŴ�����,������SIG_IGN
	// ������븴λ.
	if (current->executable)
		iput(current->executable);
	current->executable = inode;
	current->signal = 0;
	for (i = 0 ; i < 32 ; i++) {
		current->sigaction[i].sa_mask = 0;
		current->sigaction[i].sa_flags = 0;
		if (current->sigaction[i].sa_handler != SIG_IGN)
			current->sigaction[i].sa_handler = NULL;
	}
	// �ٸ����趨��ִ��ʱ�ر��ļ����(close_on_exec)λͼ��־,�ر�ָ���Ĵ��ļ�����λ�ñ�־
	for (i = 0 ; i < NR_OPEN ; i++)
		if ((current->close_on_exec >> i) & 1)
			sys_close(i);
	current->close_on_exec = 0;
	// Ȼ����ݵ�ǰ����ָ���Ļ���ַ���޳�,�ͷ�ԭ������Ĵ���κ����ݶ�����Ӧ���ڴ�ҳ��ָ���������ڴ�ҳ�漰ҳ����.��ʱ��ִ���ļ���û��ռ����
	// �ڴ����κ�ҳ��,����ڴ���������������ִ���ļ�����ʱ�ͻ�����ȱҳ�쳣�ж�,��ʱ�ڴ������򼴻�ִ��ȱҳ����ҳΪ��ִ���ļ������ڴ�ҳ���
	// �������ҳ����,���Ұ����ִ���ļ�ҳ������ڴ���.���"�ϴ�����ʹ����Э������"ָ����ǵ�ǰ����,�����ÿ�,����λʹ����Э�������ı�־.
	free_page_tables(get_base(current->ldt[1]), get_limit(0x0f));
	free_page_tables(get_base(current->ldt[2]), get_limit(0x17));
	if (last_task_used_math == current)
		last_task_used_math = NULL;
	current->used_math = 0;
	// Ȼ�����Ǹ�����ִ���ļ�ͷ�ṹ�еĴ��볤���ֶ�a_text��ֵ�޸ľֲ�������������ַ�Ͷ��޳�,����128KB�Ĳ����ͻ����ռ�ҳ����������ݶ�ĩ��.
	// ִ���������֮��,p��ʱ���ĳ������ݶ���ʼ��Ϊԭ���ƫ��ֵ,����ָ������ͻ����ռ����ݿ�ʼ��,����ת����ջָ��ֵ.Ȼ������ڲ�����create_tables()
	// ��ջ�ռ��д��������Ͳ�������ָ���,�������main()��Ϊ����ʹ��,�����ظ�ջָ��.
	p += change_ldt(ex.a_text, page);
	p -= LIBRARY_SIZE + MAX_ARG_PAGES * PAGE_SIZE;
	p = (unsigned long) create_tables((char *)p, argc, envc);
	// �������޸Ľ��̸��ֶ�ֵΪ��ִ���ļ�����Ϣ.�����������ṹ����β�ֶ�end_code����ִ���ļ��Ĵ���γ���a_text;����β�ֶ�end_data����
	// ִ���ļ��Ĵ���γ��ȼ����ݶγ���(a_data + a_text);������̶ѽ�β�ֶ�brk = a_text + a_data + a_bss.brk����ָ�����̵�ǰ���ݶ�
	// (����δ��ʼ�����ݲ���)ĩ��λ��,���ں�Ϊ���̷����ڴ�ʱָ�����俪ʼλ��.Ȼ�����ý���ջ��ʼ�ֶ�Ϊջָ������ҳ��,���������ý��̵���Ч�û�
	// id����Ч��id.
	current->brk = ex.a_bss +
		(current->end_data = ex.a_data +
		(current->end_code = ex.a_text));
	current->start_stack = p & 0xfffff000;
	current->suid = current->euid = e_uid;
	current->sgid = current->egid = e_gid;
	// ���ԭ����ϵͳ�жϵĳ����ڶ�ջ�ϵĴ���ָ���滻Ϊָ����ִ�г������ڵ�,����ջָ���滻Ϊ��ִ���ļ���ջָ��.�˺󷵻�ָ�������Щջ��
	// �ݲ�ʹ��CPUȥִ����ִ���ļ�,��˲��᷵�ص�ԭ����ϵͳ�жϵĳ�����ȥ��.
	eip[0] = ex.a_entry;												/* eip, magic happens :-) */	/* eip,ħ���������� */
	eip[3] = p;															/* stack pointer */		/* esp,��ջָ�� */
	return 0;
exec_error2:
	iput(inode);														// �Ż�i�ڵ�.
exec_error1:
	for (i = 0 ; i < MAX_ARG_PAGES ; i++)
		free_page(page[i]);												// �ͷŴ�Ų����ͻ��������ڴ�ҳ��.
	return(retval);														// ���س�����.
}

