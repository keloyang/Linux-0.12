/*
 *  linux/kernel/sys.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>

#include <linux/sched.h>
//#include <linux/tty.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>
//#include <sys/param.h>
#include <sys/resource.h>
#include <string.h>

/*
 * The timezone where the local system is located.  Used as a default by some
 * programs who obtain this value by using gettimeofday.
 */
/*
 * ��ϵͳ���ڵ�ʱ����timezone������ΪĳЩ����ʹ��gettimeofdayϵͳ���û�ȡʱ����Ĭ��ֵ��
 */
// ʱ���ṹtimezone��1���ֶΣ�tz_minuteswest����ʾ��������α�׼ʱ��GMT�����ķ���������2���ֶΣ�tz_dsttime��
// ������ʱDST��Daylight Savings Time���������͡��ýṹ������include/sys/time.h�С�
struct timezone sys_tz = { 0, 0};

extern int session_of_pgrp(int pgrp);

// �������ں�ʱ�䣨ftime - Fetch time����
// ���·���ֵ��-ENOSYS��ϵͳ���ú�������ʾ�ڱ��汾�ں��л�δʵ�֡�
int sys_ftime()
{
	return -ENOSYS;
}

int sys_break()
{
	return -ENOSYS;
}

// ���ڵ�ǰ���̶��ӽ��̽��и߶ȣ�debugging����
int sys_ptrace()
{
	return -ENOSYS;
}

// �ı䲢��ӡ�ն������á�
int sys_stty()
{
	return -ENOSYS;
}

// ȡ�ն���������Ϣ��
int sys_gtty()
{
	return -ENOSYS;
}

// �޸��ļ�����
int sys_rename()
{
	return -ENOSYS;
}

int sys_prof()
{
	return -ENOSYS;
}

/*
 * This is done BSD-style, with no consideration of the saved gid, except
 * that if you set the effective gid, it sets the saved gid too.  This
 * makes it possible for a setgid program to completely drop its privileges,
 * which is often a useful assertion to make when you are doing a security
 * audit over a program.
 *
 * The general idea is that a program which uses just setregid() will be
 * 100% compatible with BSD.  A program which uses just setgid() will be
 * 100% compatible with POSIX w/ Saved ID's.
 */
/*
 * ������BSD��ʽ��ʵ�֣�û�п��Ǳ����gid��saved git��sgid�������˵�����������Ч��gid��effective gid��
 * egid��ʱ�������gidҲ�ᱻ���á���ʹ��һ��ʹ��setid�ĳ��������ȫ��������Ȩ�������ڶ�һ��������а�ȫ���
 * ʱ����ͨ����һ�ֺܺõĴ�������
 *
 * ������Ŀ�����һ��ʹ��setregid()�ĳ��򽫻���BSDϵͳ100%�ļ��ݡ���һ��ʹ��setgid()�ͱ����gid�ĳ���
 * ����POSIX100%�ļ��ݡ�
 */
// ���õ�ǰ�����ʵ���Լ�/������Ч��ID��gid�����������û�г����û���Ȩ����ôֻ�ܻ�����ʵ����ID����Ч��ID��
// ���������г����û���Ȩ����������������Ч�ĺ�ʵ�ʵ���ID��������gid��saved gid�������ó�����Чgid��ʵ��
// ��ID��ָ���̵�ǰ��gid��
int sys_setregid(int rgid, int egid)
{
	if (rgid > 0) {
		if ((current->gid == rgid) ||
		    suser())
			current->gid = rgid;
		else
			return(-EPERM);
	}
	if (egid > 0) {
		if ((current->gid == egid) ||
		    (current->egid == egid) ||
		    suser()) {
			current->egid = egid;
			current->sgid = egid;
		} else
			return(-EPERM);
	}
	return 0;
}

/*
 * setgid() is implemeneted like SysV w/ SAVED_IDS
 */
/*
 * setgid()��ʵ�������SAVED_IDS��SYSV��ʵ�ַ������ơ�
 */
// ���ý�����ţ�gid�����������û�г����û���Ȩ��������ʹ��setgid()������Чgid��effective gid������Ϊ���䱣��
// gid��saved git������ʵ��git��real gid������������г����û���Ȩ����ʵ��gid����Чgid�ͱ���gid�������óɲ���
// ָ����gid��
int sys_setgid(int gid)
{
	if (suser())
		current->gid = current->egid = current->sgid = gid;
	else if ((gid == current->gid) || (gid == current->sgid))
		current->egid = gid;
	else
		return -EPERM;
	return 0;
}

// �򿪻�رս��̼��˹��ܡ�
int sys_acct()
{
	return -ENOSYS;
}

// ӳ�������������ڵ����̵������ַ�ռ䡣
int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}

// ���ش�1970��1��1��00:00:00 GMT��ʼ��ʱ��ʱ��ֵ���룩�����tloc��Ϊnull����ʱ��ֵҲ�洢�����
// ���ڲ�����һ��ָ�룬������ָλ�����û��ռ䣬�����Ҫʹ�ú���put_fs_long()�����ʸ�ֵ���ڽ����ں�������ʱ����
// �Ĵ���fsĬ�ϵ�ָ��ǰ�û����ݿռ䡣��˸ú����Ϳ�����fs�������û��ռ��е�ֵ��
int sys_time(long * tloc)
{
	int i;

	i = CURRENT_TIME;
	if (tloc) {
		verify_area(tloc,4);            				// ��֤�ڴ������Ƿ񹻣�������4�ֽڣ���
		put_fs_long(i,(unsigned long *)tloc);   		// �����û����ݶ�tloc����
	}
	return i;
}

/*
 * Unprivileged users may change the real user id to the effective uid
 * or vice versa.  (BSD-style)
 *
 * When you set the effective uid, it sets the saved uid too.  This
 * makes it possible for a setuid program to completely drop its privileges,
 * which is often a useful assertion to make when you are doing a security
 * audit over a program.
 *
 * The general idea is that a program which uses just setreuid() will be
 * 100% compatible with BSD.  A program which uses just setuid() will be
 * 100% compatible with POSIX w/ Saved ID's.
 */
/*
 * ����Ȩ���û����Լ�ʵ�ʵ�uid��realuid���ĳ���Ч��uid��effective uid������֮��Ȼ����BSD ��ʽ��ʵ�֣�
 *
 * ����������Ч��uidʱ��ͬʱҲ�����˱����uid����ʹ��һ��ʹ��setuid�ĳ��������ȫ��������Ȩ�������ڶ�һ���������
 * ��ȫ���ʱ����ͨ����һ�ֺܺõĴ�������������Ŀ�����һ��ʹ��setreuid()�ĳ��򽫻���BSDϵͳ100%���ݡ���һ��ʹ
 * ��setuid()�ͱ����gid�ĳ��򽫻���POSIX 100%���ݡ�
 */
// ���������ʵ���Լ�/������Ч���û�ID��uid�����������û�г����û���Ȩ����ôֻ�ܻ�����ʵ�ʵ�uid����Ч��uid�����
// ������г����û���Ȩ����������������Ч�ĺ�ʵ�ʵ��û�ID�������uid��saved uid�������ó�����Чuidֵͬ��
int sys_setreuid(int ruid, int euid)
{
	int old_ruid = current->uid;

	if (ruid > 0) {
		if ((current->euid == ruid) ||
                    (old_ruid == ruid) ||
		    suser())
			current->uid = ruid;
		else
			return(-EPERM);
	}
	if (euid > 0) {
		if ((old_ruid == euid) ||
                    (current->euid == euid) ||
		    suser()) {
			current->euid = euid;
			current->suid = euid;
		} else {
			current->uid = old_ruid;
			return(-EPERM);
		}
	}
	return 0;
}

/*
 * setuid() is implemeneted like SysV w/ SAVED_IDS
 *
 * Note that SAVED_ID's is deficient in that a setuid root program
 * like sendmail, for example, cannot set its uid to be a normal
 * user and then switch back, because if you're root, setuid() sets
 * the saved uid too.  If you don't like this, blame the bright people
 * in the POSIX commmittee and/or USG.  Note that the BSD-style setreuid()
 * will allow a root program to temporarily drop privileges and be able to
 * regain them by swapping the real and effective uid.
 */
/*
 * setuid()��ʵ�������SAVED_IDS��SYSV��ʵ�ַ������ơ�
 *
 * ��ע��ʹ��SAVED_ID��setuid()��ĳЩ�����ǲ����Ƶġ����磬һ��ʹ��setuid�ĳ����û�����sendmail������������uid
 * ���ó�һ����ͨ�û���uid��Ȼ���ٽ�����������Ϊ�������һ�������û���setuid()Ҳͬʱ�����ñ����uid������㲻ϲ��
 * �����������Ļ��������POSIX��ί���Լ�/����USG�еĴ����˰ɡ�������ע��BSD��ʽ��setreuid()ʵ���ܹ�����һ������
 * �û�������ʱ������Ȩ��������ͨ������ʵ�ʵĺ���Ч��uid���ٴλ����Ȩ��
 */
// ���������û�ID��uid�����������û�г����û���Ȩ��������ʹ��setuid()������Ч��uid��effective uid�����ó��䱣��
// ��uid��saved uid������ʵ�ʵ�uid��real uid��������û��г����û���Ȩ����ʵ�ʵ�uid����Ч��uid�ͱ����uid���ᱻ
// ���óɲ���ָ����uid��
int sys_setuid(int uid)
{
	if (suser())
		current->uid = current->euid = current->suid = uid;
	else if ((uid == current->uid) || (uid == current->suid))
		current->euid = uid;
	else
		return -EPERM;
	return(0);
}

// ����ϵͳ����ʱ�䡣����tptr�Ǵ�1970��1��1��00��00��00 GMT��ʼ��ʱ��ʱ��ֵ���룩��
// ���ý��̱�����г����û�Ȩ�ޡ�����HZ=100,���ں�ϵͳ����Ƶ�ʡ�
// ���ڲ�����һ��ָ�룬������ָλ�����û��ռ䣬�����Ҫʹ�ú���get_fs_long()�����ʸ�ֵ���ڽ����ں�������ʱ����
// �Ĵ���fs��Ĭ�ϵ�ָ��ǰ�û����ݿռ䡣��˸ú����Ϳ�����fs�������û��ռ��е�ֵ�����������ṩ�ĵ�ǰʱ��ֵ��ȥ
// ϵͳ�Ѿ����е�ʱ����ֵ��jiffies/HZ�����ǿ���ʱ����ֵ��
int sys_stime(long * tptr)
{
	if (!suser())
		return -EPERM;          					// ������ǳ����û�������أ���ɣ���
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies / HZ;
	jiffies_offset = 0;
	return 0;
}

// ��ȡ��ǰ��������ʱ��ͳ��ֵ��
// ��tbuf��ָ�û����ݿռ䴦����tms�ṹ����������ʱ��ͳ��ֵ��tms�ṹ�а��������û�����ʱ�䡢�ںˣ�ϵͳ��ʱ�䡢�ӽ�
// ���û�����ʱ�䡢�ӽ���ϵͳ����ʱ�䡣��������ֵ��ϵͳ���е���ǰ���������
int sys_times(struct tms * tbuf)
{
	if (tbuf) {
		verify_area(tbuf,sizeof *tbuf);
		put_fs_long(current->utime,(unsigned long *)&tbuf->tms_utime);
		put_fs_long(current->stime,(unsigned long *)&tbuf->tms_stime);
		put_fs_long(current->cutime,(unsigned long *)&tbuf->tms_cutime);
		put_fs_long(current->cstime,(unsigned long *)&tbuf->tms_cstime);
	}
	return jiffies;
}

// ������end_data_seg��ֵ��������ϵͳȷʵ���㹻�ڴ棬���ҽ���û�г�Խ��������ݶδ�Сʱ���ú����������ݶ�ĩβΪ
// end_data_segָ����ֵ����ֵ������ڴ����β����ҪС�ڶ�ջ��β16KB������ֵ�����ݶε��½�βֵ���������ֵ��Ҫ��
// ͬ��������д�����)���ú����������û�ֱ�ӵ��ã�����libc�⺯�����а�װ�����ҷ���ֵҲ��һ����
int sys_brk(unsigned long end_data_seg)
{
	// �������ֵ���ڴ����β������С�ڣ���ջ - 16KB���������������ݶν�βֵ��
	if (end_data_seg >= current->end_code &&
	    end_data_seg < current->start_stack - 16384)
		current->brk = end_data_seg;
	return current->brk;            			// ���ؽ��̵�ǰ�����ݶν�βֵ��
}

/*
 * This needs some heave checking ...
 * I just haven't get the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 *
 * OK, I think I have the protection semantics right.... this is really
 * only important on a multi-user system anyway, to make sure one user
 * can't send a signal to a process owned by another.  -TYT, 12/12/91
 */
/*
 * ���������ҪĳЩ�ϸ�ļ��...
 * ��ֻ��û��θ��������Щ����Ҳ����ȫ����sessions/pgrp�ȵĺ��塣�������˽����ǵ���������
 *
 * OK���������Ѿ���ȷ��ʵ���˱�������...����֮������ʵֻ�Զ��û�ϵͳ����Ҫ�ģ���ȷ��һ���û������������û��Ľ��̷���
 * �źš�-TYT 12/12/91
 */
// ����ָ������pid�Ľ������Ϊpgid��
// ����pid��ָ�����̵Ľ��̺š������Ϊ0,����pid���ڵ�ǰ���̵Ľ��̺š�����pgid��ָ���Ľ�����š������Ϊ0,����������
// ������š�����ú������ڽ����̴�һ���������Ƶ���һ�������飬���������������������ͬһ���Ự��session������������
// ���£�����pgidָ����Ҫ��������ڽ�����ID����ʱ����ĻỰID�����뽫Ҫ������̵���ͬ��
int sys_setpgid(int pid, int pgid)
{
	int i;

	// �������pidΪ0,��pidȡֵΪ��ǰ���̵Ľ��̺�pid���������pgidΪ0,��pgidҲȡֵΪ��ǰ���̵�pid��[??������POSIX��
	// ׼�������г���]����pgidС��0,�򷵻���Ч�����롣
	if (!pid)
		pid = current->pid;
	if (!pgid)
		pgid = current->pid;
	if (pgid < 0)
		return -EINVAL;
	// ɨ���������飬����ָ�����̺�pid����������ҵ��˽��̺���pid�Ľ��̣����Ҹý��̵ĸ����̾��ǵ�ǰ���̻��߸ý��̾��ǵ�
	// ǰ���̣���ô���������Ѿ��ǻỰ���죬������ء���������ĻỰ�ţ�session���뵱ǰ���̵Ĳ�ͬ������ָ���Ľ������pgid
	// ��pid��ͬ����pgid�����������Ự���뵱ǰ���������Ự�Ų�ͬ����Ҳ�����ء�����Ѳ��ҵ��Ľ��̵�pgrp����Ϊpgid����
	// ����0����û���ҵ�ָ��pid�Ľ��̣��򷵻ؽ��̲����ڳ����롣
	for (i = 0 ; i < NR_TASKS ; i++)
		if (task[i] && (task[i]->pid == pid) && ((task[i]->p_pptr == current) || (task[i] == current))) {
			if (task[i]->leader)
				return -EPERM;
			if ((task[i]->session != current->session) ||
			    ((pgid != pid) &&
			     (session_of_pgrp(pgid) != current->session)))
				return -EPERM;
			task[i]->pgrp = pgid;
			return 0;
		}
	return -ESRCH;
}

// ���ص�ǰ���̵Ľ�����š���getpgid(0)��ͬ��
int sys_getpgrp(void)
{
	return current->pgrp;
}

// ����һ���Ự��session������������leader = 1��������������Ự��=�����=����̺š�
// �����ǰ�������ǻỰ���첢�Ҳ��ǳ����û���������ء��������õ�ǰ����Ϊ�»Ự���죨leader = 1�����������õ�ǰ���̻Ự
// ��session�����pgrp�����ڽ��̺�pid���������õ�ǰ����û�п����նˡ����ϵͳ���÷��ػỰ�š�
int sys_setsid(void)
{
	if (current->leader && !suser())
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = -1;      				// ��ʾ��ǰ����û�п����նˡ�
	return current->pgrp;
}

/*
 * Supplementary group ID's
 */
/*
 * ���̵������û���š�
 */
// ȡ��ǰ�������������û���š�
// �������ݽṹ��groups[]���鱣���Ž���ͬʱ�����Ķ���û���š������鹲NGROUPS�����ĳ��ֵ��NOGROUP����Ϊ-1������
// ��ʾ�Ӹ��ʼ�Ժ���������С������������б�������û���š�
// ����gidsetsize�ǻ�ȡ���û���Ÿ�����grouplist�Ǵ洢��Щ�û���ŵ��û��ռ仺�档
int sys_getgroups(int gidsetsize, gid_t *grouplist)
{
	int	i;

	// ������֤grouplistָ����ָ���û�����ռ��Ƿ��㹻��Ȼ��ӵ�ǰ���̽ṹ��groups[]���������ȡ���û���Ų����Ƶ��û�����
	// �С��ڸ��ƹ����У����groups[]�е��������ڸ����Ĳ���gitsetsize��ָ���ĸ��������ʾ�û������Ļ���̫С���������µ�ǰ
	// ����������ţ���˴˴�ȡ��Ų���������ء������ƹ����������������᷵�ظ��Ƶ��û���Ÿ�������gidsetsize - gid
	// set size���û���ż���С����
	if (gidsetsize)
		verify_area(grouplist, sizeof(gid_t) * gidsetsize);

	for (i = 0; (i < NGROUPS) && (current->groups[i] != NOGROUP); i++, grouplist++) {
		if (gidsetsize) {
			if (i >= gidsetsize)
				return -EINVAL;
			put_fs_word(current->groups[i], (short *) grouplist);
		}
	}
	return(i);              				// ����ʵ�ʺ��е��û���Ÿ�����
}

// ���õ�ǰ����ͬʱ���������������û���š�
// ����gidsetsize�ǽ����õ��û���Ÿ�����grouplist�Ǻ����û���ŵ��û��ռ仺�档
int sys_setgroups(int gidsetsize, gid_t *grouplist)
{
	int	i;

	// ���Ȳ�Ȩ�޺Ͳ�������Ч�ԡ�ֻ�г����û������޸Ļ����õ�ǰ���̵ĸ����û���ţ��������õ��������ܳ������̵�groups[NGROUPS]
	// �����������Ȼ����û���������������û���ţ���gidsetsize����������Ƶĸ���û������group[]���������һ��������ֵΪ-1
	// ���NOGROUP�������������0��
	if (!suser())
		return -EPERM;
	if (gidsetsize > NGROUPS)
		return -EINVAL;
	for (i = 0; i < gidsetsize; i++, grouplist++) {
		current->groups[i] = get_fs_word((unsigned short *) grouplist);
	}
	if (i < NGROUPS)
		current->groups[i] = NOGROUP;
	return 0;
}

// ��鵱ǰ�����Ƿ���ָ�����û���grp��.���򷵻�1,���򷵻�0.
int in_group_p(gid_t grp)
{
	int	i;

	// �����ǰ���̵���Ч��ž���grp,���ʾ��������grp������.��������1.������ڽ��̵ĸ����û���������ɨ���Ƿ���grp�������.��
	// ������Ҳ����1.��ɨ�赽ֵΪNOGROUP����,��ʾ��ɨ����ȫ����Ч��û�з���ƥ������,��˺�������0.
	if (grp == current->egid)
		return 1;

	for (i = 0; i < NGROUPS; i++) {
		if (current->groups[i] == NOGROUP)
			break;
		if (current->groups[i] == grp)
			return 1;
	}
	return 0;
}

// utsname�ṹ����һЩ�ַ����ֶΡ����ڱ���ϵͳ�����ơ����а���5���ֶΣ��ֱ��ǣ���ǰ����ϵͳ�����ơ�����ڵ����ƣ�����������
// ��ǰ����ϵͳ���м��𡢲���ϵͳ�汾���Լ�ϵͳ���е�Ӳ���������ơ��ýṹ������include/sys/utsname.h�ļ��С������ں�ʹ��
// include/linux/config.h�ļ��еĳ����������������ǵ�Ĭ��ֵ�����Ƿֱ�Ϊ��Linux������(none)������0������0.12������i386����
static struct utsname thisname = {
	UTS_SYSNAME, UTS_NODENAME, UTS_RELEASE, UTS_VERSION, UTS_MACHINE
};

// ��ȡϵͳ���Ƶ���Ϣ��
int sys_uname(struct utsname * name)
{
	int i;

	if (!name) return -ERROR;
	verify_area(name,sizeof *name);
	for(i = 0; i < sizeof *name; i++)
		put_fs_byte(((char *) &thisname)[i], i + (char *) name);
	return 0;
}

/*
 * Only sethostname; gethostname can be implemented by calling uname()
 */
/*
 * ͨ������uname()ֻ��ʵ��sethostname��gethostname��
 */
// ����ϵͳ��������ϵͳ������ڵ�������
// ����nameָ��ָ���û��������к����������ַ����Ļ�������len���������ַ������ȡ�
int sys_sethostname(char *name, int len)
{
	int	i;

	// ϵͳ������ֻ���ɳ����û����û��޸ģ��������������Ȳ��ܳ�����󳤶�MAXHOSTNAMELEN��
	if (!suser())
		return -EPERM;
	if (len > MAXHOSTNAMELEN)
		return -EINVAL;
	for (i = 0; i < len; i++) {
		if ((thisname.nodename[i] = get_fs_byte(name + i)) == 0)
			break;
	}
	// �ڸ�����Ϻ�����û��ṩ���ַ���û�а���NULL�ַ�����ô�����Ƶ����������Ȼ�û�г���MAXHOSTNAMELEN��������������
	// ���������һ��NULL�����Ѿ�����MAXHOSTNAMELEN���ַ���������һ���ַ��ĳ�NULL�ַ�����thisname.nodename[min(
	// i,MAXHOSTNAMELEN)] = 0��
	if (thisname.nodename[i]) {
		thisname.nodename[i > MAXHOSTNAMELEN ? MAXHOSTNAMELEN : i] = 0;
	}
	return 0;
}

// ȡ��ǰ����ָ����Դ�Ľ���ֵ��
// ���̵�����ṹ�ж�����һ������rlim[RLIM_NLIMITS]�����ڿ��ƽ���ʹ��ϵͳ��Դ�Ľ��ޡ�����ÿ������һ��rlimit�ṹ��
// ���а��������ֶΡ�һ��˵�����̶�ָ����Դ�ĵ�ǰ���ƽ��ޣ�soft limit���������ƣ�����һ��˵��ϵͳ��ָ����Դ���������
// ���ޣ�hard limit����Ӳ���ƣ���rlim[]�����ÿһ���Ӧϵͳ�Ե�ǰ����һ����Դ�Ľ�����Ϣ��Linux 0.12ϵͳ����6����Դ
// �涨�˽��ޣ���RLIM_NLIMITS=6����ο�ͷ�ļ�include/sys/resource.h˵����
// ����resourceָ��������ѯ����Դ���ƣ�ʵ������������ṹ��rlim[]�����������ֵ��
// ����rlim��ָ��rlimit�ṹ���û�������ָ�룬���ڴ��ȡ�õ���Դ������Ϣ��
int sys_getrlimit(int resource, struct rlimit *rlim)
{
	// ����ѯ����Դresourceʵ�����ǽ�������ṹ��rlim[]�����������ֵ��������ֵ��Ȼ���ܴ���������������RLIM_NLIMITS��
	// ����֤��rlimָ����ָ�û������㹻�Ժ�����ͰѲ���ָ������Դresource�ṹ��Ϣ���Ƶ��û��������У�������0��
	if (resource >= RLIM_NLIMITS)
		return -EINVAL;
	verify_area(rlim, sizeof *rlim);
	put_fs_long(current->rlim[resource].rlim_cur,           // ��ǰ��������ֵ��
		    (unsigned long *) rlim);
	put_fs_long(current->rlim[resource].rlim_max,           // ϵͳ��Ӳ������ֵ��
		    ((unsigned long *) rlim) + 1);
	return 0;
}

// ���õ�ǰ����ָ����Դ�Ľ���ֵ��
// ����resourceָ���������ý��޵���Դ���ƣ�ʵ������������ṹ��rlim[]�����������ֵ��
// ����rlim��ָ��rlimit�ṹ���û�������ָ�룬�����ں˶�ȡ�µ���Դ������Ϣ��
int sys_setrlimit(int resource, struct rlimit *rlim)
{
	struct rlimit new, *old;

	// �����жϲ���resource������ṹrlim[]������ֵ����Ч�ԡ�Ȼ������rlimit�ṹָ��oldָ���������ṹ��ָ����Դ�ĵ�ǰ
	// rlimit�ṹ��Ϣ�����Ű��û��ṩ����Դ������Ϣ���Ƶ���ʱrlimit�ṹnew�С���ʱ����жϳ�new�ṹ�е������ֵ��Ӳ����
	// ֵ���ڽ��̸���ԴԭӲ����ֵ�����ҵ�ǰ���ǳ����û��Ļ����ͷ�����ɳ��������ʾnew����Ϣ������߽����ǳ����û����̣�
	// ���޸�ԭ����ָ����Դ��Ϣ����new�ṹ�е���Ϣ�����ɹ�����0��
	if (resource >= RLIM_NLIMITS)
		return -EINVAL;
	old = current->rlim + resource;
	new.rlim_cur = get_fs_long((unsigned long *) rlim);
	new.rlim_max = get_fs_long(((unsigned long *) rlim) + 1);
	if (((new.rlim_cur > old->rlim_max) || (new.rlim_max > old->rlim_max)) && !suser())
		return -EPERM;
	*old = new;
	return 0;
}

/*
 * It would make sense to put struct rusuage in the task_struct,
 * except that would make the task_struct be *really big*.  After
 * task_struct gets moved into malloc'ed memory, it would
 * make sense to do this.  It will make moving the rest of the information
 * a lot simpler!  (Which we're not doing right now because we're not
 * measuring them yet).
 */
/*
 * ��rusuage�ṹ�Ž�����ṹtask_struct����ǡ���ģ���������ʹ����ṹ���ȱ�÷ǳ����ڰ�����ṹ�����ں�malloc�����
 * �ڴ���֮����������ʹ����ṹ�ܴ�Ҳû�����ˡ��⽫ʹ��������Ϣ���ƶ���÷ǳ����㣡�����ǻ�û������������Ϊ���ǻ�û�в���
 * �����ǵĴ�С����
 *
 */
// ��ȡָ�����̵���Դ������Ϣ��
// ��ϵͳ�����ṩ��ǰ���̻�������ֹ�ĺ͵ȴ��ŵ��ӽ�����Դʹ��������������who����RUSAGE_SELF���򷵻ص�ǰ���̵���Դ����
// ��Ϣ�����ָ������who��RUSAGE_CHILDREN���򷵻ص�ǰ���̵�����ֹ�͵ȴ��ŵ��ӽ�����Դ������Ϣ�����ų���RUSAGE_SELF��
// RUSAGE_CHILDREN�Լ�rusage�ṹ��������include/sys/resource.h�ļ��С�
int sys_getrusage(int who, struct rusage *ru)
{
	struct rusage r;
	unsigned long	*lp, *lpend, *dest;

	// �����жϲ���ָ�����̵���Ч�ԡ����who������RUSAGE_SELF��ָ����ǰ���̣���Ҳ����RUSAGE_CHILDREN��ָ���ӽ��̣�������
	// ��Ч�����뷵�ء���������֤��ָ��ruָ�����û���������󣬰���ʱrusage�ṹ����r���㡣
	if (who != RUSAGE_SELF && who != RUSAGE_CHILDREN)
		return -EINVAL;
	verify_area(ru, sizeof *ru);
	memset((char *) &r, 0, sizeof(r));
	// ������who��RUSAGE_SELF�����Ƶ�ǰ������Դ������Ϣ��r�ṹ�С���ָ������who��RUSAGE_CHILDREN�����Ƶ�ǰ���̵�����ֹ
	// �͵ȴ��ŵ��ӽ�����Դ������Ϣ����ʱrusage�ṹr�С���CT_TO_SECS��CT_TO_USECS���ڰ�ϵͳ��ǰ�����ת��������ֵ��΢��ֵ
	// ��ʾ�����Ƕ�����include/linux/sched.h�ļ��С�jiffies_offset��ϵͳ���������������
	if (who == RUSAGE_SELF) {
		r.ru_utime.tv_sec = CT_TO_SECS(current->utime);
		r.ru_utime.tv_usec = CT_TO_USECS(current->utime);
		r.ru_stime.tv_sec = CT_TO_SECS(current->stime);
		r.ru_stime.tv_usec = CT_TO_USECS(current->stime);
	} else {
		r.ru_utime.tv_sec = CT_TO_SECS(current->cutime);
		r.ru_utime.tv_usec = CT_TO_USECS(current->cutime);
		r.ru_stime.tv_sec = CT_TO_SECS(current->cstime);
		r.ru_stime.tv_usec = CT_TO_USECS(current->cstime);
	}
	// Ȼ����lpָ��ָ��r�ṹ��lpendָ��r�ṹĩβ������destָ��ָ���û��ռ��е�ru�ṹ������r����Ϣ���Ƶ��û��ռ�ru�ṹ�У���
	// ����0��
	lp = (unsigned long *) &r;
	lpend = (unsigned long *) (&r + 1);
	dest = (unsigned long *) ru;
	for (; lp < lpend; lp++, dest++)
		put_fs_long(*lp, dest);
	return(0);
}

// ȡ��ϵͳ��ǰʱ�䣬����ָ����ʽ���ء�
// timeval�ṹ��timezone�ṹ��������include/sys/time.h�ļ��С�timeval�ṹ�������΢�루tv_sec��tv_usec������
// �ֶΡ�timezone�ṹ���б��ؾ�������α�׼ʱ�������ķ�������tz_minuteswest��������ʱ��������ͣ�tz_dsttime������
// �ֶΡ���dst -- Daylight Savings Time��
int sys_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	// �������������timeval�ṹָ�벻�գ����ڸýṹ�з��ص�ǰʱ�䣨��ֵ��΢��ֵ��;
	// ��������������û����ݿռ���timezone�ṹ��ָ�벻�գ���Ҳ���ظýṹ����Ϣ��
	// ������startup_time��ϵͳ����ʱ�䣨��ֵ������CT_TO_SECS��CT_TO_USECS���ڰ�ϵͳ��ǰ�����ת��������ֵ��΢��ֵ��ʾ��
	// ���Ƕ�����include/linux/sched.h�ļ��С�jiffies_offset��ϵͳ���������������
	if (tv) {
		verify_area(tv, sizeof *tv);
		put_fs_long(startup_time + CT_TO_SECS(jiffies + jiffies_offset),
			    (unsigned long *) tv);
		put_fs_long(CT_TO_USECS(jiffies + jiffies_offset),
			    ((unsigned long *) tv) + 1);
	}
	if (tz) {
		verify_area(tz, sizeof *tz);
		put_fs_long(sys_tz.tz_minuteswest, (unsigned long *) tz);
		put_fs_long(sys_tz.tz_dsttime, ((unsigned long *) tz) + 1);
	}
	return 0;
}

/*
 * The first time we set the timezone, we will warp the clock so that
 * it is ticking GMT time instead of local time.  Presumably,
 * if someone is setting the timezone then we are running in an
 * environment where the programs understand about timezones.
 * This should be done at boot time in the /etc/rc script, as
 * soon as possible, so that the clock can be set right.  Otherwise,
 * various programs will get confused when the clock gets warped.
 */
/*
 * �ڵ�1������ʱ����timezone��ʱ�����ǻ�ı�ʱ��ֵ����ϵͳʹ�ø������α�׼ʱ�䣨GMT�����У�����ʹ�ñ���ʱ�䡣�Ʋ�����
 * ˵�����ĳ��������ʱ��ʱ�䣬��ô���Ǿ������ڳ���֪��ʱ��ʱ��Ļ����С�����ʱ������Ӧ����ϵͳ�����׶Σ��������/etc/rc
 * �ű������н��С�����ʱ�ӾͿ���������ȷ������Ļ����������Ժ������ʱ��������ʱ��ʱ��ı䣬���ܻ���һЩ��������г���
 * ���⡣
 */
// ����ϵͳ��ǰʱ�䡣
// ����tv��ָ���û���������timeval�ṹ��Ϣ��ָ�롣����tz���û���������timezone�ṹ��ָ�롣�ò�����Ҫ�����û�Ȩ�ޡ����
// ���߽�Ϊ�գ���ʲôҲ��������������0��
int sys_settimeofday(struct timeval *tv, struct timezone *tz)
{
	static int	firsttime = 1;
	void 		adjust_clock();

	// ����ϵͳ��ǰʱ����Ҫ�����û�Ȩ�ޡ����tzָ�벻�գ�������ϵͳʱ����Ϣ���������û�timezone�ṹ��Ϣ��ϵͳ�е�sys_tz�ṹ
	// �С�����ǵ�1�ε��ñ�ϵͳ���ò��Ҳ���tvָ�벻�գ������ϵͳʱ��ֵ��
	if (!suser())
		return -EPERM;
	if (tz) {
		sys_tz.tz_minuteswest = get_fs_long((unsigned long *) tz);
		sys_tz.tz_dsttime = get_fs_long(((unsigned long *) tz)+1);
		if (firsttime) {
			firsttime = 0;
			if (!tv)
				adjust_clock();
		}
	}
	// ���������timeval�ṹָ��tv���գ����øýṹ��Ϣ����ϵͳʱ�ӡ����ȴ�tv��ָ����ȡ����ֵ��sec����΢��ֵ��usec����ʾ��ϵͳ
	// ʱ�䣬Ȼ������ֵ�޸�ϵͳ����ʱ��ȫ�ֱ���startup_timeֵ������΢��ֵ����ϵͳ������ֵjiffies_offset��
	if (tv) {
		int sec, usec;

		sec = get_fs_long((unsigned long *)tv);
		usec = get_fs_long(((unsigned long *)tv) + 1);

		startup_time = sec - jiffies / HZ;
		jiffies_offset = usec * HZ / 1000000 - jiffies % HZ;
	}
	return 0;
}

/*
 * Adjust the time obtained from the CMOS to be GMT time instead of
 * local time.
 *
 * This is ugly, but preferable to the alternatives.  Otherwise we
 * would either need to write a program to do it in /etc/rc (and risk
 * confusion if the program gets run more than once; it would also be
 * hard to make the program warp the clock precisely n hours)  or
 * compile in the timezone information into the kernel.  Bad, bad....
 *
 * XXX Currently does not adjust for daylight savings time.  May not
 * need to do anything, depending on how smart (dumb?) the BIOS
 * is.  Blast it all.... the best thing to do not depend on the CMOS
 * clock at all, but get the time via NTP or timed if you're on a
 * network....				- TYT, 1/1/92
 */
/*
 * �Ѵ�CMOS�ж�ȡ��ʱ��ֵ����ΪGMTʱ��ֵ���棬���Ǳ���ʱ��ֵ��
 *
 * ��������������ţ���Ҫ�����������á��������Ǿ���Ҫдһ������������/etc/rc��������������£����Ҹó���ᱻ���ִ�ж�����
 * ���ҡ�����������Ҳ�����ó����ʱ�Ӿ�ȷ�ص���nСʱ�򣩻��߰�ʱ����Ϣ������ں��С���Ȼ�������ͷǳ����ǳ����...
 *
 * Ŀǰ���溯����XXX���ĵ���������û�п��ǵ�����ʱ���⡣����BIOS�ж�ô���ܣ��޴�����Ҳ������Ͳ��ÿ����ⷽ�档��Ȼ����õ���
 * ������ȫ��������CMOSʱ�ӣ�������ϵͳͨ��NTP������ʱ��Э�飩����timed��ʱ������������ʱ�䣬��������������Ļ�...��
 */
// ��ϵͳ����ʱ�����Ϊ��GMTΪ��׼��ʱ�䡣
// startup_time����ֵ�����������Ҫ��ʱ������ֵ����60��
void adjust_clock()
{
	startup_time += sys_tz.tz_minuteswest * 60;
}

// ���õ�ǰ���̴����ļ�����������Ϊmask & 0777��������ԭ�����롣
int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;
	return (old);
}

// ���ڲ���δʵ�ֵ�System Call���á�
int sys_default(unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long code){
    printk("System Call Number:%d\r\n",code);
    printk("Arg1:%X\r\n",arg1);
    printk("Arg2:%X\r\n",arg2);
    printk("Arg3:%X\r\n",arg3);
    for(;;);
}

