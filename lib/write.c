/*
 *  linux/lib/write.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>	                        // Linux��׼ͷ�ļ�.�����˸��ַ��ų�����,�������˸��ֺ���.
			                                // �綨����__LIBRARY__,�򻹺�ϵͳ���úź���Ƕ���_syscall0()��.

// д�ļ�ϵͳ���ú�����.
// �ú�ṹ��Ӧ�ں���:int write(int fd, const char * buf, off_t count)
// ����:fd - �ļ�������;buf - д����ָ��; count - д�ֽ���.
// ����:�ɹ�ʱ����д����ֽ���(0��ʾд��0�ֽ�);����ʱ������-1,���������˳����.
_syscall3(int, write, int, fd, const char *, buf, off_t, count)

