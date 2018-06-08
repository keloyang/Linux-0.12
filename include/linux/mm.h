// mm.h���ڴ����ͷ�ļ�.������Ҫ�������ڴ�ҳ��Ĵ�С�ͼ���ҳ���ͷź���ԭ��.
#ifndef _MM_H
#define _MM_H

#define PAGE_SIZE 4096		                    // ����1ҳ�ڴ�ҳ���ֽ���.ע����ٻ���鳤����1024�ֽ�.

#include <linux/kernel.h>
#include <signal.h>

extern int SWAP_DEV;		                   // �ڴ�ҳ�潻���豸��.������mm/swap.c�ļ���.

// �ӽ����豸�����д���������ڴ�ҳ��.ll_rw_page()������blk_drv/ll_rw_block.c�ļ���.
// ����nr�����ڴ�����ҳ���;buffer�Ƕ�/д������.
#define read_swap_page(nr, buffer)   ll_rw_page(READ, SWAP_DEV, (nr), (buffer));
#define write_swap_page(nr, buffer)  ll_rw_page(WRITE, SWAP_DEV, (nr), (buffer));

extern unsigned long get_free_page(void);                                           // �����ڴ�����ȡ��������ҳ��.����Ѿ�û�п����ڴ���,�򷵻�0
extern unsigned long put_dirty_page(unsigned long page,unsigned long address);      // ��һ�������޸Ĺ��������ڴ�ҳ��ӳ�䵽���Ե�ַ�ռ䴦����put_page()������ȫһ����
extern void free_page(unsigned long addr);                                          // �ͷ������ַaddr��ʼ��1ҳ���ڴ档
extern void init_swapping(void);                                                    // �ڴ潻����ʼ��
void swap_free(int page_nr);                                                        // �ͷű��page_nr��1ҳ�潻��ҳ��
void swap_in(unsigned long *table_ptr);                                             // ��ҳ������table_ptr��һҳ�����ڴ滻���������ռ�

// ���溯����ǰ�ؼ���volatile���ڸ��߱�����gcc�ú������᷵��.��������gcc�������õĴ���,����Ҫ����ʹ������ؼ��ֿ��Ա������ĳЩ(δ
//����ʼ��������)�پ�����Ϣ.
static inline void oom(void)
{
	printk("out of memory\n\r");
    //��do_exit()Ӧ��ʹ���˳�����,����������ϢֵSIGSEGV(11).��ֵͬ�ĳ����뺬����"��Դ�ݲ�����",����ͬ��.
	do_exit(SIGSEGV);
}

// ˢ��ҳ�任���ٻ���꺯��.
// Ϊ����ߵ�ַת����Ч��,CPU�����ʹ�õ�ҳ�����ݴ����оƬ�и��ٻ�����.���޸Ĺ�ҳ����Ϣ֮��,����Ҫˢ�¸û�����.����ʹ�����¼���ҳ
// Ŀ¼���¼���ҳĿ¼��ַ�Ĵ���cr3�ķ���������ˢ��.����eax = 0,��ҳĿ¼�Ļ�ַ.
#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (0))

/* these are not to be changed without changing head.s etc */
/* ���涨������Ҫ�Ķ�,����Ҫ��head.s���ļ� �������Ϣһ��ı� */
// Linux0.12�ں�Ĭ��֧�ֵ�����ڴ�������16MB,�����޸���Щ�������ʺϸ�����ڴ�.
#define LOW_MEM 0x100000			             // ���������ڴ�Ͷ�(1MB)
extern unsigned long HIGH_MEMORY;		         // ���ʵ�������ڴ���߶˵�ַ.
#define PAGING_MEMORY (15 * 1024 * 1024)         // ��ҳ�ڴ�15MB.���ڴ������15MB.
#define PAGING_PAGES (PAGING_MEMORY >> 12)	     // ��ҳ��������ڴ�ҳ����(3840).
#define MAP_NR(addr) (((addr) - LOW_MEM) >> 12)	 // ָ���ڴ��ַӳ��Ϊҳ���.
#define USED 100				                 // ҳ�汻ռ�ñ�־.

// �ڴ�ӳ���ֽ�ͼ(1�ֽڴ���1ҳ�ڴ�).ÿ��ҳ���Ӧ���ֽ����ڱ�־ҳ�浱ǰ������(ռ��)����.��������ӳ��15MB���ڴ�ռ�.�ڳ�ʼ������
// mem_init()��,���ڲ����������ڴ���ҳ���λ�þ�����ѡ�����ó�USED(100).
extern unsigned char mem_map [ PAGING_PAGES ];

// ���涨��ķ��ų�����ӦҳĿ¼�����ҳ��(����ҳ��)���е�һЩ��־λ.
#define PAGE_DIRTY	         0x40	            // λ6,ҳ����(���޸�)
#define PAGE_ACCESSED	     0x20	            // λ5,ҳ�汻���ʹ�.
#define PAGE_USER	         0x04	            // λ2,ҳ������:1 - �û�;0 - �����û�.
#define PAGE_RW		         0x02	            // λ1,��дȨ:1 - д;0 - ��.
#define PAGE_PRESENT	     0x01	            // λ0,ҳ�����:1 - ����;0 - ������.

#endif

