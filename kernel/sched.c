/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 */
/*
 * 'sched.c'����Ҫ���ں��ļ�.���а����йظ߶ȵĻ�������(sleep_on,wakeup,schedule��)�Լ�һЩ�򵥵�ϵͳ���ú���(����getpid(),��
 * �ӵ�ǰ�����л�ȡһ���ֶ�).
 */
// �����ǵ��ȳ���ͷ�ļ�.����������ṹtask_struct,��1����ʼ���������.����һЩ�Ժ����ʽ������й��������������úͻ�ȡ��Ƕ��ʽ��ຯ������.
#include <linux/sched.h>
#include <linux/kernel.h>					// �ں�ͷ�ļ�.����һЩ�ں˳��ú�����ԭ�ζ���.
#include <linux/sys.h>						// ϵͳ����ͷ�ļ�.����82��ϵͳ����C��������,��'sys_'��ͷ.
#include <linux/fdreg.h>					// ����ͷ�ļ�.�������̿�����������һЩ����.
#include <asm/system.h>						// ϵͳͷ�ļ�.���������û��޸�������/�ж��ŵȵ�Ƕ��ʽ����.
#include <asm/io.h>							// ioͷ�ļ�.����Ӳ���˿�����/����������.
//#include <asm/segment.h>

#include <signal.h>

// �ú�ȡ�ź�nr���ź�λͼ�ж�Ӧλ�Ķ�������ֵ.�źű��1-32.�����ź�5��λͼ��ֵ����1<<(5-1)=16=00010000.
#define _S(nr) (1 << ((nr) - 1))
// ����SIGKILL��SIGSTOP�ź����������źŶ��ǿ�������.
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

// �ں˵��Ժ���.��ʾ�����nr�Ľ��̺�,����״̬���ں˶�ջ�����ֽ���(��Լ).
void show_task(int nr, struct task_struct * p)
{
	int i, j = 4096 - sizeof(struct task_struct);

	printk("%d: pid=%d, state=%d, father=%d, child=%d, ", nr, p->pid,
		p->state, p->p_pptr->pid, p->p_cptr ? p->p_cptr->pid : -1);
	i = 0;
	while (i < j && !((char *)(p + 1))[i])				// ���ָ���������ݽṹ�Ժ����0���ֽ���.
		i++;
	printk("%d/%d chars free in kstack\n\r", i, j);
	printk("   PC=%08X.", *(1019 + (unsigned long *) p));
	if (p->p_ysptr || p->p_osptr)
		printk("   Younger sib=%d, older sib=%d\n\r",
			p->p_ysptr ? p->p_ysptr->pid : -1,
			p->p_osptr ? p->p_osptr->pid : -1);
	else
		printk("\n\r");
}

// ��ʾ��������������,���̺�,����״̬���ں˶�ջ�����ֽ���(��Լ).
// NR_TASKS��ϵͳ�����ɵ�������(����)����(64��),������include/kernel/sched.h
void show_state(void)
{
	int i;

	printk("\rTask-info:\n\r");
	for (i = 0; i < NR_TASKS; i++)
		if (task[i])
			show_task(i, task[i]);
}

// PC8253��ʱоƬ������ʱ��Ƶ��ԼΪ1.193180MHz.Linux�ں�ϣ����ʱ�������жϵ�Ƶ����100Hz,Ҳ��ÿ10ms����һ��ʱ���ж�.�������
// LATCH������8253оƬ�ĳ�ֵ.
#define LATCH (1193180 / HZ)

extern void mem_use(void);              	// û���κεط���������øú�����

extern int timer_interrupt(void);			// ʱ���жϴ������(kernel/sys_call.s)
extern int system_call(void);				// ϵͳ�����жϴ������(kernel/sys_call.s)

// ÿ������(����)���ں�̬����ʱ�����Լ����ں�̬��ջ.���ﶨ����������ں�̬��ջ�ṹ.
// ���ﶨ����������(����ṹ��Ա��stack�ַ������Ա).��Ϊһ����������ݽṹ�����ں�̬��ջ����ͬһ�ڴ�ҳ��,���ԴӶ�ջ�μĴ���ss���Ի����
// ���ݶ�ѡ���.
union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];
};

// ���ó�ʼ���������.��ʼ������include/kernel/sched.h��.
static union task_union init_task = {INIT_TASK, };

// �ӿ�����ʼ����ĵδ���ʱ��ֵȫ�ֱ���(10ms/�δ�).ϵͳʱ���ж�ÿ����һ�μ�һ���δ�.ǰ����޶���volatile,Ӣ�Ľ������׸ı��,���ȶ�����˼.
// ����޶��ʵĺ������������ָ�����������ݿ��ܻ����ڱ����������޸���仯.ͨ���ڳ���������һ������ʱ,�������ᾡ�����������ͨ�üĴ�����, ����
// ebx,����߷���Ч��.��CPU����ֵ�ŵ�ebx�к�һ��Ͳ����ٹ��ĸñ�����Ӧ�ڴ�λ���е�����.����ʱ��������(�����ں˳����һ���жϹ���)�޸����ڴ���
// �ñ�����ֵ,ebx�е�ֵ��������֮����.Ϊ�˽����������ʹ�����volatile�޶���,�ô��������øñ���ʱһ��Ҫ��ָ���ڴ�λ����ȡ����ֵ.���Ｔ��Ҫ��
// gcc��Ҫ��jiffies�����Ż�����,Ҳ��ҪŲ��λ��,������Ҫ���ڴ���ȡ��ֵ.��Ϊʱ���жϴ�����̵ȳ�����޸�����ֵ.
unsigned long volatile jiffies = 0;
unsigned long startup_time = 0;						// ����ʱ��.��1970:0:0:0:0��ʼ��ʱ������.
// ������������ۼ���Ҫ������ʱ��δ���.
int jiffies_offset = 0;								/* # clock ticks to add to get "true
													   time".  Should always be less than
													   1 second's worth.  For time fanatics
													   who like to syncronize their machines
													   to WWV :-) */
/*
 * Ϊ����ʱ�Ӷ���Ҫ���ӵ�ʱ�ӵδ�,�Ի��"��ȷʱ��".��Щ�����õδ������ܺͲ�Ӧ�ó���1��.��������Ϊ����Щ��ʱ�侫ȷ��Ҫ����̵���,��������ϲ��
 * �Լ��Ļ���ʱ����WWVͬ��:-
 */

struct task_struct *current = &(init_task.task);	// ��ǰ����ָ��(��ʼ��ָ������0)
struct task_struct *last_task_used_math = NULL;		// ʹ�ù�Э�����������ָ��.

// ��������ָ������.�ڣ����ʼ��ָ���ʼ����(����)���������ݽṹ.
struct task_struct * task[NR_TASKS] = {&(init_task.task), };

// �����û���ջ,��1K��,����4K�ֽ�.���ں˳�ʼ�����������б������ں�ջ,��ʼ������Ժ󽫱���������0���û���ջ.����������0֮ǰ�����ں�ջ,
// �Ժ���������0��1���û�̬ջ.
// ����ṹ�������ö�ջss:esp(���ݶ�ѡ���,ָ��).
// ss������Ϊ�ں����ݶ�ѡ���(0x10),ָ��espָ��user_stack�������һ�����.������ΪInterl CPUִ�ж�ջ����ʱ���ȵݼ���ջָ��spֵ,
// Ȼ����spָ�봦������ջ����.
long user_stack [ PAGE_SIZE >> 2 ] ;

struct {
	long * a;
	short b;
	} stack_start = { &user_stack [PAGE_SIZE >> 2], 0x10 };

/*
 *  'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 */
/*
 * ����ǰЭ���������ݱ��浽��Э������״̬������,������ǰ�����Э���������ݼ��ؽ�Э������.
 */
// �����񱻵��Ƚ������Ժ�,�ú������Ա���ԭ�����Э������״̬(������)���ָ��µ��Ƚ����ĵ�ǰ�����Э������ִ��״̬.
void math_state_restore()
{
	// �������û���򷵻�(��һ��������ǵ�ǰ����).����"��һ������"��ָ�ձ�������ȥ������.
	if (last_task_used_math == current)
		return;
	// �ڷ���Э����������֮ǰҪ�ȱ�WAITָ��.����ϸ�����ʹ����Э������,�򱣴���״̬.
	__asm__("fwait");
	if (last_task_used_math) {
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
	}
	// ����,las_task_used_mathָ��ǰ����,�Ա���ǰ���񱻽�����ȥʱʹ��.��ʱ�����ǰ�����ù�Э������,��ָ���״̬.����Ļ�˵���ǵ�һ��ʹ��,
	// ���Ǿ���Э����������ʼ������,������ʹ��Э��������־.
	last_task_used_math = current;
	if (current->used_math) {
		__asm__("frstor %0"::"m" (current->tss.i387));
	} else {
		__asm__("fninit"::);					// ��Э����������ʼ������.
		current->used_math=1;					// ������ʹ��Э��������־.
	}
}

/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
/*
 * 'schedule()'�ǵ��Ⱥ���.���Ǹ��ܺõĴ���!û���κ����ɶ��������޸�,��Ϊ�����������еĻ����¹���(�����ܹ���IO-�߽��µúܺ�
 * ����Ӧ��).ֻ��һ����ֵ������,�Ǿ���������źŴ�������.
 *
 * ע��!!����0�Ǹ�����('idle')����,ֻ�е�û�����������������ʱ�ŵ�����.�����ܱ�ɱ��,Ҳ��˯��.����0�е�״̬��Ϣ'state'�Ǵ�
 * �����õ�.
 */
void schedule(void)
{
	int i, next, c;
	struct task_struct ** p;						// ����ṹָ���ָ��.

	/* check alarm, wake up any interruptible tasks that have got a signal */
	/* ���alarm(���̵ı�����ʱֵ),�����κ��ѵõ��źŵĿ��ж����� */

	// ���������������һ������ʼѭ�����alarm.��ѭ��ʱ������ָ����.
	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
		if (*p) {
			// ������ù�����ʱ��ʱtimeout,�����Ѿ���ʱ,��λ��ʱ��ʱֵ,������������ڿ��ж�˯��״̬TASK_INTERRUPTIBLE��,������Ϊ����
			// ״̬(TASK_RUNNING).
			if ((*p)->timeout && (*p)->timeout < jiffies) {
				(*p)->timeout = 0;
				if ((*p)->state == TASK_INTERRUPTIBLE)
					(*p)->state = TASK_RUNNING;
			}
			// ������ù�����Ķ�ʱֵalarm,�����Ѿ�����(alarm<jiffies),�����ź�λͼ����SIGALRM�ź�,����������SIGALARM�ź�.Ȼ����alarm.
			// ���źŵ�Ĭ�ϲ�������ֹ����.jiffies��ϵͳ�ӿ�����ʼ����ĵδ���(10ms/�δ�).������sched.h��.
			if ((*p)->alarm && (*p)->alarm < jiffies) {
				(*p)->signal |= (1 << (SIGALRM - 1));
				(*p)->alarm = 0;
			}
			// ����ź�λͼ�г����������ź��⻹�������ź�,���������ڿ��ж�״̬,��������Ϊ����״̬.
			// ����'~(_BLOCKABLE & (*p)->blocked)'���ں��Ա��������ź�,��SIGKILL��SIGSTOP���ܱ�����.
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) && (*p)->state == TASK_INTERRUPTIBLE)
				(*p)->state = TASK_RUNNING;
		}

	/* this is the scheduler proper: */
	/* �����ǵ��ȳ������Ҫ���� */
	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		// ��δ����Ǵ�������������һ������ʼѭ������,���������������������.�Ƚ�ÿ������״̬�����counter(��������ʱ��ĵݼ��δ����)ֵ,
		// ��һ��ֵ��,����ʱ�仹����,next��ָ���ĸ��������.
		while (--i) {
			// ��ǰ����û�н���ָ����������ǰѭ��
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}
		// ����Ƚϵó���counterֵ������0�Ľ��,�����᷽��û��һ�������е��������(��ʱc��ȻΪ-1,next=0),���˳���ʼ��ѭ��,ִ��161���ϵ������л�
		// ����.����͸���ÿ�����������Ȩֵ,����ÿһ�������counterֵ,Ȼ��ص�125�����±Ƚ�.counterֵ�ļ��㷽ʽΪcounter = counter /2 +priority.
		// ע��,���������̲����ǽ��̵�״̬.
		if (c) break;
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
	}
	// ������ĺ�(������sched.h��)�ѵ�ǰ����ָ��currentָ�������Ϊnext������,���л���������������.��146����next����ʼ��Ϊ0.�����ϵͳ��û���κ�
	// �������������ʱ,��nextʼ��Ϊ0.��˵��Ⱥ�������ϵͳ����ʱȥִ������0.��ʱ����0Ȩִ��pause()
	switch_to(next);					// �л��������Ϊnext������,������֮.
}

// pause()ϵͳ����.ת����ǰ�����״̬Ϊ���жϵĵȴ�״̬,�����µ���.
// ��ϵͳ���ý����½��̽���˯��״̬,ֱ���յ�һ���ź�.���ź�������ֹ���̻���ʹ���̵���һ���źŲ�����.ֻ�е�������һ���ź�,�����źŲ�����������,
// pause()�Ż᷵��.��ʱpause()����ֵӦ����-1,����errno����ΪEINTR.���ﻹû����ȫʵ��(ֱ��0.95��).
int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

// �ѵ�ǰ������Ϊָ����˯��״̬(���жϵĻ򲻿��жϵ�),����˯�߶���ͷָ��ָ��ǰ����.��������p�ǵȴ��������ͷָ��.ָ���Ǻ���һ��������ַ�ı���.��
// �����pʹ����ָ���ָ����ʽ'**p',������ΪC��������ֻ�ܴ�ֵ,û��ֱ�ӵķ�ʽ�ñ����ú����ı���øú��������б�����ֵ.����ָ��'*p'ָ���Ŀ��(����
// ������ṹ)��ı�,���Ϊ�����޸ĵ��øú���������ԭ������ָ�������ֵ,����Ҫ����ָ��'*p'��ָ��,��'**p'.
// ����state������˯��ʹ�õ�״̬:TASK_INTERRUPTIBLE��TASK_INTERRUPTIBLE.���ڲ����ж�˯��״̬(TASK_UNINTERRUPTIBLE)��������Ҫ�ں˳�������
// wake_up()������ȷ����֮.���ڿ��ж�˯��״̬(TASK_INTERRUPTIBLE)����ͨ���ź�,������ʱ���ֶλ���(��Ϊ����״̬TASK_RUNNING).
// *** ע��,���ڱ��ں˴��벻�Ǻܳ���,���������˯����صĴ������һЩ����,�����.
static inline void __sleep_on(struct task_struct **p, int state)
{
	struct task_struct *tmp;

	// ��ָ����Ч,���˳�.(ָ����ָ�Ķ��������NULL,��ָ�뱾����Ϊ0).
	// �����ǰ����������0,������(impossible!).
	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	// ��tmpָ���Ѿ��ڵȴ������ϵ�����(����еĻ�),����inode->i_wait.���ҽ�˯�߶���ͷ�ĵȴ�ָ��ָ��ǰ����.�����Ͱѵ�ǰ������뵽��*p�ĵȴ�������.Ȼ��
	// ����ǰ������Ϊָ���ĵȴ�״̬,��ִ�����µ���.
	tmp = *p;
	*p = current;
	current->state = state;
	// ����ǰ����˯�ߺ����̵��õ��Ⱥ������е����µĽ���ִ��
repeat:	schedule();
	// ֻ�е�����ȴ����񱻻���ʱ,����Ż᷵�ص�����,��ʾ�����ѱ���ȷ�ػ��Ѳ�ִ��.����ȴ������л��еȴ�����,���Ҷ���ͷָ��*p��ָ��������ǵ�ǰ����ʱ,˵��
	// �ڱ��������ȴ����к����������ȴ�����.��������Ӧ��ҲҪ�����������,�������Լ�Ӧ��˳������Щ���������е�������,������ｫ�ȴ�����ͷ��ָ������
	// ��Ϊ����״̬,���Լ�����Ϊ�����жϵȴ�״̬,���Լ�Ҫ�ȴ���Щ�������е����񱻻��Ѷ�ִ��ʱ�����ѱ�����.Ȼ������ִ�е��ȳ���.
	if (*p && *p != current) {
		(**p).state = 0;
		current->state = TASK_UNINTERRUPTIBLE;
		goto repeat;
	}
	// ִ�е�����,˵������������������ִ��.��ʱ�ȴ�����ͷָ��Ӧ��ָ������,����Ϊ��,���������������,������ʾ������Ϣ.���������ͷָ��ָ��������ǰ�������е�
	// ����(*p = tmp).��ȷʵ��������һ������,�������л�������(tmp��Ϊ��),�ͻ���֮.���Ƚ�����е������ڻ��Ѻ�����ʱ���ջ�ѵȴ�����ͷָ���ó�NULL.
	if (!*p)
		printk("Warning: *P = NULL\n\r");
	if (*p = tmp)
		tmp->state = 0;
}

// ����ǰ������Ϊ���жϵĵȴ�״̬(TASK_INIERRUPTIBLE),������ͷָ��*pָ���ĵȴ�������.
void interruptible_sleep_on(struct task_struct **p)
{
	__sleep_on(p, TASK_INTERRUPTIBLE);
}

// �ѵ�ǰ������Ϊ�����жϵĵȴ�״̬(TASK_UNINTERRUPTIBLE),����˯�߶���ͷָ��ָ��ǰ����.ֻ����ȷ�ػ���ʱ�Ż᷵��.�ú����ṩ�˽������жϴ������֮���
// ͬ������.
void sleep_on(struct task_struct **p)
{
	__sleep_on(p, TASK_UNINTERRUPTIBLE);
}

// ����*pָ�������.*p������ȴ�����ͷָ��.�����µȴ������ǲ����ڵȴ�����ͷָ�봦��,��˻��ѵ���������ȴ����е�����.���������Ѿ�����ֹͣ��
// ����״̬,����ʾ������Ϣ.
void wake_up(struct task_struct **p)
{
	if (p && *p) {
		if ((**p).state == TASK_STOPPED)						// ����ֹͣ״̬.
			printk("wake_up: TASK_STOPPED");
		if ((**p).state == TASK_ZOMBIE)							// ���ڽ���״̬.
			printk("wake_up: TASK_ZOMBIE");
		(**p).state=0;											// ��Ϊ����״̬TASK_RUNNING.
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
/*
 * ����,�����￪ʼ��һЩ�й����̵��ӳ���,����Ӧ�÷����ں˵���Ҫ�����е�.�����Ƿ�����������Ϊ������Ҫ��ʱ����,����������������.
 */
// ���������wait_motor[]���ڴ�ŵȴ������������������ת�ٵĽ���ָ��.��������0-3�ֱ��Ӧ����A--D.����mon_timer[]��Ÿ����������������Ҫ
// �ĵδ���.������Ĭ������ʱ��Ϊ50���δ�(0.5��).����moff_timer[]��Ÿ����������ͣת֮ǰ��ά�ֵ�ʱ��.�������趨Ϊ1000���δ�(100��).
static struct task_struct * wait_motor[4] = {NULL, NULL, NULL, NULL};
static int  mon_timer[4] = {0, 0, 0, 0};
static int moff_timer[4] = {0, 0, 0, 0};
// ���������Ӧ�����������е�ǰ��������Ĵ���.�üĴ���ÿλ��������:
// λ7-4:�ֱ����������D-A��������.1 - ����;0 - �ر�.
// λ3:1 - ����DMA���ж�����;0 - ��ֹDMA���ж�����.
// λ2:1 - �������̿�����;0 - ��λ���̿�����.
// λ1-0:00 - 11,����ѡ����Ƶ�����A-D.
// �������ó�ֵΪ:����DMA���ж�����,����FDC.
unsigned char current_DOR = 0x0C;

// ָ������������������ת״̬����ȴ�ʱ��.
// ����nr -- ������(0--3),����ֵΪ�δ�.
// �ֲ�����selected��ѡ��������־(blk_drv/floppy.c).mask����ѡ������Ӧ����������Ĵ������������λ.mask��4λ�Ǹ�������������־.
int ticks_to_floppy_on(unsigned int nr)
{
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	// ϵͳ�����4������.����Ԥ�����ú�ָ������nrͣת֮ǰ��Ҫ������ʱ��(100��).Ȼ��ȡ��ǰDOR�Ĵ���ֵ����ʱ����mask��,����ָ�����������
	// ������־��λ.
	if (nr > 3)
		panic("floppy_on: nr>3");
	moff_timer[nr] = 10000;							/* 100 s = very big :-) */	// ͣתά��ʱ��.
	cli();											/* use floppy_off to turn it off */	// ���ж�
	mask |= current_DOR;
	// �����ǰû��ѡ������,�����ȸ�λ����������ѡ��λ,Ȼ��ָ������ѡ��λ.
	if (!selected) {
		mask &= 0xFC;
		mask |= nr;
	}
	// �����������Ĵ����ĵ�ǰֵ��Ҫ���ֵ��ͬ,����FDC��������˿������ֵ(mask),�������Ҫ����������ﻹû������,������Ӧ�������������
	// ��ʱ��ֵ(HZ/2 = 0.5���50���δ�).���Ѿ�����,��������������ʱΪ2���δ�,����������do_floppy_timer()���ȵݼ����жϵ�Ҫ��.ִ�б���
	// ��ʱ�����Ҫ�󼴿�.�˺���µ�ǰ��������Ĵ���current_DOR.
	if (mask != current_DOR) {
		outb(mask, FD_DOR);
		if ((mask ^ current_DOR) & 0xf0)
			mon_timer[nr] = HZ / 2;
		else if (mon_timer[nr] < 2)
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();											// ���ж�.
	return mon_timer[nr];							// ��󷵻�������������ʱ��ֵ.
}

// �ȴ�ָ������������������һ��ʱ��,Ȼ�󷵻�.
// ����ָ���������������������ת���������ʱ,Ȼ��˯�ߵȴ�.�ڶ�ʱ�жϹ����л�һֱ�ݼ��ж������趨����ʱֵ.����ʱ����,�ͻ� ����ĵȴ�����.
void floppy_on(unsigned int nr)
{
	// ���ж�.������������ʱ��û��,��һֱ�ѵ�ǰ������Ϊ�����ж�˯��״̬������ȴ�������еĶ�����.Ȼ���ж�.
	cli();
	while (ticks_to_floppy_on(nr))
		sleep_on(nr + wait_motor);
	sti();
}

// �ùر���Ӧ�������ͣת��ʱ��(3��).
// ����ʹ�øú�����ȷ�ر�ָ�����������,������￪��100��֮��Ҳ�ᱻ�ر�.
void floppy_off(unsigned int nr)
{
	moff_timer[nr] = 3 * HZ;
}

// ���̶�ʱ�����ӳ���.�������������ʱֵ�����ر�ͣתʱֵ.���ӳ������ʱ�Ӷ�ʱ�жϹ����б�����,���ϵͳÿ����һ���δ�(10ms)�ͻᱻ
// ����һ��,��ʱ������￪����ͣת��ʱ����ֵ.���ĳһ�����ͣת��ʱ��,����������Ĵ����������λ��λ.
void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i = 0 ; i < 4 ; i++, mask <<= 1) {
		if (!(mask & current_DOR))						// �������DORָ�������������.
			continue;
		if (mon_timer[i]) {								// ������������ʱ�����ѽ���.
			if (!--mon_timer[i])
				wake_up(i + wait_motor);
		} else if (!moff_timer[i]) {					// ������ͣת��ʱ����λ��Ӧ�������λ,���Ҹ�����������Ĵ���.
			current_DOR &= ~mask;
			outb(current_DOR, FD_DOR);
		} else
			moff_timer[i]--;							// �������ͣת��ʱ�ݼ�.
	}
}

// �����ǹ��ڶ�ʱ���Ĵ���.������64����ʱ��.
#define TIME_REQUESTS 64

// ��ʱ������ṹ�Ͷ�ʱ������.�ö�ʱ������ר���ڹ������ر�����������ﶨʱ����.�������Ͷ�ʱ�������ִ�Linuxϵͳ�еĶ�̬��ʱ��(Dynamic Timer),
// �����ں�ʹ��.
static struct timer_list {
	long jiffies;										// ��ʱ�δ���.
	void (*fn)();										// ��ʱ�������.
	struct timer_list * next;							// ����ָ����һ����ʱ��.
} timer_list[TIME_REQUESTS], * next_timer = NULL;		// next_timer�Ƕ�ʱ������ͷָ��.

// ��Ӷ�ʱ��.�������Ϊָ���Ķ�ʱֵ(�δ���)����Ӧ�Ĵ������ָ��.
// ������������(floppy.c)���øú���ִ��������ر�������ʱ����.
// ����jiffies- ��10����Ƶĵδ���; *fn() - ��ʱʱ�䵽ʱִ�еĺ���.
void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list * p;

	// �����ʱ�������ָ��Ϊ��,���˳�.������ж�.
	if (!fn)
		return;
	cli();
	// �����ʱֵ<=0,�����̵����䴦�����.���Ҹö�ʱ��������������.
	if (jiffies <= 0)
		(fn)();
	else {
		// ����Ӷ�ʱ��������,��һ��������.
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)
			if (!p->fn)
				break;
		// ����Ѿ������˶�ʱ������,��ϵͳ����.������ʱ�����ݽṹ�������Ϣ,����������ͷ.
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;
		// �������ʱֵ��С��������.������ʱ��ȥ����ǰ����Ҫ�ĵδ���,�����ڴ���ʱ��ʱֻҪ�鿴����ͷ�ĵ�һ��Ķ�ʱ�Ƿ��ڼ���.
		// [[?? ��γ������û�п�����ȫ.����²���Ķ�ʱ��ֵС��ԭ����һ����ʱ��ֵʱ�����û�����ѭ����,����ʱ����Ӧ�ý��������
		// ��һ����ʱ��ֵ��ȥ�µĵ�1���Ķ�ʱֵ.�������1����ʱֵ<=��2��,���2����ʱֵ�۳���1����ֵ����,�����������ѭ���н��д���.]]
		while (p->next && p->next->jiffies < p->jiffies) {
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
		// ������������.
		if(p->next && p->next->jiffies >= p->jiffies) {
			p->next->jiffies -= p->jiffies;
		}
	}
	sti();
}

// ʱ���ж�C�����������,��sys_call.s�е�timer_interrupt������.
// ����cpl�ǵ�ǰ��Ȩ��0��3,��ʱ���жϷ���ʱ����ִ�еĴ���ѡ����е���Ȩ��.cpl=0ʱ��ʾ�жϷ���ʱ����ִ���ں˴���,cpl=3ʱ��ʾ�жϷ���ʱ����ִ���û�
// ����.����һ����������ִ��ʱ��Ƭ����ʱ,����������л�.��ִ��һ����ʱ���¹���.
void do_timer(long cpl)
{
	static int blanked = 0;

	// �����ж��Ƿ񾭹���һ��ʱ�������Ļ����(blankcount).���blankcount������Ϊ��,���ߺ�����ʱ���ʱ��blankintervalΪ0�Ļ�,��ô���Ѿ��������״̬
	// (������־blanked=1),������Ļ�ָ���ʾ.��blnkcount������Ϊ��,��ݼ�֮,���Ҹ�λ������־.
	if (blankcount || !blankinterval) {
		if (blanked)
			unblank_screen();
		if (blankcount)
			blankcount--;
		blanked = 0;
	// ����Ļ���������־ĩ��λ,������Ļ����,�������ú�����־.
	} else if (!blanked) {
		blank_screen();
		blanked = 1;
	}
	// ���Ŵ���Ӳ�̲�����ʱ����.���Ӳ�̳�ʱ�����ݼ�֮��Ϊ0,�����Ӳ�̷��ʳ�ʱ����.
	if (hd_timeout)
		if (!--hd_timeout)
			hd_times_out();							// Ӳ�̷��ʳ�ʱ����(blk_drv/hd.c).

	// �����������������,��رշ���.(��0x61�ڷ�������,��λλ0��1.λ0����8253������2�Ĺ���,λ1����������.
	if (beepcount)									// ����������ʱ��δ���(chr_drv/console.c)
		if (!--beepcount)
			sysbeepstop();

	// �����ǰ��Ȩ��(cpl)Ϊ0(���,��ʾ���ں˳����ڹ���),���ں˴���ʱ��stime����;[Linus���ں˳���ͳ��Ϊ�����û�(superviser)��
	// ����.���ֳƺ�����Intel CPU�ֲ�.]���cpl>0,���ʾ��һ���û������ڹ���,����utime.
	if (cpl)
		current->utime++;
	else
		current->stime++;

	// ����ж�ʱ������,�������1����ʱ����ֵ��1.����ѵ���0,�������Ӧ�Ĵ������,�����ô������ָ���ÿ�.Ȼ��ȥ�����ʱ��.next_timer��
	// ��ʱ�������ͷָ��.
	if (next_timer) {
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void);						// ���������һ������ָ�붨��!!

			fn = next_timer->fn;
			next_timer->fn = NULL;
			next_timer = next_timer->next;
			(fn)();									// ���ö�ʱ������.
		}
	}
	// �����ǰ���̿�����FDC����������Ĵ������������λ����λ��,��ִ�����̶�ʱ����.
	if (current_DOR & 0xf0)
		do_floppy_timer();
	// �����������ʱ�仹û��,���˳�.�����õ�ǰ�������м���ֵΪ0.����������ʱ���ж�ʱ�����ں˴����������򷵻�,�������ִ�е��Ժ���.
	if ((--current->counter) > 0) return;
	current->counter = 0;
	if (!cpl) return;								// �����ں�̬����,������counterֵ���е���.
	schedule();
}

// ϵͳ���ù��� - ���ñ�����ʱʱ��ֵ���룩��
// ������seconds����0,�������¶�ʱֵ��������ԭ��ʱʱ�̻�ʣ��ļ��ʱ�䡣���򷵻�0��
// �������ݽṹ�б�����ʱֵalarm�ĵ�λ��ϵͳ�δ�1�δ�Ϊ10���룩������ϵͳ���������ö�ʱ����ʱϵͳ�δ�ֵjiffies��ת���ɵδ�
// ��λ�Ķ�ʱֵ֮�ͣ���'jiffies + HZ*��ʱ��ֵ'��������������������Ϊ��λ�Ķ�ʱֵ����˱���������Ҫ�����ǽ���������λ��ת����
// ���г���HZ = 100,���ں�ϵͳ����Ƶ�ʡ�������inlucde/sched.h�ϡ�
// ����seconds���µĶ�ʱʱ��ֵ����λ���롣
int sys_alarm(long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

// ȡ��ǰ���̺�pid��
int sys_getpid(void)
{
	return current->pid;
}

// ȡ�����̺�ppid��
int sys_getppid(void)
{
	return current->p_pptr->pid;
}

// ȡ�û�uid��
int sys_getuid(void)
{
	return current->uid;
}

// ȡ��Ч���û���euid��
int sys_geteuid(void)
{
	return current->euid;
}

// ȡ���gid��
int sys_getgid(void)
{
	return current->gid;
}

// ȡ��Ч�����egid��
int sys_getegid(void)
{
	return current->egid;
}

// ϵͳ���ù��� -- ���Ͷ�CPU��ʹ������Ȩ�����˻����𣿣���
// Ӧ������incrementΪ����0��ֵ�������ʹ���Ƚ����󣡣�
int sys_nice(long increment)
{
	if (current->priority-increment > 0)
		current->priority -= increment;
	return 0;
}

// �ں˵��ȳ���ĳ�ʼ���ӳ���
void sched_init(void)
{
	int i;
	struct desc_struct * p;										// ��������ṹָ��

	// Linuxϵͳ����֮��,�ں˲�����.�ں˴���ᱻ�����޸�.Linus���������޸�����Щ�ؼ��Ե����ݽṹ,�����POSIX��׼�Ĳ�����.���������������ж�
	// ��䲢�ޱ�Ҫ,������Ϊ�������Լ��Լ������޸��ں˴������.
	if (sizeof(struct sigaction) != 16)							// sigaction�Ǵ���й��ź�״̬�Ľṹ.
		panic("Struct sigaction MUST be 16 bytes");
	// ��ȫ���������������ó�ʼ����(����0)������״̬���������;ֲ����ݱ�������.
	// FIRST_TSS_ENTRY��FIRST_LDT_ENTRY��ֵ�ֱ���4��5,������include/linux/sched.h��.gdt��һ��������������(include/linux/head.h),
	// ʵ���϶�Ӧ����head.s�е��������������ַ(gdt).���gdt+FIRST_TSS_ENTRY��Ϊgdt[FIRST_TSS_ENTRY](����gdt[4]),��gdt�����4��ĵ�ַ
	// �μ�include/asm/system.h
	set_tss_desc(gdt + FIRST_TSS_ENTRY, &(init_task.task.tss));
	set_ldt_desc(gdt + FIRST_LDT_ENTRY, &(init_task.task.ldt));
	// ���������������������(ע��i=1��ʼ,���Գ�ʼ���������������).��������ṹ�������ļ�include/linux/head.h��.
	p = gdt + 2 + FIRST_TSS_ENTRY;
	// ��ʼ��������һ�������������ָ��
	for(i = 1; i < NR_TASKS; i++) {
		task[i] = NULL;
		p->a = p->b = 0;
		p++;
		p->a = p->b = 0;
		p++;
	}
	/* Clear NT, so that we won't have troubles with that later on */
	/* �����־�Ĵ����е�λNT,�����Ժ�Ͳ������鷳 */
	// EFLAGS�е�NT��־λ���ڿ��������Ƕ�׵���.��NTλ��λʱ,��ô��ǰ�ж�����ִ��IRETָ��ʱ�ͻ����������л�.NTָ��TSS�е�back_link�ֶ��Ƿ���Ч.
	// NT=0ʱ��Ч.
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
	// ������0��TSS��ѡ������ص�����Ĵ���tr.���ֲ����������ѡ������ص��ֲ���������Ĵ���ldtr��.ע��!!�ǽ�GDT����ӦLDT��������ѡ������ص�ldtr.
	// ֻ��ȷ����һ��,�Ժ�������LDT�ļ���,��CPU����TSS�е�LDT���Զ�����.
	ltr(0);								// ������include/linux/sched.h
	lldt(0);							// ���в���(0)�������.
	// ����������ڳ�ʼ��8253��ʱ��.ͨ��0,ѡ������ʽ3,�����Ƽ�����ʽ.ͨ��0��������Ž����жϿ�����оƬ��IRQ0��,��ÿ10���뷢��һ��IRQ0����.
	// LATCH�ǳ�ʼ��ʱ����ֵ.
	outb_p(0x36, 0x43);					/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff, 0x40);			/* LSB */	// ��ʱֵ���ֽ�
	outb(LATCH >> 8, 0x40);				/* MSB */	// ��ʱֵ���ֽ�
	// ����ʱ���жϴ��������(����ʱ���ж���).�޸��жϿ�����������,����ʱ���ж�.
	// Ȼ������ϵͳ�����ж���.�����������ж������α�IDT���������ĺ궨�����ļ�include/asm/system.h��.���ߵ�����μ�system.h�ļ���ʼ����˵��.
	set_intr_gate(0x20, &timer_interrupt);
	outb(inb_p(0x21) & ~0x01, 0x21);
	set_system_gate(0x80, &system_call);
}

