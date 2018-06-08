#ifndef _ERRNO_H
#define _ERRNO_H

/*
 * ok, as I hadn't got any other source of information about
 * possible error numbers, I was forced to use the same numbers
 * as minix.
 * Hopefully these are posix or something. I wouldn't know (and posix
 * isn't telling me - they want $$$ for their f***ing standard).
 *
 * We don't use the _SIGN cludge of minix, so kernel returns must
 * see to the sign by themselves.
 *
 * NOTE! Remember to change strerror() if you change this file!
 */
/*
 * OK,������û�еõ��κ������йس���ŵ�����,��ֻ��ʹ����minixϵͳ��ͬ�ĳ������.
 * ϣ����Щ��POSIX���ݵĻ�����һ���̶�����������,�Ҳ�֪��(����POSIXû�и����� - Ҫ������ǵı�׼��Ҫ��Ǯ).
 *
 * ����û��ʹ��minix������_SIGN��,�����ں˵ķ���ֵ�����Լ����������.
 *
 * ע��!�����ı���ļ��Ļ�,����ҲҪ�޸�strerror()����.
 */

// ϵͳ�����Լ��ܶ�⺯������һ�������ֵ�Ա�ʾ����ʧ�ܻ����.���ֵͨ��ѡ��-1��������һЩ�ض���ֵ����ʾ.�����������ֵ��˵����������
// �����Ҫ֪�����������,����Ҫ�鿴��ʾϵͳ����ŵı���errno.�ñ�������errno.h�ļ�������.�ڳ���ʼִ��ʱ�ñ���ֵ����ʼ��Ϊ0.
extern int errno;

// �ڳ���ʱ��ϵͳ���û�ѳ���ŷ��ڱ���erron�У���ֵ����Ȼ�󷵻�-1.��˳�������Ҫ֪���������ţ�����Ҫ�鿴erron��ֵ��
#define ERROR		    99           // һ�����
#define EPERM		     1	         // ����û�����.
#define ENOENT		     2	         // �ļ���Ŀ¼������.
#define ESRCH		     3           // ָ���Ľ��̲�����
#define EINTR		     4           // �жϵ�ϵͳ����
#define EIO		         5	         // ����/�����.
#define ENXIO		     6           // ָ���豸���ַ������
#define E2BIG		     7           // �����б�̫��
#define ENOEXEC		     8           // ִ�г����ʽ����
#define EBADF		     9	         // �ļ����(������)����.
#define ECHILD		    10           // �ӽ��̲�����
#define EAGAIN		    11	         // ��Դ�ݲ�����.
#define ENOMEM		    12	         // �ڴ治��
#define EACCES		    13	         // û�����Ȩ��
#define EFAULT		    14           // ��ַ��
#define ENOTBLK		    15           // ���ǿ��豸�ļ�
#define EBUSY		    16           // ��Դ��æ
#define EEXIST		    17	         // �ļ��Ѵ���.
#define EXDEV		    18           // �Ƿ�����
#define ENODEV		    19	         // �豸������.
#define ENOTDIR		    20           // ����Ŀ¼�ļ�
#define EISDIR		    21	         // ��Ŀ¼�ļ�.
#define EINVAL		    22           // ������Ч
#define ENFILE		    23           // ϵͳ���ļ���̫��
#define EMFILE		    24	         // ���ļ���̫��.
#define ENOTTY		    25           // ��ǡ����IO���Ʋ�����û��tty�նˣ�
#define ETXTBSY		    26           // ����ʹ��
#define EFBIG		    27           // �ļ�̫��
#define ENOSPC		    28           // �豸�������豸�Ѿ�û�пռ䣩
#define ESPIPE		    29           // ��Ч���ļ�ָ���ض�λ
#define EROFS		    30           // �ļ�ϵͳֻ��
#define EMLINK		    31           // ����̫��
#define EPIPE		    32           // �ܵ���
#define EDOM		    33           // ��domain������
#define ERANGE		    34           // ���̫��
#define EDEADLK		    35           // ������Դ����
#define ENAMETOOLONG	36           // �ļ���̫��
#define ENOLCK		    37           // û����������
#define ENOSYS		    38           // ���ܻ�û��ʵ��
#define ENOTEMPTY	    39           // Ŀ¼����

/* Should never be seen by user programs */
#define ERESTARTSYS	    512         // ����ִ��ϵͳ����
#define ERESTARTNOINTR	513         // ����ִ��ϵͳ���ã����ж�

#endif

