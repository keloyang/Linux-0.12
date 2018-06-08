/*
 *  linux/fs/namei.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * Some corrections by tytso.
 */

#include <linux/sched.h>										// ���Գ���ͷ�ļ�,����������ṹtask_struct,0�����ݵ�.
//#include <linux/kernel.h>
#include <asm/segment.h>										// �β���ͷ�ļ�.�������йضμĴ���������Ƕ��ʽ��ຯ��.

#include <string.h>
#include <fcntl.h>												// �ļ�����ͷ�ļ�.�ļ������������Ĳ������Ƴ������ŵĶ���.
#include <errno.h>												// �����ͷ�ļ�.����ϵͳ�и��ֳ����.
#include <const.h>												// ��������ͷ�ļ�.Ŀǰ������i�ڵ���i_mode�ֶεĸ���־λ.
#include <sys/stat.h>											// �ļ�״̬ͷ�ļ�.�����ļ����ļ�ϵͳ״̬�ṹstat()�ͳ���.

// ���ļ������Ҷ�Ӧi�ڵ���ڲ�����.
static struct m_inode * _namei(const char * filename, struct m_inode * base, int follow_links);

// ��������Ҳ���ʽ�Ƿ��������һ������ʹ�÷���.������������һ����ʵ,�����������������±�����ʾ��������(����a[b])��ֵ��ͬ��ʹ��������ָ��(��ַ)
// ���ϸ���ƫ�Ƶ�ַ����ʽ��ֵ*(a+b),ͬʱ��֪��a[b]Ҳ���Ա�ʾ��b[a]����ʽ.��˶����ַ���������ʽΪ"LoveYou"[2](����2["LoveYou"])�͵�ͬ��*("LoveYou" + 2)
// ����,�ַ���"LoveYou"���ڴ��б��洢��λ�þ������ַ,���������"LoveYou"[2]��ֵ���Ǹ��ַ�������ֵΪ2���ַ�"v"����Ӧ��ASCII��ֵ0x76,���ð˽���
// ��ʾ����0166.��C������,�ַ�Ҳ��������ASCII��ֵ����ʾ,���������ַ���ASCII��ֵǰ���һ����б��.�����ַ�"v"���Ա�ʾ��"\x76"����"\166".���
// ���ڲ�����ʾ���ַ�(����ASCII��ֵΪ0x00--0x1f�Ŀ����ַ�)�Ϳ�����ASCII��ֵ����ʾ.
//
// �����Ƿ���ģʽ��.x��ͷ�ļ�include/fcntl.h����7�п�ʼ������ļ�����(��)��־.���������ļ����ʱ�־x��ֵ������˫�����ж�Ӧ����ֵ.˫������
// ��4���˽�����ֵ(ʵ�ʱ�ʾ4�������ַ�):"\004\002\006\377",�ֱ��ʾ��,д��ִ�е�Ȩ��Ϊ:r,w,rw��wxrwxrwx,���ҷֱ��Ӧx������ֵ0--3.����,���xΪ2,
// ��ú귵�ذ˽���ֵ006,��ʾ�ɶ���д(rw).����,����O_ACCMODE = 00003,������ֵx��������.
#define ACC_MODE(x) ("\004\002\006\377"[(x) & O_ACCMODE])

/*
 * comment out this line if you want names > NAME_LEN chars to be
 * truncated. Else they will be disallowed.
 */
/*
 * ��������ļ������� > NAME_LEN�����ַ����ص�,�ͽ����涨��ע�͵�.
 */
/* #define NO_TRUNCATE */

#define MAY_EXEC 			1	// ��ִ��(�ɽ���).
#define MAY_WRITE 			2	// ��д.
#define MAY_READ 			4	// �ɶ�.

/*
 *	permission()
 *
 * is used to check for read/write/execute permissions on a file.
 * I don't know if we should look at just the euid or both euid and
 * uid, but that should be easily changed.
 */
/*
 *	permission()
 *
 * �ú������ڼ��һ���ļ��Ķ�/д/ִ��Ȩ��.�Ҳ�֪���Ƿ�ֻ����euid,������Ҫ���euid��uid����,������������޸�.
 */
// ����ļ��������Ȩ��.
// ����:inode - �ļ���i�ڵ�ָ��;mask - ��������������.
// ����:������ɷ���1,���򷵻�0.
static int permission(struct m_inode * inode, int mask)
{
	int mode = inode->i_mode;								// �ļ���������.

	/* special case: not even root can read/write a deleted file */
	/* �������:��ʹ�ǳ����û�(root)Ҳ���ܶ�/дһ���ѱ�ɾ�����ļ�. */
	// ���i�ڵ��ж�Ӧ���豸,����i�ڵ�����Ӽ���ֵ����0,��ʾ���ļ��ѱ�ɾ��,�򷵻�.
	if (inode->i_dev && !inode->i_nlinks)
		return 0;
	// ������̵���Ч�û�id(euid)��i�ڵ���û�id��ͬ,��ȡ�ļ������ķ���Ȩ��
	else if (current->euid == inode->i_uid)
		mode >>= 6;
	// ���������Ч��id(egid)��i�ڵ����id��ͬ,��ȡ���û��ķ���Ȩ��
	else if (in_group_p(inode->i_gid))
		mode >>= 3;
	// ����ж������ȡ�ĵķ���Ȩ������������ͬ,�����ǳ����û�,�򷵻�1,���򷵻�0.
	if (((mode & mask & 0007) == mask) || suser())
		return 1;
	return 0;
}

/*
 * ok, we cannot use strncmp, as the name is not in our data space.
 * Thus we'll have to use match. No big problem. Match also makes
 * some sanity tests.
 *
 * NOTE! unlike strncmp, match returns 1 for success, 0 for failure.
 */
/*
 * ok,���ǲ���ʹ��strncmp�ַ����ȽϺ���,��Ϊ���Ʋ������ǵ����ݿռ�(�����ں˿ռ�).�������ֻ��ʹ��match().���ⲻ��,match()ͬ��Ҳ����һЩ
 * �����Ĳ���.
 *
 * ע��!��strncmp��ͬ����match()�ɹ�ʱ����1,ʧ��ʱ����0.
 */
// ָ�������ַ����ȽϺ���.
// ����:len - �Ƚϵ��ַ�������;name - �ļ���ָ��;de - Ŀ¼��ṹ.
// ����:��ͬ����1,��ͬ����0.
static int match(int len, const char * name, struct dir_entry * de)
{
	register int same __asm__("ax");

	// �����жϺ�����������Ч��.���Ŀ¼��ָ���,����Ŀ¼��i�ڵ����0,����Ҫ�Ƚϵ��ַ������ȳ����ļ�������,�򷵻�0(��ƥ��).
	if (!de || !de->inode || len > NAME_LEN)
		return 0;
	/* "" means "." ---> so paths like "/usr/lib//libc.a" work */
    /* ""����"."������ ---> �������ܴ�����"/usr/lib//libc.a"������·���� */
    // ����Ƚϵĳ���len����0����Ŀ¼�����ļ����ĵ�1���ַ���'.',����ֻ����ôһ���ַ�,��ô���Ǿ���Ϊ����ͬ��,��˷���1(ƥ��)
	if (!len && (de->name[0] == '.') && (de->name[1] == '\0'))
		return 1;
	// ���Ҫ�Ƚϵĳ���lenС��NAME_LEN,����Ŀ¼�����ļ������ȳ���len,��Ҳ����0(��ƥ��)
	// ��75���϶�Ŀ¼�����ļ��������Ƿ񳬹�len���жϷ����Ǽ��name[len]�Ƿ�ΪNULL.�����ȳ���len,��name[len]������һ������NULL����ͨ�ַ�.�����ڳ���
	// Ϊlen���ַ���name,�ַ�name[len]��Ӧ����NULL.
	if (len < NAME_LEN && de->name[len])
		return 0;
	// Ȼ��ʹ��Ƕ���������п��ٱȽϲ���.�������û����ݿռ�(fs��)ִ���ַ����ıȽϲ���.%0 - eax(�ȽϽ��same);%1 - eax(eax��ֵ0);%2 - esi(����ָ��);
	// %3 - edi(Ŀ¼����ָ��);%4 - ecs(�Ƚϵ��ֽڳ���ֵlen).
	__asm__(\
		"cld\n\t"							// �巽���־λ.
		"fs ; repe ; cmpsb\n\t"				// �û��ռ�ִ��ѭ���Ƚ�[esi++]��[edi++]����.
		"setz %%al"							// ���ȽϽ��һ��(zf=0)����al=1(same=eax).
		:"=a" (same)
		:"0" (0), "S" ((long) name), "D" ((long) de->name), "c" (len)
		:);
	return same;							// ���رȽϽ��.
}

/*
 *	find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 *
 * This also takes care of the few special cases due to '..'-traversal
 * over a pseudo-root and a mount point.
 */
/*
 *	find_entry()
 *
 * ��ָ��Ŀ¼��Ѱ��һ��������ƥ���Ŀ¼��.����һ�������ҵ�Ŀ¼��ĸ��ٻ�����Լ�Ŀ¼���(��Ϊһ������--res_dir).�ú���������ȡĿ¼
 * ���i�ڵ�--�����Ҫ�Ļ����Լ�����.
 *
 * ������'..'Ŀ¼��,����ڲ����ڼ�Ҳ��Լ�����������ֱ���--�����Խһ��α��Ŀ¼�Լ���װ��.
 */
// ����ָ��Ŀ¼���ļ�����Ŀ¼��.
// ����:*dir - ָ��Ŀ¼i�ڵ��ָ��;name - �ļ���;namelen - �ļ�������;�ú�����ָ��Ŀ¼������(�ļ�)������ָ���ļ�����Ŀ¼��.����ָ��
// �ļ�����'..'��������ݵ�ǰ���е�������ý������⴦��.
// ����:�ɹ��򷵻ظ��ٻ�����ָ��,����*res_dir�����ص�Ŀ¼��ṹָ��.ʧ���򷵻ؿ�ָ��NULL.
static struct buffer_head * find_entry(struct m_inode ** dir,
	const char * name, int namelen, struct dir_entry ** res_dir)
{
	int entries;
	int block,i;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;

	// ͬ��,������һ��ʼҲ��Ҫ�Ժ�����������Ч�Խ����жϺ���֤.���������ǰ���30�ж����˷��ų���NO_TRUNCATE,��ô����ļ������ȳ�����󳤶�NAME_LEN,
	// ���账��.���û�ж����NO_TRUNCATE,��ô���ļ������ȳ�����󳤶�NAME_LENʱ�ض�֮.
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
	// ���ȼ��㱾Ŀ¼��Ŀ¼������entries.Ŀ¼i�ڵ�i_size�ֶ��к��б�Ŀ¼���������ݳ���,��������һ��Ŀ¼��ĳ���(16�ֽ�)���ɵõ���Ŀ¼��Ŀ¼����.Ȼ��
	// �ÿշ���Ŀ¼��ṹָ��.
	entries = (*dir)->i_size / (sizeof (struct dir_entry));
	*res_dir = NULL;
	// ���������Ƕ�Ŀ¼���ļ�����'..'������������⴦��.�����ǰ����ָ���ĸ�i�ڵ���Ǻ�������ָ����Ŀ¼,��˵�����ڱ�������˵,���Ŀ¼��������α��Ŀ¼,������
	// ֻ�ܷ��ʸ�Ŀ¼�е���������˵��丸Ŀ¼��ȥ.Ҳ�����ڸý��̱�Ŀ¼����ͬ���ļ�ϵͳ�ĸ�Ŀ¼.���������Ҫ���ļ����޸�Ϊ'.'.
	// ����,�����Ŀ¼��i�ڵ�ŵ���ROOT_INO(1��)�Ļ�,˵��ȷʵ���ļ�ϵͳ�ĸ�i�ڵ�.��ȡ�ļ�ϵͳ�ĳ�����.�������װ����i�ڵ����,���ȷŻ�ԭi�ڵ�,Ȼ��Ա�
	// ��װ����i�ڵ���д���.����������*dirָ��ñ���װ����i�ڵ�;���Ҹ�i�ڵ����������1.������������,�������Ľ�����"͵������"����:)
	/* check for '..', as we might have to do some "magic" for it */
	/* ���Ŀ¼��'..',��Ϊ���ǿ�����Ҫ����������⴦�� */
	if (namelen == 2 && get_fs_byte(name) == '.' && get_fs_byte(name + 1) == '.') {
		/* '..' in a pseudo-root results in a faked '.' (just change namelen) */
		/* α���е�'..'��ͬһ����'.'(ֻ��ı����ֳ���) */
		if ((*dir) == current->root)
			namelen = 1;
		else if ((*dir)->i_num == ROOT_INO) {
			/* '..' over a mount-point results in 'dir' being exchanged for the mounted
			   directory-inode. NOTE! We set mounted, so that we can iput the new dir */
			/* ��һ����װ���ϵ�'..'������Ŀ¼����������װ�ļ�ϵͳ��Ŀ¼i�ڵ���.ע��! ��������������mounted��־,��������ܹ��Żظ���Ŀ¼ */
			sb = get_super((*dir)->i_dev);
			if (sb->s_imount) {
				iput(*dir);
				(*dir)=sb->s_imount;
				(*dir)->i_count++;
			}
		}
	}
	// �������ǿ�ʼ��������������ָ���ļ�����Ŀ¼����ʲô�ط������������Ҫ��ȡĿ¼�����ݣ���ȡ��Ŀ¼i�ڵ��Ӧ���豸�������е����ݿ飨�߼��飩��Ϣ����Щ�߼����
	// ��ű�����i�ڵ�ṹ��i_zone[9]������.������ȡ���е�1�����.���Ŀ¼i�ڵ�ָ��ĵ�һ��ֱ���̿��Ϊ0,��˵����Ŀ¼��Ȼ��������,�ⲻ����.���Ƿ���NULL�˳�.
	// �������Ǿʹӽڵ������豸��ȡָ����Ŀ¼�����ݿ�.��Ȼ,������ɹ�,��Ҳ����NULL�˳�.
	if (!(block = (*dir)->i_zone[0]))
		return NULL;
	if (!(bh = bread((*dir)->i_dev, block)))
		return NULL;
	// ��ʱ���Ǿ��������ȡ��Ŀ¼i�ڵ����ݿ�������ƥ��ָ���ļ�����Ŀ¼��.������deָ�򻺳���е����ݿ鲿��,���ڲ�����Ŀ¼�����ݵ�������,ѭ��ִ������.����i��Ŀ¼
	// �е�Ŀ¼��������,��ѭ����ʼʱ��ʼ��Ϊ0.
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (i < entries) {
		// �����ǰĿ¼�����ݿ��Ѿ�������,��û���ҵ�ƥ���Ŀ¼��,���ͷŵ�ǰĿ¼�����ݿ�.�ٶ���Ŀ¼����һ���߼���.�����Ϊ��,��ֻҪ��û��������Ŀ¼�е�����Ŀ¼��,��
		// �����ÿ�,������Ŀ¼����һ�߼���.���ÿ鲻��,����deָ������ݿ�,Ȼ�������м�������.����141����i/DIR_ENTRIES_PER_BLOCK�ɵõ���ǰ������Ŀ¼������Ŀ¼�ļ��е�
		// ���,��bmap()����(inode.c)��ɼ�������豸�϶�Ӧ���߼����.
		if ((char *)de >= BLOCK_SIZE + bh->b_data) {
			brelse(bh);
			bh = NULL;
			if (!(block = bmap(*dir, i / DIR_ENTRIES_PER_BLOCK)) ||
			    !(bh = bread((*dir)->i_dev, block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// ����ҵ�ƥ���Ŀ¼��Ļ�,�򷵻�Ŀ¼��ṹָ��de�͸�Ŀ¼��i�ڵ�ָ��*dir�Լ���Ŀ¼�����ݿ�ָ��bh,���˳�����.���������Ŀ¼�����ݿ��бȽ���һ��Ŀ¼��.
		if (match(namelen, name, de)) {
			*res_dir = de;
			return bh;
		}
		de++;
		i++;
	}
	// ���ָ��Ŀ¼�е�����Ŀ¼���������,��û���ҵ���Ӧ��Ŀ¼��,���ͷ�Ŀ¼�����ݿ�,��󷵻�NULL(ʧ��).
	brelse(bh);
	return NULL;
}

/*
 *	add_entry()
 *
 * adds a file entry to the specified directory, using the same
 * semantics as find_entry(). It returns NULL if it failed.
 *
 * NOTE!! The inode part of 'de' is left at 0 - which means you
 * may not sleep between calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 */
/*
 *      add_entry()
 * ʹ����find_entry()ͬ���ķ�������ָ��Ŀ¼�����һָ���ļ�����Ŀ¼����ʧ���򷵻�NULL��
 *
 * ע�⣡��'de'��ָ��Ŀ¼��ṹָ�룩��i�ڵ㲿�ֱ�����Ϊ0 - ���ʾ�ڵ��øú�������Ŀ¼���������Ϣ֮�䲻��ȥ˯�ߣ�
 * ��Ϊ���˯�ߣ���ô�����ˣ����̣����ܻ�ʹ�ø�Ŀ¼�
 */
// ����ָ����Ŀ¼���ļ������Ŀ¼�
// ������dir - ָ��Ŀ¼��i�ڵ㣻name - �ļ�����namelen - �ļ������ȣ�
// ���أ����ٻ�����ָ�룻res_dir - ���ص�Ŀ¼��ṹָ�롣
static struct buffer_head * add_entry(struct m_inode * dir,
	const char * name, int namelen, struct dir_entry ** res_dir)
{
	int block, i;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ͬ����������һ��ʼҲ��Ҫ�Ժ�����������Ч�Խ����жϺ���֤�����������ǰ�涨���˷��ų���NO_TRUNCATE����ô����ļ�
	// �����ȳ�����󳤶�NAME_LEN�����账�����û�ж����NO_TRUNCATE����ô���ļ����ȳ�����󳤶�NAME_LENʱ�ض�֮��
	*res_dir = NULL;                							// ���ڷ���Ŀ¼��ṹָ�롣
#ifdef NO_TRUNCATE
	if (namelen > NAME_LEN)
		return NULL;
#else
	if (namelen > NAME_LEN)
		namelen = NAME_LEN;
#endif
	// �������ǿ�ʼ��������ָ��Ŀ¼�����һ��ָ���ļ�����Ŀ¼����������Ҫ�ȶ�ȡĿ¼�����ݣ���ȡ��Ŀ¼i�ڵ��Ӧ���豸
	// �������е����ݿ飨�߼��飩��Ϣ����Щ�߼���Ŀ�ű�����i�ڵ�ṹ��i_zone[9]�����С�������ȡ���1����š����Ŀ¼
	// i�ڵ�ָ��ĵ�һ��ֱ�Ӵ��̿��Ϊ0,��˵����Ŀ¼��Ȼ�������ݣ��ⲻ���������Ƿ���NULL�˳����������Ǿʹӽڵ������豸��ȡ
	// ָ����Ŀ¼�����ݿ顣������ɹ�����Ҳ����NULL�˳������⣬��������ṩ���ļ������ȵ���0,��Ҳ����NULL�˳���
	if (!namelen)
		return NULL;
	if (!(block = dir->i_zone[0]))
		return NULL;
	if (!(bh = bread(dir->i_dev, block)))
		return NULL;
	// ��ʱ���Ǿ������Ŀ¼i�ڵ����ݿ���ѭ���������δʹ�õĿ�Ŀ¼�������Ŀ¼��ṹָ��deָ�򻺳���е����ݿ鲿�֣�����
	// һ��Ŀ¼�������i��Ŀ¼�е�Ŀ¼�������ţ���ѭ����ʼʱ��ʼ��Ϊ0��
	i = 0;
	de = (struct dir_entry *) bh->b_data;
	while (1) {
		// �����ǰĿ¼�����ݿ��Ѿ�������ϣ�����û���ҵ���Ҫ�Ŀ�Ŀ¼����ͷŵ�ǰĿ¼�����ݿ飬�ٶ���Ŀ¼����һ���߼��顣���
		// ��Ӧ���߼��鲻���ھʹ���һ�顣����ȡ�򴴽�����ʧ���򷵻ؿա�����˴ζ�ȡ�Ĵ����߼������ݷ��صĻ����ָ��Ϊ�գ�˵����
		// ���߼����������Ϊ�����ڶ��´����Ŀտ飬���Ŀ¼������ֵ����һ���߼����������ɵ�Ŀ¼����DIR_ENTRIES_PER_BLOCK��
		// ���������ÿ鲢��������������˵���¶���Ŀ�����Ŀ¼�����ݣ�������Ŀ¼��ṹָ��deָ��ÿ�Ļ�������ݲ��֣�Ȼ��������
		// ��������������i/DIR_ENTRIES_PER_BLOCK�ɼ���õ���ǰ������Ŀ¼��i����Ŀ¼�ļ��еĿ�ţ���create_block()����
		// ��inode.c����ɶ�ȡ�򴴽������豸�϶�Ӧ���߼��顣
		if ((char *)de >= BLOCK_SIZE + bh->b_data) {
			brelse(bh);
			bh = NULL;
			block = create_block(dir, i / DIR_ENTRIES_PER_BLOCK);
			if (!block)
				return NULL;
			if (!(bh = bread(dir->i_dev, block))) {          			// �����������ÿ������
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct dir_entry *) bh->b_data;
		}
		// �����ǰ��������Ŀ¼�����i���Ͻṹ��С���ó���ֵ�Ѿ�����Ŀ¼i�ڵ���Ϣ��ָ����Ŀ¼���ݳ���ֵi_size����˵������Ŀ¼
		// �ļ�������û������ɾ���ļ����µĿ�Ŀ¼��������ֻ�ܰ���Ҫ��ӵ���Ŀ¼��ӵ�Ŀ¼�ļ����ݵ�ĩ�˴������ǶԸô�Ŀ¼
		// ��������ã��ø�Ŀ¼���i�ڵ�ָ��Ϊ�գ��������¸�Ŀ¼�ļ��ĳ���ֵ������һ��Ŀ¼��ĳ��ȣ���Ȼ������Ŀ¼��i�ڵ����޸�
		// ��־���ٸ��¸�Ŀ¼�ĸı�ʱ��Ϊ��ǰʱ�䡣
		if (i * sizeof(struct dir_entry) >= dir->i_size) {
			de->inode = 0;
			dir->i_size = (i + 1) * sizeof(struct dir_entry);
			dir->i_dirt = 1;
			dir->i_ctime = CURRENT_TIME;
		}
		// ����ǰ������Ŀ¼��de��i�ڵ�Ϊ�գ����ʾ�ҵ�һ����δʹ�õĿ���Ŀ¼�������ӵ���Ŀ¼����Ǹ���Ŀ¼���޸�ʱ��Ϊ��ǰ
		// ʱ�䣬�����û������������ļ�������Ŀ¼����ļ����ֶΣ��ú��б�Ŀ¼�����Ӧ���ٻ�������޸ı�־�����ظ�Ŀ¼���ָ���Լ�
		// �ø��ٻ�����ָ�룬�˳���
		if (!de->inode) {
			dir->i_mtime = CURRENT_TIME;
			for (i = 0; i < NAME_LEN ; i++)
				de->name[i] = (i < namelen) ? get_fs_byte(name + i) : 0;
			bh->b_dirt = 1;
			*res_dir = de;
			return bh;
		}
		de++;           												// �����Ŀ¼���Ѿ���ʹ�ã�����������һ��Ŀ¼�
		i++;
	}
	// ������ִ�в��������Ҳ����Linus��д��δ���ʱ���ȸ���������find_entry()�����Ĵ��룬�����޸ĳɱ������ġ�
	brelse(bh);
	return NULL;
}

// ���ҷ������ӵ�i�ڵ�.
// ����:dir - Ŀ¼i�ڵ�;inode - Ŀ¼��i�ڵ�.
// ����:���ط������ӵ��ļ���i�ڵ�ָ��.������NULL.
static struct m_inode * follow_link(struct m_inode * dir, struct m_inode * inode)
{
	unsigned short fs;													// ������ʱ����fs�μĴ���ֵ.
	struct buffer_head * bh;

	// �����жϺ�����������Ч��.���û�и���Ŀ¼i�ڵ�,���Ǿ�ʹ�ý�������ṹ�����õĸ�i�ڵ�,������������1.���û�и���Ŀ¼
	// ��i�ڵ�,��Ż�Ŀ¼i�ڵ�󷵻�NULL.���ָ��Ŀ¼���һ����������,��ֱ�ӷ���Ŀ¼���Ӧ��i�ڵ�inode.
	if (!dir) {
		dir = current->root;
		dir->i_count++;
	}
	if (!inode) {
		iput(dir);
		return NULL;
	}
	if (!S_ISLNK(inode->i_mode)) {
		iput(dir);
		return inode;
	}
	// Ȼ��ȡfs�μĴ���ֵ.fsͨ��������ָ���������ݶε�ѡ���0x17.���fsû��ָ���û����ݶ�,���߸�����Ŀ¼��i�ڵ��1��ֱ�ӿ�
	// ��ŵ���0,�����Ƕ�ȡ��1��ֱ�ӿ����,��Ż�dir��inode����i�ڵ㲢����NULL�˳�.����˵������fs��ָ���û����ݶ�,������
	// ���Ѿ��ɹ��ض�ȡ�˷�������Ŀ¼����ļ�����,�����ļ������Ѿ���bhָ��Ļ������������.ʵ����,���������������н�����һ
	// ������ָ����ļ�·�����ַ���.
	__asm__("mov %%fs, %0":"=r" (fs));
	if (fs != 0x17 || !inode->i_zone[0] ||
	   !(bh = bread(inode->i_dev, inode->i_zone[0]))) {
		iput(dir);
		iput(inode);
		return NULL;
	}
	// ��ʱ�����Ѿ�����Ҫ��������Ŀ¼���i�ڵ���,���ǰ����Ż�.��������һ������,�Ǿ����ں˺���������û����ݶ��Ǵ�����û�����
	// �ռ��е�,��ʹ����fs�μĴ��������û��ռ䴫�����ݵ��ں˿ռ���.��������Ҫ���������ȴ���ں˿ռ���.���Ϊ����ȷ�ش���λ��
	// �ں��е��û�����,������Ҫ��fs�μĴ�����ʱָ���ں˿ռ�,����fs=0x10.���ڵ��ú�����������ٻָ�ԭfs��ֵ.����ͷ���Ӧ����
	// ��,������_namei()�����õ���������ָ����ļ�i�ڵ�.
	iput(inode);
	__asm__("mov %0, %%fs"::"r" ((unsigned short) 0x10));
	inode = _namei(bh->b_data,dir,0);
	__asm__("mov %0, %%fs"::"r" (fs));
	brelse(bh);
	return inode;
}

/*
 *	get_dir()
 *
 * Getdir traverses the pathname until it hits the topmost directory.
 * It returns NULL on failure.
 */
/*
 *	get_dir()
 *
 * �ú������ݸ�����·������������,ֱ���ﵽ��˵�Ŀ¼.���ʧ���Ƿ���NULL.
 */
// ��ָ��Ŀ¼��ʼ��Ѱָ��·������Ŀ¼(���ļ���)��i�ڵ�.
// ����:pathname - ·����;inode - ָ����ʼĿ¼��i�ڵ�.
// ����:Ŀ¼���ļ���i�ڵ�ָ��.ʧ��ʱ����NULL.
static struct m_inode * get_dir(const char * pathname, struct m_inode * inode)
{
	char c;
	const char * thisname;
	struct buffer_head * bh;
	int namelen, inr;
	struct dir_entry * de;
	struct m_inode * dir;

	// �����жϲ�����Ч��.���������ָ��Ŀ¼��i�ڵ�ָ��inodeΪ��,��ʹ�õ�ǰ���̵Ĺ���Ŀ¼i�ڵ�.
	if (!inode) {
		inode = current->pwd;									// ���̵ĵ�ǰ����Ŀ¼i�ڵ�.
		inode->i_count++;
	}
	// ����û�ָ��·�����ĵ�1���ַ���'/',��˵��·�����Ǿ���·����.��Ӧ�ôӵ�ǰ��������ṹ�����õĸ�(��α��)i�ڵ㿪ʼ����.
	// ����������Ҫ�ȷŻز���ָ���Ļ����趨��Ŀ¼i�ڵ�,��ȡ�ý���ʹ�õĸ�i�ڵ�.Ȼ��Ѹ�i�ڵ�����ü�����1,
	// ��ɾ��·�����ĵ�1���ַ�'/'.�����Ϳ��Ա�֤����ֻ�������趨�ĸ�i�ڵ���Ϊ���������.
	if ((c = get_fs_byte(pathname)) == '/') {
		iput(inode);											// �Ż�ԭi�ڵ�.
		inode = current->root;									// Ϊ����ָ���ĸ�i�ڵ�.
		pathname++;
		inode->i_count++;
	}
	// Ȼ�����·�����еĸ���Ŀ¼�����ֺ��ļ�������ѭ��������ѭ����������У�������Ҫ�Ե�ǰ���ڴ����Ŀ¼�����ֵ�i�ڵ������Ч���жϣ����Ұ�
	// ����thisnameָ��ǰ���ڴ����Ŀ¼�����֡������i�ڵ������ǰ�����Ŀ¼�����ֲ���Ŀ¼���ͣ�����û�пɽ����Ŀ¼�ķ�����ɣ���Żظ�i�ڵ�
	// ������NULL�˳�����Ȼ�ڸս���ѭ��ʱ����ǰĿ¼�ģ�ڵ�inode���ǽ��̸�i�ڵ�����ǵ�ǰ����Ŀ¼��i�ڵ㣬�����ǲ���ָ����ĳ��������ʼĿ¼��i�ڵ㡣
	while (1) {
		thisname = pathname;
		if (!S_ISDIR(inode->i_mode) || !permission(inode, MAY_EXEC)) {
			iput(inode);
			return NULL;
		}
		// ÿ��ѭ�����Ǵ���·������һ��Ŀ¼��(���ļ���)����.�����ÿ��ѭ�������Ƕ�Ҫ��·�����ַ����з����һ��Ŀ¼��(���ļ���).�����Ǵӵ�ǰ·����ָ��
		// pathname��ʼ����������ַ�,ֱ���ַ���һ����β��(NULL)������һ��'/'�ַ�.��ʱ����namelen�����ǵ�ǰ����Ŀ¼�����ֵĳ���,������thisname��ָ��
		// ��Ŀ¼�����ֵĿ�ʼ��.��ʱ����ַ��ǽ�β��NULL,������Ѿ�������·����ĩβ,���ѵ������ָ��Ŀ¼�����ļ���,�򷵻ظ�i�ڵ�ָ���˳�.
		// ע��!���·���������һ������Ҳ��һ��Ŀ¼��,�������û�м���'/'�ַ�,�������᷵�ظ����Ŀ¼����i�ڵ�!����:����·��/usr/src/linux,�ú�����
		// ֻ����src/Ŀ¼����i�ڵ�.
		for(namelen = 0; (c = get_fs_byte(pathname++)) && (c != '/'); namelen++)
			/* nothing */ ;
		if (!c)
			return inode;
		// �ڵõ���ǰĿ¼������(���ļ���)��,���ǵ��ò���Ŀ¼���find_entry()�ڵ�ǰ�����Ŀ¼��Ѱ��ָ�����Ƶ�Ŀ¼��.���û���ҵ�,��Żظ�i�ڵ�,������
		// NULL�˳�.Ȼ�����ҵ���Ŀ¼����ȡ����i�ڵ��inr���豸��idev,�ͷŰ�����Ŀ¼��ĸ��ٻ���鲢�Żظ�i�ڵ�.Ȼ��ȡ�ڵ��inr��i�ڵ�inode,���Ը�Ŀ¼
		// ��Ϊ��ǰĿ¼����ѭ������·�����е���һĿ¼������(���ļ���).�����ǰ�����Ŀ¼����һ������������,��ʹ��follow_link()�Ϳ��Եõ���ָ���Ŀ¼����i�ڵ�.
		if (!(bh = find_entry(&inode, thisname, namelen, &de))) {
			iput(inode);
			return NULL;
		}
		inr = de->inode;										// ��ǰĿ¼�����ֵ�i�ڵ��.
		brelse(bh);
		dir = inode;
		if (!(inode = iget(dir->i_dev, inr))) {					// ȡi�ڵ�����.
			iput(dir);
			return NULL;
		}
		if (!(inode = follow_link(dir, inode)))
			return NULL;
        }
}

/*
 *	dir_namei()
 *
 * dir_namei() returns the inode of the directory of the
 * specified name, and the name within that directory.
 */
/*
 *	dir_namei()
 *
 * dir_namei()��������ָ��Ŀ¼����i�ڵ�ָ��,�Լ������Ŀ¼������.
 */
// ����:pathname - Ŀ¼·����;namelen - ·��������;name - ���ص����Ŀ¼��.
// base - ������ʼĿ¼��i�ڵ�.
// ����:ָ��Ŀ¼������i�ڵ�ָ������Ŀ¼���Ƽ�����.����ʱ����NULL.
// ע��!!����"���Ŀ¼"��ָ·���������ĩ�˵�Ŀ¼.
static struct m_inode * dir_namei(const char * pathname,
	int * namelen, const char ** name, struct m_inode * base)
{
	char c;
	const char * basename;
	struct m_inode * dir;

	// ����ȡ��ָ��·�������Ŀ¼��i�ڵ�.Ȼ���·����pathname�����������,������һ��'/'�ַ�����������ַ���,�����䳤��,����
	// �������Ŀ¼��i�ڵ�ָ��.ע��!���·�������һ���ַ���б���ַ�'/',��ô���ص�Ŀ¼��Ϊ��,���ҳ���Ϊ0.�����ص�i�ڵ�ָ����Ȼ
	// ָ�����һ��'/'�ַ�ǰĿ¼����i�ڵ�.
	if (!(dir = get_dir(pathname, base)))					// base��ָ������ʼĿ¼i�ڵ�.
		return NULL;
	basename = pathname;
	while (c = get_fs_byte(pathname++))
		if (c == '/')
			basename = pathname;
	*namelen = pathname - basename - 1;
	*name = basename;
	return dir;
}

// ȡָ��·������i�ڵ��ڲ�����.
// ����:pathname - ·����;base - �������Ŀ¼i�ڵ�;follow_links - �Ƿ����������ӵı�־,1 - ��Ҫ,0 ����Ҫ.
struct m_inode * _namei(const char * pathname, struct m_inode * base,
	int follow_links)
{
	const char * basename;
	int inr, namelen;
	struct m_inode * inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ���Ȳ���ָ��·���������Ŀ¼��Ŀ¼�����õ���i�ڵ�.��������,�򷵻�NULL�˳�.������ص�������ֵĳ�����0,���ʾ��·������һ��Ŀ¼��Ϊ
	// ���һ��.���˵�������Ѿ��ҵ���ӦĿ¼��i�ڵ�,����ֱ�ӷ��ظ�i�ڵ��˳�.
	if (!(base = dir_namei(pathname, &namelen, &basename, base)))
		return NULL;
	if (!namelen)										/* special case: '/usr/' etc */
		return base;									/* ��Ӧ��'/usr/'����� */
	// Ȼ���ڷ��صĶ���Ŀ¼��Ѱ��ָ���ļ���Ŀ¼���i�ڵ�.ע��!��Ϊ������Ҳ��һ��Ŀ¼��,�����û�м�'/',�򲻻᷵�ظ����Ŀ¼��i�ڵ�!����:/usr/src/linux,
	// ��ֻ����src/Ŀ¼����i�ڵ�.��Ϊ����dir_namei()������'/'���������һ�����ֵ���һ���ļ���������,���������Ҫ�������������ʹ��Ѱ��Ŀ¼��i�ڵ㺯��
	// find_entry()���д���.��ʱde�к���Ѱ�ҵ���Ŀ¼��ָ��,��base�ǰ�����Ŀ¼���Ŀ¼��i�ڵ�ָ��.
	bh = find_entry(&base, basename, namelen, &de);
	if (!bh) {
		iput(base);
		return NULL;
	}
	// ����ȡ��Ŀ¼���i�ڵ��,���ͷŰ�����Ŀ¼��ĸ��ٻ���鲢�Ż�Ŀ¼i�ڵ�.Ȼ��ȡ��Ӧ�ڵ�ŵ�i�ڵ�,�޸��䱻����ʱ��Ϊ��ǰʱ��,�������޸ı�־.��󷵻�
	// ��i�ڵ�ָ��inode.�����ǰ�����Ŀ¼����һ������������,��ʹ��follow_link()�õ���ָ���Ŀ¼������i�ڵ�.
	inr = de->inode;
	brelse(bh);
	if (!(inode = iget(base->i_dev, inr))) {
		iput(base);
		return NULL;
	}
	if (follow_links)
		inode = follow_link(base, inode);
	else
		iput(base);
	inode->i_atime = CURRENT_TIME;
	inode->i_dirt = 1;
	return inode;
}

// ȡָ��·������i�ڵ㣬������������ӡ�
// ������pathname - ·������
// ���أ���Ӧ��i�ڵ㡣
struct m_inode * lnamei(const char * pathname)
{
	return _namei(pathname, NULL, 0);
}

/*
 *	namei()
 *
 * is used by most simple commands to get the inode of a specified name.
 * Open, link etc use their own routines, but this is enough for things
 * like 'chmod' etc.
 */
/*
 *	namei()
 *
 * �ú�����������������ȡ��ָ��·�����Ƶ�i�ڵ�.open,link����ʹ�������Լ�����Ӧ����.���������޸�ģʽ"chmod"������������,��
 * �������㹻����.
 */
// ȡָ��·������i�ڵ�,�����������.
// ����:pathname - ·����.
// ����:��Ӧ��i�ڵ�.
struct m_inode * namei(const char * pathname)
{
	return _namei(pathname, NULL, 1);
}

/*
 *	open_namei()
 *
 * namei for open - this is in fact almost the whole open-routine.
 */
/*
 * 	open_namei()
 *
 * open()����ʹ�õ�namei���� - ����ʵ�����������Ĵ��ļ�����.
 */
// �ļ���namei����.
// ����filename���ļ���,flag�Ǵ��ļ���־,����ȡֵ:O_RDONLY(ֻ��),O_WRONLY(ֻд)��O_RDWR(��д),�Լ�O_CREAT(����),
// O_EXCL(�������ļ����벻����),O_APPEND(���ļ�β�������)������һЩ��־�����,��������ô�����һ�����ļ�,��mode������ָ��
// �ļ����������.��Щ������S_IRWXU(�ļ��������ж�,д��ִ��Ȩ��),S_IRUSR(�û����ж��ļ�Ȩ��),S_IRWXG(���Ա�ж�,д
// ִ��)�ȵ�.�����´������ļ�,��Щ����ֻӦ���ڽ������ļ��ķ���,������ֻ���ļ��Ĵ򿪵���Ҳ������һ����д���ļ����.�������
// �����ɹ�,�򷵻��ļ����(�ļ�������),���򷵻س�����.�μ�sys/stat.h,fcntl.h.
// ����:�ɹ�����0,���򷵻س�����;res_inode - ���ض�Ӧ�ļ�·������i�ڵ�ָ��.
int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode)
{
	const char * basename;
	int inr, dev, namelen;
	struct m_inode * dir, *inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ���ȶԺ����������к���Ĵ���.����ļ�����ģʽ��־��ֻ��(O),�����ļ������־O_TRUNCȴ��λ��,�����ļ��򿪱�־�����ֻд��־
	// O_WRONLY.��������ԭ�������ڽ����־O_TRUNC�������ļ���д�������Ч.
	if ((flag & O_TRUNC) && !(flag & O_ACCMODE))
		flag |= O_WRONLY;
	// ʹ�õ�ǰ���̵��ļ��������������,���ε�����ģʽ�е���Ӧλ,��������ͨ�ļ���־I_REGULAR.
	// �ñ�־�����ڴ򿪵��ļ������ڶ���Ҫ�����ļ�ʱ,��Ϊ���ļ���Ĭ������
	mode &= 0777 & ~current->umask;
	mode |= I_REGULAR;													// �����ļ���־.���μ�include/const.h�ļ�.
	// Ȼ�����ָ����·����Ѱ�ҵ���Ӧ��i�ڵ�,�Լ����Ŀ¼�����䳤��.��ʱ������Ŀ¼������Ϊ0(����'/usr/'����·���������),��ô
	// ���������Ƕ�д,�������ļ����Ƚ�0,���ʾ���ڴ�һ��Ŀ¼���ļ�����.����ֱ�ӷ��ظ�Ŀ¼��i�ڵ㲢����0�˳�.����˵�����̲����Ƿ�,����
	// �Żظ�i�ڵ�,���س�����.
	if (!(dir = dir_namei(pathname, &namelen, &basename, NULL)))
		return -ENOENT;
	// �ļ�����Ϊ�գ��򷵻�
	if (!namelen) {														/* special case: '/usr/' etc */
		if (!(flag & (O_ACCMODE | O_CREAT | O_TRUNC))) {
			*res_inode = dir;
			return 0;
		}
		iput(dir);
		return -EISDIR;
	}
	// ���Ÿ�������õ������Ŀ¼����i�ڵ�dir,�����в���ȡ��·�����ַ����������ļ�����Ӧ��Ŀ¼��ṹde,��ͬʱ�õ���Ŀ¼�����ڵĸ��ٻ���
	// ��ָ��.����ø��ٻ���ָ��ΪNULL,���ʾû���ҵ���Ӧ�ļ�����Ŀ¼��,���ֻ�����Ǵ����ļ�����.��ʱ������Ǵ����ļ�,��Żظ�Ŀ¼��i�ڵ�,����
	// ������˳�.����û��ڸ�Ŀ¼û��д��Ȩ��,��Żظ�Ŀ¼��i�ڵ�,���س�����˳�.
	bh = find_entry(&dir, basename, namelen, &de);
	if (!bh) {
		if (!(flag & O_CREAT)) {                						// ���Ǵ����ļ����Ż�i�ڵ�
			iput(dir);
			return -ENOENT;
		}
		if (!permission(dir, MAY_WRITE)) {       						// û��дȨ�ޣ��Ż�i�ڵ�
			iput(dir);
			return -EACCES;
		}
		// ��������ȷ�����Ǵ�������������д������ɡ�������Ǿ���Ŀ¼i�ڵ��Ӧ�豸������һ���µ�i�ڵ��·������ָ�����ļ�ʹ�á�
		// ��ʧ����Ż�Ŀ¼��i�ڵ㣬������û�пռ�����롣����ʹ�ø���i�ڵ㣬������г�ʼ���ã��ýڵ���û�id����Ӧ�ڵ����ģʽ��
		// �����޸ı�־��Ȼ����ָ��Ŀ¼dir�����һ����Ŀ¼�
		inode = new_inode(dir->i_dev);
		if (!inode) {
			iput(dir);
			return -ENOSPC;
		}
		inode->i_uid = current->euid;
		inode->i_mode = mode;
		inode->i_dirt = 1;
		bh = add_entry(dir, basename, namelen, &de);
		// ������ص�Ӧ�ú�����Ŀ¼��ĵ���������ָ��ΪNULL�����ʾ���Ŀ¼�����ʧ�ܡ����ǽ�����i�ڵ���������Ӽ�����1,�Żظ�
		// i�ڵ���Ŀ¼��i�ڵ㲢���س������˳�������˵�����Ŀ¼������ɹ����������������ø���Ŀ¼���һЩ��ʼֵ����i�ڵ��Ϊ����
		// �뵽��i�ڵ�ĺ��룻���ø��ٻ������޸ı�־��Ȼ���ͷŸø��ٻ��������Ż�Ŀ¼��i�ڵ㡣������Ŀ¼���i�ڵ�ָ�룬���ɹ��˳���
		if (!bh) {
			inode->i_nlinks--;
			iput(inode);
			iput(dir);
			return -ENOSPC;
		}
		de->inode = inode->i_num;
		bh->b_dirt = 1;
		brelse(bh);
		iput(dir);
		*res_inode = inode;
		return 0;
    }
	// ������(411��)��Ŀ¼��ȡ�ļ�����ӦĿ¼��ṹ�Ĳ����ɹ�(��bh��ΪNULL),��˵��ָ���򿪵��ļ��Ѿ�����.����ȡ����Ŀ¼���i�ڵ���������豸��,���ͷ�
	// �ø��ٻ������Լ��Ż�Ŀ¼��i�ڵ�.�����ʱ��ռ������־O_EXCL��λ,�������ļ��Ѿ�����,�򷵻��ļ��Ѵ��ڳ������˳�.
	inr = de->inode;
	dev = dir->i_dev;
	brelse(bh);
	if (flag & O_EXCL) {
		iput(dir);
		return -EEXIST;
	}
	// Ȼ�����Ƕ�ȡ��Ŀ¼���i�ڵ�����.����i�ڵ���һ��Ŀ¼��i�ڵ㲢�ҷ���ģʽ��ֻд���д,����û�з��ʵ����Ȩ��,��Żظ�i�ڵ�,���ط���Ȩ�޳������˳�.
	if (!(inode = follow_link(dir, iget(dev, inr))))
		return -EACCES;
	if ((S_ISDIR(inode->i_mode) && (flag & O_ACCMODE)) ||
	    !permission(inode, ACC_MODE(flag))) {
		iput(inode);
		return -EPERM;
	}
	// �������Ǹ��¸�i�ڵ�ķ���ʱ���ֶ�ֵΪ��ǰʱ��.��������˽�0��־,�򽫸�i�ڵ���ļ����Ƚ�Ϊ0.��󷵻ظ�Ŀ¼��i�ڵ��ָ��.������0(�ɹ�).
	inode->i_atime = CURRENT_TIME;
	if (flag & O_TRUNC)
		truncate(inode);
	*res_inode = inode;
	return 0;
}

// ����һ���豸�����ļ�����ͨ�ļ��ڵ㣨node����
// �ú�����������Ϊfilename����mode��devָ�����ļ�ϵͳ�ڵ㣨��ͨ�ļ����豸�����ļ��������ܵ�����
// ������filename - ·������mode - ָ��ʹ������Լ��������ڵ�����ͣ�dev - �豸�š�
int sys_mknod(const char * filename, int mode, int dev)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ���ȼ�������ɺͲ�������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣������ǳ����û����򷵻ط�����ɳ����롣
	// ������ǳ����û����򷵻ط�����ɳ����롣
	if (!suser())
		return -EPERM;
	// ����Ҳ�����Ӧ·�����ж���Ŀ¼��i�ڵ㣬�򷵻س����롣
	if (!(dir = dir_namei(filename, &namelen, &basename, NULL)))
		return -ENOENT;
	// �����˵��ļ�������Ϊ0����˵��������·�������û��ָ���ļ������Żظ�Ŀ¼i�ڵ㣬���س������˳���
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	// ����ڸ�Ŀ¼��û��д��Ȩ�ޣ���Żظ�Ŀ¼��i�ڵ㣬���ط�����ɳ������˳���
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// Ȼ����������һ��·����ָ�����ļ��Ƿ��Ѿ����ڡ����Ѿ��������ܴ���ͬ���ļ��ڵ㡣�����Ӧ·�����������ļ�����
	// Ŀ¼���Ѿ����ڣ����ͷŰ�����Ŀ¼��Ļ������鲢�Ż�Ŀ¼��i�ڵ㣬�����ļ��Ѿ����ڵĳ����˳���
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}
	// �������Ǿ�����һ���µ�i�ڵ㣬�����ø�i�ڵ������ģʽ��
	inode = new_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}
	inode->i_mode = mode;
	// ���Ҫ�������ǿ��豸�ļ��������ַ��豸�ļ�������i�ڵ��ֱ���߼���ָ��0�����豸�š��������豸�ļ���˵��
	// ��i�ڵ��i_zone[0]�д�ŵ��Ǹ��豸�ļ��������豸���豸�š�
	if (S_ISBLK(mode) || S_ISCHR(mode))
		inode->i_zone[0] = dev;
	// ���ø�i�ڵ���޸�ʱ�䡢����ʱ��Ϊ��ǰʱ�䣬������i�ڵ����޸ı�־��
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	inode->i_dirt = 1;
	// ����Ϊ����µ�i�ڵ���Ŀ¼�������һ��Ŀ¼����ʧ�ܣ�������Ŀ¼��ĸ��ٻ����ָ��ΪNULL������Ż�Ŀ¼��i�ڵ㣻
	// ���������i�ڵ��������Ӽ�����λ�����Żظ�i�ڵ㣬���س������˳���
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		inode->i_nlinks = 0;
		iput(inode);
		return -ENOSPC;
	}
	// �������Ŀ¼�����Ҳ�ɹ��ˣ������������������Ŀ¼�����ݡ����Ŀ¼���i�ڵ��ֶε�����i�ڵ�ţ����ø��ٻ���������
	// �ı�־���Ż�Ŀ¼���µ�i�ڵ㣬�ͷŸ��ٻ���������󷵻�0���ɹ�����
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	iput(dir);
	iput(inode);
	brelse(bh);
	return 0;
}

// ����һ��Ŀ¼��
// ������pathname - ·������mode - Ŀ¼ʹ�õ�Ȩ�����ԡ�
// ���أ��ɹ��򷵻�0,���򷵻س����롣
int sys_mkdir(const char * pathname, int mode)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh, *dir_block;
	struct dir_entry * de;

	// ���ȼ���������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣����Ҳ�����Ӧ·�����ж���Ŀ¼��i�ڵ㣬�򷵻س����롣
	if (!(dir = dir_namei(pathname,&namelen,&basename, NULL)))
		return -ENOENT;
	// �������ļ�������Ϊ0,��˵��������·�������û��ָ���ļ������Żظ�Ŀ¼i�ڵ㣬���س������˳���
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	// ����ڸ�Ŀ¼��û��д��Ȩ�ޣ���Żظ�Ŀ¼i�ڵ㣬���ط�����ɳ������˳���
	// ������ǳ����û����򷵻ط�����ɳ����롣
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// Ȼ����������һ��·����ָ����Ŀ¼���Ƿ��Ѿ����ڡ����Ѿ��������ܴ���ͬ��Ŀ¼�ڵ㡣�����Ӧ·����������Ŀ¼����Ŀ¼
	// ���Ѿ����ڣ����ͷŰ�����Ŀ¼��Ļ������鲢�Ż�Ŀ¼��i�ڵ㣬�����ļ��Ѿ����� �ĳ������˳����������Ǿ�����һ���µ�i��
	// �㣬�����ø�i�ڵ������ģʽ���ø���i�ڵ��Ӧ���ļ�����Ϊ32�ֽڣ�2��Ŀ¼��Ĵ�С�����ýڵ����޸ı�־���Լ��ڵ���޸�
	// ʱ��ͷ���ʱ�䡣2��Ŀ¼��ֱ�����'.'��'..'Ŀ¼��
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}
	inode = new_inode(dir->i_dev);
	if (!inode) {           						// �����ɹ���Ż�Ŀ¼��i�ڵ㣬�����޿ռ�����롣
		iput(dir);
		return -ENOSPC;
	}
	inode->i_size = 32;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	// ����Ϊ����i�ڵ�����һ���ڱ���Ŀ¼�����ݵĴ��̿飬����i�ڵ�ĵ�һ��ֱ�ӿ�ָ����ڸÿ�š��������ʧ����Żض�ӦĿ¼
	// ��i�ڵ㣻��λ�������i�ڵ����Ӽ������Żظ��µ�i�ڵ㣬����û�пռ�������˳��������ø��µ�i�ڵ����޸ı�־��
	if (!(inode->i_zone[0] = new_block(inode->i_dev))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ENOSPC;
	}
	inode->i_dirt = 1;
	// ���豸�϶�ȡ������Ĵ��̿飨Ŀ���ǰѶ�Ӧ��ŵ����ٻ������У�����������Żض�ӦĿ¼��i�ڵ㣻�ͷ�����Ĵ��̿飻��λ��
	// �����i�ڵ����Ӽ������Żظ��µ�i�ڵ㣬����û�пռ�������˳���
	if (!(dir_block = bread(inode->i_dev, inode->i_zone[0]))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ERROR;
	}
	// Ȼ�������ڻ�����н�����������Ŀ¼�ļ��е�2��Ĭ�ϵ���Ŀ¼�'.'��'..'���ṹ���ݡ�������deָ����Ŀ¼������ݿ飬Ȼ
	// ���ø�Ŀ¼���i�ڵ���ֶε����������i�ڵ�ţ������ֶε��ڡ�.����Ȼ��deָ����һ��Ŀ¼��ṹ�����ڸýṹ�д���ϼ�Ŀ¼��
	// i�ڵ�ź����֡�..����Ȼ�����øø��ٻ�������޸ı�־�����ͷŸû������顣�ٳ�ʼ��������i�ڵ��ģʽ�ֶΣ����ø�i�ڵ����޸�
	// ��־��
	de = (struct dir_entry *) dir_block->b_data;
	de->inode = inode->i_num;         				// ����'.'Ŀ¼�
	strcpy(de->name, ".");
	de++;
	de->inode = dir->i_num;         				// ����'..'Ŀ¼�
	strcpy(de->name, "..");
	inode->i_nlinks = 2;
	dir_block->b_dirt = 1;
	brelse(dir_block);
	inode->i_mode = I_DIRECTORY | (mode & 0777 & ~current->umask);
	inode->i_dirt = 1;
	// ����������ָ��Ŀ¼�������һ��Ŀ¼����ڴ���½�Ŀ¼��i�ڵ��Ŀ¼�������ʧ�ܣ�������Ŀ¼��ĸ��ٻ�����ָ��ΪNULL����
	// ��Ż�Ŀ¼��i�ڵ㣻�������i�ڵ��������Ӽ�����λ�����Żظ�i�ڵ㡣���س������˳���
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		inode->i_nlinks = 0;
		iput(inode);
		return -ENOSPC;
	}
	// ��������Ŀ¼���i�ڵ��ֶε�����i�ڵ�ţ����ø��ٻ�������޸ı�־���Ż�Ŀ¼���µ�i�ڵ㣬�ͷŸ��ٻ���������󷵻�0���ɹ�����
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	dir->i_nlinks++;
	dir->i_dirt = 1;
	iput(dir);
	iput(inode);
	brelse(bh);
	return 0;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
/*
 * ���ڼ��ָ����Ŀ¼�Ƿ�Ϊ�յ��ӳ�������rmdirϵͳ���ã���
 */
// ���ָ��Ŀ¼�Ƿ�Ϊ�ա�
// ������inode - ָ��Ŀ¼��i�ڵ�ָ�롣
// ���أ�1 - Ŀ¼���ǿյģ�0 - ���ա�
static int empty_dir(struct m_inode * inode)
{
	int nr, block;
	int len;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ���ȼ���ָ��Ŀ¼������Ŀ¼���������鿪ʼ�����ض�Ŀ¼������Ϣ�Ƿ���ȷ��һ��Ŀ¼��Ӧ��������2��Ŀ¼�����.���͡�..����
	// ���Ŀ¼���������2�����߸�Ŀ¼i�ڵ�ĵ�1��ֱ�ӿ�û��ָ���κδ��̿�ţ����߸�ֱ�ӿ������������ʾ������Ϣ���豸dev��
	// Ŀ¼��������0��ʧ�ܣ���
	len = inode->i_size / sizeof (struct dir_entry);        		// Ŀ¼��Ŀ¼�������
	if (len < 2 || !inode->i_zone[0] ||
	    !(bh = bread(inode->i_dev, inode->i_zone[0]))) {
	    	printk("warning - bad directory on dev %04x\n", inode->i_dev);
		return 0;
	}
	// ��ʱbh��ָ������к���Ŀ¼�����ݡ�������Ŀ¼��ָ��deָ�򻺳���е�1��Ŀ¼����ڵ�1��Ŀ¼���.����������i�ڵ���ֶ�
	// inodeӦ�õ��ڵ�ǰĿ¼��i�ڵ�š����ڵ�2��Ŀ¼���..�������ڵ���ֶ�inodeӦ�õ�����һ��Ŀ¼��i�ڵ�ţ�����Ϊ0.��ˣ�
	// �����1��Ŀ¼���i�ڵ���ֶ�ֵ�����ڸ�Ŀ¼��i�ڵ�ţ����ߵ�2��Ŀ¼���i�ڵ���ֶ�Ϊ�㣬��������Ŀ¼��������ֶβ��ֱ�
	// ���ڡ�.���͡�..��������ʾ��������Ϣ���豸dev��Ŀ¼����������0��
	de = (struct dir_entry *) bh->b_data;
	if (de[0].inode != inode->i_num || !de[1].inode ||
	    strcmp(".", de[0].name) || strcmp("..", de[1].name)) {
	    	printk("warning - bad directory on dev %04x\n", inode->i_dev);
		return 0;
	}
	// Ȼ��������nr����Ŀ¼����ţ���0��ʼ�ƣ���deָ�������Ŀ¼���ѭ������Ŀ¼���������еģ�len - 2����Ŀ¼�����û��
	// Ŀ¼���i�ڵ���ֶβ�Ϊ0����ʹ�ã���
	nr = 2;
	de += 2;
	while (nr < len) {
		// ����ÿ���̿��е�Ŀ¼���Ѿ�ȫ�������ϣ����ͷŸô��̿�Ļ���飬����ȡĿ¼�����ļ�����һ�麬��Ŀ¼��Ĵ��̿顣��ȡ�ķ�
		// ���Ǹ��ݵ�ǰ����Ŀ¼�����nr�������ӦĿ¼����Ŀ¼�����ļ��е����ݿ�ţ�nr/DIR_ENTRIES_PER_BLOCK����Ȼ��ʹ��bmap()
		// ����ȡ�ö�Ӧ���̿��block����ʹ�ö��豸�麯��bread()����Ӧ�̿���뻺����У������ظû�����ָ�롣������ȡ����Ӧ�̿�û��
		// ʹ�ã����Ѿ����ã����ļ��Ѿ�ɾ���ȣ������������һ�飬�����������������0.������deָ�������ĵ�1��Ŀ¼�
		if ((void *) de >= (void *) (bh->b_data + BLOCK_SIZE)) {
			brelse(bh);
			block = bmap(inode, nr / DIR_ENTRIES_PER_BLOCK);
			if (!block) {
				nr += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			if (!(bh = bread(inode->i_dev, block)))
				return 0;
			de = (struct dir_entry *) bh->b_data;
		}
		// ����deָ��ĵ�ǰĿ¼������Ŀ¼���i�ڵ���ֶβ�����0,���ʾ��Ŀ¼��Ŀǰ����ʹ�ã����ͷŸø��ٻ�����������0�˳�������
		// ����û�в�ѯ���Ŀ¼�е�����Ŀ¼����Ŀ¼�����nr��1��deָ����һ��Ŀ¼�������⡣
		if (de->inode) {
			brelse(bh);
			return 0;
		}
		de++;
		nr++;
	}
	// ִ�е�����˵����Ŀ¼��û���ҵ����õ�Ŀ¼���Ȼ����ͷ�������⣩�����ͷŻ���鷵��1��
	brelse(bh);
	return 1;
}

// ɾ��Ŀ¼��
// ������name - Ŀ¼����·��������
// ���أ�����0��ʾ�ɹ������򷵻س���š�
int sys_rmdir(const char * name)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ���ȼ���������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣����Ҳ�����Ӧ·�����ж���Ŀ¼��i�ڵ㣬�򷵻س����롣������
	// �ļ�������Ϊ0,��˵��������·�������û��ָ���ļ������Żظ�Ŀ¼i�ڵ㣬���س������˳�������ڸ�Ŀ¼��û��д��Ȩ�ޣ�
	// ��Żظ�Ŀ¼i�ڵ㣬���ط�����ɳ������˳���������ǳ����û����򷵻ط�����ɳ����롣
	if (!(dir = dir_namei(name, &namelen, &basename, NULL)))
		return -ENOENT;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (!permission(dir,MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// Ȼ�����ָ��Ŀ¼��i�ڵ��Ŀ¼�����ú���find_entry()Ѱ�Ҷ�ӦĿ¼������ذ�����Ŀ¼��Ļ����ָ��bh��������Ŀ¼
	// ���Ŀ¼��i�ڵ�ָ��dir�͸�Ŀ¼��ָ��de���ٸ��ݸ�Ŀ¼��de�е�i�ڵ������iget()�����õ���Ӧ��i�ڵ�inode�������Ӧ
	// ·���������Ŀ¼������Ŀ¼����ڣ����ͷŰ�����Ŀ¼��ĸ��ٻ��������Ż�Ŀ¼��i�ڵ㣬�����ļ������ڳ����룬���˳���
	// ���ȡĿ¼���i�ڵ������Ż�Ŀ¼��i�ڵ㣬���ͷź���Ŀ¼��ĸ��ٻ����������س���š�
	bh = find_entry(&dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		return -ENOENT;
	}
	if (!(inode = iget(dir->i_dev, de->inode))) {
		iput(dir);
		brelse(bh);
		return -EPERM;
	}
	// ��ʱ�������а���Ҫ��ɾ��Ŀ¼���Ŀ¼i�ڵ�dir��Ҫ��ɾ��Ŀ¼���i�ڵ�inode��Ҫ��ɾ��Ŀ¼��ָ��de����������ͨ������3
	// ����������Ϣ�ļ������֤ɾ�������Ŀ����ԡ�
	// ����Ŀ¼����������ɾ����־���ҽ��̵���Ч�û�id��euid������root�����ҽ��̵���Ч�û�id��euid�������ڸ�i�ڵ���û�
	// id�����ʾ��ǰ����û��Ȩ��ɾ����Ŀ¼�����ǷŻذ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i�ڵ㣬Ȼ���ͷŸ��ٻ�������
	// ���س����롣
	if ((dir->i_mode & S_ISVTX) && current->euid &&
	    inode->i_uid != current->euid) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}
	// ���Ҫ��ɾ����Ŀ¼��i�ڵ���豸�Ų����ڰ�����Ŀ¼���Ŀ¼���豸�ţ����߸ñ�ɾ��Ŀ¼���������Ӽ�������1����ʾ�з�����
	// �ӵȣ�������ɾ����Ŀ¼�������ͷŰ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i�ڵ㣬�ͷŸ��ٻ���飬���س����롣
	if (inode->i_dev != dir->i_dev || inode->i_count > 1) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}
	// ���Ҫ��ɾ��Ŀ¼��Ŀ¼��i�ڵ�͵��ڰ�������ɾ��Ŀ¼��Ŀ¼i�ڵ㣬���ʾ��ͼɾ����.��Ŀ¼�����ǲ�����ġ����ǷŻذ���Ҫɾ
	// ��Ŀ¼����Ŀ¼i�ڵ��Ҫɾ��Ŀ¼��i�ڵ㣬�ͷŸ��ٻ���飬���س����롣
	if (inode == dir) {						/* we may not delete ".", but "../dir" is ok */
		iput(inode);
		iput(dir);
		brelse(bh);
		return -EPERM;
	}
	// ��Ҫ��ɾ��Ŀ¼i�ڵ�����Ա����ⲻ��һ��Ŀ¼����ɾ��������ǰ����ȫ�����ڡ����ǷŻذ���ɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��
	// Ŀ¼��i�ڵ㣬�ͷŸ��ٻ���飬���س����롣
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -ENOTDIR;
	}
	// ������Ҫ��ɾ����Ŀ¼���գ���Ҳ����ɾ�������ǷŻذ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i�ڵ㣬�ͷŸ��ٻ���飬����
	// �����롣
	if (!empty_dir(inode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -ENOTEMPTY;
	}
	// ����һ����Ŀ¼����Ŀ¼��������Ӧ��Ϊ2�����ӵ��ϲ�Ŀ¼�ͱ�Ŀ¼���������豻ɾ��Ŀ¼��i�ڵ��������������2,����ʾ������Ϣ��
	// ��ɾ��������Ȼִ�С������ø���ɾ��Ŀ¼��Ŀ¼���i�ڵ���ֶ�Ϊ0,��ʾ��Ŀ¼���ʹ�ã����ú��и�Ŀ¼��ĵ�����������޸�
	// ��־�����ͷŸû���顣Ȼ�����ñ�ɾ��Ŀ¼i�ڵ��������Ϊ0����ʾ���У�������i�ڵ����޸ı�־��
	if (inode->i_nlinks != 2)
		printk("empty directory has nlink!=2 (%d)", inode->i_nlinks);
	de->inode = 0;
	bh->b_dirt = 1;
	brelse(bh);
	inode->i_nlinks = 0;
	inode->i_dirt = 1;
	// �ٽ�������ɾ��Ŀ¼����Ŀ¼��i�ڵ����Ӽ�����1,�޸���ı�ʱ����޸�ʱ��Ϊ��ǰʱ�䣬���øýڵ����޸ı�־�����Żذ���Ҫɾ��
	// Ŀ¼����Ŀ¼i�ڵ�͸�Ҫɾ��Ŀ¼��i�ڵ㣬����0��ɾ�������ɹ�����
	dir->i_nlinks--;
	dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	dir->i_dirt = 1;
	iput(dir);
	iput(inode);
	return 0;
}

// ɾ�����ͷţ��ļ�����Ӧ��Ŀ¼�
// ���ļ�ϵͳɾ��һ�����֡�������ļ������һ�����ӣ�����û�н������򿪸��ļ�������ļ�Ҳ����ɾ�������ͷ���ռ�õ��豸�ռ䡣
// ������name - �ļ�����·��������
// ���أ��ɹ��򷵻�0,���򷵻س���š�
int sys_unlink(const char * name)
{
	const char * basename;
	int namelen;
	struct m_inode * dir, * inode;
	struct buffer_head * bh;
	struct dir_entry * de;

	// ���ȼ���������Ч�Բ�ȡ·�����ж���Ŀ¼��i�ڵ㡣����Ҳ�����Ӧ·�����ж���Ŀ¼��i�ڵ㣬�򷵻س����롣������
	// �ļ�������Ϊ0,��˵��������·�������û��ָ���ļ������Żظ�Ŀ¼i�ڵ㣬���س������˳�������ڸ�Ŀ¼��û��д��Ȩ�ޣ�
	// ��Żظ�Ŀ¼i�ڵ㣬���ط�����ɳ������˳���������ǳ����û����򷵻ط�����ɳ����롣
	if (!(dir = dir_namei(name, &namelen, &basename, NULL)))
		return -ENOENT;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	// Ȼ�����ָ��Ŀ¼��i�ڵ��Ŀ¼�����ú���find_entry()Ѱ�Ҷ�ӦĿ¼������ذ�����Ŀ¼��Ļ����ָ��bh��������Ŀ¼
	// ���Ŀ¼��i�ڵ�ָ��dir�͸�Ŀ¼��ָ��de���ٸ��ݸ�Ŀ¼��de�е�i�ڵ������iget()�����õ���Ӧ��i�ڵ�inode�������Ӧ
	// ·���������Ŀ¼������Ŀ¼����ڣ����ͷŰ�����Ŀ¼��ĸ��ٻ��������Ż�Ŀ¼��i�ڵ㣬�����ļ������ڳ����룬���˳���
	// ���ȡĿ¼���i�ڵ������Ż�Ŀ¼��i�ڵ㣬���ͷź���Ŀ¼��ĸ��ٻ����������س���š�
	bh = find_entry(&dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		return -ENOENT;
	}
	if (!(inode = iget(dir->i_dev, de->inode))) {
		iput(dir);
		brelse(bh);
		return -ENOENT;
	}
	// ��ʱ�������а���Ҫ��ɾ��Ŀ¼���Ŀ¼i�ڵ�dir��Ҫ��ɾ��Ŀ¼���i�ڵ�inode��Ҫ��ɾ��Ŀ¼��ָ��de����������ͨ������3
	// ����������Ϣ�ļ������֤ɾ�������Ŀ����ԡ�
	// ����Ŀ¼����������ɾ����־���ҽ��̵���Ч�û�id��euid������root�����ҽ��̵���Ч�û�id��euid�������ڸ�i�ڵ���û�
	// id�����ҽ��̵�euidҲ������Ŀ¼i�ڵ���û�id�����ʾ��ǰ����û��Ȩ��ɾ����Ŀ¼�����ǷŻذ���Ҫɾ��Ŀ¼����Ŀ¼i�ڵ�
	// �͸�Ҫɾ��Ŀ¼��i�ڵ㣬Ȼ���ͷŸ��ٻ����������س����롣
	if ((dir->i_mode & S_ISVTX) && !suser() &&
	    current->euid != inode->i_uid &&
	    current->euid != dir->i_uid) {
		iput(dir);
		iput(inode);
		brelse(bh);
		return -EPERM;
	}
	// �����ָ���ļ�����һ��Ŀ¼����Ҳ����ɾ�����Żظ�Ŀ¼i�ڵ�͸��ļ���Ŀ¼���i�ڵ㣬�ͷŰ�����Ŀ¼��Ļ���飬���س���š�
	if (S_ISDIR(inode->i_mode)) {
		iput(inode);
		iput(dir);
		brelse(bh);
		return -EPERM;
	}
	// �����i�ڵ�����Ӽ���ֵ�Ѿ�Ϊ0,����ʾ������Ϣ����������Ϊ1��
	if (!inode->i_nlinks) {
		printk("Deleting nonexistent file (%04x:%d), %d\n",
			inode->i_dev, inode->i_num, inode->i_nlinks);
		inode->i_nlinks = 1;
	}
	// �������ǿ���ɾ���ļ�����Ӧ��Ŀ¼���ˡ����ǽ����ļ���Ŀ¼���е�i�ڵ���ֶ���Ϊ0,��ʾ�ͷŸ�Ŀ¼������ð�����Ŀ¼��Ļ�
	// ������޸ı�־���ͷŸø��ٻ���顣
	de->inode = 0;
	bh->b_dirt = 1;
	brelse(bh);
	// Ȼ����ļ�����Ӧi�ڵ����������1,�����޸ı�־�����¸ı�ʱ��Ϊ��ǰʱ�䡣���Żظ�i�ڵ��Ŀ¼��i�ڵ㣬����0���ɹ��������
	// ���ļ������һ�����ӣ���i�ڵ���������1�����0,���Ҵ�ʱû�н������򿪸��ļ�����ô�ڵ���iput()�Ż�i�ڵ�ʱ�����ļ�Ҳ����ɾ��
	// ���ͷ���ռ�õ��豸�ռ䡣�μ�fs/inode.c��
	inode->i_nlinks--;
	inode->i_dirt = 1;
	inode->i_ctime = CURRENT_TIME;
	iput(inode);
	iput(dir);
	return 0;
}

// �����������ӡ�
// Ϊһ���Ѵ����ļ�����һ���������ӣ�Ҳ��Ϊ������ - hard link����
// ������oldname - ԭ·������newname - �µ�·������
// ���أ����ɹ��򷵻�0�����򷵻س���š�
int sys_symlink(const char * oldname, const char * newname)
{
	struct dir_entry * de;
	struct m_inode * dir, * inode;
	struct buffer_head * bh, * name_block;
	const char * basename;
	int namelen, i;
	char c;

	// ���Ȳ�����·���������Ŀ¼��i�ڵ�dir�������������ļ������䳤�ȡ����Ŀ¼��i�ڵ�û���ҵ����򷵻س���š������·����
	// �в������ļ�������Ż���·����Ŀ¼��i�ڵ㣬���س���š����⣬����û�û������Ŀ¼��д��Ȩ�ޣ���Ҳ���ܽ������ӣ����ǷŻ�
	// ��·����Ŀ¼��i�ڵ㣬���س���š�
	dir = dir_namei(newname, &namelen, &basename, NULL);
	if (!dir)
		return -EACCES;
	if (!namelen) {
		iput(dir);
		return -EPERM;
	}
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EACCES;
	}
	// ����������Ŀ¼ָ���豸������һ���µ�i�ڵ㣬�����ø�i�ڵ�ģʽΪ�������������Լ����̹涨��ģʽ�����롣�������ø�i�ڵ�����
	// �ı�־��
	if (!(inode = new_inode(dir->i_dev))) {
		iput(dir);
		return -ENOSPC;
	}
	inode->i_mode = S_IFLNK | (0777 & ~current->umask);
	inode->i_dirt = 1;
	// Ϊ�˱����������·�����ַ�����Ϣ��������ҪΪ��i�ڵ�����һ�����̿飬����i�ڵ�ĵ�1��ֱ�ӿ��i_zone[0]���ڵõ����߼���š�
	// Ȼ����i�ڵ����޸ı�־���������ʧ����Żض�ӦĿ¼��i�ڵ㣻��λ�������i�ڵ����Ӽ������Żظ��µ�i�ڵ㣬����û�пռ������
	// �˳���
	if (!(inode->i_zone[0] = new_block(inode->i_dev))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ENOSPC;
	}
	inode->i_dirt = 1;
	// Ȼ����豸�϶�ȡ������Ĵ��̿飨Ŀ���ǰѶ�Ӧ��ŵ����ٻ������У�����������Żض�ӦĿ¼��i�ڵ㣻��λ�������i�ڵ����Ӽ�����
	// �Żظ��µ�i�ڵ㣬����û�пռ�������˳���
	if (!(name_block = bread(inode->i_dev, inode->i_zone[0]))) {
		iput(dir);
		inode->i_nlinks--;
		iput(inode);
		return -ERROR;
	}
	// �������ǿ��԰ѷ������������ַ�����������̿����ˡ��̿鳤��Ϊ1024�ֽڣ����Ĭ�Ϸ����������������Ҳֻ����1024�ֽڡ����ǰ���
	// ���ռ��еķ����������ַ������Ƶ��̿����ڵĻ�����У����û�������޸ı�־��Ϊ��ֹ�û��ṩ���ַ���û����NULL��β�������ڻ����
	// ���������һ���ֽڴ�����һ��NULL��Ȼ���ͷŸû���飬������i�ڵ��Ӧ�ļ������ݳ��ȵ��ڷ����������ַ������ȣ�����i�ڵ����޸�
	// ��־��
	i = 0;
	while (i < 1023 && (c = get_fs_byte(oldname++)))
		name_block->b_data[i++] = c;
	name_block->b_data[i] = 0;
	name_block->b_dirt = 1;
	brelse(name_block);
	inode->i_size = i;
	inode->i_dirt = 1;
	// Ȼ����������һ��·����ָ���ķ����������Ƿ��Ѿ����ڡ����Ѿ��������ܴ���ͬ��Ŀ¼��i�ڵ㡣�����Ӧ���������ļ����Ѿ����ڣ���
	// �ͷŰ�����Ŀ¼��Ļ������飬��λ�������i�ڵ����Ӽ�������ʩ��Ŀ¼��i�ڵ㣬�����ļ��Ѿ����ڵĳ������˳���
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		inode->i_nlinks--;
		iput(inode);
		brelse(bh);
		iput(dir);
		return -EEXIST;
	}
	// ����������ָ��Ŀ¼�������һ��Ŀ¼����ڴ���½����������ļ�����i�ڵ�ź�Ŀ¼�������ʧ�ܣ�������Ŀ¼��ĸ��ٻ�����ָ��Ϊ
	// NULL������Ż�Ŀ¼��i�ڵ㣻�������i�ڵ��������Ӽ�����λ�����Żظ�i�ڵ㡣���س������˳���
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		inode->i_nlinks--;
		iput(inode);
		iput(dir);
		return -ENOSPC;
	}
	// ��������Ŀ¼���i�ڵ��ֶε�����i�ڵ�ţ����ø��ٻ�������޸ı�־���ͷŸ��ٻ���飬�Ż�Ŀ¼���µ�i�ڵ㣬��󷵻�0���ɹ�����
	de->inode = inode->i_num;
	bh->b_dirt = 1;
	brelse(bh);
	iput(dir);
	iput(inode);
	return 0;
}

// Ϊ�ļ�����һ���ļ���Ŀ¼�
// Ϊһ���Ѵ��ڵ��ļ�����һ�������ӣ�Ҳ��ΪӲ���� - hard link����
// ������oldname - ԭ·������newname - �µ�·������
// ���أ����ɹ��򷵻�0,���򷵻س���š�
int sys_link(const char * oldname, const char * newname)
{
	struct dir_entry * de;
	struct m_inode * oldinode, * dir;
	struct buffer_head * bh;
	const char * basename;
	int namelen;

	// ���ȶ�ԭ�ļ���������Ч����֤����Ӧ�ô��ڲ��Ҳ���һ��Ŀ¼��������������ȡԭ�ļ�·������Ӧ��i�ڵ�oldinode�����Ϊ0����
	// ��ʾ�������س���š����ԭ·������Ӧ����һ��Ŀ¼������Żظ�i�ڵ㣬Ҳ���س���š�
	oldinode = namei(oldname);
	if (!oldinode)
		return -ENOENT;
	if (S_ISDIR(oldinode->i_mode)) {
		iput(oldinode);
		return -EPERM;
	}
	// Ȼ�������·���������Ŀ¼��i�ڵ�dir�������������ļ������䳤�ȡ����Ŀ¼��i�ڵ�û���ҵ�����Ż�ԭ·������i�ڵ㣬��
	// �س���š������·�����в������ļ�������Ż�ԭ·����i�ڵ����·����Ŀ¼��i�ڵ㣬���س���š�
	dir = dir_namei(newname, &namelen, &basename, NULL);
	if (!dir) {
		iput(oldinode);
		return -EACCES;
	}
	if (!namelen) {
		iput(oldinode);
		iput(dir);
		return -EPERM;
	}
	// ���ǲ��ܿ��豸����Ӳ���ӡ���������·��������Ŀ¼���豸����ԭ·�������豸�Ų�һ������Ż���·����Ŀ¼��i�ڵ��ԭ·����
	// ��i�ڵ㣬���س���š����⣬����û�û������Ŀ¼��д��Ȩ�ޣ���Ҳ���ܽ������ӣ����ǷŻ���·����Ŀ¼��i�ڵ��ԭ·������i�ڵ�
	// ���س���š�
	if (dir->i_dev != oldinode->i_dev) {
		iput(dir);
		iput(oldinode);
		return -EXDEV;
	}
	if (!permission(dir, MAY_WRITE)) {
		iput(dir);
		iput(oldinode);
		return -EACCES;
	}
	// ���ڲ�ѯ����·�����Ƿ��Ѿ����ڣ����������Ҳ���ܽ������ӡ������ͷŰ������Ѵ���Ŀ¼��ĸ��ٻ���飬�Ż���·����Ŀ¼��i�ڵ�
	// ��ԭ·������i�ڵ㣬���س���š�
	bh = find_entry(&dir, basename, namelen, &de);
	if (bh) {
		brelse(bh);
		iput(dir);
		iput(oldinode);
		return -EEXIST;
	}
	// �������������������ˣ�������������Ŀ¼�����һ��Ŀ¼���ʧ����Żظ�Ŀ¼��i�ڵ��ԭ·������i�ڵ㣬���س���š������ʼ
	// ���ø�Ŀ¼���i�ڵ�ŵ���ԭ·������i�ڵ�ţ����ð���������Ŀ¼�Ļ�������޸ı�־���ͷŸû���飬�Ż�Ŀ¼��i�ڵ㡣
	bh = add_entry(dir, basename, namelen, &de);
	if (!bh) {
		iput(dir);
		iput(oldinode);
		return -ENOSPC;
	}
	de->inode = oldinode->i_num;
	bh->b_dirt = 1;
	brelse(bh);
	iput(dir);
	// �ٽ�ԭ�ڵ�����Ӽ�����1,�޸���ı�ʱ��Ϊ��ǰʱ�䣬������i�ڵ����޸ı�־�����Ż�ԭ·������i�ڵ㣬������0���ɹ�����
	oldinode->i_nlinks++;
	oldinode->i_ctime = CURRENT_TIME;
	oldinode->i_dirt = 1;
	iput(oldinode);
	return 0;
}



