/*
 *  linux/mm/swap.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This file should contain most things doing the swapping from/to disk.
 * Started 18.12.91
 */
/*
 * ������Ӧ�ð������󲿷�ִ���ڴ潻���Ĵ���(���ڴ浽���̻�֮).
 * ��91��12��18�տ�ʼ����.
 */

#include <string.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <linux/kernel.h>

// ÿ���ֽ�8λ,���1ҳ(4096B)����32768��λ.��1��λ��Ӧ1ҳ�ڴ�,�����ɹ���32768��ҳ��,��Ӧ128MB�ڴ�����.
#define SWAP_BITS (4096 << 3)

// λ������.ͨ��������ͬ��"op",�ɶ����ָ������λ���в���,���û�������ֲ���.
// ����addr��ָ�����Ե�ַ;nr��ָ����ַ����ʼ�ı���λƫ��λ.�ú�Ѹ�����ַaddr����nr������λ��ֵ�����λ��־,���û�λ�ñ���λ�����ؽ�λ��־
// ֵ(��ԭλֵ).
// ��25���ϵ�һ��ָ����"op"�ַ��Ĳ�ͬ������γɲ�ͬ��ָ��:
// ��op=""ʱ,����ָ��bt - (Bit Test)���Բ���ԭֵ���ý�λλ.
// ��op="s"ʱ,����ָ��bts - (Bit Test and Set)���ñ���λֵ����ԭ�����ý�λλ.
// ��op="r"ʱ,����ָ��btr - (Bit Test and Reset)��λ����λֵ��ԭֵ���ý�λλ.
// ����:%0 - (����ֵ);%1 - λƫ��(nr);%2 - ��ַ(addr);%3 - �Ӳ����Ĵ�����ֵ(0).
// ��Ƕ������ѻ���ַ(%2)�ͱ���ƫ��ֵ(%1)��ָ���ı���λֵ�ȱ��浽��λ��־CF��,Ȼ������(��λ)�ñ���λ.ָ��abcl�Ǵ���λλ��,���ڸ��ݽ�λλCF
// ���ò�����(%0).���CF=1�򷵻ؼĴ���ֵ=1,���򷵻ؼĴ���ֵ=0.
#define bitop(name, op) \
static inline int name(char * addr, unsigned int nr) \
{ \
int __res; \
__asm__ __volatile__("bt" op " %1, %2; adcl $0, %0" \
:"=g" (__res) \
:"r" (nr),"m" (*(addr)),"0" (0)); \
return __res; \
}

// ������ݲ�ͬ��op�ַ�����3����Ƕ����.
bitop(bit, "")								// ������Ƕ����bit(char * addr, unsigned int nr).
bitop(setbit, "s")							// ������Ƕ����setbit(char * addr, unsigned int nr).
bitop(clrbit, "r")							// ������Ƕ����clrbit(char * addr, unsigned int nr).

static char * swap_bitmap = NULL;
int SWAP_DEV = 0;		// �ں˳�ʼ��ʱ���õĽ����豸��.

/*
 * We never page the pages in task[0] - kernel memory.
 * We page all other pages.
 */
/*
 * ���ǴӲ���������0(task[0])��ҳ��,���������ں�ҳ��,����ֻ������ҳ����н�������.
 */
// ��1�������ڴ�ҳ��,��������0ĩ��(64MB)����ʼ�������ڴ�ҳ��.
#define FIRST_VM_PAGE (TASK_SIZE >> 12)							// = 64MB/4KB = 16384
#define LAST_VM_PAGE (1024 * 1024)								// = 4GB/4KB = 1048576 4G��Ӧ��ҳ��
#define VM_PAGES (LAST_VM_PAGE - FIRST_VM_PAGE)					// = 1032192(��0��ʼ��)(���ܵ�ҳ������ȥ��0�������ҳ����)

// ����1ҳ����ҳ��.
// ɨ����������ӳ��λͼ(����Ӧλͼ�����λ0����),����ֵΪ1�ĵ�һ������λ��,��Ŀǰ���еĽ���ҳ���.�������ɹ��򷵻ؽ���ҳ���,���򷵻�0.
static int get_swap_page(void)
{
	int nr;

	if (!swap_bitmap)
		return 0;
	for (nr = 1; nr < 32768 ; nr++)
		if (clrbit(swap_bitmap, nr))
			return nr;					// ����Ŀǰ���еĽ���ҳ���.
	return 0;
}

// �ͷŽ����豸��ָ���Ľ���ҳ��.
// �ڽ���λͼ������ָ��ҳ��Ŷ�Ӧ��λ(��1).��ԭ����λ�͵���1,���ʾ�����豸��ԭ����ҳ���û�б�ռ��,����λͼ����.������ʾ������Ϣ������.
// ����ָ������ҳ���.
void swap_free(int swap_nr)
{
	if (!swap_nr)
		return;
	if (swap_bitmap && swap_nr < SWAP_BITS)
		if (!setbit(swap_bitmap, swap_nr))
			return;
	printk("Swap-space bad (swap_free())\n\r");
	return;
}

// ��ָ��ҳ�潻�����ڴ���
// ��ָ��ҳ����Ķ�Ӧҳ��ӽ����豸�ж��뵽��������ڴ�ҳ����.�޸Ľ���λͼ�ж�Ӧλ(��λ),ͬʱ�޸�ҳ��������,����ָ����ڴ�ҳ��,��������Ӧ��־.
void swap_in(unsigned long *table_ptr)
{
	int swap_nr;
	unsigned long page;

	// ���ȼ�齻��λͼ�Ͳ�����Ч��.�������λͼ������,����ָ��ҳ�����Ӧ��ҳ���Ѵ������ڴ���,���߽���ҳ���Ϊ0,����ʾ������Ϣ���˳�.�����ѷŵ�����
	// �豸��ȥ���ڴ�ҳ��,��Ӧҳ�����д�ŵ�Ӧ�ǽ���ҳ���*2,��(swap_nr << 1).
	if (!swap_bitmap) {
		printk("Trying to swap in without swap bit-map");
		return;
	}
	if (1 & *table_ptr) {
		printk("trying to swap in present page\n\r");
		return;
	}
	swap_nr = *table_ptr >> 1;
	if (!swap_nr) {
		printk("No swap page in swap_in\n\r");
		return;
	}
	// Ȼ������һҳ�����ڴ沢�ӽ����豸�ж���ҳ���Ϊswap_nr��ҳ��.�ڰ�ҳ�潻��������,�Ͱѽ���λͼ�ж�Ӧ����λ��λ.�����ԭ��������λ��,˵���˴����ٴ�
	// �ӽ����豸�ж�����ͬ��ҳ��,������ʾһ�¾�����Ϣ.�����ҳ��ָ�������ҳ��,������ҳ�����޸�,�û��ɶ�д�ʹ��ڱ�־(Dirty,U/S,R/W,P).
	if (!(page = get_free_page()))
		oom();
	read_swap_page(swap_nr, (char *) page);
	if (setbit(swap_bitmap, swap_nr))
		printk("swapping in multiply from same page\n\r");
	*table_ptr = page | (PAGE_DIRTY | 7);
}

// ���԰�ҳ�潻����ȥ.
// ��ҳ��û�б��޸Ĺ��򲻱ر����ڽ����豸��,��Ϊ��Ӧҳ�滹������ֱ�Ӵ���Ӧӳ���ļ��ж���.���ǿ���ֱ���ͷŵ���Ӧ����ҳ������.���������һ������ҳ���,Ȼ��
// ��ҳ�潻����ȥ.��ʱ����ҳ���Ҫ�����ڶ�Ӧҳ������,��������Ҫ����ҳ�������λP=0.������ҳ����ָ��.ҳ�滻���ͷųɹ�����1,���򷵻�0.
int try_to_swap_out(unsigned long * table_ptr)
{
	unsigned long page;
	unsigned long swap_nr;

	// �����жϲ�������Ч��.����Ҫ������ȥ���ڴ�ҳ�沢������(�����Ч),�򼴿��˳�.��ҳ����ָ��������ҳ���ַ���ڷ�ҳ������ڴ�߶�PAGING_MEMORY(15MB),
	// Ҳ�˳�.
	page = *table_ptr;
	if (!(PAGE_PRESENT & page))
		return 0;
	if (page - LOW_MEM > PAGING_MEMORY)
		return 0;
	// ���ڴ�ҳ���ѱ��޸Ĺ�,���Ǹ�ҳ���Ǳ������,��ôΪ���������Ч��,����ҳ�治�˱�������ȥ,����ֱ���˳�,��������0.���������һ����ҳ���,������������ҳ��
	// ����,Ȼ���ҳ�潻����ȥ���ͷŶ�Ӧ�����ڴ�ҳ��.
	if (PAGE_DIRTY & page) {
		page &= 0xfffff000;									// ȡ����ҳ���ַ.
		if (mem_map[MAP_NR(page)] != 1)
			return 0;
		if (!(swap_nr = get_swap_page()))					// ���뽻��ҳ���.
			return 0;
		// ����Ҫ�����豸�е�ҳ��,��Ӧҳ�����н���ŵ���(swap_nr << 1).��2(����1λ)��Ϊ�˿ճ�ԭ��ҳ����Ĵ���λ(P).ֻ�д���λP=0����ҳ�������ݲ�Ϊ0��ҳ��Ż���
		// �����豸��.Intel�ֲ�����ȷָ��,��һ������Ĵ���λP=0ʱ(��Чҳ����),��������λ(λ31-1)�ɹ�����ʹ��.����д����ҳ����write_swap_page(nr,buffer)��
		// ����Ϊll_rw_page(WRITE,SWAP_DEV,(nr),(buffer)).
		*table_ptr = swap_nr << 1;
		invalidate();										// ˢ��CPUҳ�任���ٻ���.
		write_swap_page(swap_nr, (char *) page);
		free_page(page);
		return 1;
	}
	// �������ҳ��û���޸Ĺ�.��ô�Ͳ��ý�����ȥ,��ֱ���ͷż���.
	*table_ptr = 0;
	invalidate();
	free_page(page);
	return 1;
}

/*
 * Ok, this has a rather intricate logic - the idea is to make good
 * and fast machine code. If we didn't worry about that, things would
 * be easier.
 */
/*
 * OK,�����������һ���ǳ����ӵ��߼�,���ڲ����߼��Ժò����ٶȿ�Ļ�����.������ǲ��Դ˲��ĵĻ�,��ô������ܸ�����Щ.
 */
// ���ڴ�ҳ��ŵ������豸��.
// �����Ե�ַ64MB��Ӧ��Ŀ¼��(FIRST_VM_PAGE>>10)��ʼ,��������4GB���Կռ�,����ЧҳĿ¼����ҳ��ָ���������ڴ�ҳ��ִ�н���
// �������豸��ȥ�ĳ���.һ���ɹ��ؽ�����һ��ҳ��,�ͷ���-1.���򷵻�0.�ú�������get_free_page()�б�����.
int swap_out(void)
{
	static int dir_entry = FIRST_VM_PAGE >> 10;	// ������1�ĵ�1��Ŀ¼������.
	static int page_entry = -1;
	int counter = VM_PAGES;						// ��ʾ��ȥ����0������������������ҳ��Ŀ
	int pg_table;

	// ��������ҳĿ¼��,���Ҷ���ҳ����ڵ�ҳĿ¼��pg_table.�ҵ����˳�ѭ��,�������ҳĿ¼������Ӧʣ�����ҳ������counter,Ȼ�����
	// �����һ��Ŀ¼��.��ȫ�������껹û���ҵ��ʺϵ�(���ڵ�)ҳĿ¼��,����������.
	while (counter > 0) {
		pg_table = pg_dir[dir_entry];			// ҳĿ¼������.
		if (pg_table & 1)
			break;
		counter -= 1024;						// 1��ҳ���Ӧ1024��ҳ֡
		dir_entry++;							// ��һĿ¼��.
		// �������4GB��1024��ҳĿ¼�����������ֻص���1���������¿�ʼ���
		if (dir_entry >= 1024)
			dir_entry = FIRST_VM_PAGE >> 10;
	}
	// ��ȡ�õ�ǰĿ¼���ҳ��ָ���,��Ը�ҳ���е�����1024��ҳ��,��һ���ý�������try_to_swap_out()���Խ�����ȥ.һ��ĳ��ҳ��ɹ������������豸
	// �оͷ���1.��������Ŀ¼�������ҳ���ѳ���ʧ��,����ʾ"�����ڴ�����"�ľ���,������0.
	pg_table &= 0xfffff000;						// ҳ��ָ��(��ַ)(ҳ����)
	while (counter-- > 0) {
		page_entry++;
		// ����Ѿ����Դ����굱ǰҳ�������û���ܹ��ɹ��ؽ�����һ��ҳ��,����ʱҳ�����������ڵ���1024,����ͬǰ���135-143��ִ����ͬ�Ĵ�����ѡ��һ��
		// ����ҳ����ڵ�ҳĿ¼��,��ȡ����Ӧ����ҳ��ָ��.
		if (page_entry >= 1024) {
			page_entry = 0;
		repeat:
			dir_entry++;
			if (dir_entry >= 1024)
				dir_entry = FIRST_VM_PAGE >> 10;
			pg_table = pg_dir[dir_entry];		// ҳĿ¼������.
			if (!(pg_table & 1))
				if ((counter -= 1024) > 0)
					goto repeat;
				else
					break;
			pg_table &= 0xfffff000;				// ҳ��ָ��.
		}
		if (try_to_swap_out(page_entry + (unsigned long *) pg_table))
			return 1;
        }
	printk("Out of swap-memory\n\r");
	return 0;
}

/*
 * Get physical address of first (actually last :-) free page, and mark it
 * used. If no free pages left, return 0.
 */
/*
 * ��ȡ�׸�(ʵ���������1��:-)����ҳ��,����־Ϊ��ʹ��.���û�п���ҳ��,�ͷ���0.
 */
// �����ڴ���������1ҳ��������ҳ��.
// ����Ѿ�û�п�������ҳ��,�����ִ�н�������.Ȼ���ٴ�����ҳ��.
// ����:%1(ax=0) - 0;%2(LOW_MEM)�ڴ��ֽ�λͼ�������ʼλ��;%3(cx=PAGING_PAGES);%4(edi=mem_map+PAGING_PAGES-1).
// ���:����%0(ax=����ҳ����ʼ��ַ).����������ҳ��������ַ.
// ����%4�Ĵ���ʵ��ָ��mem_map[]�ڴ��ֽ�λͼ�����һ���ֽ�.��������λͼĩ�˿�ʼ��ǰɨ������ҳ���־(ҳ������ΪPAGING_AGES),����ҳ�����
// (�ڴ�λͼ�ֽ�Ϊ0)�򷵻�ҳ���ַ.ע��!������ֻ��ָ�������ڴ�����һҳ��������ҳ��,����û��ӳ�䵽ĳ�����̵ĵ�ַ�ռ���ȥ.�����put_page()����
// �����ڰ�ָ��ҳ��ӳ�䵽ĳ�����̵ĵ�ַ�ռ���.��Ȼ�����ں�ʹ�ñ�����������Ҫ��ʹ��put_page()����ӳ��,��Ϊ�ں˴�������ݿռ�(16MB)�Ѿ��Ե�
// ��ӳ�䵽�����ַ�ռ�.
unsigned long get_free_page(void)
{
register unsigned long __res;

// �������ڴ�ӳ���ֽ�λͼ�в���ַΪ0���ֽ���,Ȼ��Ѷ�Ӧ�����ڴ�ҳ������.����õ���ҳ���ַ����ʵ�������ڴ�����������Ѱ��.���û���ҵ�����ҳ����
// ȥ����ִ�н�������,�����²���.��󷵻ؿ�������ҳ���ַ.
repeat:
	__asm__(
		"std ; repne ; scasb						/* �÷���λ,al(0)���Ӧÿ��ҳ���(di)���ݱȽ�, */\n\t"
		"jne 1f										/* ���û�е���0���ֽ�,����ת����(����0). */\n\t"
		"movb $1, 1(%%edi)							/* 1 =>[1+edi],����Ӧҳ���ڴ�ӳ�����λ��1. */\n\t"
		"sall $12, %%ecx							/* ҳ����*4K = ���ҳ����ʼ��ַ. */\n\t"
		"addl %2, %%ecx								/* �ټ��ϵͶ��ڴ��ַ,��ҳ��ʵ��������ʼ��ַ. */\n\t"
		"movl %%ecx, %%edx							/* ��ҳ��ʵ����ʼ��ַ->edx�Ĵ���. */\n\t"
		"movl $1024, %%ecx							/* �Ĵ���ecx�ü���ֵ1024. */\n\t"
		"leal 4092(%%edx), %%edi					/* ��4092+edx��λ��->edi(��ҳ���ĩ��). */\n\t"
		"rep ; stosl								/* ��edi��ָ�ڴ�����(������,������ҳ������). */\n\t"
		"movl %%edx, %%eax							/* ��ҳ����ʼ��ַ->eax(����ֵ). */\n\t"
		"1:\n\t"
		"cld"
		:"=a" (__res)
		:"0" (0), "i" (LOW_MEM), "c" (PAGING_PAGES),
		"D" (mem_map + PAGING_PAGES - 1)
		:"dx");
	if (__res >= HIGH_MEMORY)						// ҳ���ַ����ʵ���ڴ�����������Ѱ��
		goto repeat;
	if (!__res && swap_out())						// ��û�еõ�����ҳ����ִ�н�������,�����²���.
		goto repeat;
	return __res;									// ���ؿ�������ҳ���ַ.
}

// �ڴ潻����ʼ��.
void init_swapping(void)
{
	// blk_size[]ָ��ָ�����豸�ŵĿ��豸��������.�ÿ�������ÿһ���Ӧһ���豸����ӵ�е����ݿ�����(1���С=1KB).
	extern int *blk_size[];							// blk_drv/ll_rw_blk.c
	int swap_size, i, j;

	// ���û�ж��彻���豸�򷵻�.��������豸û�����ÿ�������,����ʾ������.
	if (!SWAP_DEV)
		return;
	if (!blk_size[MAJOR(SWAP_DEV)]) {
		printk("Unable to get size of swap device\n\r");
		return;
	}
	// ȡָ�������豸�ŵĽ��������ݿ�����swap_size.��Ϊ0�򷵻�,���ܿ���С��100������ʾ��Ϣ"�����豸��̫С",Ȼ���˳�.
	swap_size = blk_size[MAJOR(SWAP_DEV)][MINOR(SWAP_DEV)];
	if (!swap_size)
		return;
	if (swap_size < 100) {
		printk("Swap device too small (%d blocks)\n\r", swap_size);
		return;
	}
	// ÿҳ4�����ݿ�,����swap_size >>= 2���������ҳ������.
	// �������ݿ�����ת���ɶ�Ӧ�ɽ���ҳ������.��ֵ���ܴ���SWAP_BITS���ܱ�ʾ��ҳ����.������ҳ���������ô���32768.
	swap_size >>= 2;
	if (swap_size > SWAP_BITS)
		swap_size = SWAP_BITS;
	// Ȼ������һҳ�����ڴ�����Ž���ҳ��ӳ������swap_bitmap,����ÿ1���ش���1ҳ����ҳ��
	swap_bitmap = (char *) get_free_page();
	if (!swap_bitmap) {
		printk("Unable to start swapping: out of memory :-)\n\r");
		return;
	}
	// read_swap_page(nr,buffer)������Ϊll_rw_page(READ,SWAP_DEV,(nr),(buffer)).����ѽ����豸�ϵ�ҳ�棰����swap_bitmapҳ����.��ҳ��
	//���ǽ���������ҳ��.���е�4086�ֽڿ�ʼ�����У������ַ��Ľ����豸�����ַ���"SWAP-SPACE".��û���ҵ��������ַ���,��˵������һ����Ч�Ľ����豸.
	// ������ʾ��Ϣ,�ͷŸ����������ҳ�沢�˳�����.���������ַ����ֽ�����.
	read_swap_page(0, swap_bitmap);
	if (strncmp("SWAP-SPACE", swap_bitmap + 4086, 10)) {
		printk("Unable to find swap-space signature\n\r");
		free_page((long) swap_bitmap);
		swap_bitmap = NULL;
		return;
	}
	// �������豸�ı�־�ַ���"SWAP-SPACE"�ַ������
	memset(swap_bitmap + 4086, 0, 10);
	// Ȼ�������Ľ���λӳ��ͼ.Ӧ��32768��λȫΪ0,��λͼ������λ��λ0,���ʾλͼ������,������ʾ������Ϣ,�ͷ�λͼռ�õ�ҳ�沢�˳�����.Ϊ�˼ӿ����ٶ�,
	// �������Ƚ���ѡ�鿴λͼ0�����һ������ҳ���Ӧ��λ,��swap_size����ҳ���Ӧ��λ,�Լ����SWAP_BITS(32768)λ.
	for (i = 0 ; i < SWAP_BITS ; i++) {
		if (i == 1)
			i = swap_size;
		if (bit(swap_bitmap, i)) {
			printk("Bad swap-space bit-map\n\r");
			free_page((long) swap_bitmap);
			swap_bitmap = NULL;
			return;
		}
	}
	// Ȼ������ϸ�ؼ��λ1��λswap_size����λ�Ƿ�Ϊ0.�����ڲ���0��λ,���ʾλͼ������,�����ͷ�λͼռ�õ�ҳ�沢�˳�����.������ʾ�����豸���������Լ�����ҳ��
	// �ͽ����ռ����ֽ���.
	j = 0;
	for (i = 1 ; i < swap_size ; i++)
		if (bit(swap_bitmap, i))
			j++;
	if (!j) {
		free_page((long) swap_bitmap);
		swap_bitmap = NULL;
		return;
	}
	Log(LOG_INFO_TYPE, "<<<<< Swap device ok: %d pages (%d bytes) swap-space >>>>>\n\r", j, j * 4096);
}

