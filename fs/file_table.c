/*
 *  linux/fs/file_table.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/fs.h>		// �ļ�ϵͳͷ�ļ�,�����ļ���ṹ(file_,buffer_head,m_inode��).

struct file file_table[NR_FILE];	// �ļ�������(64��).

