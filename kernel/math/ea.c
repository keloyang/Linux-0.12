/*
 * linux/kernel/math/ea.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * Calculate the effective address.
 */
/*
 * ������Ч��ַ��
 */

#include <stddef.h>     // ��׼����ͷ�ļ���������ʹ�������е�offsetof()���塣

#include <linux/math_emu.h>
#include <asm/segment.h>

// info�ṹ�и����Ĵ����ڽṹ�е�ƫ��λ�á�offsetof()������ָ���ֶ��ڽṹ�е�ƫ��λ�á��μ�include/stddef.h�ļ���
static int __regoffset[] = {
	offsetof(struct info,___eax),
	offsetof(struct info,___ecx),
	offsetof(struct info,___edx),
	offsetof(struct info,___ebx),
	offsetof(struct info,___esp),
	offsetof(struct info,___ebp),
	offsetof(struct info,___esi),
	offsetof(struct info,___edi)
};

// ȡinfo�ṹ��ָ��λ�ô��Ĵ������ݡ�
#define REG(x) (*(long *)(__regoffset[(x)]+(char *) info))

// ��2�ֽ�Ѱַģʽ�е�2������ָʾ�ֽ�SIB��Scale, Index, Base����ֵ��
static char * sib(struct info * info, int mod)
{
	unsigned char ss,index,base;
	long offset = 0;

// ���ȴ��û��������ȡ��SIB�ֽڣ�Ȼ��ȡ�������ֶ�λֵ��
	base = get_fs_byte((char *) EIP);
	EIP++;
	ss = base >> 6;                 // �������Ӵ�Сss��
	index = (base >> 3) & 7;        // ����ֵ��������index��
	base &= 7;                      // ����ַ����base��
// �����������Ϊ0b100����ʾ������ƫ��ֵ����������ƫ��ֵoffset=��Ӧ�Ĵ������ݡ��������ӡ�
	if (index == 4)
		offset = 0;
	else
		offset = REG(index);
	offset <<= ss;
// �����һMODRM�ֽ��е�MOD��Ϊ�㣬����Base������0b101�����ʾ��ƫ��ֵ��baseָ���ļĴ����С����ƫ��offset��Ҫ�ټ���base
// ��Ӧ�Ĵ����е����ݡ�
	if (mod || base != 5)
		offset += REG(base);
// ���MOD=1�����ʾƫ��ֵΪ1�ֽڡ�������MOD=2������base=0b101����ƫ��ֵΪ4�ֽڡ�
	if (mod == 1) {
		offset += (signed char) get_fs_byte((char *) EIP);
		EIP++;
	} else if (mod == 2 || base == 5) {
		offset += (signed) get_fs_long((unsigned long *) EIP);
		EIP += 4;
	}
// ��󱣴沢����ƫ��ֵ��
	I387.foo = offset;
	I387.fos = 0x17;
	return (char *) offset;
}

// ����ָ����Ѱַģʽ�ֽڼ�����Ч��ֵַ��
char * ea(struct info * info, unsigned short code)
{
	unsigned char mod,rm;
	long * tmp = &EAX;
	int offset = 0;

// ����ȡ�����е�MOD�ֶκ�R/M�ֶ�ֵ�����MOD=0b11����ʾ�ǵ��ֽ�ָ�û��ƫ���ֶΡ����R/M�ֶ�=0b100������MOD��Ϊ0b11��
// ��ʾ��2�ֽڵ�ַģʽѰַ����˵���sib()���ƫ��ֵ�����ؼ��ɡ�
	mod = (code >> 6) & 3;          // MOD�ֶΡ�
	rm = code & 7;                  // R/M�ֶΡ�
	if (rm == 4 && mod != 3)
		return sib(info,mod);
// ���R/M�ֶ�Ϊ0b101������MODΪ0����ʾ�ǵ��ֽڵ�ַģʽ�����Һ���32�ֽ�ƫ��ֵ������ȡ���û�������4�ֽ�ƫ��ֵ�����沢����
// ֮��
	if (rm == 5 && !mod) {
		offset = get_fs_long((unsigned long *) EIP);
		EIP += 4;
		I387.foo = offset;
		I387.fos = 0x17;
		return (char *) offset;
	}
// ������������������MOD���д�������ȡ��R/M�����Ӧ�Ĵ������ݵ�ֵ��Ϊָ��tmp������MOD=0����ƫ��ֵ������MOD=1�������
// ��1�ֽ�ƫ��ֵ������MOD=2���������4�ֽ�ƫ��ֵ����󱣴沢������Ч��ֵַ��
	tmp = & REG(rm);
	switch (mod) {
		case 0: offset = 0; break;
		case 1:
			offset = (signed char) get_fs_byte((char *) EIP);
			EIP++;
			break;
		case 2:
			offset = (signed) get_fs_long((unsigned long *) EIP);
			EIP += 4;
			break;
		case 3:
			math_abort(info,1<<(SIGILL-1));
	}
	I387.foo = offset;
	I387.fos = 0x17;
	return offset + (char *) *tmp;
}

