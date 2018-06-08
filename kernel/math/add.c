/*
 * linux/kernel/math/add.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real addition routine.
 *
 * NOTE! These aren't exact: they are only 62 bits wide, and don't do
 * correct rounding. Fast hack. The reason is that we shift right the
 * values by two, in order not to have overflow (1 bit), and to be able
 * to move the sign into the mantissa (1 bit). Much simpler algorithms,
 * and 62 bits (61 really - no rounding) accuracy is usually enough. The
 * only time you should notice anything weird is when adding 64-bit
 * integers together. When using doubles (52 bits accuracy), the
 * 61-bit accuracy never shows at all.
 */
/*
 * ��ʱʵ���ӷ��ӳ���
 * 
 * ע�⣡��Щ������ȷ�����ǵĿ��ֻ��62λ�����Ҳ��ܽ�����ȷ�������������Щ���ǲݾ�֮����ԭ����Ϊ�˲��������1λ��������
 * ��ֵ������2λ������ʹ�÷���λ��1λ���ܹ�����β���С����Ƿǳ��򵥵��㷨����62λ��ʵ������61λ - û�����룩�ľ���ͨ��
 * Ҳ�㹻�ˡ�ֻ�е����64λ���������ʱ�Żᷢ��һЩ��ֵ����⡣��ʹ��˫���ȣ�52λ���ȣ�����ʱ������Զ�����ܳ���61λ����
 * �ġ�
 */

#include <linux/math_emu.h>

// ��һ�����ĸ����������Ʋ��룩��ʾ��
// ����ʱʵ��β������Ч����ȡ�����ټ�1��
// ����a����ʱʵ���ṹ������a��b�ֶ������ʵ������Ч����
#define NEGINT(a) \
__asm__("notl %0 ; notl %1 ; addl $1,%0 ; adcl $0,%1" \
	:"=r" (a->a),"=r" (a->b) \
	:"0" (a->a),"1" (a->b))

// β�����Ż���
// ������ʱʵ���任��ָ����������ʾ��ʽ�����ڷ������㡣����������Ϊ�����ʽ��
static void signify(temp_real * a)
{
// ��64λ������β������2λ�����ָ����Ҫ��2������Ϊָ���ֶ�exponent�����λ�Ƿ���λ��������ָ��ֵС���㣬˵�������Ǹ�����
// �������β���ò����ʾ��ȡ������Ȼ���ָ��ȡ��ֵ����ʱβ���в��������ƹ�2λ����Ч�������һ�������ֵ�ķ���λ��
// 30���ϣ�%0 - a->a��%1 - a->b�����ָ�shrdl $2, %1, %0��ִ��˫���ȣ�64λ�����ƣ��������β��<b,a>����2λ������
// ���ƶ���������ı�%1��a->b���е�ֵ����˻���Ҫ������������2λ��
	a->exponent += 2;
	__asm__("shrdl $2,%1,%0 ; shrl $2,%1"   // ʹ��˫����ָ���β������2λ��
		:"=r" (a->a),"=r" (a->b)
		:"0" (a->a),"1" (a->b));
	if (a->exponent < 0)                    // �Ǹ�������β���ò����ʾ��ȡ��ֵ����
		NEGINT(a);
	a->exponent &= 0x7fff;                  // ȥ������λ�����У���
}

// β���Ƿ��Ż���
// �������ʽת��Ϊ��ʱʵ����ʽ������ָ����������ʾ��ʵ��ת��Ϊ��ʱʵ����ʽ��
static void unsignify(temp_real * a)
{
// ����ֵΪ0�������ô���ֱ�ӷ��ء����������ȸ�λ��ʱʵ����ʽ�ķ���λ��Ȼ���ж�β���ĸ�λlong�ֶ�a->b�Ƿ���з���λ��
// ���У�����exponent�ֶ���ӷ���λ��ͬʱ��β�����޷�������ʽ��ʾ��ȡ����������β�����й�񻯴���ͬʱָ��ֵ����Ӧ��
// ������ִ�����Ʋ�����ʹ��β�������Чλ��Ϊ0�����a->bֵ����Ϊ��ֵ����
	if (!(a->a || a->b)) {                          // ��ֵΪ0�ͷ��ء�
		a->exponent = 0;
		return;
	}
	a->exponent &= 0x7fff;                          // ȥ������λ�����У���
	if (a->b < 0) {                                 // ȥ��������β��ȡ��ֵ��
		NEGINT(a);
		a->exponent |= 0x8000;                  // ��ʱʵ������÷���λ��
	}
	while (a->b >= 0) {                             // ��β�����й�񻯴���
		a->exponent--;
		__asm__("addl %0,%0 ; adcl %1,%1"
			:"=r" (a->a),"=r" (a->b)
			:"0" (a->a),"1" (a->b));
	}
}

// ���渡��ӷ�ָ�����㡣
// ��ʱʵ������src1 + src2 -> result��
void fadd(const temp_real * src1, const temp_real * src2, temp_real * result)
{
	temp_real a,b;
	int x1,x2,shift;

// ����ȡ��������ָ��ֵx1��x2��ȥ������λ����Ȼ���ñ���a���ڻ������ֵ��shiftΪָ����ֵ�������2�ı���ֵ����
	x1 = src1->exponent & 0x7fff;
	x2 = src2->exponent & 0x7fff;
	if (x1 > x2) {
		a = *src1;
		b = *src2;
		shift = x1-x2;
	} else {
		a = *src2;
		b = *src1;
		shift = x2-x1;
	}
// ���������̫�󣬴��ڵ���2��64�η��������ǿ��Ժ���С���Ǹ�������bֵ������ֱ�ӷ���aֵ���ɡ������������ڵ���2��32��
// ������ô���ǿ��Ժ���Сֵb�еĵ�32λֵ���������ǰ�b�ĸ�long�ֶ�ֵb.b����32λ�����ŵ�b.a�С�Ȼ���b��ָ��ֵ��Ӧ������
// 32�η�����ָ����ֵ��ȥ32����������֮����ӵ���������β��������������ͬ�����С�
	if (shift >= 64) {
		*result = a;
		return;
	}
	if (shift >= 32) {
		b.a = b.b;
		b.b = 0;
		shift -= 32;
	}
// �����ٽ���ϸ�µĵ������Խ�������ߵ�������ͬ�����������ǰ�Сֵb��β������shift��λ���������ߵ�ָ����ͬ������ͬһ����������
// ���Ǿ�Ҫ�Զ�β��������������ˡ����֮ǰ������Ҫ�Ȱ�����ת���ɷ��������ʽ���ڼӷ�������ٱ任����ʱʵ����ʽ��
	__asm__("shrdl %4,%1,%0 ; shrl %4,%1"                   // ˫���ȣ�64λ�����ơ�
		:"=r" (b.a),"=r" (b.b)
		:"0" (b.a),"1" (b.b),"c" ((char) shift));
	signify(&a);                                            // �任��ʽ��
	signify(&b);
	__asm__("addl %4,%0 ; adcl %5,%1"                       // ִ�мӷ����㡣
		:"=r" (a.a),"=r" (a.b)
		:"0" (a.a),"1" (a.b),"g" (b.a),"g" (b.b));
	unsignify(&a);                                          // �ٱ任����ʱʵ����ʽ��
	*result = a;
}

