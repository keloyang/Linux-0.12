#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>

typedef int sig_atomic_t;                           // �����ź�ԭ�Ӳ������͡�
typedef unsigned int sigset_t;		                /* 32 bits */	// �����źż�����

#define _NSIG        32                             // �����ź����� -- 32�֡�
#define NSIG		 _NSIG                          // NSIG = _NSIG

#define SIGHUP		 1                              // Hang Up              -- �ҶϿ����ն˻����
#define SIGINT		 2                              // Interrupt	        -- ���Լ��̵��ж�
#define SIGQUIT		 3                              // Quit		            -- ���Լ��̵��˳�
#define SIGILL		 4                              // Illeagle             -- �Ƿ�ָ��
#define SIGTRAP		 5                              // Trap                 -- ���ٶϵ�
#define SIGABRT		 6                              // Abort                -- �쳣����
#define SIGIOT		 6                              // IO Trap              -- ͬ��
#define SIGUNUSED	 7                              // Unused               -- û��ʹ��
#define SIGFPE		 8                              // FPE                  -- Э����������
#define SIGKILL		 9	                            // Kill		            -- ǿ�Ƚ�����ֹ
#define SIGUSR1		10                              // User1                -- �û��ź�1�����̿�ʹ��
#define SIGSEGV		11	                            // Segment Violation    -- ��Ч�ڴ�����
#define SIGUSR2		12                              // User2                -- �û��ź�2,���̿�ʹ��
#define SIGPIPE		13                              // Pipe                 -- �ܵ�д�����޶���
#define SIGALRM		14	                            // Alarm	            -- ʵʱ��ʱ������
#define SIGTERM		15                              // Terminate            -- ������ֹ
#define SIGSTKFLT	16                              // Stack Fault          -- ջ����Э��������
#define SIGCHLD		17                              // Child                -- �ӽ���ֹͣ����ֹ
#define SIGCONT		18                              // Continue             -- �ָ����̼���ִ��
#define SIGSTOP		19	                            // Stop		            -- ֹͣ���̵�ִ��
#define SIGTSTP		20                              // TTY Stop	            -- tty����ֹͣ����,�ɺ���
#define SIGTTIN		21                              // TTY In               -- ��̨������������
#define SIGTTOU		22	                            // TTY Out	            -- ��̨�����������

/* Ok, I haven't implemented sigactions, but trying to keep headers POSIX */
/* OK�� �һ�û��ʵ��sigactions�ı��ƣ�����ͷ�ļ�����ϣ������POSIX��׼ */
// ����ԭע���Ѿ���ʱ����Ϊ��0.12�ں����Ѿ�ʵ����sigaction()��������sigaction�ṹsa_flags��־�ֶο�ȡ�ķ��ų���ֵ��
#define SA_NOCLDSTOP	1                          // ���ӽ��̴���ֹͣ״̬���Ͳ���SIGCHLD����
#define SA_INTERRUPT	0x20000000                 // ϵͳ���ñ��ź��жϺ���������ϵͳ���á�
#define SA_NOMASK	    0x40000000                 // ����ֹ��ָ�����źŴ�����������յ����źš�
#define SA_ONESHOT	    0x80000000                 // �źž��һ�������ù��ͻָ���Ĭ�ϴ�������

// ���³�������sigprocmask��how���� -- �ı������źż��������룩�����ڸı�ú�������Ϊ��
#define SIG_BLOCK          0	                   /* for blocking signals */              // �������źż��м��ϸ����źš�
#define SIG_UNBLOCK        1	                   /* for unblocking signals */            // �������źż���ɾ��ָ���źš�
#define SIG_SETMASK        2	                   /* for setting the signal mask */       // ���������źż���

// ���������������Ŷ���ʾָ���޷���ֵ����ָ�룬�Ҷ���һ��int���Ͳ�����������ָ��ֵ���߼��Ͻ�ʵ���ϲ����ܳ��ֵĺ�����ֵַ��
// ����Ϊ����signal�����ĵڶ������������ڸ�֪�ںˣ����ں˴����źŻ���Զ��źŵĴ���ʹ�÷����μ�kernle/signal.c����
#define SIG_DFL		((void (*)(int))0)	           /* default signal handling */           // Ĭ���źŴ�������źž������
#define SIG_IGN		((void (*)(int))1)	           /* ignore signal */                     // �����źŵĴ������
#define SIG_ERR		((void (*)(int))-1)	           /* error return from signal */          // �źŴ����ش���

// ���涨���ʼ��������sigaction�ṹ�ź�������ĺꡣ
#ifdef notdef
#define sigemptyset(mask) ((*(mask) = 0), 1)       // ��mask���㡣
#define sigfillset(mask) ((*(mask) = ~0), 1)       // ��mask����λ��λ��
#endif

// ������sigaction�����ݽṹ��
// as_handler�Ƕ�Ӧĳ�ź�ָ��Ҫ��ȡ���ж��������������SIG_DEL����SIG_IGN�����Ը��źţ�Ҳ������ָ������źź�����
// һ��ָ�롣
// as_mask�����˶��źŵ������룬���źų���ִ��ʱ����������Щ�źŵĴ���
// as_flagsָ���ı��źŴ�����̵��źż���������37-40�е�λ��־����ġ�
// as_restorer�ǻָ�����ָ�룬�ɺ�����Libc�ṩ�����������û�̬��ջ���μ�signal.c��
// ���⣬���𴥷��źŴ�����ź�Ҳ��������������ʹ����SA_NOMASK��־��
struct sigaction {
	void (*sa_handler)(int);                       // ��Ӧĳ�ź�ָ��Ҫ��ȡ���ж��������������SIG_DEL����SIG_IGN�����Ը��źţ�Ҳ������ָ������źź�����һ��ָ��
	sigset_t sa_mask;                              // ���źŵ������룬���źų���ִ��ʱ����������Щ�źŵĴ���
	int sa_flags;                                  // �ı��źŴ�����̵��źż���������37-40�е�λ��־����ġ�
	void (*sa_restorer)(void);                     // �ָ�����ָ�룬�ɺ�����Libc�ṩ�����������û�̬��ջ
};

// ����signal����������Ϊ�ź�_sig��װһ�µ��źŴ�������źž��������sigaction()���ơ��ú�����������������ָ����Ҫ�����
// �ź�_sig������һ���������޷���ֵ�ĺ���ָ��_func���ú�������ֵҲ�Ǿ���һ��int���������һ��(int)�����޷���ֵ�ĺ���ָ�룬
// ���Ǵ�����źŵ�ԭ��������
void (*signal(int _sig, void (*_func)(int)))(int);
// �������������ڷ����źš�kill()�������κν��̻�����鷢���źš�raise()������ǰ�����������źš������õȼ���kill(getpid(),
// sig)���μ�kernel/exit.c��
int raise(int sig);
int kill(pid_t pid, int sig);
// �ڽ�������ṹ�У�����һ���Ա���λ��ʾ��ǰ���̴������32λ�ź��ֶ�signal���⣬����һ��ͬ���Ա���λ��ʾ���������ν��̵�ǰ
// �����źż��������źż������ֶ�blocked��Ҳ��32λ��ÿ�����ش���һ����Ӧ�������źš��޸Ľ��̵������źż�������������������
// ָ�����źš�������������������ڲ������������źż�����Ȼʵ�������ܼ򵥣������汾�ں��л�δʵ�֡�
// ����sigaddset()��sigdelset()���ڶ��źż��е��źŽ�������ɾ�޸ġ�sigaddset()������maskָ����źż�������ָ���ź�signo��
// sigdelset��֮������sigemptyset()��sigfillset()���ڳ�ʼ�����������źż���ÿ��������ʹ���źż�ǰ������Ҫʹ������������
// ֮һ�������źż����г�ʼ����sigemptyset()����������ε������źţ�Ҳ����Ӧ���е��źš�sigfillset()���źż������������źţ�
// Ҳ�����������źš���ǰSIGINT��SIGSTOP�ǲ��ܱ����εġ�
// sigismember()���ڲ���һ��ָ���ź��Ƿ����źż��У�1 - �ǣ�0 - ���ǣ� -1 - ������
int sigaddset(sigset_t *mask, int signo);
int sigdelset(sigset_t *mask, int signo);
int sigemptyset(sigset_t *mask);
int sigfillset(sigset_t *mask);
int sigismember(sigset_t *mask, int signo); /* 1 - is, 0 - not, -1 error */
// ��set�е��źŽ��м�⣬���Ƿ��й�����źš���set�з��ؽ����е�ǰ���������źż���
int sigpending(sigset_t *set);
// ���溯�����ڸı����Ŀǰ���������źż����ź������룩����oldset�ǲ�NULL����ͨ���䷵�ؽ��̵�ǰ�����źż�����setָ�벻��NULL��
// �����howָʾ�޸Ľ��������źż���
int sigprocmask(int how, sigset_t *set, sigset_t *oldset);
// ���溯����sigmask��ʱ�滻���̵��ź������룬Ȼ����ͣ�ý���ֱ���յ�һ���źš�����׽��ĳһ�źŲ��Ӹ��źŴ�������з��أ���ú���
// Ҳ���أ������ź��������ָ�������ǰ��ֵ��
int sigsuspend(sigset_t *sigmask);
// sigaction()�������ڸı�������յ�ָ���ź�ʱ����ȡ���ж������ı��źŵĴ��������μ���kernel/signal.c�����˵����
int sigaction(int sig, struct sigaction *act, struct sigaction *oldact);

#endif /* _SIGNAL_H */

