/*
 *  linux/fs/bitmap.c
 *
 *  (C) 1991  Linus Torvalds
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */
/* bitmap.c�����д���i�ڵ�ʹ��̿�λͼ�Ĵ��� */
#include <string.h>
#include <linux/sched.h>							// ���ȳ���ͷ�ļ�,��������ṹtask_struct,����0����.
#include <linux/kernel.h>

// ��ָ����ַ(addr)����һ��1024�ֽ��ڴ�����.
// ����:eax = 0;ecx = ���ֽ�Ϊ��λ�����ݿ鳤��(BLOCK_SIZE/4);edi = ָ����ʼ��ַaddr.
#define clear_block(addr) \
__asm__("cld\n\t"               		/* �巽��λ */\
		"rep\n\t" 						/* �ظ�ִ�д洢����(0) */\
		"stosl" \
		::"a" (0), "c" (BLOCK_SIZE / 4), "D" ((long) (addr)):)

// ��ָ����ַ��ʼ�ĵ�nr��λƫ�ƴ���λ��λ(nr�ɴ���32!).����ԭλֵ.
// ����:%0 - eax(����ֵ);%1 - eax(0); %2 - nr,λƫ��ֵ;%3 -(addr),addr������.
// ��20�ж�����һ���ֲ��Ĵ�������res.�ñ�������������ָ����eax�Ĵ�����,�Ա��ڸ�Ч���ʺͲ���.���ֶ�������ķ�
// ����Ҫ������Ƕ��������.�����궨����һ�������ʽ,�ñ��ʽֵ�����res��ֵ.��21���ϵ�btslָ�����ڲ��Բ�
// ����λ(Bit Test and Set).�ѻ���ַ(%3)��λƫ��ֵ(%2)��ָ����λֵ�ȱ��浽��λ��־CF��,Ȼ�����ø�λΪ1.
// ָ��setb���ڸ��ݽ�λ��־CF���ò�����(%al).���CF=1��%al=1,����%al=0.
#define set_bit(nr, addr) ({\
register int res __asm__("ax"); \
__asm__ __volatile__("btsl %2, %3\n\tsetb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

// ��λָ����ַ��ʼ�ĵ�nrλƫ�ƴ���λ������ԭλֵ�ķ��롣
// ���룺%0 -eax������ֵ����%1 -eax��0����%2 -nr��λƫ��ֵ��%3 -��addr����addr�����ݡ�btrlָ��������Բ�
// ��λλ��Bit Test and Reset�����������������btsl���ƣ����Ǹ�λָ��λ��ָ��setnb���ڸ��ݽ�λ��־CF����
// ��������%al�������CF = 1��%al = 0,����%al = 1��
#define clear_bit(nr, addr) ({\
register int res __asm__("ax"); \
__asm__ __volatile__("btrl %2, %3\n\tsetnb %%al": \
"=a" (res):"0" (0), "r" (nr), "m" (*(addr))); \
res;})

// ��addr��ʼѰ�ҵ�1��0ֵλ.
// ����:%0 - ecx(����ֵ);%1 - ecx(0);%2 - esi(addr).
// ��addrָ����ַ��ʼ��λͼ��Ѱ�ҵ�1����0��λ,���������addr��λƫ��ֵ����.addr�ǻ�����������ĵ�ַ,ɨ��Ѱ��
// �ķ�Χ��1024�ֽ�(8192λ).
#define find_first_zero(addr) ({ \
int __res; \
__asm__(\
	"cld\n"                         				/* �巽��λ */\
	"1:\tlodsl\n\t" 								/* ȡ[esi]->eax */\
	"notl %%eax\n\t"                				/* eax��ÿλȡ�� */\
	"bsfl %%eax, %%edx\n\t"          				/* ��λ0ɨ��eax����1�ĵ�1��λ,��ƫ��ֵ->edx */\
	"je 2f\n\t"                     				/* ���eax��ȫ��0,����ǰ��ת�����2�� */\
	"addl %%edx, %%ecx\n\t"          				/* ƫ��ֵ����ecx(ecx��λͼ�׸�0ֵλ��ƫ��ֵ) */\
	"jmp 3f\n"                      				/* ��ǰ��ת�����3��(����) */\
	"2:\taddl $32, %%ecx\n\t"        				/* δ�ҵ�0ֵλ,��ecx��1�����ֵ�λƫ����32 */\
	"cmpl $8192, %%ecx\n\t"          				/* �Ѿ�ɨ����8192λ(1024�ֽ�)����? */\
	"jl 1b\n"                       				/* ����û��ɨ����1������,����ǰ��ת�����1�� */\
	"3:"                            				/* ����.��ʱecx����λƫ���� */\
	:"=c" (__res):"c" (0), "S" (addr):"ax", "dx"); \
__res;})

// �ͷ��豸dev���������е��߼���block��
// ��λָ���߼���block��Ӧ���߼���λͼλ���ɹ��򷵻�1,���򷵻�0.
// ������dev���豸�ţ�block���߼���ţ��̿�ţ���
int free_block(int dev, int block)
{
	struct super_block * sb;
	struct buffer_head * bh;

	// ����ȡ�豸dev���ļ�ϵͳ�ĳ�������Ϣ������������������ʼ�߼���ź��ļ�ϵͳ���߼���������Ϣ�жϲ���block����Ч
	// �ԡ����ָ���豸�����鲻���ڣ������ͣ�������߼����С��������������1���߼���Ż��ߴ����豸�����߼�������Ҳ��
	// ��ͣ����
	if (!(sb = get_super(dev)))             						// fs/super.c
		panic("trying to free block on nonexistent device");
	if (block < sb->s_firstdatazone || block >= sb->s_nzones)
		panic("trying to free block not in datazone");
	bh = get_hash_table(dev, block);
	// Ȼ���hash����Ѱ�Ҹÿ����ݡ����ҵ������ж�����Ч�ԣ��������޸ĺ͸��±�־���ͷŸ����ݿ顣�öδ������Ҫ��;�����
	// ���߼���Ŀǰ�����ڸ��ٻ������У����ͷŶ�Ӧ�Ļ���顣
	if (bh) {
		if (bh->b_count > 1) {          							// ������ô�������1,�����brelse()��
			brelse(bh);             								// b_count--���˳����ÿ黹�����á�
			return 0;
		}
		bh->b_dirt = 0;                   							// ����λ���޸ĺ��Ѹ��±�־��
		bh->b_uptodate = 0;
		if (bh->b_count)                							// ����ʱb_countΪ1,�����brelse()�ͷ�֮��
			brelse(bh);
	}
	// �������Ǹ�λblock���߼���λͼ�е�λ����0�����ȼ���block����������ʼ����������߼���ţ���1��ʼ��������Ȼ����߼�
	// �飨���飩λͼ���в�������λ��Ӧ��λ�������Ӧλԭ������0,�����ͣ��������1���������1024�ֽڣ���8192λ�����
	// block/8192���ɼ����ָ����block���߼�λͼ�е��ĸ����ϡ���block&8191���Եõ�block���߼���λͼ��ǰ���е�λƫ��
	// λ�á�
	block -= sb->s_firstdatazone - 1 ;
	if (clear_bit(block & 8191, sb->s_zmap[block / 8192]->b_data)) {
		printk("block (%04x:%d) ", dev, block + sb->s_firstdatazone - 1);
		printk("free_block: bit already cleared\n");
	}
	// �������Ӧ�߼���λͼ���ڻ��������޸ı�־��
	sb->s_zmap[block / 8192]->b_dirt = 1;
	return 1;
}

// ���豸����һ���߼���(�̿�,����).
// ��������ȡ���豸�ĳ�����,���ڳ������е��߼���λͼ��Ѱ�ҵ�һ��0ֵλ(����һ�������߼���).Ȼ����λ��Ӧ�߼������߼�λͼ
// �е�λ.����Ϊ���߼����ڻ�������ȡ��һ���Ӧ�����.��󽫸û��������,���������Ѹ��±�־�����޸ı�־.�������߼����.
// ����ִ�гɹ��򷵻��߼����(�̿��),���򷵻�0.
int new_block(int dev)
{
	struct buffer_head * bh;
	struct super_block * sb;
	int i,j;

	// ���Ȼ�ȡ�豸dev�ĳ�����.���ָ���豸�ĳ����鲻����,�����ͣ��.Ȼ��ɨ���ļ�ϵͳ��8���߼���λͼ,Ѱ���׸�0ֵλ,��Ѱ��
	// �����߼���,��ȡ���ø��߼���Ŀ��.���ȫ��ɨ����8���߼���λͼ������λ(i >=8 �� j >= 8192)��û���ҵ�0ֵλ����λͼ
	// ���ڵĻ����ָ����Ч(bn = NULL)�򷵻�0�˳�(û�п����߼���).
	if (!(sb = get_super(dev)))
		panic("trying to get new block from nonexistant device");
	j = 8192;
	for (i = 0 ; i < 8 ; i++)
		if (bh = sb->s_zmap[i])
			if ((j = find_first_zero(bh->b_data)) < 8192)
				break;
	if (i >= 8 || !bh || j >= 8192)
		return 0;
	// ���������ҵ������߼���j��Ӧ�߼���λͼ�е�λ.����Ӧλ�Ѿ���λ,�����ͣ��.�����ô���λͼ�Ķ�Ӧ�����������޸ı�־.��Ϊ
	// �߼���λͼ����ʾ�������������߼����ռ�����,���߼���λͼ��λƫ��ֵ��ʾ����������ʼ������Ŀ��,���������Ҫ��������
	// ����1���߼���Ŀ��,��jת�����߼����.��ʱ������߼�����ڸ��豸�ϵ����߼�����,��˵��ָ���߼����ڶ�Ӧ�豸�ϲ�����.
	// ����ʧ��,����0�˳�.
	if (set_bit(j, bh->b_data))
		panic("new_block: bit already set");
	bh->b_dirt = 1;
	j += i * 8192 + sb->s_firstdatazone - 1;
	if (j >= sb->s_nzones)
		return 0;
	// Ȼ���ڸ��ٻ�������Ϊ���豸��ָ�����߼����ȡ��һ�������,�����ػ����ͷָ��.
	// ��Ϊ��ȡ�õ��߼��������ô���һ��Ϊ1(getblk()�л�����),�������Ϊ1��ͣ��.������߼�������,���������Ѹ��±�־����
	// �޸ı�־.Ȼ���ͷŶ�Ӧ�����,�����߼����.
	if (!(bh = getblk(dev, j)))
		panic("new_block: cannot get block");
	if (bh->b_count != 1)
		panic("new block: count is != 1");
	clear_block(bh->b_data);
	bh->b_uptodate = 1;
	bh->b_dirt = 1;
	brelse(bh);
	return j;
}

// �ͷ�ָ����i�ڵ㡣
// �ú��������жϲ���������i�ڵ�ŵ���Ч�ԺͿ��ͷ��ԡ���i�ڵ���Ȼ��ʹ�������ܱ��ͷš�Ȼ�����ó�������Ϣ��i�ڵ�λͼ����
// ��������λi�ڵ�Ŷ�Ӧ��i�ڵ�λͼ��λ�������i�ڵ�ṹ��
void free_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;

	// �����жϲ�����������Ҫ�ͷŵ�i�ڵ���Ч�Ի�Ϸ��ԡ����i�ڵ�ָ��=NULL�����˳���
	// ���i�ڵ��ϵ��豸���ֶ�Ϊ0,˵���ýڵ�û��ʹ�á�������0��ն�Ӧi�ڵ���ռ�ڴ���������memset()������include/string.h
	// ���������ʾ��0��дinodeָ��ָ������������sizeof(*inode)���ڴ�顣
	if (!inode)
		return;
	if (!inode->i_dev) {
		memset(inode, 0, sizeof(*inode));
		return;
	}
	// �����i�ڵ㻹�������������ã����ͷţ�˵���ں������⣬ͣ��������ļ���������Ϊ0,���ʾ���������ļ�Ŀ¼����ʹ�øýڵ㣬
	// ���Ҳ��Ӧ�ͷţ���Ӧ�÷Żصȡ�
	if (inode->i_count > 1) {
		printk("trying to free inode with count=%d\n", inode->i_count);
		panic("free_inode");
	}
	if (inode->i_nlinks)
		panic("trying to free inode with links");
	// ���ж���i�ڵ�ĺ�����֮�����ǿ�ʼ�����䳬������Ϣ����i�ڵ�λͼ���в���������ȡi�ڵ������豸�ĳ����飬�����豸�Ƿ���ڡ�
	// Ȼ���ж�i�ڵ�ŵķ�Χ�Ƿ���ȷ�����i�ڵ�ŵ���0����ڸ��豸��i�ڵ������������0��i�ڵ㱣��û��ʹ�ã��������i�ڵ��Ӧ
	// �Ľڵ�λͼ�����ڣ��������Ϊһ��������i�ڵ�λͼ��8192����λ�����i_num>>13����i_num/8192�����Եõ���ǰi�ڵ����
	// �ڵ�s_imap[]��������̿顣
	if (!(sb = get_super(inode->i_dev)))
		panic("trying to free inode on nonexistent device");
	if (inode->i_num < 1 || inode->i_num > sb->s_ninodes)
		panic("trying to free inode 0 or nonexistant inode");
	if (!(bh = sb->s_imap[inode->i_num >> 13]))
		panic("nonexistent imap in superblock");
	// �������Ǹ�λi�ڵ��Ӧ�Ľڵ�λͼ�е�λ�������λ�Ѿ�����0,����ʾ��������Ϣ�������i�ڵ�λͼ���ڻ��������޸ı�־�������
	// ��i�ڵ�ṹ��ռ�ڴ�����
	if (clear_bit(inode->i_num & 8191, bh->b_data))
		printk("free_inode: bit already cleared.\n\r");
	bh->b_dirt = 1;
	memset(inode, 0, sizeof(*inode));
}

// Ϊ�豸dev����һ����i�ڵ㡣��ʼ�������ظ���i�ڵ��ָ�롣
// ���ڴ�i�ڵ���л�ȡһ������i�ڵ�������i�ڵ�λͼ����һ������i�ڵ㡣
struct m_inode * new_inode(int dev)
{
	struct m_inode * inode;
	struct super_block * sb;
	struct buffer_head * bh;
	int i, j;

	// ���ȴ�����i�ڵ��inode_table���л�ȡһ������i�ڵ������ȡָ���豸�ĳ�����ṹ��Ȼ��ɨ�賬������8��i�ڵ�λͼ��
	// Ѱ�ҵ�1��0λ��Ѱ�ҿ��нڵ㣬��ȡ���ø�i�ڵ�Ľڵ�š����ȫ��ɨ���껹û�ҵ�������λͼ���ڵĻ������Ч��bh = NULL��
	// ��Ż���ǰ�����i�ڵ���е�i�ڵ㣬�����ؿ�ָ���˳���û�п���i�ڵ㣩��
	if (!(inode = get_empty_inode()))
		return NULL;
	if (!(sb = get_super(dev)))
		panic("new_inode with unknown device");
	j = 8192;
	for (i = 0 ; i < 8 ; i++)
		if (bh = sb->s_imap[i])
			if ((j = find_first_zero(bh->b_data)) < 8192)
				break;
	if (!bh || j >= 8192 || j + i * 8192 > sb->s_ninodes) {
		iput(inode);
		return NULL;
	}
	// ���������Ѿ��ҵ��˻�δʹ�õ�i�ڵ��j��������λi�ڵ�j��Ӧ��i�ڵ�λͼ��Ӧ����λ������Ѿ���λ���������Ȼ����i�ڵ�
	// λͼ���ڻ�������޸ı�־������ʼ����i�ڵ�ṹ��i_ctime��i�ڵ����ݸı�ʱ�䣩��
	if (set_bit(j, bh->b_data))
		panic("new_inode: bit already set");
	bh->b_dirt = 1;
	inode->i_count = 1;               										// ���ü�����
	inode->i_nlinks = 1;              										// �ļ�Ŀ¼����������
	inode->i_dev = dev;               										// i�ڵ����ڵ��豸�š�
	inode->i_uid = current->euid;     										// i�ڵ������û�id��
	inode->i_gid = current->egid;     										// ��id��
	inode->i_dirt = 1;                										// ���޸ı�־��λ��
	inode->i_num = j + i * 8192;      										// ��Ӧ�豸�е�i�ڵ�š�
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;        // ����ʱ�䡣
	return inode;                   										// ���ظ�i�ڵ�ָ�롣
}

