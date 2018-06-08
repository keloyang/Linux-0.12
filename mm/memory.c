/*
 *  linux/mm/memory.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * demand-loading started 01.12.91 - seems it is high on the list of
 * things wanted, and it should be easy to implement. - Linus
 */
/*
 * ��������Ǵ�91.12.1��ʼ��д��ȷ - �ڳ�����Ʊ����ƺ�������Ҫ�ĳ��򣬲���Ӧ���Ǻ����ױ��Ƶ� - Linus
 */

/*
 * Ok, demand-loading was easy, shared pages a little bit tricker. Shared
 * pages started 02.12.91, seems to work. - Linus.
 *
 * Tested sharing by executing about 30 /bin/sh: under the old kernel it
 * would have taken more than the 6M I have free, but it worked well as
 * far as I could see.
 *
 * Also corrected some "invalidate()"s - I wasn't doing enough of them.
 */
/*
 * Ok����������ǱȽ����ױ�д�ģ�������ҳ��ȴ��Ҫ�е㼼�ɡ�����ҳ�������91.12.2��ʼ��д�ģ������ܹ����� - Linus��
 *
 * ͨ��ִ�д�Լ30��/bin/sh�Թ�����������˲��ԣ������ں˵�����Ҫռ�ö���6MB�����ڣ���Ŀǰȴ���á�����
 * ���������úܺá�
 *
 * �ԡ�invalidate()������Ҳ�������޸�--���ⷽ���һ����ò�����
 */

/*
 * Real VM (paging to/from disk) started 18.12.91. Much more work and
 * thought has to go into this. Oh, well..
 * 19.12.91  -  works, somewhat. Sometimes I get faults, don't know why.
 *		Found it. Everything seems to work now.
 * 20.12.91  -  Ok, making the swap-device changeable like the root.
 */
/*
 * 91.12.18��ʼ��д�����������ڴ����VB������ҳ�浽/�Ӵ��̣�����Ҫ�Դ˿��Ǻܶಢ����Ҫ���ܶ๤�����Ǻǣ�Ҳֻ�������ˡ�
 * 91.12.19 - ��ĳ�ֳ����Ͽ��Թ����ˣ�����ʱ�������֪����ô���¡�
 *            �ҵ������ˣ����ں���һ�ж��ܹ����ˡ�
 * 91.12.20 - OK���ѽ����豸�޸ĳɿɸ��ĵ��ˣ�������ļ��豸������
 */

#include <signal.h>

#include <asm/system.h>

#include <linux/sched.h>						// ���ȳ���ͷ�ļ�,��������ṹtask_struct,����0������.
//#include <linux/head.h>
//#include <linux/kernel.h>

// CODE_SPACE(addr) ((((addr)+oxfff)&~0xfff)<current->start_code+current->end_code)��
// �ú������жϸ������Ե�ַ�Ƿ�λ�ڵ�ǰ���̵Ĵ�����У���(((addr)+4095)&~4095)������ȡ�����Ե�ַaddr�����ڴ�ҳ���
// ĩ�˵�ַ��
#define CODE_SPACE(addr) ((((addr) + 4095) & ~4095) < \
current->start_code + current->end_code)

unsigned long HIGH_MEMORY = 0;					// ȫ�ֱ���,���ʵ�������ڴ���߶˵�ַ.

// ��from������һҳ�ڴ浽to��(4KB)
#define copy_page(from, to) \
__asm__("pushl %%edi; pushl %%esi; cld ; rep ; movsl; popl %%esi; popl %%edi"::"S" (from), "D" (to), "c" (1024):)
//#define copy_page(from,to) \
		__asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024):)

// �ڴ�ӳ���ֽ�ͼ(1�ֽڴ���1ҳ�ڴ�).ÿ��ҳ���Ӧ���ֽ����ڱ�־ҳ�浱ǰ������(ռ��)����.��������ӳ��15MB���ڴ�ռ�.�ڳ�ʼ������
// mem_init()��,���ڲ����������ڴ���ҳ���λ�þ�����ѡ�����ó�USED(100).
unsigned char mem_map [ PAGING_PAGES ] = {0, };

/*
 * Free a page of memory at physical address 'addr'. Used by
 * 'free_page_tables()'
 */
/*
 * �ͷ������ַ"addr"����һҳ�ڴ�.���ں���free_page_tables().
 */
// �ͷ������ַaddr��ʼ��1ҳ���ڴ�.
// �����ַ1MB���µ��ڴ�ռ������ں˳���ͻ���,����Ϊ����ҳ����ڴ�ռ�.��˲���addr��Ҫ����1MB
void free_page(unsigned long addr)
{
	// �����жϲ��������������ַaddr�ĺ�����.��������ַaddrС���ڴ�Ͷ�(1MB),���ʾ���ں˳������ٻ�����,�Դ˲��账��.��������ַ
	// addr >=ϵͳ���������ڴ���߶�,����ʾ������Ϣ�����ں�ֹͣ����.
	if (addr < LOW_MEM) return;
	if (addr >= HIGH_MEMORY)
		panic("trying to free nonexistent page");
	// ����Բ���addr��֤ͨ��,��ô�͸�����������ַ������ڴ�Ͷ˿�ʼ������ڴ�ҳ���.ҳ��� = (addr - LOW_MEME)/4096.�ɼ�ҳ��Ŵ�0��
	// ��ʼ����.��ʱaddr�д����ҳ���.�����ҳ��Ŷ�Ӧ��ҳ��ӳ���ֽڲ�����0,���1����.��ʱ��ӳ���ֽ�ֵӦ��Ϊ0,��ʾҳ�����ͷ�.�����Ӧҳ��ԭ����
	// ��0,��ʾ������ҳ�汾�����ǿ��е�,˵���ں˴��������.������ʾ������Ϣ��ͣ��.
	addr -= LOW_MEM;
	addr >>= 12;
	if (mem_map[addr]--) return;
	// ִ�е��˴���ʾҪ�ͷſ��е�ҳ�棬�򽫸�ҳ������ô�������Ϊ0
	mem_map[addr] = 0;
	panic("trying to free free page");
}

/*
 * This function frees a continuos block of page tables, as needed
 * by 'exit()'. As does copy_page_tables(), this handles only 4Mb blocks.
 */
/*
 * ���溯���ͷ�ҳ���������ڴ��,exit()��Ҫ�ú���.��copy_page_tables()����,�ú���������4MB���ȵ��ڴ��.
 */
// ����ָ�������Ե�ַ���޳�(ҳ�����),�ͷŶ�Ӧ�ڴ�ҳ��ָ�����ڴ�鲢�������.
// ҳĿ¼λ�������ַ0��ʼ��,��1024��,ÿ��4�ֽ�,��ռ4KB.ÿ��Ŀ¼��ָ��һ��ҳ��.�ں�ҳ�������ַ0x1000����ʼ(������Ŀ¼�ռ�),
// ��4��ҳ��.ÿ��ҳ����1024��,ÿ��4B.���Ҳռ4KB(1ҳ)�ڴ�.������(�������ں˴����еĽ���0��1)��ҳ����ռ�ݵ�ҳ���ڽ��̱�����ʱ��
// �ں�Ϊ�������ڴ�������õ�.ÿ��ҳ�����Ӧ1ҳ�����ڴ�,���һ��ҳ������ӳ��4MB�������ڴ�.
// ����:from - ��ʼ���Ի���ַ;size - �ͷŵ��ֽڳ���.
int free_page_tables(unsigned long from, unsigned long size)
{
	unsigned long *pg_table;
	unsigned long * dir, nr;

	// ���ȼ�����from���������Ի���ַ�Ƿ���4MB�ı߽紦.��Ϊ�ú���ֻ�ܴ����������.��from = 0,�����.˵����ͼ�ͷ��ں˺ͻ�����ռ�ռ�.
	if (from & 0x3fffff)
		panic("free_page_tables called with wrong alignment");
	if (!from)
		panic("Trying to free up swapper memory space");
	// Ȼ��������size�����ĳ�����ռ��ҳĿ¼��(4MB�Ľ�λ������),Ҳ����ռҳ����.
	// ��Ϊ1��ҳ��ɹ���4MB�����ڴ�,��������������22λ�ķ�ʽ����Ҫ���Ƶ��ڴ泤��ֵ����4MB.���м���0x3fffff(��4MB-1)���ڵõ���λ������
	// ���,�������������������1.����,���ԭsize = 4.01MB,��ô�ɵõ����size = 2.
	size = (size + 0x3fffff) >> 22;
	// ���ż�����������Ի���ַ��Ӧ����ʼĿ¼��, ��Ӧ��Ŀ¼��� = from >>22.��Ϊÿ���4�ֽ�,��������ҳĿ¼��������ַ0��ʼ���,
	// ���ʵ��Ŀ¼��ָ�� = Ŀ¼���<<2,Ҳ��(from >> 20),"��"��0xffcȷ��Ŀ¼��ָ�뷶Χ��Ч.
	// dir��ʾ��ʼ��ҳĿ¼�������ַ
	dir = (unsigned long *) ((from >> 20) & 0xffc); 			/* _pg_dir = 0 */
	// ��ʱsize���ͷŵ�ҳ�����,��ҳĿ¼����,��dir����ʼĿ¼��ָ��.���ڿ�ʼѭ������ҳĿ¼��,�����ͷ�ÿ��ҳ���е�ҳ����.�����ǰĿ¼����Ч(
	// Pλ=0),��ʾ��Ŀ¼��û��ʹ��(��Ӧ��ҳ������),�����������һ��ҳ����.�����Ŀ¼����ȡ��ҳ���ַpg_table,���Ը�ҳ���е�1024������
	// ���д���,�ͷ���Чҳ��(Pλ=1)��Ӧ�������ڴ�ҳ��,���ߴӽ����豸���ͷ���Чҳ����(Pλ=0)��Ӧ��ҳ��,���ͷŽ����豸�ж�Ӧ���ڴ�ҳ��(��Ϊ
	// ҳ������Ѿ�������ȥ).Ȼ��Ѹ�ҳ��������,������������һҳ����.��һ��ҳ�����б��������Ͼ��ͷŸ�ҳ������ռ�ݵ��ڴ�ҳ��,������������
	// һҳĿ¼��.���ˢ��ҳ�任���ٻ���,������0.
	for ( ; size-- > 0 ; dir++) {
		// �����Ŀ¼�����ҳ�����ֱ��������ҳ����
		if (!(1 & *dir))
			continue;
		pg_table = (unsigned long *) (0xfffff000 & *dir);		// ȡҳ���ַ.
		for (nr = 0 ; nr < 1024 ; nr++) {
			if (*pg_table) {									// ����ָҳ�������ݲ�Ϊ0,����������Ч,���ͷŶ�Ӧ��.
				if (1 & *pg_table)
					free_page(0xfffff000 & *pg_table);
				else											// �����ͷŽ����豸�ж�Ӧҳ.
					swap_free(*pg_table >> 1);
				*pg_table = 0;									// ��ҳ������������.
			}
			pg_table++;											//ָ��ҳ������һ��.
		}
		free_page(0xfffff000 & *dir);							// �ͷŸ�ҳ����ռ�ڴ�ҳ��.
		*dir = 0;												// ��Ӧҳ���Ŀ¼������.
	}
	invalidate();												// ˢ��CPUҳ�任���ٻ���.
	return 0;
}

/*
 *  Well, here is one of the most complicated functions in mm. It
 * copies a range of linerar addresses by copying only the pages.
 * Let's hope this is bug-free, 'cause this one I don't want to debug :-)
 *
 * Note! We don't copy just any chunks of memory - addresses have to
 * be divisible by 4Mb (one page-directory entry), as this makes the
 * function easier. It's used only by fork anyway.
 *
 * NOTE 2!! When from==0 we are copying kernel space for the first
 * fork(). Then we DONT want to copy a full page-directory entry, as
 * that would lead to some serious memory waste - we just copy the
 * first 160 pages - 640kB. Even that is more than we need, but it
 * doesn't take any more memory - we don't copy-on-write in the low
 * 1 Mb-range, so the pages can be shared with the kernel. Thus the
 * special case for nr=xxxx.
 */
/*
 * ����,�������ڴ����mm����Ϊ���ӵĳ���֮һ.��ͨ��ֻ�����ڴ�ҳ��������һ����Χ�����Ե�ַ�е�����.ϣ��������û�д���,��Ϊ�Ҳ���
 * �ٵ�����������:-)
 *
 * ע��!���ǲ��������κ��ڴ��,�ڴ��ĵ�ַ��Ҫ��4MB�ı���(����һ��ҳĿ¼���Ӧ���ڴ泤��),��Ϊ���������ʹ������.��������,������
 * fork()ʹ��.
 *
 * ע��2!!��from==0ʱ,˵������Ϊ��һ��fork()���ø����ں˿ռ�.��ʱ���ǾͲ��븴������ҳĿ¼���Ӧ���ڴ�,��Ϊ�������ᵼ���ڴ������˷�
 * ����ֻ�븴�ƿ�ͷ160��ҳ��,��Ӧ640KB.��ʹ�Ǹ�����Щҳ��Ҳ�Ѿ��������ǵ�����,���ⲻ��ռ�ø�����ڴ�,�ڵ�1MB�ڴ淶Χ�ڲ�ִ��дʱ����
 * ����,������Щҳ��������ں˹���.�������nr=xxxx���������(nr�ڳ�����ָҳ����).
 */
// ����Ŀ¼�����ҳ����.
// ����ָ�����Ե�ַ�ͳ����ڴ��Ӧ��ҳĿ¼���ҳ����,�Ӷ������Ƶ�ҳĿ¼��ҳ���Ӧ��ԭ�����ڴ�ҳ����������ҳ��ӳ�������ʹ��.����ʱ,������
// ��ҳ���������ҳ��,ԭ�����ڴ�����������.�˺���������(�����̺����ӽ���)�������ڴ���,ֱ����һ������ִ��д����ʱ,�ں˲Ż�Ϊд�������̷���
// �µ��ڴ�ҳ(дʱ���ƻ���).
// ����from,to�����Ե�ַ,size����Ҫ����(����)���ڴ泤��,��λ���ֽ�.
int copy_page_tables(unsigned long from, unsigned long to, long size)
{
	unsigned long * from_page_table;
	unsigned long * to_page_table;
	unsigned long this_page;
	unsigned long * from_dir, * to_dir;
	unsigned long new_page;
	unsigned long nr;

	// ���ȼ�����������Դ��ַfrom��Ŀ�ĵ�ַto����Ч��.Դ��ַ��Ŀ�ĵ�ַ����Ҫ��4MB�ڴ�߽��ַ��.�����������.��������Ҫ������Ϊһ��ҳ���
	// 1024��ɹ���4MB�ڴ�.Դ��ַfrom��Ŀ�ĵ�ַtoֻ���������Ҫ����ܱ�֤��һ��ҳ��ĵ�1�ʼ����ҳ����,������ҳ��������������Ч��.Ȼ��
	// ȡ��Դ��ַ��Ŀ�ĵ�ַ����ʼĿ¼��ָ��(from_dir��do_dir).�ٸ��ݲ��������ĳ���size����Ҫ���Ƶ��ڴ��ռ�õ�ҳ����(��Ŀ¼����)
	if ((from & 0x3fffff) || (to & 0x3fffff))
		panic("copy_page_tables called with wrong alignment");
	from_dir = (unsigned long *) ((from >> 20) & 0xffc); 				/* _pg_dir = 0 */
	to_dir = (unsigned long *) ((to >> 20) & 0xffc);
	size = ((unsigned) (size + 0x3fffff)) >> 22;
	// �ڵõ���Դ��ʼĿ¼��ָ��from_dir��Ŀ����ʼĿ¼��ָ��to_dir�Լ���Ҫ���Ƶ�ҳ�����size��,���濪ʼ��ÿ��ҳĿ¼����������1ҳ�ڴ��������Ӧ�����,
	// ���ҿ�ʼҳ����Ʋ���.���Ŀ��Ŀ¼��ָ����ҳ���Ѿ�����(P=1),���������.���ԴĿ¼����Ч,��ָ����ҳ������(P=0),�����ѭ��������һ��ҳĿ¼��.
	for( ; size-- > 0 ; from_dir++, to_dir++) {
		if (1 & *to_dir)
			panic("copy_page_tables: already exist");
		if (!(1 & *from_dir))
			continue;
		// ����֤�˵�ǰԴĿ¼���Ŀ��������֮��,ȡԴĿ¼����ҳ���ַfrom_page_table.Ϊ�˱���Ŀ��Ŀ¼���Ӧ��ҳ��,��Ҫ�����ڴ���������1ҳ�����ڴ�ҳ.���ȡ
		// ����ҳ�溯��get_free_page()����0,��˵��û�����뵽�����ڴ�ҳ��,�������ڴ治��.���Ƿ���-1ֵ�˳�.
		from_page_table = (unsigned long *) (0xfffff000 & *from_dir);
		if (!(to_page_table = (unsigned long *) get_free_page()))
			return -1;													/* Out of memory, see freeing */
		// ������������Ŀ��Ŀ¼����Ϣ,�����3λ��λ,����ǰĿ��Ŀ¼��"��"��7,��ʾ��Ӧҳ��ӳ����ڴ�ҳ�����û�����,���ҿɶ�д,����(User,R/W,Present).(���
		// U/Sλ��0,��R/W��û������.���U/S��1,��R/W��0,��ô�������û���Ĵ����ֻ�ܶ�ҳ��.���U/S��R/W����λ,����ж�д��Ȩ��).
		*to_dir = ((unsigned long) to_page_table) | 7;
		// Ȼ����Ե�ǰ�����ҳĿ¼���Ӧ��ҳ��,������Ҫ���Ƶ�ҳ������.��������ں˿ռ�,����踴��ͷ160ҳ��Ӧ��ҳ����(nr = 160),
		// ��Ӧ�ڿ�ʼ640KB�����ڴ�.������Ҫ����һ��ҳ���е�����1024��ҳ����(nr= 1024),��ӳ��4MB�����ڴ�.
		nr = (from == 0) ? 0xA0 : 1024;
		// ��ʱ���ڵ�ǰҳ��,��ʼѭ������ָ����nr���ڴ�ҳ�����.��ȡ��Դҳ��������,�����ǰԴҳ��û��ʹ��(������Ϊ0),���ø��Ƹñ���,����������һ��.
		for ( ; nr-- > 0 ; from_page_table++, to_page_table++) {
			this_page = *from_page_table;
			// ���Դҳ�����ڣ���ֱ�ӿ�����һҳ��
			if (!this_page)
				continue;
			// ����ñ���������,���������λP=0,��ñ����Ӧ��ҳ������ڽ����豸��.����������1ҳ�ڴ�,���ӽ����豸�ж����ҳ��(�������豸���еĻ�).Ȼ�󽫸�ҳ����Ƶ�
			// Ŀ��ҳ������.���޸�Դҳ��������ָ�����������ڴ�ҳ.
			if (!(1 & this_page)) {
				// ����һҳ�µ��ڴ�Ȼ�󽫽����豸�е����ݶ�ȡ����ҳ����
				if (!(new_page = get_free_page()))
					return -1;
				// �ӽ����豸�н�ҳ���ȡ����
				read_swap_page(this_page >> 1, (char *) new_page);
				// Ŀ��ҳ����ָ��Դҳ����ֵ
				*to_page_table = this_page;
				// ���޸�Դҳ��������ָ�����������ڴ�ҳ,�����ñ����־Ϊ"ҳ����"����7
				*from_page_table = new_page | (PAGE_DIRTY | 7);
				// ����������һҳ����
				continue;
			}
			// ��λҳ������R/W��־(λ1��0),����ҳ�����Ӧ���ڴ�ҳ��ֻ��,Ȼ�󽫸�ҳ����Ƶ�Ŀ��ҳ����
			this_page &= ~2;
			*to_page_table = this_page;
			// �����ҳ������ָ����ҳ��ĵ�ַ��1MB����,����Ҫ�����ڴ�ҳ��ӳ������mem_map[],���Ǽ���ҳ���,������Ϊ������ҳ��ͬ������Ӧ�����������ô���.������λ��1MB����
			// ��ҳ��,˵�����ں�ҳ��,��˲���Ҫ��mem_map[]��������.��Ϊmem_map[]�����ڹ������ڴ����е�ҳ��ʹ������.��˶����ں��ƶ�������0�в��ҵ���fork()��������1ʱ
			// (��������init()),���ڴ�ʱ���Ƶ�ҳ�滹��Ȼ�����ں˴�������,��������ж��е���䲻��ִ��,����0��ҳ����Ȼ������ʱ��д.ֻ�е�����fork()�ĸ����̴��봦�����ڴ�
			// (ҳ��λ�ô���1MB)ʱ�Ż�ִ��.���������Ҫ�ڽ��̵���execve(),��װ��ִ�����³������ʱ�Ż����.
			// 157����京������Դҳ������ָ�ڴ�ҳҲΪֻ��.��Ϊ���ڿ�ʼ�����������̹����ڴ�����.������1��������Ҫ���в���,�����ͨ��ҳ�쳣д��������Ϊִ��д�����Ľ��̷���
			// 1ҳ�¿���ҳ��,Ҳ������дʱ����(copy_on_write)����.
			if (this_page > LOW_MEM) {
				*from_page_table = this_page;		// ��Դҳ����Ҳֻ��.
				this_page -= LOW_MEM;
				this_page >>= 12;
				mem_map[this_page]++;
			}
        }
    }
	invalidate();									// ˢ��ҳ�任���ٻ���.
	return 0;
}

/*
 * This function puts a page in memory at the wanted address.
 * It returns the physical address of the page gotten, 0 if
 * out of memory (either when trying to access page-table or
 * page.)
 */
/*
 * ���溯����һ�ڴ�ҳ�����(ӳ��)��ָ�����Ե�ַ��.������ҳ��������ַ,����ڴ治��(�ڷ���ҳ���ҳ��ʱ),�򷵻�0.
 */
// ��һ�����ڴ�ҳ��ӳ�䵽���Ե�ַ�ռ�ָ����.
// ����˵�ǰ����Ե�ַ�ռ���ָ����ַaddress����ҳ��ӳ�䵽���ڴ���ҳ��page��.��Ҫ�����������ҳĿ¼���ҳ����������ָ��ҳ�����Ϣ.���ɹ��򷵻�����ҳ���ַ.
// �ڴ���ȱҳ�쳣��C����do_no_page()�л���ô˺���.����ȱҳ������쳣,�����κ�ȱҳԵ�ʶ���ҳ�����޸�ʱ,������Ҫˢ��CPU��ҳ�任����(���Translation Lookaside
// Buffer,TLB),��ʹҳ�����б�־P����0�޸ĳ�1.��Ϊ��Чҳ��ᱻ����,��˵��޸���һ����Ч��ҳ����ʱ����Ҫˢ��.�ڴ˾ͱ���Ϊ���õ���Invalidate()����.
// ����page�Ƿ�������ڴ�����ĳһҳ��(ҳ֡,ҳ��)��ָ��;address�����Ե�ַ.
static unsigned long put_page(unsigned long page, unsigned long address)
{
	unsigned long tmp, *page_table;

	/* NOTE !!! This uses the fact that _pg_dir=0 */
	/* ע��!!!����ʹ����ҳĿ¼�����ַpg_dir=0������ */

	// �����жϲ������������ڴ�ҳ��page����Ч��.�����ҳ��λ�õ���LOW_MEM(1MB)�򳬳�ϵͳʵ�ʺ����ڴ�߶�HIGH_MEMORY,�򷢳�����.LOW_MEM�����ڴ��������е�
	// ��С��ʼλ��.��ϵͳ����ڴ�С�ڻ����6MBʱ,���ڴ���ʼ��LOW_MEM��.�ٲ鿴һ�¸�pageҳ���ǲ��Ѿ������ҳ��,���ж������ڴ�ҳ��ӳ���ֽ�ͼmem_map[]����Ӧ
	// �ֽ��Ƿ�����λ.��û�����跢������.
	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printk("Trying to put page %p at %p\n", page, address);
	if (mem_map[(page - LOW_MEM) >> 12] != 1)
		printk("mem_map disagrees with %p at %p\n", page, address);
	// Ȼ����ݲ���ָ�������Ե�ַaddress��������ҳĿ¼���ж�Ӧ��Ŀ¼��ָ��,������ȡ��һ��ҳ���ַ.�����Ŀ¼����Ч(P=1),��ָ����ҳ�����ڴ���,�����ȡ��ָ��ҳ��
	// ��ַ�ŵ�page_table������.��������һ����ҳ���ҳ��ʹ��,���ڶ�ӦĿ¼��������Ӧ��־(7 - User,U/S,R/W).Ȼ�󽫸�ҳ���ַ�ŵ�page_table������.
	page_table = (unsigned long *) ((address >> 20) & 0xffc);
	if ((*page_table) & 1)
		page_table = (unsigned long *) (0xfffff000 & *page_table);
	else {
		if (!(tmp = get_free_page()))
			return 0;
		*page_table = tmp | 7;
		page_table = (unsigned long *) tmp;
	}
	// ������ҵ���ҳ��page_table���������ҳ��������,��������ҳ��page�ĵ�ַ�������ͬʱ��λ3����־(U/S,W/R,P).��ҳ������ҳ���е�����ֵ�������Ե�ַλ21~λ12
	// ��ɵ�10λ��ֵ.ÿ��ҳ������1024��(0~0x3ff).
	page_table[(address >> 12) & 0x3ff] = page | 7;
	/* no need for invalidate */
	/* ����Ҫˢ��ҳ�任���ٻ��� */
	return page;					// ��������ҳ���ַ.
}

/*
 * The previous function doesn't work very well if you also want to mark
 * the page dirty: exec.c wants this, as it has earlier changed the page,
 * and we want the dirty-status to be correct (for VM). Thus the same
 * routine, but this time we mark it dirty too.
 */
/*
 * �����Ҳ������ҳ�����޸ı�־,����һ�����������ò��Ǻܺ�:exec.c������Ҫ��������.��Ϊexec.c�к������ڷ���ҳ��֮ǰ�޸Ĺ�ҳ������.Ϊ��ʵ��VM,������Ҫ����ȷ����
 * ���޸�״̬��־.��������������������ͬ�ĺ���,���Ǹú����ڷ���ҳ��ʱ���ҳ���־Ϊ���޸�״̬.
 */
// ��һ�������޸Ĺ��������ڴ�ҳ��ӳ�䵽���Ե�ַ�ռ�ָ����.
// �ú�����һ������put_page()������ȫһ��,���˱������ڵ�223������ҳ��������ʱ,ͬʱ��������ҳ�����޸ı�־(λ6,PAGE_DIRTY).
unsigned long put_dirty_page(unsigned long page, unsigned long address)
{
	unsigned long tmp, *page_table;

	/* NOTE !!! This uses the fact that _pg_dir=0 */

	// �����жϲ������������ڴ�ҳ��page����Ч��.�����ҳ��λ�õ���LOW_MEM(1MB)�򳬳�ϵͳʵ�ʺ����ڴ�߶�HIGH_MEMORY,�򷢳�����.LOW_MEM�����ڴ��������е�
	// ��С��ʼλ��.��ϵͳ����ڴ�С�ڻ����6MBʱ,���ڴ���ʼ��LOW_MEM��.�ٲ鿴һ�¸�pageҳ���ǲ��Ѿ������ҳ��,���ж������ڴ�ҳ��ӳ���ֽ�ͼmem_map[]����Ӧ
	// �ֽ��Ƿ�����λ.��û�����跢������.
	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printk("Trying to put page %p at %p\n", page, address);
	if (mem_map[(page-LOW_MEM)>>12] != 1)
		printk("mem_map disagrees with %p at %p\n", page, address);
	// Ȼ����ݲ���ָ�������Ե�ַaddress��������ҳĿ¼���ж�Ӧ��Ŀ¼��ָ��,������ȡ��һ��ҳ���ַ.�����Ŀ¼����Ч(P=1),��ָ����ҳ�����ڴ���,�����ȡ��ָ��ҳ��
	// ��ַ�ŵ�page_table������.��������һ����ҳ���ҳ��ʹ��,���ڶ�ӦĿ¼��������Ӧ��־(7 - User,U/S,R/W).Ȼ�󽫸�ҳ���ַ�ŵ�page_table������.
	page_table = (unsigned long *) ((address >> 20) & 0xffc);
	if ((*page_table) & 1)
		page_table = (unsigned long *) (0xfffff000 & *page_table);
	else {
		if (!(tmp = get_free_page()))
			return 0;
		*page_table = tmp | 7;
		page_table = (unsigned long *) tmp;
	}
	// ������ҵ���ҳ��page_table���������ҳ��������,��������ҳ��page�ĵ�ַ�������ͬʱ��λ3����־(U/S,W/R,P).��ҳ������ҳ���е�����ֵ�������Ե�ַλ21~λ12
	// ��ɵ�10λ��ֵ.ÿ��ҳ������1024��(0~0x3ff).
	page_table[(address >> 12) & 0x3ff] = page | (PAGE_DIRTY | 7);
	/* no need for invalidate */
	/* ����Ҫˢ��ҳ�任���ٻ��� */
	return page;
}

// ȡ��д����ҳ�溯��.
// ����ҳ�쳣�жϹ�����д�����쳣�Ĵ���(дʱ����).���ں˴�������ʱ,�½����븸���̱����óɹ������������ڴ�ҳ��,����������Щҳ��������ó�ֻ��ҳ��.�����½��̻�ԭ
// ������Ҫ���ڴ�ҳ��д����ʱ,CPU�ͻ��⵽������������ҳ��д�����쳣.����������������ں˾ͻ������ж�Ҫд��ҳ���Ƿ񱻹���.��û�����ҳ�����óɿ�дȻ���˳�.��ҳ��
// ���ڹ���״̬,��Ҫ��������һ��ҳ�沢���Ʊ�дҳ������,�Թ�д���̵���ʹ��.����ȡ��.
// �������Ϊҳ�����ָ��,�������ַ.[un_wp_page -- Un-Write Protect Page]
void un_wp_page(unsigned long * table_entry)
{
	unsigned long old_page, new_page;

	// ����ȡ����ָ����ҳ����������ҳ��λ��(��ַ)���жϸ�ҳ���ǲ��ǹ���ҳ��.���ԭҳ���ַ�����ڴ�Ͷ�LOW_MEM(��ʾ�����ڴ�����),��������ҳ��ӳ���ֽ�ͼ������ֵΪ1(��ʾ
	// ҳ���������1��,ҳ��û�б�����),���ڸ�ҳ���ҳ������ R/W��־(��д),��ˢ��ҳ�任���ٻ���,Ȼ�󷵻�.��������ڴ�ҳ���ʱֻ��һ������ʹ��,���Ҳ����ں��еĽ���,��ֱ��
	// �����Ը�Ϊ��д����,������������һ����ҳ��.
	old_page = 0xfffff000 & *table_entry;				// ȡָ��ҳ����������ҳ���ַ.
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)] == 1) {
		*table_entry |= 2;
		invalidate();
		return;
	}
	// �������Ҫ�����ڴ���������һҳ����ҳ���ִ��д�����Ľ��̵���ʹ��,ȡ��ҳ�湲��.���ԭҳ������ڴ�Ͷ�(����ζ��mem_map[]>1,ҳ���ǹ����),��ԭҳ���ҳ��ӳ���ֽ�����
	// ֵ�ݼ�1.Ȼ��ָ��ҳ�������ݸ���Ϊ��ҳ���ַ,���ÿɶ�д��־(U/S,R/W,P).��ˢ��ҳ�任���ٻ���֮��,���ԭҳ�����ݸ��Ƶ���ҳ��.
	if (!(new_page = get_free_page()))
		oom();											// �ڴ治������.
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	copy_page(old_page, new_page);
	// ���µ�ҳ������Ϊ�ɶ���д�Ҵ���
	*table_entry = new_page | 7;
	// ˢ�¸��ٻ���
	invalidate();
}

/*
 * This routine handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * If it's in code space we exit with a segment error.
 */
/*
 * ���û���ͼסһ����ҳ����дʱ,�ú��������Ѵ��ڵ��ڴ�ҳ��(дʱ����),����ͨ����ҳ�渴�Ƶ�һ���µ�ַ�ϲ��ҵݼ�ԭҳ��Ĺ���
 * ����ֵʵ�ֵ�.
 *
 * ������ڴ���ռ�,���Ǿ���ʾ�γ�����Ϣ���˳�.
 */
// ִ��д����ҳ�洦��.
// ��д����ҳ�洦����.��ҳ�쳣�жϴ�������е��õ�C����.��page.s�����б�����.
// ��������error_code��address�ǽ�����дд����ҳ��ʱ��CPU�����쳣���Զ����ɵ�.error_codeָ����������;address�ǲ����쳣��ҳ��
// ���Ե�ַ.д����ҳ��ʱ�踴��ҳ��(дʱ����).
void do_wp_page(unsigned long error_code, unsigned long address)
{
	// �����ж�CPU���ƼĴ���CR2����������ҳ���쳣�����Ե�ַ��ʲô��Χ��.���addressС��TASK_SIZE(0x4000000,��64MB),��ʾ�쳣ҳ��λ��
	// ���ں˻�����0������1���������Ե�ַ��Χ��,���Ƿ���������Ϣ"�ں˷�Χ�ڴ汻д����";���(address - ��ǰ���̴�����ʼ��ַ)����һ�����̵�
	// ����(64MB),��ʾaddress��ָ�����Ե�ַ���������쳣�Ľ������Ե�ַ�ռ䷶Χ��,���ڷ���������Ϣ���˳�.
	if (address < TASK_SIZE)
		printk("\n\rBAD! KERNEL MEMORY WP-ERR!\n\r");
	if (address - current->start_code > TASK_SIZE) {
		printk("Bad things happen: page error in do_wp_page\n\r");
		do_exit(SIGSEGV);
	}
#if 0
	/* we cannot do this yet: the estdio library writes to code space */
	/* stupid, stupid. I really want the libc.a from GNU */
	/* �������ڻ�����������:��Ϊestdio����ڴ���ռ�ִ��д���� */
	/* ����̫�޴���.�������GNU�õ�libca��. */
	// ������Ե�ַλ�ڽ��̵Ĵ���ռ���,����ִֹ�г���.��Ϊ������ֻ����.
	if (CODE_SPACE(address))
		do_exit(SIGSEGV);
#endif
	// �������溯��un_wp_page()������ȡ��ҳ�汣��.��������ҪΪ��׼���ò���. ���������Ե�ַaddressָ��ҳ����ҳ���е�ҳ����ָ��,����㷽����:1
	// ((address>>10) & 0xffc):����ָ�����Ե�ַ��ҳ������ҳ���е�ƫ�Ƶ�ַ;��Ϊ�������Ե�ַ�ṹ,(address >> 12)����ҳ����������,��ÿ��ռ4
	// ���ֽ�,��˳�4��:(address>>12)<<2=(address>>10)&0xffc�Ϳɵõ�ҳ�����ڱ��е�ƫ�Ƶ�ַ.�����&0xffc�������Ƶ�ַ��Χ��һ��ҳ����.����Ϊ
	// ֻ�ƶ���10λ,������2λ�����Ե�ַ��12λ�е����2λ,ҲӦ���ε�.��������Ե�ַ��ҳ������ҳ����ƫ�Ƶ�ֱַ��һЩ�ı�ʾ������(((address>>12)&0x3ff)<<2)
	// 2:(0xfffff000&*((address>>20)&0xffc)):����ȡĿ¼����ҳ��ĵ�ֵַ;����,((address>>20)&0xffc)����ȡ���Ե�ַ�е�Ŀ¼��������Ŀ¼���е�ƫ��
	// λ��.��Ϊaddress>>22��Ŀ¼������ֵ,��ÿ��4���ֽ�,��˳���4��:(address>>22)<<2=(address>>20)����ָ������Ŀ¼���е�ƫ�Ƶ�ַ.&0xffc��������
	// Ŀ¼������ֵ�����2λ.��Ϊֻ�ƶ���20λ,������2λ��ҳ������������,Ӧ�����ε�.��*((address>>20)&0xffc)����ȡָ��Ŀ¼���������ж�Ӧҳ��
	// �������ַ.�������0xffffff000�������ε�ҳĿ¼�������е�һЩ��־λ(Ŀ¼���12λ).ֱ�۱�ʾΪ(0xffffff000 & *((unsigned long *) (((
	// address>>22) & 0x3ff)<<2))).3:��1��ҳ������ҳ����ƫ�Ƶ�ַ����2��Ŀ¼���������ж�Ӧҳ��������ַ���ɵõ�ҳ�����ָ��(�����ַ).�����
	// �����ҳ����и���.
	un_wp_page((unsigned long *)
		(((address >> 10) & 0xffc) + (0xfffff000 &
		*((unsigned long *) ((address >> 20) & 0xffc)))));

}

// дҳ����֤.
// ��ҳ�治��д,����ҳ��.��fork.c�б��ڴ���֤ͨ�ú���verify_area()����.
// ����address��ָ��ҳ����4GB�ռ��е����Ե�ַ.
void write_verify(unsigned long address)
{
	unsigned long page;

	// ����ȡָ�����Ե�ַ��Ӧ��ҳĿ¼��,����Ŀ¼���еĴ���λ(P)�ж�Ŀ¼���Ӧ��ҳ���Ƿ����(����λP=1?),��������(P=0)�򷵻�.��������
	// ����Ϊ���ڲ����ڵ�ҳ��û�й����дʱ���ƿ���,����������Դ˲����ڵ�ҳ��ִ��д����ʱ,ϵͳ�ͻ���Ϊȱҳ�쳣��ȥִ��do_no_page(),
	// ��Ϊ����ط�ʹ��put_page()����ӳ��һ������ҳ��.���ų����Ŀ¼����ȡҳ���ַ,����ָ��ҳ����ҳ���е�ҳ����ƫ��ֵ,�ö�Ӧ��ַ��ҳ��
	// ��ָ��.�ڸñ����а����Ÿ������Ե�ַ��Ӧ������ҳ��.
	if (!( (page = *((unsigned long *) ((address >> 20) & 0xffc)) ) & 1))
		return;
	page &= 0xfffff000;
	// �õ�ҳ����������ַ
	page += ((address >> 10) & 0xffc);
	// Ȼ���жϸ�ҳ������λ1(P/W),λ0(P)��־.�����ҳ�治��д(R/W=0)�Ҵ���,��ô��ִ�й������͸���ҳ�����(дʱ����).����ʲôҲ����,
	// ֱ���˳�.
	if ((3 & *(unsigned long *) page) == 1)  /* non-writeable, present */
		un_wp_page((unsigned long *) page);
	return;
}

// ȡ��һҳ�����ڴ沢ӳ�䵽ָ�����Ե�ַ��.
// get_free_page()��������ȡ�������ڴ�����һҳ�����ڴ�.���������򲻽��ǻ�ȡ��һҳ�����ڴ�ҳ��,����һ������put_page(),������ҳ��ӳ�䵽ָ�������Ե�ַ��.
// ����address��ָ��ҳ������Ե�ַ.
void get_empty_page(unsigned long address)
{
	unsigned long tmp;

	// ������ȡ��һ����ҳ��,���߲��ܽ���ȡҳ����õ�ָ����ַ��,����ʾ�ڴ治������Ϣ.292����Ӣ��ע�͵ĺ�����:free_page()�����Ĳ���tmp��0Ҳû�й�ϵ,�ú����������
	// ������������.
	if (!(tmp = get_free_page()) || !put_page(tmp, address)) {
		free_page(tmp);		/* 0 is ok - ignored */
		oom();
	}
}

/*
 * try_to_share() checks the page at address "address" in the task "p",
 * to see if it exists, and if it is clean. If so, share it with the current
 * task.
 *
 * NOTE! This assumes we have checked that p != current, and that they
 * share the same executable or library.
 */
/*
 * tty_to_share()����p���м��λ�ڵ�ַ��address������ҳ�棬��ҳ���Ƿ���ڣ��Ƿ�ɾ������
 * �ɾ��Ļ������뵱ǰ������
 *
 * ע�⣡���������Ѽٶ�p!=��ǰ���񣬲������ǹ���ͬһ��ִ�г��������
 */
// ���ԶԵ�ǰ����ָ����ַ����ҳ����й�����
// ��ǰ���������p��ͬһִ�д��룬Ҳ������Ϊ��ǰ��������p����ִ��fork���������Ľ��̣�������ǵĴ�������һ�������δ������
// �����������޸���ô���ݶ�����ҲӦһ��������address�ǽ����е��߼���ַ�����ǵ�ǰ��������p���̹���ҳ����߼�ҳ���ַ������
// p�ǽ�������ҳ��Ľ��̡����p����address����ҳ����ڲ���û�б��޸Ĺ��Ļ������õ�ǰ������p���̹���֮��ͬʱ����Ҫ��ָ֤��
// �ĵ�ַ���Ƿ��Ѿ�������ҳ�棬���������������
// ���أ�1 - ҳ�湲����ɹ���0 - ʧ�ܡ�
static int try_to_share(unsigned long address, struct task_struct * p)
{
	unsigned long from;
	unsigned long to;
	unsigned long from_page;
	unsigned long to_page;
	unsigned long phys_addr;

	// ���ȷֱ����ָ������p�к͵�ǰ�������߼���ַaddress��Ӧ��ҳĿ¼�Ϊ�˼��㷽�������ָ���߼���ַaddress���ġ��߼���ҳ
	// Ŀ¼�ţ����Խ��̿ռ䣨0 - 64MB�������ҳĿ¼��š��á��߼���ҳĿ¼��ż��Ͻ���p��CPU 4GB���Կռ�����ʼ��ַ��Ӧ��ҳĿ¼
	// ����õ�����p�е�ַaddress��ҳ������Ӧ��4GB���Կռ���ʵ��ҳĿ¼��from_page�������߼���ҳĿ¼��ż��ϵ�ǰ����CPU 4GB
	// ���Կռ��е�ʵ��ҳĿ¼��to_page��
	from_page = to_page = ((address >> 20) & 0xffc);
	from_page += ((p->start_code >> 20) & 0xffc);             		// p����Ŀ¼�
	to_page += ((current->start_code >> 20) & 0xffc);         		// ��ǰ����Ŀ¼�
	// �ڵõ�p���̺͵�ǰ����address��Ӧ��Ŀ¼�������ֱ�Խ���p�͵�ǰ���̴������ȶ�p���̵ı�����в�����Ŀ¼��ȡ��p������
	// address��Ӧ����������ҳ���ַ�����Ҹ�����ҳ����ڣ����Ҹɾ���û�б��޸Ĺ������ࣩ��
	// ��������ȡĿ¼�����ݡ������Ŀ¼��ԪЧ��P=0������ʾĿ¼���Ӧ�Ķ���ҳ�����ڣ����Ƿ��ء�����ȡ��Ŀ¼���Ӧҳ���ַfrom��
	// �Ӷ�������߼���ַaddress��Ӧ��ҳ����ָ�룬��ȡ���������������ʱ������phys_addr�С�
	/* is there a page-directory at from? */
	/* ��from���Ƿ����ҳĿ¼� */
	from = *(unsigned long *) from_page;                    		// p����Ŀ¼�����ݡ�
	if (!(from & 1))
		return 0;
	from &= 0xfffff000;                                     		// ҳ���ַ��
	from_page = from + ((address >> 10) & 0xffc);             		// ҳ����ָ�롣
	phys_addr = *(unsigned long *) from_page;               		// ҳ�������ݡ�
	// ���ſ���ҳ����ӳ�������ҳ���Ƿ���ڲ��Ҹɾ���0x41��Ӧҳ�����е�D��Dirty����P��present����־�����ҳ�治�ɾ�����Ч�򷵻ء�
	// Ȼ�����ǴӸñ�����ȡ������ҳ���ַ�ٱ�����phys_addr�С���������ټ��һ���������ҳ���ַ����Ч�ԣ�������Ӧ�ó����������
	// �����ֵַ��Ҳ��Ӧ��С�����ڵͶˣ�1MB����
	/* is the page clean and present? */
	/* ����ҳ��ɾ����Ҵ����� */
	if ((phys_addr & 0x41) != 0x01)
		return 0;
	phys_addr &= 0xfffff000;                                		// ����ҳ���ַ��
	if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)
		return 0;
	// �������ȶԵ�ǰ���̵ı�����в�����Ŀ����ȡ�õ�ǰ������address��Ӧ��ҳ�����ַ�����Ҹ�ҳ���û��ӳ������ҳ�棬����P=0��
	// ����ȡ��ǰ����ҳĿ¼������->to�������Ŀ¼��ԪЧ��P=0������Ŀ¼���Ӧ�Ķ���ҳ�����ڣ�������һ����ҳ�������ҳ��������
	// Ŀ¼��to_page���ݣ�����ָ����ڴ�ҳ�档
	to = *(unsigned long *) to_page;                        		// ��ǰ����Ŀ¼�����ݡ�
	if (!(to & 1))
		if (to = get_free_page())
			*(unsigned long *) to_page = to | 7;
		else
			oom();
	// ����ȡĿ¼���е�ҳ���ַ->to������ҳ������ֵ<<2����ҳ�����ڱ���ƫ�Ƶ�ַ���õ�ҳ�����ַ->to_page����Ը�ҳ������
	// ��ʱ���Ǽ������Ӧ������ҳ���Ѿ����ڣ���ҳ����Ĵ���λP=1,��˵��ԭ�������빲�����p�ж�Ӧ������ҳ�棬�����������Լ��Ѿ�
	// ռ���ˣ�ӳ���У�����ҳ�档����˵���ں˳���������
	to &= 0xfffff000;                                       		// ��ǰ���̵�ҳ���ַ��
	to_page = to + ((address >> 10) & 0xffc);                 		// ��ǰ���̵�ҳ�����ַ��
	if (1 & *(unsigned long *) to_page)
		panic("try_to_share: to_page already exists");
	/* share them: write-protect */
	/* �����ǽ��й�����д������*/
	*(unsigned long *) from_page &= ~2;
	*(unsigned long *) to_page = *(unsigned long *) from_page;
	// ���ˢ��ҳ�任���ٻ��塣��������������ҳ���ҳ��ţ�������Ӧҳ��ӳ���ֽ��������е����õ���1.��󷵻�1,��ʾ������ɹ���
	invalidate();
	phys_addr -= LOW_MEM;
	phys_addr >>= 12;                       						// ��ҳ��š�
	mem_map[phys_addr]++;
	return 1;
}

/*
 * share_page() tries to find a process that could share a page with
 * the current one. Address is the address of the wanted page relative
 * to the current data space.
 *
 * We first check if it is at all feasible by checking executable->i_count.
 * It should be >1 if there are other tasks sharing this inode.
 */
/*
 * share_page()��ͼ�ҵ�һ������,�������뵱ǰ���̹���ҳ��.����address�ǵ�ǰ�������ݿռ������������ĳҳ���ַ.
 *
 * ��������ͨ�����executable->i_count����֤�Ƿ����.��������������ѹ����inode,����Ӧ�ô���1.
 */
// ����ҳ�洦��.
// �ڷ���ȱҳ�쳣ʱ,���ȿ����ܷ�������ͬһ��ִ���ļ�����������ҳ�湲����.�ú��������ж�ϵͳ���Ƿ�����һ������Ҳ�������뵱ǰ����
// һ����ִ���ļ�.����,����ϵͳ��ǰ����������Ѱ������������.���ҵ�������������ͳ������乲��ָ����ַ����ҳ��.��ϵͳ��û����������
// ���������뵱ǰ������ͬ��ִ���ļ�,��ô����ҳ�������ǰ������������,��˺��������˳�.�ж�ϵͳ���Ƿ�����һ������Ҳ��ִ��ͬһ��ִ
// ���ļ��ķ��������ý����������ݽṹ�е�executable�ֶ�(��library�ֶ�).���ֶ�ָ���������ִ�г���(��ʹ�õĿ��ļ�)���ڴ��е�i��
// ��.���ݸ�i�ڵ�����ô���i_count���ǿ��Խ��������ж�.���ڵ��i_countֵ����1,�����ϵͳ��������������������ͬһ��ִ���ļ�(���
// �ļ�),���ǿ����ٶ�����ṹ��������������Ƚ��Ƿ�����ͬ��executable�ֶ�(��library�ֶ�)�����ȷ�����������������ִͬ���ļ���
// ���.����inode�������й���ҳ�����ִ���ļ����ڴ�i�ڵ�.address�ǽ����е��߼���ַ,����ǰ��������p���̹���ҳ����߼�ҳ���ַ.��
// ��1 - ��������ɹ�,0 - ʧ��.
static int share_page(struct m_inode * inode, unsigned long address)
{
	struct task_struct ** p;

	// ���ȼ��һ�²���ָ�����ڴ�i�ڵ����ü���ֵ.������ڴ�i�ڵ�����ü���ֵ����1(executalbe->i_count=1)����i�ڵ�ָ���,��ʾ��ǰϵ
	// ͳ��ֻ��1�����������и�ִ���ļ������ṩ��i�ڵ���Ч.����޹������,ֱ���˳�����.
	if (inode->i_count < 2 || !inode)
		return 0;
	// ��������������������������.Ѱ���뵱ǰ���̿ɹ���ҳ��Ľ���,��������ִͬ���ļ�����һ������,�����Զ�ָ����ַ��ҳ����й���.��������
	// ����ַaddressС�ڽ��̿��ļ����߼���ַ�ռ����ʼ��ַLIBRARY_OFFSET,����������ҳ���ڽ���ִ���ļ���Ӧ���߼���ַ�ռ䷶Χ��,����
	// ���һ��ָ��i�ڵ��Ƿ�����̵�ִ���ļ�i�ڵ�(������executable��ͬ,������ͬ�����Ѱ��.�������߼���ַaddress���ڵ��ڽ��̿��ļ���
	// �߼���ַ�ռ����ʼ��ַLIBRARY_OFFSET,�������Ҫ�����ҳ���ڽ���ʹ�õĿ��ļ���,���Ǽ��ָ���ڵ�inode�Ƿ�����̵Ŀ��ļ�i�ڵ���ͬ,
	// ������ͬ�����Ѱ��.����ҵ�ĳ������p,��executable��library��ָ���Ľڵ�inode��ͬ,�����ҳ����̽����try_to_share()����ҳ�湲
	// ��.����������ɹ�,��������1.���򷵻�0,��ʾ����ҳ�����ʧ��.
	for (p = &LAST_TASK ; p > &FIRST_TASK ; --p) {
		if (!*p)								// ��������������,�����Ѱ��.
			continue;
		if (current == *p)						// ����ǵ�ǰ����,Ҳ����Ѱ��.
			continue;
		if (address < LIBRARY_OFFSET) {
			if (inode != (*p)->executable)		// ����ִ���ļ�i�ڵ�.
				continue;
		} else {
			if (inode != (*p)->library)			// ����ʹ�ÿ��ļ�i�ڵ�.
				continue;
		}
		if (try_to_share(address, *p))			// ���Թ���ҳ��.
			return 1;
	}
	return 0;
}

// ִ��ȱҳ����.
// �Ƿ��ʲ�����ҳ�洦����.ҳ�쳣�жϴ�������е��õĺ���.��page.s�����б�����.��������error_code��address�ǽ����ڷ���ҳ��ʱ��CPU��
// ȱҳ�����쳣���Զ�����.error_codeָ����������;address�����쳣��ҳ�����Ե�ַ.
// �ú������Ȳ鿴��ȱҳ�Ƿ��ڽ����豸��,�����򽻻�����.���������Ѽ��ص���ͬ�ļ�����ҳ�湲��,����ֻ�����ڽ��̶�̬�����ڴ�ҳ���ֻ��ӳ��һҳ
// �����ڴ�ҳ����.������������ɹ�,��ôֻ�ܴ���Ӧ�ļ��ж�����ȱ������ҳ�浽ָ�����Ե�ַ��.
void do_no_page(unsigned long error_code, unsigned long address)
{
	int nr[4];
	unsigned long tmp;
	unsigned long page;
	int block, i;
	struct m_inode * inode;

	// �����ж�CPU���ƼĴ���CR2����������ҳ���쳣�����Ե�ַ��ʲô��Χ��.���addressС��TASK_SIZE(0x4000000,��64MB),��ʾ�쳣ҳ��λ�����ں�
	// ������0������1���������Ե�ַ��Χ��,���Ƿ���������Ϣ"�ں˷�Χ�ڴ汻д����";���(address-��ǰ���̴�����ʼ��ַ)����һ�����̵ĳ���(64MB),��ʾ
	// address��ָ�����Ե�ַ���������쳣�Ľ������Ե�ַ�ռ䷶Χ��,���ڷ���������Ϣ���˳�
	if (address < TASK_SIZE)
		printk("\n\rBAD!! KERNEL PAGE MISSING\n\r");
	if (address - current->start_code > TASK_SIZE) {
		printk("Bad things happen: nonexistent page error in do_no_page\n\r");
		do_exit(SIGSEGV);
	}
	// Ȼ�����ָ�������Ե�ַaddress������Ӧ�Ķ���ҳ����ָ��,�����ݸ�ҳ���������ж�address����ҳ���Ƿ��ڽ����豸��.���������ҳ�沢�˳�.����������
	// ȡָ�����Ե�ַaddress��Ӧ��Ŀ¼������.�����Ӧ�Ķ���ҳ�����,��ȡ����Ŀ¼���ж���ҳ��ĵ�ַ,����ҳ����ƫ��ֵ���õ����Ե�ַaddress��ҳ���Ӧ��
	// ҳ����ָ��,�Ӷ����ҳ��������.��ҳ�����ݲ�Ϊ0����ҳ�������λP=0,��˵����ҳ����ָ��������ҳ��Ӧ���ڽ����豸��.���Ǵӽ����豸�е���ָ��ҳ����˳�����.
	page = *(unsigned long *) ((address >> 20) & 0xffc);				// ȡĿ¼������.
	if (page & 1) {
		page &= 0xfffff000;												// ����ҳ���ַ.
		page += (address >> 10) & 0xffc;								// ҳ����ָ��.
		tmp = *(unsigned long *) page;									// ҳ��������.
		if (tmp && !(1 & tmp)) {
			swap_in((unsigned long *) page);							// �ӽ����豸��ҳ��.
			return;
		}
	}
	// ����ȡ���Կռ���ָ����ַaddress��ҳ���ַ,�����ָ�����Ե�ַ�ڽ��̿ռ�������ڽ��̻�ַ��ƫ�Ƴ���ֵtmp,����Ӧ���߼���ַ.�Ӷ��������ȱҳҳ����ִ���ļ�ӳ��
	// �л��ڿ��ļ��еľ�����ʼ���ݿ��.
	address &= 0xfffff000;												// address��ȱҳҳ���ַ.
	tmp = address - current->start_code;								// ȱҳҳ���Ӧ�߼���ַ.
	// ���ȱҳ��Ӧ���߼���ַtmp���ڿ�ӳ���ļ��ڽ����߼��ռ��е���ʼλ��,˵��ȱ�ٵ�ҳ���ڿ�ӳ���ļ���.���Ǵӵ�ǰ�����������ݽṹ�п���ȡ�ÿ�ӳ���ļ���i�ڵ�library,
	// ���������ȱҳ�ڿ��ļ��е���ʼ���ݿ��block.
	// ��Ϊ�����ϴ�ŵ�ִ���ļ�ӳ���1�������ǳ���ͷ�ṹ,����ڶ�ȡ���ļ�ʱ��Ҫ������1������.������Ҫ���ȼ���ȱҳ�������ݿ��.��Ϊÿ�����ݳ���ΪBLOCK_SIZE = 1KB,���
	// һҳ�ڴ�ɴ��4�����ݿ�.�����߼���ַtmp�������ݿ��С�ټ�1���ɵó�ȱ�ٵ�ҳ����ִ��ӳ���ļ��е���ʼ���block.
	if (tmp >= LIBRARY_OFFSET ) {
		inode = current->library;										// ���ļ�i�ڵ��ȱҳ��ʼ���.
		block = 1 + (tmp - LIBRARY_OFFSET) / BLOCK_SIZE;
	// ���ȱҳ��Ӧ���߼���ַtmpС�ڽ��̵�ִ��ӳ���ļ����߼���ַ�ռ��ĩ��λ��,��˵��ȱ�ٵ�ҳ���ڽ���ִ���ļ�ӳ����,���ǿ�
	// �Դӵ�ǰ�����������ݻ�����ȡ��ִ���ļ���i�ڵ��executable,���������ȱҳ��ִ���ļ�ӳ���е���ʼ���ݿ��block.���߼���ַtmp�Ȳ���ִ���ļ�ӳ��ĵ�ַ��Χ��,
	} else if (tmp < current->end_data) {
		inode = current->executable;									// ִ���ļ�i�ڵ��ȱҳ��ʼ���.
		block = 1 + tmp / BLOCK_SIZE;
	// Ҳ���ڿ��ļ��ռ䷶Χ��,��˵��ȱҳ�ǽ��̷��ʶ�̬������ڴ�ҳ����������,���û�ж�Ӧi�ڵ�����ݿ��(���ÿ�).
	} else {
		inode = NULL;													// �Ƕ�̬��������ݻ�ջ�ڴ�ҳ��.
		block = 0;
	}
	// ���ǽ��̷����䶯̬�����ҳ���Ϊ�˴��ջ��Ϣ�������ȱҳ�쳣,��ֱ������һҳ�����ڴ�ҳ�沢ӳ�䵽���Ե�ַaddress������.
	if (!inode) {														// �Ƕ�̬����������ڴ�ҳ��.
		get_empty_page(address);
		return;
	}
	// ����˵����ȱҳ�����ִ���ļ�����ļ���Χ��,���Ǿͳ��Թ���ҳ�����,���ɹ����˳�.
	if (share_page(inode, tmp))											// �����߼���ַtmp��ҳ��Ĺ���.
		return;
	// ��������ɹ���ֻ������һҳ�����ڴ�ҳ��page,Ȼ����豸�϶�ȡִ���ļ��е���Ӧҳ�沢����(ӳ��)������ҳ���߼���ַtmp��.
	if (!(page = get_free_page()))										// ����һҳ�����ڴ�.
		oom();
	/* remember that 1 block is used for header */
	/* ��ס,(����)ͷҪʹ��1�����ݿ� */
	// ���������ź�ִ���ļ���i�ڵ�,���ǾͿ��Դ�ӳ��λͼ���ҵ���Ӧ���豸�ж�Ӧ���豸�߼����(������nr[]������).����break_page()
	// ���ɰ���4���߼�����뵽����ҳ��page��.
	for (i = 0 ; i < 4 ; block++, i++)
		nr[i] = bmap(inode, block);
	bread_page(page, inode->i_dev, nr);
	// �ڶ��豸�߼������ʱ,���ܻ��������һ�����,����ִ���ļ��еĶ�ȡҳ��λ�ÿ������ļ�β����1��ҳ��ĳ���.��˾Ϳ��ܶ���һЩ����
	// ����Ϣ.����Ĳ������ǰ��ⲿ�ֳ���ִ���ļ�end_data�Ժ�Ĳ��ֽ������㴦��.��Ȼ,����ҳ����ĩ�˳���1ҳ,˵�����Ǵ�ִ���ļ�ӳ����
	// ��ȡ��ҳ��,���Ǵӿ��ļ��ж�ȡ��,��˲���ִ���������.
	i = tmp + 4096 - current->end_data;									// �������ֽڳ���ֵ.
	if (i > 4095)														// ��ĩ�˳���1ҳ��������.
		i = 0;
	tmp = page + 4096;
	while (i-- > 0) {
		tmp--;															// tmpָ��ҳ��ĩ��.
		*(char *)tmp = 0;       										// ҳ��ĩ��i�ֽ�����.
	}
	// ��������ȱҳ�쳣��һҳ����ҳ��ӳ�䵽ָ�����Ե�ַaddress��.�������ɹ��ͷ���.������ͷ��ڴ�ҳ,��ʾ�ڴ治��.
	if (put_page(page, address))
		return;
	free_page(page);
	oom();
}

// �����ڴ�����ʼ��.
// �ú�����1MB�����ڴ�������ҳ��Ϊ��λ���й���ǰ�ĳ�ʼ�����ù���.һ��ҳ�泤��Ϊ4KB�ֽ�.�ú��� 1MB�������������ڴ滮�ֳ�һ����ҳ��,��
// ʹ��һ��ҳ��ӳ���ֽ�����mem_map[]������������Щҳ��.���ھ���16MB�ڴ������Ļ���,�����鹲��3840��((16MB-1MB)/4KB),���ɹ���3840
// ������ҳ��.ÿ��һ�������ڴ�ҳ�汻ռ��ʱ�Ͱ�mem_map[]�ж�Ӧ���ֽ�ֵ��1;���ͷ�һ������ҳ��,�ͰѶ�Ӧ�ֽ�ֵ��1.���ֽ�ֵΪ0,���ʾ��Ӧҳ�����;
// ���ֽ�ֵ���ڻ����1,���ʾ��Ӧҳ�汻ռ�û򱻲�ͬ������ռ��.�ڸð汾��Linux�ں���,����ܹ���16MB�������ڴ�,����16MB���ڴ潫����
// ����.���ھ���16MB�ڴ��PCϵͳ,��û������������RAMDISK�������,����3072������ҳ��ɹ�����.����Χ0~1MB�ڴ�ռ������ں�ϵͳ(��ʵ�ں�
// ֻʹ��0~640KB,ʣ�µĲ��ֱ����ָ��ٻ�����豸�ڴ�ռ��).
// ����start_mem�ǿ�����ҳ���������ڴ�����ʼ��ַ(��ȥ��RAMDISK��ռ�ڴ�ռ�).end_mem��ʵ�������ڴ�����ַ.����ַ��Χstart_mem��
// end_mem�����ڴ���.
void mem_init(long start_mem, long end_mem)
{
	int i;

	// ���Ƚ�1MB��16MB��Χ�������ڴ�ҳ���Ӧ���ڴ�ӳ���ֽ���������Ϊ��ռ��״̬,�������ֽ�ֵȫ�����ó�USED(100).PAGING_PAGES������Ϊ(
	// PAGING_MEMORY>>12),��1MB�������������ڴ��ҳ����ڴ�ҳ����(15MB/4KB = 3840).
	HIGH_MEMORY = end_mem;									// �����ڴ���߶�(16MB).
	for (i = 0; i < PAGING_PAGES; i++)
		mem_map[i] = USED;
	// Ȼ��������ڴ�����ʼ�ڴ�start_mem��ҳ���Ӧ�ڴ�ӳ���ֽ����������i�����ڴ���ҳ����.��ʱmem_map[]����ĵ�i������Ӧ���ڴ����е�1��ҳ��.
	// ������ڴ�����ҳ���Ӧ������������(��ʾ����).���ھ���16MB�����ڴ��ϵͳ,mem_map[]�ж�Ӧ4MB~16MB���ڴ����������.
	i = MAP_NR(start_mem);									// ���ڴ�����ʼλ�ô�ҳ���.
	end_mem -= start_mem;
	// �õ����ڴ�����ҳ�������
	end_mem >>= 12;											// ���ڴ����е���ҳ����.
	// �����ڴ�����Ӧ��ҳ������Ӧ��������
	while (end_mem-- > 0)
		mem_map[i++] = 0;									// ���ڴ���ҳ���Ӧ�ֽ�ֵ����.
}

// ��ʾϵͳ�ڴ���Ϣ.
// �����ڴ�ӳ���ֽ�����mem_map[]�е���Ϣ�Լ�ҳĿ¼��ҳ������ͳ��ϵͳ��ʹ�õ��ڴ�ҳ���������ڴ������������ڴ�ҳ����.�ú�����chr_drv/keyboard.S����
// ������.
// ��������"Shift + Scroll Lock"��ϼ�ʱ����ʾϵͳ�ڴ�ͳ����Ϣ.
void show_mem(void)
{
	int i, j, k, free = 0, total = 0;
	int shared = 0;
	unsigned long * pg_tbl;

	// �����ڴ�ӳ���ֽ�����mem_map[],ͳ��ϵͳ���ڴ���ҳ������total,�Լ����п���ҳ����free�ͱ������ҳ����shared.����ʾ��Щ��Ϣ.
	printk("Mem-info:\n\r");
	for(i = 0 ; i < PAGING_PAGES ; i++) {
		if (mem_map[i] == USED)								// 1MB�����ڴ�ϵͳռ�õ�ҳ��.
			continue;
		// ͳ�����ڴ��е�ҳ����
		total++;
		if (!mem_map[i])
			// ͳ��δʹ�õ����ڴ�ҳ����
			free++;											// ���ڴ�������ҳ��ͳ��.
		else
			// ͳ�ƹ���ҳ����
			shared += mem_map[i] - 1;						// �����ҳ����(�ֽ�ֵ>1)
	}
	printk("%d free pages of %d\n\r", free, total);
	printk("%d pages shared\n\r", shared);
	// ͳ�ƴ�������ҳ�����߼�ҳ����.ҳĿ¼��ǰ4��ں˴���ʹ��,����Ϊͳ�Ʒ�Χ,���ɨ�账���ҳĿ¼��ӵ�5�ʼ.������ѭ����������ҳĿ¼��
	// (��ǰ4����),����Ӧ�Ķ���ҳ�����,��ô��ͳ�ƶ���ҳ����ռ�õ��ڴ�ҳ��,Ȼ��Ը�ҳ��������ҳ�����Ӧҳ���������ͳ��.
	k = 0;													// һ������ռ��ҳ��ͳ��ֵ.
	for(i = 4 ; i < 1024 ;) {
		if (1 & pg_dir[i]) {
			// (���ҳĿ¼���Ӧ����ҳ���ַ���ڻ�����������ڴ��ַHIGH_MEMORY,˵����Ŀ¼��������.������ʾ��Ŀ¼����Ϣ������������һ��Ŀ¼��.
			if (pg_dir[i] > HIGH_MEMORY) {					// Ŀ¼�����ݲ�����.
				printk("page directory[%d]: %08X\n\r",
					i, pg_dir[i]);
				continue;
			}
			// ���ҳĿ¼���Ӧ����ҳ���"��ַ"����LOW_MEM(��1MB),���һ������ռ�õ������ڴ�ҳͳ��ֵk��1,��ϵͳռ�õ����������ڴ�ҳͳ��ֵfree��1.
			// Ȼ��˶�Ӧҳ���ַpg_tb1,���Ը�ҳ��������ҳ�������ͳ��.�����ǰҳ������ָ����ҳ����ڲ��Ҹ�����ҳ��"��ַ"����LOW_MEME,��ô�ͽ�ҳ�����Ӧҳ��
			// ����ͳ��ֵ.
			if (pg_dir[i] > LOW_MEM)
				free++, k++;								// ͳ��ҳ��ռ��ҳ��.
			pg_tbl = (unsigned long *) (0xfffff000 & pg_dir[i]);
			for(j = 0 ; j < 1024 ; j++)
				if ((pg_tbl[j]&1) && pg_tbl[j] > LOW_MEM)
					// (��������ҳ���ַ���ڻ�����������ڴ��ַHIGH_MEMORY,��˵����ҳ��������������,������ʾ��ҳ��������.����ҳ�����Ӧҳ������ͳ��ֵ.)
					if (pg_tbl[j] > HIGH_MEMORY)
						printk("page_dir[%d][%d]: %08X\n\r",
							i, j, pg_tbl[j]);
					else
						k++, free++;						// ͳ��������Ӧҳ��.
		}
		// ��ÿ���������Կռ䳤����64MB,����һ������ռ��16��Ŀ¼��.�����ÿͳ����16��Ŀ¼��Ͱѽ��̵�����ṹռ�õ�ҳ��ͳ�ƽ���.����ʱk=0���ʾ��ǰ��16��ҳ
		// Ŀ¼����Ӧ�Ľ�����ϵͳ�в�����(û�д��������Ѿ���ֹ).����ʾ�˶�Ӧ���̺ź���ռ�õ������ڴ�ҳͳ��ֵk��,��k����,������ͳ����һ������ռ�õ��ڴ�ҳ����.
		i++;
		if (!(i & 15) && k) {
			k++, free++;									/* one page/process for task_struct */
			printk("Process %d: %d pages\n\r", (i >> 4) - 1, k);
			k = 0;
		}
	}
	// �����ʾϵͳ������ʹ�õ��ڴ�ҳ������ڴ������ܵ��ڴ�ҳ����.
	printk("Memory found: %d (%d)\n\r\n\r", free - shared, total);
}

