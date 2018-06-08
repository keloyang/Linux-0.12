/*
 * linux/kernel/math/mul.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real multiplication routine.
 */
/*
 * ��ʱʵ���˷��ӳ���
 */

#include <linux/math_emu.h>

// ��cָ�봦��16�ֽ�ֵ����1λ����2����
static void shift(int * c)
{
	__asm__("movl (%0),%%eax ; addl %%eax,(%0)\n\t"
		"movl 4(%0),%%eax ; adcl %%eax,4(%0)\n\t"
		"movl 8(%0),%%eax ; adcl %%eax,8(%0)\n\t"
		"movl 12(%0),%%eax ; adcl %%eax,12(%0)"
		::"r" ((long) c):"ax");
}

// 2����ʱʵ����ˣ��������cָ�봦��16�ֽڣ���
static void mul64(const temp_real * a, const temp_real * b, int * c)
{
	__asm__("movl (%0),%%eax\n\t"           // ȡa->a��ֵ��eax��
		"mull (%1)\n\t"                 // ��b->a��ֵ��ˡ�
		"movl %%eax,(%2)\n\t"           // �˻��ĵ�λ����c[0]��
		"movl %%edx,4(%2)\n\t"          // �˻��ĸ�λ����c[1]��
		"movl 4(%0),%%eax\n\t"          // ȡa->b��ֵ��eax��
		"mull 4(%1)\n\t"                // ��b->b��ֵ��ˡ�
		"movl %%eax,8(%2)\n\t"          // �˻��ĵ�λ����c[2]��
		"movl %%edx,12(%2)\n\t"         // �˻��ĸߵͷ���c[3]��
		"movl (%0),%%eax\n\t"           // ȡa->a��ֵ��eax��
		"mull 4(%1)\n\t"                // ��b->b��ֵ��ˡ�
		"addl %%eax,4(%2)\n\t"          // �˻��ĵ�λ��c[1]��ӷ���c[1]��
		"adcl %%edx,8(%2)\n\t"          // �˻��ĸ�λ��c[2]����ټӽ�λ��Ȼ�����a[2]��
		"adcl $0,12(%2)\n\t"            // ��0��c[3]����ټӽ�λ��Ȼ�����c[3]��
		"movl 4(%0),%%eax\n\t"          // ȡa->b��ֵ��eax��
		"mull (%1)\n\t"                 // ��b->a��ֵ��ˡ�
		"addl %%eax,4(%2)\n\t"          // �˻��ĵ�λ��c[1]��ӷ���c[1]��
		"adcl %%edx,8(%2)\n\t"          // �˻��ĸ�λ��c[2]����ټӽ�λ��Ȼ�����a[2]��
		"adcl $0,12(%2)"                // ��0��c[3]����ټӽ�λ��Ȼ�����c[3]��
		::"b" ((long) a),"c" ((long) b),"D" ((long) c)
		:"ax","dx");
}

// ���渡��ָ��FMUL��
// ��ʱʵ��src1 * src2 -> result����
void fmul(const temp_real * src1, const temp_real * src2, temp_real * result)
{
	int i,sign;
	int tmp[4] = {0,0,0,0};

// ����ȷ��������˵ķ��š�����ֵ�������߷���λ���ֵ��Ȼ�����˺��ָ��ֵ�����ʱָ��ֵ��Ҫ��ӡ���������ָ��ʹ��ƫ��
// ��ʽ���棬��������ָ�����ʱƫ����Ҳ���������Σ������Ҫ����һ��ƫ����ֵ����ʱʵ����ƫ������16383����
	sign = (src1->exponent ^ src2->exponent) & 0x8000;
	i = (src1->exponent & 0x7fff) + (src2->exponent & 0x7fff) - 16383 + 1;
// ������ָ������˸�ֵ����ʾ������˺�������硣����ֱ�ӷ��ش����ŵ���ֵ��������ָ������0x7fff����ʾ�������磬����
// ����״̬������쳣��־λ�������ء�
	if (i<0) {
		result->exponent = sign;
		result->a = result->b = 0;
		return;
	}
	if (i>0x7fff) {
		set_OE();       // ��λ�����־λ��
		return;
	}
// �������β����˺�����Ϊ0����Խ��β�����й�񻯴��������ƽ��β��ֵ��ʹ�������ЧλΪ1��ͬʱ��Ӧ�ص���ָ��ֵ�����
// ������˺�16�ֽڵĽ�β��Ϊ0����Ҳ����ָ��ֵΪ0��������˽����������ʱʵ������result�С�
	mul64(src1,src2,tmp);
	if (tmp[0] || tmp[1] || tmp[2] || tmp[3])
		while (i && tmp[3] >= 0) {
			i--;
			shift(tmp);
		}
	else
		i = 0;
	result->exponent = i | sign;
	result->a = tmp[2];
	result->b = tmp[3];
}


