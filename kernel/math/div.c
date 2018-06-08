/*
 * linux/kernel/math/div.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real division routine.
 */

#include <linux/math_emu.h>

// ��ָ��cָ���4�ֽ�����������1λ��
static void shift_left(int * c)
{
	__asm__ __volatile__("movl (%0),%%eax ; addl %%eax,(%0)\n\t"
		"movl 4(%0),%%eax ; adcl %%eax,4(%0)\n\t"
		"movl 8(%0),%%eax ; adcl %%eax,8(%0)\n\t"
		"movl 12(%0),%%eax ; adcl %%eax,12(%0)"
		::"r" ((long) c):"ax");
}

// ��ָ��cָ���4�ֽ�����������1λ��
static void shift_right(int * c)
{
	__asm__("shrl $1,12(%0) ; rcrl $1,8(%0) ; rcrl $1,4(%0) ; rcrl $1,(%0)"
		::"r" ((long) c));
}

// �������㡣
// 16�ֽڼ������㣬b-a ->a���������Ƿ��н�λ��CF=1������OK�����޽�λ��CF=0����ok = 1������ok = 0��
static int try_sub(int * a, int * b)
{
	char ok;

	__asm__ __volatile__("movl (%1),%%eax ; subl %%eax,(%2)\n\t"
		"movl 4(%1),%%eax ; sbbl %%eax,4(%2)\n\t"
		"movl 8(%1),%%eax ; sbbl %%eax,8(%2)\n\t"
		"movl 12(%1),%%eax ; sbbl %%eax,12(%2)\n\t"
		"setae %%al":"=a" (ok):"c" ((long) a),"d" ((long) b));
	return ok;
}

// 16�ֽڳ�����
// ����a/b -> c�����ü���ģ����ֽڳ�����
static void div64(int * a, int * b, int * c)
{
	int tmp[4];     // ����������
	int i;
	unsigned int mask = 0;  // ����λ��

	c += 4;
// 16�ֽڹ�64λ��
	for (i = 0 ; i<64 ; i++) {
		if (!(mask >>= 1)) {
			c--;
			mask = 0x80000000;
		}
// ������ֵtmp��ʼ��Ϊaֵ��
		tmp[0] = a[0]; tmp[1] = a[1];
		tmp[2] = a[2]; tmp[3] = a[3];
		if (try_sub(b,tmp)) {           // �Ƿ��н�λ��
			*c |= mask;             // ����޽�λ���õ�ǰ����λ������������a�������´β�����
			a[0] = tmp[0]; a[1] = tmp[1];
			a[2] = tmp[2]; a[3] = tmp[3];
		}
		shift_right(b);         // ����һλ��ʹb��ֵ��ֵ������ͬһ����
	}
}

// ���渡��ָ��FDIV��
// ��ʱʵ��src1 / src2 -> result����
void fdiv(const temp_real * src1, const temp_real * src2, temp_real * result)
{
	int i,sign;
	int a[4],b[4],tmp[4] = {0,0,0,0};

// ����ȷ������������ķ��š�����ֵ�������߷���λ���ֵ��Ȼ���жϳ���src2ֵ�Ƿ�Ϊ0������ǣ����ñ�����쳣��
	sign = (src1->exponent ^ src2->exponent) & 0x8000;
	if (!(src2->a || src2->b)) {
		set_ZE();               // �ñ�����쳣��
		return;
	}
// Ȼ���������ָ��ֵ�����ʱָ��ֵ��Ҫ�������������ָ��ʹ��ƫ�ø�ʽ���棬��������ָ�����ʱƫ����Ҳ����ȥ�ˣ������
// Ҫ����ƫ����ֵ����ʱʵ����ƫ������16383����
        i = (src1->exponent & 0x7fff) - (src2->exponent & 0x7fff) + 16383;
// ������ָ������˸�ֵ����ʾ���������������硣����ֱ�ӷ��ش����ŵ���ֵ��
	if (i<0) {
		set_UE();
		result->exponent = sign;        // ���÷���λ��
		result->a = result->b = 0;      // ���÷���ֵΪ0��
		return;
	}
// ����ʱʵ��src1��src2��Ч��������������a��b��
	a[0] = a[1] = 0;
	a[2] = src1->a;
	a[3] = src1->b;
	b[0] = b[1] = 0;
	b[2] = src2->a;
	b[3] = src2->b;
// ���b[3]���ڵ���0������й�񻯴�������b�������Ƶ���b[3]Ϊ������
	while (b[3] >= 0) {
		i++;
		shift_left(b);
	}
// ����64λ����������
	div64(a,b,tmp);
// ��������tmp[0]��tmp[1]��tmp[2]��tmp[3]��Ϊ0�Ļ���˵�����Ϊ0,������ָ��iΪ0��������й�񻯴���
	if (tmp[0] || tmp[1] || tmp[2] || tmp[3]) {
		while (i && tmp[3] >= 0) {      // ���й�񻯴���
			i--;
			shift_left(tmp);
		}
		if (tmp[3] >= 0)                // ���tmp[3]���ڵ���0������״̬�ַǸ�ʽ���쳣��־λ��
			set_DE();
	} else
		i = 0;          // ���ý��ָ��Ϊ0��
// ������ָ������0x7fff����ʾ�������磬��������״̬������쳣��־λ�������ء�
	if (i>0x7fff) {
		set_OE();
		return;
	}
// ���tmp[0]��tmp[1]��Ϊ0��������״̬�־����쳣��־λ
	if (tmp[0] || tmp[1])
		set_PE();
	result->exponent = i | sign;            // ���÷�����ʱʵ���ķ���λ��ָ��ֵ��
	result->a = tmp[2];                     // ���÷�����ʱʵ������Чֵ��
	result->b = tmp[3];
}

