/*
 * This file has definitions for some important file table
 * structures etc.
 */
/*
 * ���ļ�����ĳЩ��Ҫ�ļ���ṹ�Ķ����.
 */

#ifndef _FS_H
#define _FS_H

#include <sys/types.h>					// ����ͷ�ļ�.�����˻�����ϵͳ��������.

/* devices are as follows: (same as minix, so we can use the minix
 * file system. These are major numbers.)
 *
 * 0 - unused (nodev)
 * 1 - /dev/mem
 * 2 - /dev/fd
 * 3 - /dev/hd
 * 4 - /dev/ttyx
 * 5 - /dev/tty
 * 6 - /dev/lp
 * 7 - unnamed pipes
 */
/*
 * ϵͳ�������豸����:(��minixϵͳ��һ��,�������ǿ���ʹ��minix���ļ�ϵͳ.������Щ�����豸��.)
 *
 * 0 - unused (nodev)	û���õ�
 * 1 - /dev/mem			�ڴ��豸
 * 2 - /dev/fd			�����豸
 * 3 - /dev/hd			Ӳ���豸
 * 4 - /dev/ttyx		tty�����ն��豸
 * 5 - /dev/tty			tty�ն��豸
 * 6 - /dev/lp			��ӡ�豸
 * 7 - unnamed pipes	û�������Ĺܵ�
 */

#define IS_SEEKABLE(x) ((x) >= 1 && (x) <= 3)               // �ж��豸�Ƿ��ǿ���Ѱ�Ҷ�λ�ġ�

// ���豸��������
#define READ 		0          		// ��
#define WRITE 		1         		// д
#define READA 		2				/* read-ahead - don't pause */  // Ԥ��
#define WRITEA 		3				/* "write-ahead" - silly, but somewhat useful */        // Ԥд

void buffer_init(long buffer_end);						// ���ٻ�������ʼ������.

#define MAJOR(a) (((unsigned)(a)) >> 8)					// ȡ���ֽ�(���豸��)
#define MINOR(a) ((a) & 0xff)							// ȡ���ֽ�(���豸��)

#define NAME_LEN 14										// ���ֳ���ֵ.
#define ROOT_INO 1										// ��i�ڵ�.

#define I_MAP_SLOTS 8									// i�ڵ�λͼ����.
#define Z_MAP_SLOTS 8									// �߼���(���ο�)λͼ����.
#define SUPER_MAGIC 0x137F								// �ļ�ϵͳħ��.

#define NR_OPEN 		20								// ���������ļ���.
#define NR_INODE 		64								// ϵͳͬʱ���ʹ��I�ڵ����.
#define NR_FILE 		64								// ϵͳ����ļ�����(�ļ���������).
#define NR_SUPER 		8								// ϵͳ�������������(��������������).
#define NR_HASH 		307								// ������Hash����������ֵ.
#define NR_BUFFERS 		nr_buffers						// ϵͳ�����������.��ʼ�����ٸı�.
#define BLOCK_SIZE 		1024							// ���ݿ鳤��(�ֽ�ֵ)
#define BLOCK_SIZE_BITS 10								// ���ݿ鳤����ռ����λ��.
#ifndef NULL
#define NULL ((void *) 0)
#endif

#define INODES_PER_BLOCK ((BLOCK_SIZE) / (sizeof (struct d_inode)))               // ÿ���߼���ɴ�ŵ�i�ڵ���.
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE) / (sizeof (struct dir_entry)))        // ÿ���߼���ɴ�ŵ�Ŀ¼����.

// �ܵ�ͷ���ܵ�β���ܵ���С���ܵ��գ����ܵ��������ܵ�ͷָ�������
#define PIPE_READ_WAIT(inode) ((inode).i_wait)
#define PIPE_WRITE_WAIT(inode) ((inode).i_wait2)
#define PIPE_HEAD(inode) ((inode).i_zone[0])
#define PIPE_TAIL(inode) ((inode).i_zone[1])
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode) - PIPE_TAIL(inode)) & (PAGE_SIZE - 1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode) == PIPE_TAIL(inode))
#define PIPE_FULL(inode) (PIPE_SIZE(inode) == (PAGE_SIZE - 1))

#define NIL_FILP	((struct file *)0)      			// ���ļ��ṹָ�롣
#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

typedef char buffer_block[BLOCK_SIZE];          		// �黺������

// �����ͷ���ݽṹ.(��Ϊ��Ҫ!!!)
// �ڳ����г���bh����ʾbuffer_head���͵���д.
struct buffer_head {
	char * b_data;										/* pointer to data block (1024 bytes) */	// ָ��
	unsigned long b_blocknr;							/* block number */	// ���.
	unsigned short b_dev;								/* device (0 = free) */	// ����Դ���豸��.
	unsigned char b_uptodate;       					// ���±�־:��ʾ�����Ƿ��Ѹ���.
	unsigned char b_dirt;								/* 0-clean,1-dirty */	// �޸ı�־:0δ�޸�,1���޸�.
	unsigned char b_count;								/* users using this block */	// ʹ���û���.
	unsigned char b_lock;								/* 0 - ok, 1 -locked */	// �������Ƿ�����.
	struct task_struct * b_wait;						// ָ��ȴ��û���������������.
	struct buffer_head * b_prev;						// hash������ǰһ��(���ĸ�ָ�����ڻ������Ĺ���).
	struct buffer_head * b_next;						// hash��������һ��.
	struct buffer_head * b_prev_free;					// ���б���ǰһ��.
	struct buffer_head * b_next_free;					// ���б��Ϻ�һ��.
};

// �����ϵ������ڵ�(i�ڵ�)���ݽṹ.
struct d_inode {
	unsigned short i_mode;								// �ļ����ͺ�����(rwxλ).
	unsigned short i_uid;								// �û�id(�ļ�ӵ���߱�ʶ��).
	unsigned long i_size;								// �ļ���С(�ֽ���).
	unsigned long i_time;								// �޸�ʱ��(��1970.1.1.:0����,��).
	unsigned char i_gid;								// ��id(�ļ�ӵ�������ڵ���).
	unsigned char i_nlinks;								// ������(���ٸ��ļ�Ŀ¼��ָ���i�ڵ�).
	unsigned short i_zone[9];							// ֱ��(0-6),���(7)��˫�ؼ��(8)�߼����.
														// zone��������˼,���������,���߼���.
};

// ��ʱ�ڴ��е�i�ڵ�ṹ.ǰ7����d_inode��ȫһ��.
struct m_inode {
	unsigned short i_mode;								// �ļ����ͺ�����(rwxλ).
	unsigned short i_uid;								// �û�id(�ļ�ӵ���߱�ʶ��).
	unsigned long i_size;								// �ļ���С(�ֽ���).
	unsigned long i_mtime;								// �޸�ʱ��(��1970.1.1.:0����,��).
	unsigned char i_gid;								// ��id(�ļ�ӵ�������ڵ���).
	unsigned char i_nlinks;								// ������(���ٸ��ļ�Ŀ¼��ָ���i�ڵ�).
	unsigned short i_zone[9];							// ֱ��(0-6),���(7)��˫�ؼ��(8)�߼����.
	/* these are in memory also */
	struct task_struct * i_wait;						// �ȴ���i�ڵ�Ľ���.
	struct task_struct * i_wait2;						/* for pipes */
	unsigned long i_atime;								// ������ʱ��.
	unsigned long i_ctime;								// i�ڵ������޸�ʱ��.
	unsigned short i_dev;								// i�ڵ����ڵ��豸��.
	unsigned short i_num;								// i�ڵ��.
	unsigned short i_count;								// i�ڵ㱻ʹ�õĴ���,0��ʾ��i�ڵ����.
	unsigned char i_lock;								// ������־.
	unsigned char i_dirt;								// ���޸�(��)��־.
	unsigned char i_pipe;								// �ܵ���־.
	unsigned char i_mount;								// ��װ��־.
	unsigned char i_seek;								// ��Ѱ��־(lseekʱ).
	unsigned char i_update;								// ���±�־.
};

// �ļ��ṹ(�������ļ������i�ڵ�֮�佨����ϵ).
struct file {
	unsigned short f_mode;								// �ļ�����ģʽ(RWλ).
	unsigned short f_flags;								// �ļ��򿪺Ϳ��Ƶı�־.
	unsigned short f_count;								// ��Ӧ�ļ����ü���ֵ.
	struct m_inode * f_inode;							// ָ���Ӧi�ڵ�.
	off_t f_pos;										// �ļ�λ��(��дƫ��ֵ).
};

// �ڴ��д��̳�����ṹ.
struct super_block {
	unsigned short s_ninodes;							// �ڵ���.
	unsigned short s_nzones;							// �߼�����.
	unsigned short s_imap_blocks;						// i�ڵ�λͼ��ռ�õ����ݿ���.
	unsigned short s_zmap_blocks;						// �߼���λͼ��ռ�õ����ݿ���.
	unsigned short s_firstdatazone;						// ��һ�������߼����.
	unsigned short s_log_zone_size;						// log(���ݿ���/�߼���).(��2Ϊ��)
	unsigned long s_max_size;							// �ļ���󳤶�.
	unsigned short s_magic;								// �ļ�ϵͳħ��.
	/* These are only in memory */
	struct buffer_head * s_imap[8];						// i�ڵ�λͼ�����ָ������(ռ��8��,�ɱ�ʾ64M).
	struct buffer_head * s_zmap[8];						// �߼���λͼ�����ָ������(ռ��8��).
	unsigned short s_dev;								// �����������豸��.
	struct m_inode * s_isup;							// ����װ���ļ�ϵͳ��Ŀ¼��i�ڵ�.(isup-superi)
	struct m_inode * s_imount;							// ����װ����i�ڵ�.
	unsigned long s_time;								// �޸�ʱ��.
	struct task_struct * s_wait;						// �ȴ��ó�����Ľ���.
	unsigned char s_lock;								// ��������־.
	unsigned char s_rd_only;							// ֻ����־.
	unsigned char s_dirt;								// ���޸�(��)��־.
};

// �����ϳ�����ṹ.
struct d_super_block {
	unsigned short s_ninodes;							// �ڵ���.
	unsigned short s_nzones;							// �߼�����
	unsigned short s_imap_blocks;						// i�ڵ�λͼ��ռ�õ����ݿ���.
	unsigned short s_zmap_blocks;						// �߼���λͼ��ռ�õ����ݿ���.
	unsigned short s_firstdatazone;						// ��һ�������߼����.
	unsigned short s_log_zone_size;						// log(���ݿ���/�߼���).(��2Ϊ��)
	unsigned long s_max_size;							// �ļ���󳤶�.
	unsigned short s_magic;								// �ļ�ϵͳħ��.
};

// �ļ�Ŀ¼��ṹ.
struct dir_entry {
	unsigned short inode;								// i�ڵ��.
	char name[NAME_LEN];								// �ļ���,����NAME_LEN=14.
};

extern struct m_inode inode_table[NR_INODE];            // ����i�ڵ������(64��).
extern struct file file_table[NR_FILE];                 // �ļ�������(64��).
extern struct super_block super_block[NR_SUPER];        // ����������(8��).
extern struct buffer_head * start_buffer;              	// ��������ʼ�ڴ�λ��.
extern int nr_buffers;

// ���̲�������ԭ�͡�
extern void check_disk_change(int dev);                         // ����������������Ƿ�ı䡣
extern int floppy_change(unsigned int nr);                      // ���ָ�����������̸��������������̸������򷵻�1,���򷵻�0.
extern int ticks_to_floppy_on(unsigned int dev);                // ��������ָ������������ȴ�ʱ�䣨���õȴ���ʱ������
extern void floppy_on(unsigned int dev);                        // ����ָ����������
extern void floppy_off(unsigned int dev);                       // �ر�ָ����������������
// �������ļ�ϵͳ���������õĺ���ԭ�͡�
extern void truncate(struct m_inode * inode);                   // ��i�ڵ�ָ�����ļ���Ϊ0��
extern void sync_inodes(void);                                  // ˢ��i�ڵ���Ϣ��
extern void wait_on(struct m_inode * inode);                    // �ȴ�ָ����i�ڵ㡣
extern int bmap(struct m_inode * inode,int block);              // �߼��飨���Σ����̿飩λͼ������ȡ���ݿ�block���豸�϶�Ӧ���߼���š�
extern int create_block(struct m_inode * inode,int block);      // �������ݿ�block���豸�϶�Ӧ���߼��飬���������豸�ϵ��߼���š�

extern struct m_inode * namei(const char * pathname);           // ��ȡָ��·������i�ڵ��.
extern struct m_inode * lnamei(const char * pathname);          // ȡָ��·������i�ڵ㣬������������ӡ�
extern int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode);                           	// ����·����Ϊ���ļ�������׼����
extern void iput(struct m_inode * inode);                       // �ͷ�һ��i�ڵ㣨��д���豸����
extern struct m_inode * iget(int dev,int nr);                   // ���豸��ȡָ���ڵ�ŵ�һ��i�ڵ�.
extern struct m_inode * get_empty_inode(void);                  // ��i�ڵ��inode_table���л�ȡһ������i�ڵ��
extern struct m_inode * get_pipe_inode(void);                   // ��ȡ������һ���ܵ��ڵ㡣����Ϊi�ڵ�ָ�루�����NULL��ʧ�ܣ���
extern struct buffer_head * get_hash_table(int dev, int block); // �ڹ�ϣ���в���ָ�������ݿ顣�����ҵ��Ļ���ͷָ�롣
extern struct buffer_head * getblk(int dev, int block);         // ���豸��ȡָ����(���Ȼ���hash���в���).
extern void ll_rw_block(int rw, struct buffer_head * bh);       // ��/д���ݿ顣
extern void ll_rw_page(int rw, int dev, int nr, char * buffer); // ��/д����ҳ�棬��ÿ��4�����ݿ顣
extern void brelse(struct buffer_head * buf);                   // �ͷ�ָ������顣
extern struct buffer_head * bread(int dev,int block);           // ��ȡָ�������ݿ�.
extern void bread_page(unsigned long addr,int dev,int b[4]);    // ��ȡ�豸��һ��ҳ��(4�������)�����ݵ�ָ���ڴ��ַ����
extern struct buffer_head * breada(int dev,int block,...);      // ��ȡͷһ��ָ�������ݿ�,����Ǻ�����Ҫ���Ŀ�.
extern int new_block(int dev);                                  // ���豸dev����һ�����̿飨���Σ��߼��飩�������߼���š�
extern int free_block(int dev, int block);                      // �ͷ��豸�������е��߼��飨���Σ��߼��飩block��
extern struct m_inode * new_inode(int dev);                     // Ϊ�豸dev����һ����i�ڵ㣬����i�ڵ�š�
extern void free_inode(struct m_inode * inode);                 // �ͷ�һ��i�ڵ㣨ɾ���ļ�ʱ����
extern int sync_dev(int dev);                                   // ˢ��ָ���豸�������顣
extern struct super_block * get_super(int dev);                 // ��ȡָ���豸�ĳ�����.
extern int ROOT_DEV;
extern void put_super(int dev);									// �ͷų�����
extern void invalidate_inodes(int dev);							// �ͷ��豸dev���ڴ�i�ڵ���е�����i�ڵ�

extern void mount_root(void);                                   // ��װ���ļ�ϵͳ��

#endif

