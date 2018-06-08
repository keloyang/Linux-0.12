/*
 * Why isn't this a .c file?  Enquiring minds....
 */
/*
 * Ϊʲô�ⲻ��һ��.c�ļ�?�����Խ��Լ�����......
 */

#include <linux/sched.h>

extern int sys_setup();		    // 0 - ϵͳ������ʼ�����ú�����   ��kernel/blk_drv/hd.c��
extern int sys_exit();          // 1 - �����˳���               ��kernel/exit.c��
extern int sys_fork();		    // 2 - �������̡�               ��kernel/sys_call.s��
extern int sys_read();          // 3 - ���ļ���                 ��fs/read_write.c��
extern int sys_write();		    // 4 - д�ļ���                 ��fs/read_write.c��
extern int sys_open();          // 5 - ���ļ���               ��fs/open.c��
extern int sys_close();         // 6 - �ر��ļ���               ��fs/open.c��
extern int sys_waitpid();       // 7 - �ȴ�������ֹ��            ��kernel/exit.c��
extern int sys_creat();         // 8 - �����ļ���               ��fs/open.c��
extern int sys_link();          // 9 - ����һ���ļ���Ӳ���ӡ�     ��fs/namei.c��
extern int sys_unlink();        // 10 - ɾ��һ���ļ�������ɾ���ļ�������fs/namei.c��
extern int sys_execve();	    // 11 - ִ�г���.			(kernel/sys_call.s)
extern int sys_chdir();         // 12 - ���ĵ�ǰĿ¼��           ��fs/open.c��
extern int sys_time();          // 13 - ȡ��ǰʱ�䡣              (kernel/sys.c)
extern int sys_mknod();         // 14 - ������/�ַ������ļ���     ��fs/namei.c��
extern int sys_chmod();         // 15 - �޸��ļ����ԡ�           ��fs/open.c��
extern int sys_chown();         // 16 - �޸��ļ������������顣    ��fs/open.c��
extern int sys_break();         // 17                          ��kernel/sys.c��*
extern int sys_stat();          // 18 - ʹ��·����ȡ�ļ�״̬��Ϣ�� ��fs/stat.c��
extern int sys_lseek();         // 19 - ���¶�λ��/д�ļ�ƫ�ơ�   ��fs/read_write.c��
extern int sys_getpid();        // 20 - ȡ����id��              ��kernel/sched.c��
extern int sys_mount();         // 21 - ��װ�ļ�ϵͳ��           ��fs/super.c��
extern int sys_umount();        // 22 - ж���ļ�ϵͳ��           ��fs/super.c��
extern int sys_setuid();        // 23 - ���ý����û�id��         ��kernel/sys.c��
extern int sys_getuid();        // 24 - ȡ�����û�id��           ��kernel/sched.c��
extern int sys_stime();         // 25 - ����ϵͳʱ�����ڡ�        ��kernel/sys.c��*
extern int sys_ptrace();        // 26 - ������ԡ�              ��kernel/sys.c��*
extern int sys_alarm();         // 27 - ���ñ�����              ��kernel/sched.c��
extern int sys_fstat();         // 28 - ʹ���ļ����ȡ�ļ���״̬��Ϣ����fs/stat.c��
extern int sys_pause();		    // 29 - ��ͣ�������С�           ��kernel/sched.c��
extern int sys_utime();         // 30 - �ı��ļ��ķ��ʺ��޸�ʱ�䡣 ��fs/open.c��
extern int sys_stty();          // 31 - �޸��ն������á�         ��kernel/sys.c��*
extern int sys_gtty();          // 32 - ȡ�ն���������Ϣ��       ��kernel/sys.c��*
extern int sys_access();        // 33 - ����û���һ���ļ��ķ���Ȩ�ޡ���fs/open.c��
extern int sys_nice();          // 34 - ���ý���ִ������Ȩ��      ��kernel/sched.c��
extern int sys_ftime();         // 35 - ȡ���ں�ʱ�䡣           ��kernel/sys.c��*
extern int sys_sync();          // 36 - ͬ�����ٻ������豸������    (fs/buffer.c)
extern int sys_kill();          // 37 - ��ֹһ�����̡�           ��kernel/exit.c��
extern int sys_rename();        // 38 - �����ļ�����             ��kernel/sys.c��*
extern int sys_mkdir();         // 39 - ����Ŀ¼��              ��fs/namei.c��
extern int sys_rmdir();         // 50 - ɾ��Ŀ¼��              ��fs/namei.c��
extern int sys_dup();		    // 41 - �����ļ����.             (fs/fcntl.c)
extern int sys_pipe();          // 42 - �����ܵ���              ��fs/pipe.c��
extern int sys_times();         // 43 - ȡ����ʱ�䡣            ��kernel/sys.c)
extern int sys_prof();          // 44 - ����ִ��ʱ������       ��kernel/sys.c��*
extern int sys_brk();           // 45 - �޸����ݶγ���.           (kernel/sys.c)
extern int sys_setgid();        // 46 - ���ý�����id��          ��kernel/sys.c��
extern int sys_getgid();        // 47 - ȡ������id��            ��kernel/sys.c��
extern int sys_signal();        // 48 - �źŴ���              ��kernel/signal.c��
extern int sys_geteuid();       // 49 - ȡ������Ч�û�id��       ��kernel/sched.c��
extern int sys_getegid();       // 50 - ȡ������Ч��id��        ��kernel/sched.c��
extern int sys_acct();          // 51 - ���̼��ˡ�              ��kernel/sys.c��*
extern int sys_phys();          // 52 -                       ��kernel/sys.c��*
extern int sys_lock();          // 53 -                       ��kernel/sys.c��*
extern int sys_ioctl();         // 54 - �豸�����������.          (fs/ioctl.c)
extern int sys_fcntl();         // 55 - �ļ�������Ʋ�����       ��fs/fcntl.c��
extern int sys_mpx();           // 56 -                       ��kernel/sys.c��*
extern int sys_setpgid();       // 57 - ���ý�����id��          ��kernel/sys.c��
extern int sys_ulimit();        // 58 - ͳ�ƽ���ʹ����Դ�����    ��kernel/sys.c��
extern int sys_uname();         // 59 - ��ʾϵͳ��Ϣ��          ��kernel/sys.c��
extern int sys_umask();         // 60 - ȡĬ���ļ����������롣    ��kernel/sys.c��
extern int sys_chroot();        // 61 - �ı��Ŀ¼��            ��fs/open.c��
extern int sys_ustat();         // 62 - ȡ�ļ�ϵͳ��Ϣ��         ��fs/open.c��
extern int sys_dup2();          // 63 - �����ļ������          ��fs/fcntl.c��
extern int sys_getppid();       // 64 - ȡ������id��            ��kernel/sched.c��
extern int sys_getpgrp();       // 65 - ȡ������id������getpgid(0)����kernel/sys.c��
extern int sys_setsid();        // 66 - ���»Ự�����г���      ��kernel/sys.c��
extern int sys_sigaction();     // 67 - �ı��źŴ�����̡�       ��kernel/signal.c��
extern int sys_sgetmask();      // 68 - ȡ�ź������롣          ��kernel/signal.c��
extern int sys_ssetmask();      // 69 - �����ź������롣         ��kernel/signal.c��
extern int sys_setreuid();      // 70 - ������ʵ��/����Ч�û�id�� ��kernel/sys.c��
extern int sys_setregid();      // 71 - ������ʵ��/����Ч��id��  ��kernel/sys.c��
extern int sys_sigpending();    // 73 - �����δ������źš�      ��kernel/signal.c��
extern int sys_sigsuspend();    // 72 - ʹ���������������̡�    ��kernel/signal.c��
extern int sys_sethostname();   // 74 - ������������            ��kernel/sys.c��
extern int sys_setrlimit();     // 75 - ������Դʹ�����ơ�       ��kernel/sys.c��
extern int sys_getrlimit();     // 76 - ȡ�ý���ʹ����Դ�����ơ�  ��kernel/sys.c��
extern int sys_getrusage();     // 77 -
extern int sys_gettimeofday();  // 78 - ��ȡ����ʱ�䡣          ��kernel/sys.c��
extern int sys_settimeofday();  // 79 - ���õ���ʱ�䡣          ��kernel/sys.c��
extern int sys_getgroups();     // 80 - ȡ�ý����������ʶ�š�    ��kernel/sys.c��
extern int sys_setgroups();     // 81 - ���ý��̱�ʶ�����顣     ��kernel/sys.c��
extern int sys_select();        // 82 - �ȴ��ļ�������״̬�ı䡣  ��fs/select.c��
extern int sys_symlink();       // 83 - �����������ӡ�          ��fs/namei.c��
extern int sys_lstat();         // 84 - ȡ���������ļ�״̬��     ��fs/stat.c��
extern int sys_readlink();      // 85 - ��ȡ���������ļ���Ϣ��    ��fs/stat.c��
extern int sys_uselib();        // 86 - ѡ����⡣            ��fs/exec.c��

// ϵͳ���ú���ָ���.����ϵͳ�����жϴ������(int 0x80),��Ϊ��ת��.
fn_ptr sys_call_table[] = { sys_setup, sys_exit, sys_fork, sys_read,
sys_write, sys_open, sys_close, sys_waitpid, sys_creat, sys_link,
sys_unlink, sys_execve, sys_chdir, sys_time, sys_mknod, sys_chmod,
sys_chown, sys_break, sys_stat, sys_lseek, sys_getpid, sys_mount,
sys_umount, sys_setuid, sys_getuid, sys_stime, sys_ptrace, sys_alarm,
sys_fstat, sys_pause, sys_utime, sys_stty, sys_gtty, sys_access,
sys_nice, sys_ftime, sys_sync, sys_kill, sys_rename, sys_mkdir,
sys_rmdir, sys_dup, sys_pipe, sys_times, sys_prof, sys_brk, sys_setgid,
sys_getgid, sys_signal, sys_geteuid, sys_getegid, sys_acct, sys_phys,
sys_lock, sys_ioctl, sys_fcntl, sys_mpx, sys_setpgid, sys_ulimit,
sys_uname, sys_umask, sys_chroot, sys_ustat, sys_dup2, sys_getppid,
sys_getpgrp, sys_setsid, sys_sigaction, sys_sgetmask, sys_ssetmask,
sys_setreuid,sys_setregid, sys_sigsuspend, sys_sigpending, sys_sethostname,
sys_setrlimit, sys_getrlimit, sys_getrusage, sys_gettimeofday,
sys_settimeofday, sys_getgroups, sys_setgroups, sys_select, sys_symlink,
sys_lstat, sys_readlink, sys_uselib };

/* So we don't have to do any more manual updating.... */
/*���������������,���Ǿ������ֹ�����ϵͳ������Ŀ�ˡ�*/
int NR_syscalls = sizeof(sys_call_table)/sizeof(fn_ptr);

