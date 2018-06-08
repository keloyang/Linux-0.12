/*
 *  linux/kernel/blk_drv/ramdisk.c
 *
 *  Written by Theodore Ts'o, 12/2/91
 */
/*
 * ��Theodore Ts'o����,12/2/91
 */
// Theodore Ts'o(Ted Ts'0)��Linux�����е���������.Linux�����緶Χ�ڵ�����Ҳ�����ܴ�Ĺ���.����Linux����ϵͳ������ʱ,���ͻ��ż���
// ������ΪLinux�ķ�չ�ṩ�˵����ʼ��б����maillist,���ڱ�����������������Linux��ftp������վ��(tsx-11.mit.edu),����������Ϊ���
// Linux�û��ṩ����.����Linux�����������֮һ�������ʵ����ext2�ļ�ϵͳ.���ļ�ϵͳ�ѳ�ΪLinux��������ʵ�ϵ��ļ�ϵͳ��׼.��������
// �Ƴ���ext3�ļ�ϵͳ,���������ļ�ϵͳ���ȶ���,�ɻָ��Ժͷ���Ч��.��Ϊ�������Ƴ�,��97��(2002��5��)��LinuxJournal��Ū������Ϊ��
// ��������,���������˲ɷ�.Ŀǰ��ΪIMBLinux�������Ĺ���,���������й�LSB(Linux Standard Base)�ȷ���Ĺ���.(���ĸ���������:http://
// thunk.org/tytso/)

#include <string.h>								// �ַ���ͷ�ļ�.��Ҫ������һЩ�й��ַ���������Ƕ�뺯��.

// #include <linux/config.h>					// �ں�����ͷ�ļ�.����������Ժ�Ӳ������(HD_TYPE)��ѡ��.
// #include <linux/sched.h>						// ���Գ���ͷ�ļ�,����������ṹtask_struct,����0������,����һЩ�й��������������úͻ�ȡ��Ƕ��ʽ
												// �������.
#include <linux/fs.h>							// �ļ�ϵͳͷ�ļ�.�����ļ���ṹ(file,m_inode)��.
// #include <linux/kernel.h>					// �ں�ͷ�ļ�.����һЩ�ں˳��ú�����ԭ�Ͷ���.
// #include <asm/system.h>						// ϵͳͷ�ļ�.���������û��޸�������/�ж��ŵ�Ƕ��ʽ����.
// #include <asm/segment.h>						// �β���ͷ�ļ�.�������йضμĴ���������Ƕ��ʽ��ຯ��.
// #include <asm/memory.h>						// �ڴ濽��ͷ�ļ�.����memcpy()Ƕ��ʽ���꺯��.

// ����RAM�����豸�ŷ��ų���.���������������豸�ű������blk.h�ļ�֮ǰ������.
// ��Ϊblk.h�ļ���Ҫ��������ų���ֵ��ȷ��һЩ�е������������źͺ�.
#define MAJOR_NR 1
#include "blk.h"

// ���������ڴ��е���ʼλ��.��λ�û��ڵ�52���ϳ�ʼ������rd_init()��ȷ��.�μ��ں˳�ʼ������init/main.c.'rd'��
// 'ramdisk'����д.
char	*rd_start;								// ���������ڴ��еĿ�ʼ��ַ.
int	rd_length = 0;								// ��������ռ�ڴ��С(�ֽ�).

// �����̵�ǰ���������������
// �ú����ĳ���ṹ��Ӳ�̵�do_hd_request()�������ơ��ڵͼ����豸�ӿں���ll_rw_block()������������(rd)�������
// ��ӵ�rd��������֮��,�ͻ���øú�����rd��ǰ��������д���.�ú������ȼ��㵱ǰ��������ָ����ʼ������Ӧ�����������ڴ�
// ����ʼλ��addr��Ҫ�����������Ӧ���ֽڳ���ֵlen,Ȼ������������е�������в���.����д����WRITE,�Ͱ���������ָ��
// �����е�����ֱ�Ӹ��Ƶ��ڴ�λ��addr�������Ƕ���������֮�����ݸ�����ɺ󼴿�ֱ�ӵ���end_request()�Ա���������������
// ����Ȼ����ת��������ʼ����ȥ������һ�����������û�����������˳���
void do_rd_request(void)
{
	int	len;
	char	*addr;

	// ���ȼ��������ĺϷ���,����û�����������˳�(�μ�blk.h).Ȼ�����������������������ʼ�����������ڴ��ж�Ӧ�ĵ�ַ
	// addr��ռ�õ��ڴ��ֽڳ���ֵlen.�¾�����ȡ���������е���ʼ������Ӧ���ڴ���ʼλ�ú��ڴ泤��.����sector<<9��ʾ
	// sector * 512,������ֽ�ֵ.CURRENT������Ϊ(blk_dev[MAJOR_NR].current_request).
	INIT_REQUEST;
	addr = rd_start + (CURRENT->sector << 9);
	len = CURRENT->nr_sectors << 9;
	// �����ǰ�����������豸�Ų�Ϊ1���߶�Ӧ�ڴ���ʼλ�ô���������ĩβ������������������ת��repeat��ȥ������һ������
	// ����������repeat�����ں�INIT_REQUEST�ڣ�λ�ں�Ŀ�ʼ�����μ�blk.h�ļ���
	if ((MINOR(CURRENT->dev) != 1) || (addr + len > rd_start + rd_length)) {
		end_request(0);
		goto repeat;
	}
	// Ȼ�����ʵ�ʵĶ�д�����������д����(WRITE)�����������л����������ݸ��Ƶ���ַaddr��������Ϊlen�ֽڡ�����Ƕ���
	// ��(READ)����addr��ʼ���ڴ����ݸ��Ƶ�����������У�����Ϊlen�ֽڡ�������ʾ������ڣ�������
	if (CURRENT-> cmd == WRITE) {
		(void ) memcpy(addr,
			      CURRENT->buffer,
			      len);
	} else if (CURRENT->cmd == READ) {
		(void) memcpy(CURRENT->buffer,
			      addr,
			      len);
	} else
		panic("unknown ramdisk-command");
	// Ȼ����������ɹ������ø��±�־�������������豸����һ�����
	end_request(1);
	goto repeat;
}

/*
 * Returns amount of memory which needs to be reserved.
 */
/* �����ڴ�������ramdisk������ڴ��� */
// �����̳�ʼ������.
// �ú������������������豸�����������ָ��ָ��do_rd_request(),Ȼ��ȷ���������������ڴ��е���ʼ��ַ,ռ���ֽڳ���
// ֵ.��������������������.��󷵻���������.��linux/Makefile�ļ������ù�RAMDISKֵ��Ϊ��ʱ,��ʾϵͳ�лᴴ��RAM������
// �豸.����������µ��ں˳�ʼ��������,�������ͻᱻ����(init/main.c).�ú����ڵ�2������length�ᱻ��ֵ��RAMDISK*1024
// ��λΪ�ֽ�.
long rd_init(long mem_start, int length)
{
	int	i;
	char	*cp;

	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	rd_start = (char *) mem_start;
	rd_length = length;
	cp = rd_start;
	// ���ڴ�ռ�����
	for (i = 0; i < length; i++)
		*cp++ = '\0';
	return(length);
}

/*
 * If the root device is the ram disk, try to load it.
 * In order to do this, the root device is originally set to the
 * floppy, and we later change it to be ram disk.
 */
/*
 * ������ļ��豸(root device)��ramdisk�Ļ�,���Լ�����.root deviceԭ����ָ�����̵�,���ǽ����ĳ�ָ��ramdisk.
 */
// ���Ը��ļ�ϵͳ���ص���������.
// �ú��������ں����ú���setup()(hd.c)�б�����.����,1���̿� = 1024�ֽ�.����block=256��ʾ���ļ�ϵͳӳ�񱻴洢��boot�̵�
// 256���̿鿪ʼ��.
void rd_load(void)
{
	struct buffer_head *bh;								// ���������ͷָ��.
	struct super_block	s;								// �ļ�������ṹ.
	int		block = 256;								/* Start at block 256 */	/* ��ʼ��256�̿� */
	int		i = 1;
	int		nblocks;									// �ļ�ϵͳ�̿�����.
	char		*cp;									/* Move pointer */

	// ���ȼ�������̵���Ч�Ժ�������.���ramdisk�ĳ���Ϊ��,���˳�.������ʾramdisk�Ĵ�С�Լ��ڴ���ʼλ��.�����ʱ���ļ��豸��������
	// �豸,��Ҳ�˳�.
	if (!rd_length)
		return;
	printk("Ram disk: %d bytes, starting at 0x%x, dev = 0x%x \n", rd_length, (int) rd_start, ROOT_DEV);
	if (MAJOR(ROOT_DEV) != 2)
		return;
	// Ȼ������ļ�ϵͳ�Ļ�������.�������̿�256+1,256��256+2.����block+1��ָ�����ϵĳ�����.breada()���ڶ�ȡָ�������ݿ�,���������Ҫ���Ŀ�,
	// Ȼ�󷵻غ������ݿ�Ļ�����ָ��.�������NULL,���ʾ���ݿ鲻�ɶ�(fs/buffer.c).Ȼ��ѻ������еĴ��̳�����(d_super_block�Ǵ��̳���
	// ��ṹ)���Ƶ�s������,���ͷŻ�����.�������ǿ�ʼ�Գ��������Ч�Խ����ж�.���������ļ�ϵͳħ������,��˵�����ص����ݿ鲻��MINIX�ļ�
	// ϵͳ,�����˳�.
	bh = breada(ROOT_DEV, block + 1, block, block + 2, -1);
	if (!bh) {
		printk("Disk error while looking for ramdisk!\n");
		return;
	}
	*((struct d_super_block *) &s) = *((struct d_super_block *) bh->b_data);
	brelse(bh);
	if (s.s_magic != SUPER_MAGIC)
		/* No ram disk image present, assume normal floppy boot */
        /* ������û��ramdiskӳ���ļ�,�˳�ȥִ��ͨ������������ */
		return;
	// Ȼ��������ͼ���������ļ�ϵͳ�������ڴ�����������.����һ���ļ�ϵͳ��˵,�䳬����ṹ��s_nzones�ֶ��б��������߼�����(���Ϊ������).һ��
	// �߼����к��е����ݿ��������ֶ�s_log_zone_sizeָ��.����ļ�ϵͳ�е����ݿ�����nblocks�͵���(�߼����� *2^(ÿ���ο����Ĵη�)),��
	// nblocks=(s_nzones * 2^s_log_zone_size).��������ļ�ϵͳ�����ݿ����������ڴ��������������ɵĿ��������,����ִ�м��ز���,��ֻ����ʾ
	// ������Ϣ������.
	nblocks = s.s_nzones << s.s_log_zone_size;
	if (nblocks > (rd_length >> BLOCK_SIZE_BITS)) {
		printk("Ram disk image too big!  (%d blocks, %d avail)\n",
			nblocks, rd_length >> BLOCK_SIZE_BITS);
		return;
	}
	// �������������ɵ����ļ�ϵͳ�����ݿ���,��������ʾ����������Ϣ,����cpָ���ڴ���������ʼ��,Ȼ��ʼִ��ѭ�������������ϸ��ļ�ϵͳӳ����ص�
	// ��������.�ڲ���������,���һ����Ҫ���ص��̿�������2��,���Ǿ����ó�ǰԤ������breada(),�����ʹ��bread()�������е����ȡ.���ڶ��̹���
	// �г���I/O��������,��ֻ�ܷ������ع��̷���.����ȡ�Ĵ��̿��ʹ��memcpy()�����Ӹ��ٻ������и��Ƶ��ڴ���������Ӧλ�ô�,ͬʱ��ʾ�Ѽ��صĿ���.
	// ��ʾ�ַ����еİ˽�����'\010'��ʾ��ʾһ���Ʊ��.
	printk("Loading %d bytes into ram disk... (0k)",
		nblocks << BLOCK_SIZE_BITS);
	cp = rd_start;
	while (nblocks) {
		if (nblocks > 2)  								// ����ȡ��������2������ó�ǰԤ��.
			bh = breada(ROOT_DEV, block, block + 1, block + 2, -1);
		else											// ����͵����ȡ.
			bh = bread(ROOT_DEV, block);
		if (!bh) {
			printk("I/O error on block %d, aborting load\n",
				block);
			return;
		}
		(void) memcpy(cp, bh->b_data, BLOCK_SIZE);		// ���Ƶ�cp��.
		brelse(bh);
		cp += BLOCK_SIZE;								// ������ָ��ǰ��.
		block++;
		nblocks--;
		i++;
	}
	// ��boot���д�256�̿鿪ʼ�������ļ�ϵͳ������Ϻ�,������ʾ"done",����Ŀǰ���ļ��豸���޸ĳ������̵��豸��0x0101, ����.
	ROOT_DEV = 0x0101;
}

