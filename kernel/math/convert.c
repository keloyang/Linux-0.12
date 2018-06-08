/*
 * linux/kernel/math/convert.c
 *
 * (C) 1991 Linus Torvalds
 */

#include <linux/math_emu.h>

/*
 * NOTE!!! There is some "non-obvious" optimisations in the temp_to_long
 * and temp_to_short conversion routines: don't touch them if you don't
 * know what's going on. They are the adding of one in the rounding: the
 * overflow bit is also used for adding one into the exponent. Thus it
 * looks like the overflow would be incorrectly handled, but due to the
 * way the IEEE numbers work, things are correct.
 *
 * There is no checking for total overflow in the conversions, though (ie
 * if the temp-real number simply won't fit in a short- or long-real.)
 */
/*
 * ע�⣡������temp_to_long��temp_to_short��������ת���ӳ�������Щ�������ԡ����Ż�������������Ͳ�Ҫ�����޸ġ���������
 * ������еļ�1�����λҲͬ����������ָ���м�1.��˿���ȥ�������û�б���ȷ�ش�����������IEEE��������׼���涨���ݸ�ʽ�Ĳ�
 * ����ʽ����Щ��������ȷ�ġ�
 * 
 * ��������û�ж�ת�������������������⣨����ʱʵ���Ƿ��ܹ��򵥵ط����ʵ����ʵ����ʽ�У���
 */

// ��ʵ��ת������ʱʵ����ʽ��
// ��ʵ��������32λ������Ч����β����������23λ��ָ����8λ������1������λ��
void short_to_temp(const short_real * a, temp_real * b)
{
// ���ȴ���ת���Ķ�ʵ����0���������Ϊ0,�����ö�Ӧ��ʱʵ��b����Ч��Ϊ0��Ȼ����ݶ�ʵ������λ������ʱʵ���ķ���λ����exponent
// �������Чλ��
	if (!(*a & 0x7fffffff)) {
		b->a = b->b = 0;                // ����ʱʵ������Ч�� = 0��
		if (*a)
			b->exponent = 0x8000;   // ���÷���λ��
		else
			b->exponent = 0;
		return;
	}
// ����һ���ʵ������ȷ����Ӧ��ʱʵ����ָ��ֵ��������Ҫ�õ�������ƫ�ñ�ʾ�����ĸ����ʵ��ָ����ƫ������127������ʱʵ��ָ����ƫ��
// ����16383�������ȡ����ʵ����ָ��ֵ����Ҫ������е�ƫ����Ϊ16383����ʱ���γ�����ʱʵ����ʽ��ָ��ֵexponent�����⣬�����ʵ��
// �Ǹ���������Ҫ������ʱʵ���ķ���λ��λ79������һ������β��ֵ�������ǰѶ�ʵ������8λ����23λβ�������Чλ������ʱʵ����λ62����
// ����ʱʵ��β��λ63����Ҫ����һ��1������Ҫ����0x80000000����������ʱʵ����32λ��Ч����
	b->exponent = ((*a>>23) & 0xff)-127+16383;      // ȡ����ʵ��ָ��λ������ƫ������
	if (*a<0)
		b->exponent |= 0x8000;                  // ��Ϊ���������÷���λ��
	b->b = (*a<<8) | 0x80000000;                    // ����β������ӹ̶�1ֵ��
	b->a = 0;
}

// ��ʵ��ת������ʱʵ����ʽ��
// ������short_to_temp()��ȫһ����������ʵ��ָ��ƫ������1034��
void long_to_temp(const long_real * a, temp_real * b)
{
	if (!a->a && !(a->b & 0x7fffffff)) {
		b->a = b->b = 0;                // ����ʱʵ������Ч�� = 0��
		if (a->b)
			b->exponent = 0x8000;   // ���÷���λ��
		else
			b->exponent = 0;
		return;
	}
	b->exponent = ((a->b >> 20) & 0x7ff)-1023+16383;        // ȡ��ʵ��ָ��������ƫ������
	if (a->b<0)
		b->exponent |= 0x8000;          // ��Ϊ���������÷���λ��
	b->b = 0x80000000 | (a->b<<11) | (((unsigned long)a->a)>>21);
	b->a = a->a<<11;                        // ����β������1��
}

// ��ʱʵ��ת���ɶ�ʵ����ʽ��
// ������short_to_temp()�෴������Ҫ�����Ⱥ��������⡣
void temp_to_short(const temp_real * a, short_real * b)
{
// ���ָ������Ϊ0����������޷���λ���ö�ʵ��Ϊ-0��0��
	if (!(a->exponent & 0x7fff)) {
		*b = (a->exponent)?0x80000000:0;
		return;
	}
// �ȴ���ָ�����֡���������ʱʵ��ָ��ƫ������16383��Ϊ��ʵ����ƫ����127��
	*b = ((((long) a->exponent)-16383+127) << 23) & 0x7f800000;
	if (a->exponent < 0)                    // ���Ǹ��������÷���λ��
		*b |= 0x80000000;
	*b |= (a->b >> 8) & 0x007fffff;         // ȡ��ʱʵ����Ч����23λ��
// ���ݿ������е���������ִ�����������
	switch (ROUNDING) {
		case ROUND_NEAREST:
			if ((a->b & 0xff) > 0x80)
				++*b;
			break;
		case ROUND_DOWN:
			if ((a->exponent & 0x8000) && (a->b & 0xff))
				++*b;
			break;
		case ROUND_UP:
			if (!(a->exponent & 0x8000) && (a->b & 0xff))
				++*b;
			break;
	}
}

// ��ʱʵ��ת���ɳ�ʵ����
void temp_to_long(const temp_real * a, long_real * b)
{
// ���ָ������Ϊ0����������޷���λ���ó�ʵ��Ϊ-0��0��
	if (!(a->exponent & 0x7fff)) {
		b->a = 0;
		b->b = (a->exponent)?0x80000000:0;
		return;
	}
// �ȴ���ָ�����֡���������ʱʵ��ָ��ƫ������16383��Ϊ��ʵ����ƫ����1023.
	b->b = (((0x7fff & (long) a->exponent)-16383+1023) << 20) & 0x7ff00000;
	if (a->exponent < 0)            // ���Ǹ��������÷���λ��
		b->b |= 0x80000000;
	b->b |= (a->b >> 11) & 0x000fffff;      // ȡ��ʱʵ����Ч����20λ��
	b->a = a->b << 21;
	b->a |= (a->a >> 11) & 0x001fffff;
// ���ݿ������е���������ִ�����������
	switch (ROUNDING) {
		case ROUND_NEAREST:
			if ((a->a & 0x7ff) > 0x400)
				__asm__("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
		case ROUND_DOWN:
			if ((a->exponent & 0x8000) && (a->b & 0xff))
				__asm__("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
		case ROUND_UP:
			if (!(a->exponent & 0x8000) && (a->b & 0xff))
				__asm__("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
	}
}

// ��ʱʵ��ת������ʱ������ʽ��
// ��ʱ����Ҳ��10�ֽڱ�ʾ�����е�8�ֽ����޷�������ֵ����2�ֽڱ�ʾָ��ֵ�ͷ���λ�������2�ֽ������ЧλΪ1�����ʾ�Ǹ�����
// ��λ0����ʾ��������
void real_to_int(const temp_real * a, temp_int * b)
{
// ����ֵ���ֵ��2��63�η���������ʱʵ��ƫ��ֵ16383,��ʾһ������ֵת��Ϊ��ʱʵ������ʱʵ��ָ�����ֵ����ȥ��ʱʵ��ָ����
// �õ�ָ����ֵ���൱�����������ֵ�Ĳ�ֵ����
	int shift =  16383 + 63 - (a->exponent & 0x7fff);
	unsigned long underflow;

	b->a = b->b = underflow = 0;    // ��ʼ����ʱ����ֵΪ0��
	b->sign = (a->exponent < 0);    // ����ʱ������������ʱʵ������һ�¡�
	if (shift < 0) {                // ���ָ����ֵС��0��˵�������ʱʵ�����ܷ�����ʱ�����У�
		set_OE();               // ��״̬�����λ��
		return;
	}
// �����ֵ��ֵС��2��32�η���ֱ�Ӱ�ʵ��ֵ��������ֵ��
	if (shift < 32) {
		b->b = a->b; b->a = a->a;
// �����ֵ��ֵ����2��32�η���64�η�֮�䣬��ʵ����λa->b����������λb->a��Ȼ���ʵ����λ�������������underflow��ָ����ֵ
// shift-32��
	} else if (shift < 64) {
		b->a = a->b; underflow = a->a;
		shift -= 32;
// �����ֵ��ֵ����2��64�η���96�η�֮�䣬��ʵ����λa->b�������������underflow��ָ����ֵshift-64��
	} else if (shift < 96) {
		underflow = a->b;
		shift -= 64;
// ���򷵻�0��
	} else
		return;
// �����ٽ���ϸ�µĵ��������������ǰ���ʱ����b�������������underflow����shiftλ��
	__asm__("shrdl %2,%1,%0"
		:"=r" (underflow),"=r" (b->a)
		:"c" ((char) shift),"0" (underflow),"1" (b->a));
// Ȼ�����ʱ����b��β��b->a����shiftλ��
	__asm__("shrdl %2,%1,%0"
		:"=r" (b->a),"=r" (b->b)
		:"c" ((char) shift),"0" (b->a),"1" (b->b));
// ������ʱ����b��β��b->b����shiftλ��
	__asm__("shrl %1,%0"
		:"=r" (b->b)
		:"c" ((char) shift),"0" (b->b));
// ���ݿ������е���������ִ�����������
	switch (ROUNDING) {
		case ROUND_NEAREST:
			__asm__("addl %4,%5 ; adcl $0,%0 ; adcl $0,%1"
				:"=r" (b->a),"=r" (b->b)
				:"0" (b->a),"1" (b->b)
				,"r" (0x7fffffff + (b->a & 1))
				,"m" (*&underflow));
			break;
		case ROUND_UP:
			if (!b->sign && underflow)
				__asm__("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
		case ROUND_DOWN:
			if (b->sign && underflow)
				__asm__("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
	}
}

// ��ʱ����ת������ʱʵ����ʽ��
void int_to_real(const temp_int * a, temp_real * b)
{
// ����ԭֵ������������ת������ʱʵ��ʱָ��������Ҫ����ƫ����16383�⣬��Ҫ����63����ʾ
	b->a = a->a;
	b->b = a->b;
	if (b->a || b->b)
		b->exponent = 16383 + 63 + (a->sign? 0x8000:0);
	else {
		b->exponent = 0;
		return;
	}
// �Ը�ʽת�������ʱʵ�����й�񻯴���������Ч�������Чλ����0��
	while (b->b >= 0) {
		b->exponent--;
		__asm__("addl %0,%0 ; adcl %1,%1"
			:"=r" (b->a),"=r" (b->b)
			:"0" (b->a),"1" (b->b));
	}
}

