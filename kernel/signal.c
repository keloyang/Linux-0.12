/*
 *  linux/kernel/signal.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/sched.h>
//#include <linux/kernel.h>
#include <asm/segment.h>

#include <signal.h>
#include <errno.h>

// ��ȡ��ǰ�����ź�����λͼ��������������룩��sgetmask�ɷֽ�Ϊsignal-get-mask���������ơ�
int sys_sgetmask()
{
	return current->blocked;
}

// �����µ��ź�����λͼ���ź�SIGKILL��SIGSTOP���ܱ����Ρ�����ֵ��ԭ�ź�����λͼ��
int sys_ssetmask(int newmask)
{
	int old = current->blocked;

	current->blocked = newmask & ~(1 << (SIGKILL - 1)) & ~(1 << (SIGSTOP - 1));
	return old;
}

// ��Ⲣȡ�ý����յ��ĵ������Σ����������źš���δ�����źŵ�λͼ��������set�С�
int sys_sigpending(sigset_t *set)
{
    /* fill in "set" with signals pending but blocked. */
    /* �û�δ�����ұ������źŵ�λͼ����setָ����ָλ�ô� */
	// ������֤�����ṩ���û��洢�ռ����4���ֽڡ�Ȼ��ѻ�δ�����ұ������źŵ�λͼ����setָ����ָλ�ô���
    verify_area(set, 4);
    put_fs_long(current->blocked & current->signal, (unsigned long *)set);
    return 0;
}

/* atomically swap in the new signal mask, and wait for a signal.
 *
 * we need to play some games with syscall restarting.  We get help
 * from the syscall library interface.  Note that we need to coordinate
 * the calling convention with the libc routine.
 *
 * "set" is just the sigmask as described in 1003.1-1988, 3.3.7.
 * 	It is assumed that sigset_t can be passed as a 32 bit quantity.
 *
 * "restart" holds a restart indication.  If it's non-zero, then we
 * 	install the old mask, and return normally.  If it's zero, we store
 * 	the current mask in old_mask and block until a signal comes in.
 */
/*
 * �Զ��ظ������µ��ź������룬���ȴ��źŵĵ�����
 *
 * ������Ҫ��ϵͳ���ã�syscall����һЩ�������ǻ��ϵͳ���ÿ�ӿ�ȡ��ĳЩ��Ϣ��ע�⣬������Ҫ�ѵ��ù�����libc��
 * �е��ӳ���ͳһ���ǡ�
 *
 * ��set������POSIX��׼1003.1-1998��3.3.7�������������ź�������sigmask��������Ϊ����sigset_t�ܹ���Ϊһ��32λ
 * �����ݡ�
 *
 * ��restart���б�����������־�����Ϊ��0ֵ����ô���Ǿ�����ԭ���������룬�����������ء������Ϊ0,��ô���ǾͰѵ�ǰ��
 * �����뱣����oldmask�в����������̣�ֱ���յ��κ�һ���ź�Ϊֹ��
 */
// ��ϵͳ������ʱ�ѽ����ź��������滻�ɲ����и�����set��Ȼ�������̣�ֱ���յ�һ���ź�Ϊֹ��
// restart��һ�����жϵ�ϵͳ��������������־������1�ε��ø�ϵͳ����ʱ������0.�����ڸú����л�ѽ���ԭ����������
// blocked����������old_mask����������restartΪ��0ֵ����˵����̵�2�ε��ø�ϵͳ����ʱ�����ͻ�ָ�����ԭ��������
// old_mask�е������롣
int sys_sigsuspend(int restart, unsigned long old_mask, unsigned long set)
{
	// pause()ϵͳ���ý����µ������Ľ��̽���˯��״̬��ֱ���յ�һ���źš����źŻ��߻���ֹ����ʱִ�У����ߵ��½���ȥִ��
	// ��Ӧ���źŲ�������
    extern int sys_pause(void);

	// ���restart��־��Ϊ0,��ʾ�����ó����������������ǻָ�ǰ�汣����old_mask�е�ԭ���������롣��������-EINTR��ϵͳ
	// ���ñ��ź��жϣ���
    if (restart) {
			/* we're restarting */  /* ����������������ϵͳ���� */
			current->blocked = old_mask;
			return -EINTR;
    }
	// �����ʾrestart��־��ֵ��0.��ʾ��1�ε��á�������������restart��־����Ϊ1����������̵�ǰ������blocked��old_mask
	// �У����ѽ��̵��������滻��set��Ȼ�����pause()�ý���˯�ߣ��ȴ��źŵĵ������������յ�һ���ź�ʱ��pause()�ͻ᷵�أ�����
	// ���̻�ȥִ���źŴ�������Ȼ�󱾵��÷���-ERESTARTNOINTR���˳������������˵���ڴ������źź�Ҫ�󷵻ص���ϵͳ�����м���
	// ���У�����ϵͳ���ò��ᱻ�жϡ�
    /* we're not restarting.  do the work */
    /* ���ǲ����������У���ô�͸ɻ�� */
    //*(&restart) = 1;
	__asm__("movl $1, %0\n\t" \
			: \
			:"m"(restart));
    //*(&old_mask) = current->blocked;
	__asm__("movl %%eax, %0\n\t" \
			: \
			:"m"(old_mask), "a"(current->blocked));
    current->blocked = set;
    (void) sys_pause();			/* return after a signal arrives */
    return -ERESTARTNOINTR;		/* handle the signal, and come back */
}

// ����sigaction���ݵ�fs���ݶ�to���������ں˿ռ临�Ƶ��û����������ݶ��С�
static inline void save_old(char * from, char * to)
{
	int i;

	// ������֤to�����ڴ�ռ��Ƿ��㹻��Ȼ���һ��sigaction�ṹ��Ϣ���Ƶ�fs�Σ��û����ռ��С��꺯��put_fs_byte()��
	// include/asm/segment.h��ʵ�֡�
	verify_area(to, sizeof(struct sigaction));
	for (i = 0 ; i < sizeof(struct sigaction) ; i++) {
		put_fs_byte(*from, to);
		from++;
		to++;
	}
}

// ��sigaction���ݴ�fs���ݶ�fromλ�ø��Ƶ�to���������û����ݿռ�ȡ���ں����ݶ��С�
static inline void get_new(char * from, char * to)
{
	int i;

	for (i = 0 ; i < sizeof(struct sigaction) ; i++)
		*(to++) = get_fs_byte(from++);
}

// signal()ϵͳ���á�������sigaction()��Ϊָ�����źŰ�װ�µ��źž�����źŴ�����򣩡�
// �źž���������û�ָ���ĺ�����Ҳ������SIG_DFL��Ĭ�Ͼ������SIG_IGN�����ԣ���
// ����signum -- ָ�����źţ� handler -- ָ���ľ���� restorer -- �ָ�����ָ�룬�ú�����Libc���ṩ���������ź�
// ������������ָ�ϵͳ���÷���ʱ�����Ĵ�����ԭ��ֵ�Լ�ϵͳ���õķ���ֵ���ͺ���ϵͳ����û��ִ�й��źŴ�������ֱ��
// ���ص��û�����һ������������ԭ�źž����
int sys_signal(int signum, long handler, long restorer)
{
	struct sigaction tmp;

	// ������֤�ź�ֵ����Ч��Χ��1--32���ڣ����Ҳ������ź�SIGKILL��SIGSTOP����Ϊ�������źŲ��ܱ����̲���
	if (signum < 1 || signum > 32 || signum == SIGKILL || signum == SIGSTOP)
		return -EINVAL;
	// Ȼ������ṩ�Ĳ����齨sigaction�ṹ���ݡ�sa_handler��ָ�����źŴ���������������sa_mask��ִ���źŴ�����ʱ��
	// �ź������롣sa_flags��ִ��ʱ��һЩ��־��ϡ������趨���źŴ�����ֻʹ��1�κ�ͻָ���Ĭ��ֵ���������ź����Լ��Ĵ���
	// ������յ���
	tmp.sa_handler = (void (*)(int)) handler;
	tmp.sa_mask = 0;
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;
	tmp.sa_restorer = (void (*)(void)) restorer;    				// ����ָ�������ָ�롣
	// ����ȡ���ź�ԭ���Ĵ������������ø��źŵ�sigaction�ṹ����󷵻�ԭ�źž����
	handler = (long) current->sigaction[signum - 1].sa_handler;
	current->sigaction[signum - 1] = tmp;
	return handler;
}

// sigaction()ϵͳ���á��ı�������յ�һ���ź�ʱ�Ĳ�����signum�ǳ���SIGKILL������κ��źš�[����²�����action����Ϊ��]
// ���²�������װ�����oldactionָ�벻Ϊ�գ���ԭ������������oldaction���ɹ��򷵻�0,����Ϊ-EINVAL��
int sys_sigaction(int signum, const struct sigaction * action,
	struct sigaction * oldaction)
{
	struct sigaction tmp;

	// ������֤�ź�ֵ����Ч��Χ��1--32���ڣ����Ҳ������ź�SIGKILL��SIGSTOP����Ϊ�������źŲ��ܱ����̲���
	if (signum < 1 || signum > 32 || signum == SIGKILL || signum == SIGSTOP)
		return -EINVAL;
	// ���źŵ�sigaction�ṹ�������µĲ����������������oldactionָ�벻Ϊ�յĻ�����ԭ����ָ�뱣�浽oldaction��ָ��λ�á�
	tmp = current->sigaction[signum - 1];
	get_new((char *) action,
		(char *) (signum - 1 + current->sigaction));
	if (oldaction)
		save_old((char *) &tmp,(char *) oldaction);
	// ��������ź����Լ����źž�����յ�������������Ϊ0,�����������α��źš�
	if (current->sigaction[signum - 1].sa_flags & SA_NOMASK)
		current->sigaction[signum - 1].sa_mask = 0;
	else
		current->sigaction[signum - 1].sa_mask |= (1 << (signum - 1));
	return 0;
}

/*
 * Routine writes a core dump image in the current directory.
 * Currently not implemented.
 */
/*
 * �ڵ�ǰĿ¼�в���core dumpӳ���ļ����ӳ���Ŀǰ��û��ʵ�֡�
 */
int core_dump(long signr)
{
	return(0);	/* We didn't do a dump */
}

// ϵͳ���õ��жϴ���������������ź�Ԥ���������kernel/sys_call.s������δ������Ҫ�����ǽ��źŴ��������뵽
// �û������ջ�У����ڱ�ϵͳ���ý������غ�����ִ���źž������Ȼ�����ִ���û��ĳ���
// �����Ĳ����ǽ���ϵͳ���ô������sys_call.s��ʼ��ֱ�����ñ�����ǰ��ѹ���ջ��ֵ����Щֵ������
// 1��CPUִ���ж�ָ��ѹ����û�ջ��ַss��esp����־�Ĵ���eflags�ͷ��ص�ַcs��eip��
// 2����85--91���ڸս���system_callʱѹ��ջ�ĶμĴ���ds��es��fs�Լ��Ĵ���eax��orig_eax����edx��ecx��ebx��ֵ��
// 3����100�е���sys_call_tables��ѹ��ջ�е���Ӧϵͳ���ô������ķ���ֵ��eax����
// 4����124��ѹ��ջ�еĵ�ǰ������ź�ֵ��signr����
int do_signal(long signr, long eax, long ebx, long ecx, long edx, long orig_eax,
	long fs, long es, long ds,
	long eip, long cs, long eflags,
	unsigned long * esp, long ss)
{
	unsigned long sa_handler;
	long old_eip = eip;
	struct sigaction * sa = current->sigaction + signr - 1;			// �õ���Ӧ�źŵĴ������ݽṹ
	int longs;                      								// ��current->sigaction[signr-1]��

	unsigned long * tmp_esp;

	// �����ǵ�����䡣��������notdefʱ���ӡ�����Ϣ��
#ifdef notdef
	printk("pid: %d, signr: %d, eax=%d, oeax = %d, int=%d\n",
		current->pid, signr, eax, orig_eax,
		sa->sa_flags & SA_INTERRUPT);
#endif	/* Continue, execute handler */
	// �������ϵͳ���ö��������ж�ִ�й����е��õı�����ʱ��roig_eaxֵΪ-1����˵�orig_eax������-1ʱ��˵������ĳ��ϵͳ����
	// ���������˱���������kernel/exit.c��waitpid()�����У�����յ���SIGCHLD�źţ������ڶ��ܵ�����fs/pipe.c�У��ܵ�
	// ��ǰ�����ݵ�û�ж����κ����ݵ�����£������յ����κ�һ�����������źţ��򶼻�-ERESTARTSYS����ֵ���ء�����ʾ���̿��Ա�
	// �жϣ������ڼ���ִ�к����������ϵͳ���á�������-ERESTARTNOINTR˵���ڴ������źź�Ҫ�󷵻ص�ԭϵͳ�����м������У���ϵͳ
	// ���ò��ᱻ�жϡ�
	// ����������˵���������ϵͳ�����е��õı�������������Ӧϵͳ���õķ�����eax����-ERESTARTSYS��-ERESTARTNOINTRʱ��������
	// �Ĵ���ʵ���ϻ�û�������ص��û������У���
	if ((orig_eax != -1) &&
	    ((eax == -ERESTARTSYS) || (eax == -ERESTARTNOINTR))) {
		// ���ϵͳ���÷�������-ERESTARTSYS����������ϵͳ���ã�������sigaction�к��б�־SA_INTERRUPT��ϵͳ���ñ��ź��жϺ�����
		// ����ϵͳ���ã������ź�ֵС��SIGCONT�����ź�ֵ����SIGTTOU�����źŲ���SIGCONT��SIGSTOP��SIGTSTP��SIGTTIN��SIGTTOU����
		// ���޸�ϵͳ���õķ���ֵΪeax = -EINTR�������ź��жϵ�ϵͳ���á�
		if ((eax == -ERESTARTSYS) && ((sa->sa_flags & SA_INTERRUPT) ||
		    signr < SIGCONT || signr > SIGTTOU))
			*(&eax) = -EINTR;
		// ����ͻָ����̼Ĵ���eax�ڵ���ϵͳ����֮ǰ��ֵ�����Ұ�Դ����ָ��ָ��ص�2���ֽڡ����������û�����ʱ���ó�����������ִ�б��ź�
		// �жϵ�ϵͳ���á�
		else {
			*(&eax) = orig_eax;     				// orig_eaxϵͳ���ú�
			//*(&eip) = old_eip -= 2;
			// ϵͳ���÷��ص��û�̬��ʱ���ٴ�ִ�б���ϵͳ����
			old_eip -= 2;
			__asm__ ("movl %%eax, %0\n\t" \
					: \
					:"m"(eip), "a"(old_eip));
		}
	}
	// ����źž��ΪSIG_IGN��1,Ĭ�Ϻ��Ծ�����򲻶��źŽ��д����ֱ�ӷ��ء�
	sa_handler = (unsigned long) sa->sa_handler;
	if (sa_handler == 1)
		return(1);   								/* Ignore, see if there are more signals... */
	// ������ΪSIG_DFL��0,Ĭ�ϴ���������ݾ�����źŽ��зֱ���
	if (!sa_handler) {
		switch (signr) {
		// ����ź�������������Ҳ����֮��������
		case SIGCONT:
		case SIGCHLD:
			return(1);  							/* Ignore, ... */

		// ����ź�������4���ź�֮һ����ѵ�ǰ����״̬��Ϊֹͣ״̬TASK_STOPPED������ǰ���̸����̶�SIGCHLD�źŵ�sigaction
		// �����־SA_NOCLDSTOP�������ӽ���ִֹͣ�л��ּ���ִ��ʱ��Ҫ����SIGCHLD�źţ�û����λ����ô�͸������̷���SIGCHLD
		// �źš�
		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			current->state = TASK_STOPPED;
			current->exit_code = signr;
			if (!(current->p_pptr->sigaction[SIGCHLD - 1].sa_flags &
					SA_NOCLDSTOP))
				current->p_pptr->signal |= (1 << (SIGCHLD - 1));
			return(1);  							/* Reschedule another event */

		// ����ź�������6���ź�֮һ����ô���źŲ�����core_dump�������˳���Ϊsignr|0x80����do_exit()�˳��������˳�������ź�
		// ֵ��do_exit()�Ĳ����Ƿ�����ͳ����ṩ���˳�״̬��Ϣ������Ϊwait()��waitpid()������״̬��Ϣ���μ�sys/wait.h��
		// wait()��waidpid()������Щ��Ϳ���ȡ���ӽ��̵��˳�״̬����ӽ�����ֹ��ԭ���źţ���
		case SIGQUIT:
		case SIGILL:
		case SIGTRAP:
		case SIGIOT:
		case SIGFPE:
		case SIGSEGV:
			if (core_dump(signr))
				do_exit(signr | 0x80);
			/* fall through */
		default:
			do_exit(signr);
		}
	}
	/*
	 * OK, we're invoking a handler
	 */
        /*
         * OK����������׼�����źž�����õ�����
         */
	// ������źž��ֻ�豻����һ�Σ��򽫸þ���ÿա�ע�⣬���źž��ǰ���Ѿ�������sa_handlerָ���С�
	// ��ϵͳ���ý����ں�ʱ���û����򷵻ص�ַ��eip��cs�����������ں�̬ջ�С�������δ����޸��ں�̬��ջ���û�����ʱ
	// �Ĵ���ָ��eipΪָ���źŴ�������ͬʱҲ��sa_restorer��signr�����������루���SA_NOMASKû��λ����eax��
	// ecx��edx��Ϊ�����Լ�ԭ����ϵͳ���õĳ��򷵻�ָ�뼰��־�Ĵ���ֵѹ���û���ջ������ڱ���ϵͳ�����жϷ����û�
	// ����ʱ������ִ���û��źž������Ȼ�����ִ���û�����
	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;
	// ���ں�̬ջ���û�����ϵͳ���� ��һ������ָ��eipָ����źŴ�����������C�����Ǵ�ֵ��������˸�eip��ֵʱ��Ҫ
	// ʹ�á�*(&eip)������ʽ�����⣬��������ź��Լ��Ĵ������յ��ź��Լ�����Ҳ��Ҫ�����̵�������ѹ���ջ��
	// ������ע�⣬ʹ�����·�ʽ����193�У�����ͨC�������������޸��ǲ������õġ���Ϊ����������ʱ��ջ�ϵĲ������ᱻ
	// �����߶���������֮���Կ���ʹ�����ַ�ʽ������Ϊ�ú����Ǵӻ������б����õģ������ں������غ������û�а�
	// ����do_signal()ʱ�����в�����������eip����Ȼ�ڶ�ջ�С�
	// sigaction�ṹ��sa_mask�ֶθ������ڵ�ǰ�źž�����ź�������������ִ���ڼ�Ӧ�ñ����ε��źż���ͬʱ������
	// �źž��ִ�е��ź�Ҳ�ᱻ���Ρ�������sa_flags��ʹ����SA_NOMASK��־����ô�����źž��ִ�е��źŽ����ᱻ����
	// ������������ź��Լ��Ĵ����������յ��ź��Լ�����Ҳ��Ҫ�����̵��ź�������ѹ���ջ��
	*(&eip) = sa_handler;
	longs = (sa->sa_flags & SA_NOMASK)?7:8;
	// ��ԭ���ó�����û���ջָ��������չ7����8�������֣�������ŵ����źž���Ĳ����ȣ���������ڴ�ʹ����������ڴ泬
	// ���������ҳ�ȣ���
	//*(&esp) -= longs;
	__asm__("subl %1, %0\n\t" \
			: \
			:"m"(esp), "a"(longs * 4));
	verify_area(esp, longs * 4);
	// ���û���ջ�д��µ��ϴ��sa_restorer���ź�signr��������blocked�����SA_NOMASK��λ����eax��ecx��edx��eflags
	// ���û�����ԭ����ָ�롣
	tmp_esp = esp;
	put_fs_long((long) sa->sa_restorer, tmp_esp++);
	put_fs_long(signr, tmp_esp++);
	if (!(sa->sa_flags & SA_NOMASK))
		put_fs_long(current->blocked, tmp_esp++);
	put_fs_long(eax, tmp_esp++);
	put_fs_long(ecx, tmp_esp++);
	put_fs_long(edx, tmp_esp++);
	put_fs_long(eflags, tmp_esp++);
	put_fs_long(old_eip, tmp_esp++);
	current->blocked |= sa->sa_mask;                // ���������루�����룩����as_mask�е��롣
	return(0);										/* Continue, execute handler */
}

