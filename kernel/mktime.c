/*
 *  linux/kernel/mktime.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <time.h>	// ʱ��ͷ�ļ�,�����˱�׼ʱ�����ݽṹtm��һЩ����ʱ�亯��ԭ��.

/*
 * This isn't the library routine, it is only used in the kernel.
 * as such, we don't care about years<1970 etc, but assume everything
 * is ok. Similarly, TZ etc is happily ignored. We just do everything
 * as easily as possible. Let's find something public for the library
 * routines (although I think minix times is public).
 */
/*
 * PS. I hate whoever though up the year 1970 - couldn't they have gotten
 * a leap-year instead? I also hate Gregorius, pope or no. I'm grumpy.
 */
/*
 * �ⲻ�ǿ⺯�����������ں�ʹ�á�������ǲ�����С��1970 �����ݵȣ����ٶ�һ�о���������
 * ͬ����ʱ������TZ ����Ҳ�Ⱥ��ԡ�����ֻ�Ǿ����ܼ򵥵ش������⡣������ҵ�һЩ�����Ŀ⺯��
 * ����������Ϊminix ��ʱ�亯���ǹ����ģ���
 * ���⣬�Һ��Ǹ�����1970 �꿪ʼ���� - �ѵ����ǾͲ���ѡ���һ�����꿪ʼ���Һ޸����������
 * ����̻ʡ����̣���ʲô�����ں������Ǹ�Ƣ��������ˡ�
 */

#define MINUTE 60				// 1���ӵ�����.
#define HOUR (60 * MINUTE)		// 1Сʱ������.
#define DAY (24 * HOUR)			// 1�������.
#define YEAR (365 * DAY)		// 1�������.

/* interestingly, we assume leap-years */
/* ��Ȥ�������ǿ��ǽ������� */
// ��������Ϊ����,������ÿ���¿�ʼʱ������ʱ��.
static int month[12] = {
	0,
	DAY*(31),
	DAY*(31+29),
	DAY*(31+29+31),
	DAY*(31+29+31+30),
	DAY*(31+29+31+30+31),
	DAY*(31+29+31+30+31+30),
	DAY*(31+29+31+30+31+30+31),
	DAY*(31+29+31+30+31+30+31+31),
	DAY*(31+29+31+30+31+30+31+31+30),
	DAY*(31+29+31+30+31+30+31+31+30+31),
	DAY*(31+29+31+30+31+30+31+31+30+31+30)
};

// �ú��������1970��1��1��0ʱ�𵽿������վ���������,��Ϊ����ʱ��.
// ����tm�и��ֶ��Ѿ���init/main.c�б���ֵ,��Ϣȡ��CMOS.
long kernel_mktime(struct tm * tm)
{
	long res;
	int year;

	// ���ȼ���1970�굽���ھ���������.��Ϊ��2λ��ʾ��ʽ,���Ի���2000������.���ǿ��Լ򵥵�����ǰ�����һ�����������������:
	// if(tm->tm_year<70) tm->tm_year += 100;����UNIX�����y�Ǵ�1970������.��1972�����һ������,��˹�3��(71,72,73)
	// ���ǵ�1������,������1970�꿪ʼ�����������㷽����Ӧ����1+(y-3)/4,��Ϊ(y+1)/4.res=��Щ�꾭��������ʱ��+ÿ������ʱ��1��
	// ������ʱ��+���굽����ʱ������.����,month[]�������Ѿ���2�·ݵ������а�����������ʱ������,��2�·�����������1��.���,��
	// ���겻�����겢�ҵ�ǰ�·ݴ���2�·ݵĻ�,���Ǿ�Ҫ��ȥ����.��Ϊ��70��ʼ����,���Ե�����������жϷ�����(y+2)�ܱ�4����.������
	// ����(������)�Ͳ�������.
	if(tm->tm_year < 70) tm->tm_year += 100;				//����2000������
	year = tm->tm_year - 70;
	/* magic offsets (y+1) needed to get leapyears right.*/
	/* Ϊ�˻����ȷ��������,������Ҫ����һ��ħ��ֵ(y+1) */
	res = YEAR * year + DAY * ((year + 1) / 4);
	res += month[tm->tm_mon];
	/* and (y+2) here. If it wasn't a leap-year, we have to adjust */
	/* �Լ�(y+2).���(y+2)��������,��ô���Ǿͱ�����е���(��ȥһ�������ʱ��). */
	if (tm->tm_mon > 1 && ((year + 2) % 4))
		res -= DAY;
	res += DAY * (tm->tm_mday - 1);							// �ټ��ϱ��¹�ȥ������������ʱ��.
	res += HOUR * tm->tm_hour;								// �ټ��ϵ����ȥ��Сʱ��������ʱ��.
	res += MINUTE * tm->tm_min;								// �ټ���1Сʱ�ڹ�ȥ�ķ�����������ʱ��.
	res += tm->tm_sec;										// �ټ���1�������ѹ�������.
	return res;												// �����ڴ�1970����������������ʱ��.
}




