/*
 * linux/kernel/math/compare.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real comparison routines
 */
/*
 * �ۼ�������ʱʵ���Ƚ��ӳ���
 */

#include <linux/math_emu.h>

// ��λ״̬���е�C3��C2��C1��C0����λ��
#define clear_Cx() (I387.swd &= ~0x4500)

// ����ʱʵ��a���й�񻯴�������ʾ��ָ������Ч����ʽ��
// ���磺102.345��ʾ��1.02345 X 10^2��0.0001234��ʾ��1.234 X 10^-1����Ȼ���������Ƕ����Ʊ�ʾ��
static void normalize(temp_real * a)
{
	int i = a->exponent & 0x7fff;           // ȡָ��ֵ����ȥ����λ����
	int sign = a->exponent & 0x8000;        // ȡ����λ��

// �����ʱʵ��a��64λ��Ч����β����Ϊ0����ô˵��a����0��������a��ָ�������ء�
	if (!(a->a || a->b)) {
		a->exponent = 0;
		return;
	}
// ���a��β���������0ֵλ����ô��β�����ƣ�ͬʱ����ָ��ֵ���ݼ�����ֱ��β����b�ֶ������ЧλMSB��1λ�ã���ʱb����Ϊ��ֵ��
// �������ӷ���λ��
	while (i && a->b >= 0) {
		i--;
		__asm__("addl %0,%0 ; adcl %1,%1"
			:"=r" (a->a),"=r" (a->b)
			:"0" (a->a),"1" (a->b));
	}
	a->exponent = i | sign;
}

// ���渡��ָ��FTST��
// ��ջ���ۼ���ST(0)��0�Ƚϣ������ݱȽϽ����������λ����ST > 0.0����C3��C2��C0�ֱ�Ϊ000����ST < 0.0��������λΪ001����
// ST == 0.0��������λ��100�������ɱȽϣ�������λΪ111��
void ftst(const temp_real * a)
{
	temp_real b;

// ������״̬����������־λ�����ԱȽ�ֵb��ST�����й�񻯴�����b�������㲢�������˷���λ���Ǹ�����������������λC0����������
// ����λC3��
	clear_Cx();
	b = *a;
	normalize(&b);
	if (b.a || b.b || b.exponent) {
		if (b.exponent < 0)
			set_C0();
	} else
		set_C3();
}

// ���渡��ָ��FCOM��
// �Ƚ���������src1��src2�������ݱȽϽ����������λ����src1 > src2����C3��C2��C0�ֱ�Ϊ000����src1 < src2��������λΪ
// 001����������ȣ�������λ��100��
void fcom(const temp_real * src1, const temp_real * src2)
{
	temp_real a;

	a = *src1;
	a.exponent ^= 0x8000;           // ����λȡ����
	fadd(&a,src2,&a);               // ������ӣ����������
	ftst(&a);                       // ���Խ������������λ��
}

// ���渡��ָ��FUCOM���޴���Ƚϣ���
// ���ڲ�����֮һ��NaN�ıȽϡ�
void fucom(const temp_real * src1, const temp_real * src2)
{
	fcom(src1,src2);
}

