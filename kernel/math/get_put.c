/*
 * linux/kernel/math/get_put.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * This file handles all accesses to user memory: getting and putting
 * ints/reals/BCD etc. This is the only part that concerns itself with
 * other than temporary real format. All other cals are strictly temp_real.
 */
/*
 * �����������ж��û��ڴ�ķ��ʣ���ȡ�ʹ���ָ��/ʵ��ֵ/BCD��ֵ�ȡ������漰��ʱʵ������������ʽ���еĲ��֡�������������
 * ȫ��ʹ����ʱʵ����ʽ��
 */

#include <linux/math_emu.h>
#include <asm/segment.h>

// ȡ�û��ڴ��еĶ�ʵ����������ʵ������
// ���ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ö�ʵ��������Ч��ַ��math/ea.c����Ȼ����û�
// ��������ȡ��Ӧʵ��ֵ�������û���ʵ��ת������ʱʵ����math/convert.c����
// ������tmp - ת������ʱʵ�����ָ�룻info - info�ṹָ�룻code - ָ����롣
void get_short_real(temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	short_real sr;

	addr = ea(info,code);                           // ������Ч��ַ��
	sr = get_fs_long((unsigned long *) addr);       // ȡ�û��������е�ֵ��
	short_to_temp(&sr,tmp);                         // ת������ʱʵ����ʽ��
}

// ȡ�û��ڴ��еĳ�ʵ����˫����ʵ������
// ���ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ó�ʵ��������Ч��ַ��math/ec.c����Ȼ���
// �û���������ȡ��Ӧʵ��ֵ�������û�ʵ��ֵת������ʱʵ����math/convert.c����
// ������tmp - ת������ʱʵ�����ָ�룻info - info�ṹָ�룻code - ָ����롣
void get_long_real(temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	long_real lr;

	addr = ea(info,code);                           // ȡָ���е���Ч��ַ��
	lr.a = get_fs_long((unsigned long *) addr);     // ȡ��8�ֽ�ʵ����
	lr.b = get_fs_long(1 + (unsigned long *) addr); // ת������ʱʵ����ʽ��
	long_to_temp(&lr,tmp);
}

// ȡ�û��ڴ��е���ʱʵ����
// ���ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ����ʱʵ��������Ч��ַ��math/ea.c����Ȼ��
// ���û���������ȡ��Ӧ��ʱʵ��ֵ��
// ������tmp - ת������ʱʵ�����ָ�룻info - info�ṹָ�룻code - ָ����롣
void get_temp_real(temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;

	addr = ea(info,code);           // ȡָ���е���Ч��ֵַ��
	tmp->a = get_fs_long((unsigned long *) addr);
	tmp->b = get_fs_long(1 + (unsigned long *) addr);
	tmp->exponent = get_fs_word(4 + (unsigned short *) addr);
}

// ȡ�û��ڴ��еĶ�������ת������ʱʵ����ʽ��
// ��ʱ����Ҳ��10�ֽڱ�ʾ�����е�8�ֽ����޷�������ֵ����2�ֽڱ�ʾָ��ֵ�ͷ���λ�������2�ֽ������ЧλΪ1,���ʾ�Ǹ�����
// �������Чλ��0,��ʾ��������
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ö�����������Ч��ַ��math/ea.c��
// Ȼ����û���������ȡ��Ӧ����ֵ��������Ϊ��ʱ������ʽ��������ʱ����ֵת������ʱʵ����math/convert.c����
// ������tmp - ת������ʱʵ�����ָ�룻info - info�ṹָ�룻code - ָ����롣
void get_short_int(temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);           // ȡָ���е���Ч��ֵַ��
	ti.a = (signed short) get_fs_word((unsigned short *) addr);
	ti.b = 0;
	if (ti.sign = (ti.a < 0))       // ���Ǹ�������������ʱ��������λ��
		ti.a = - ti.a;          // ��ʱ������β��������Ϊ�޷�������
	int_to_real(&ti,tmp);           // ����ʱ����ת������ʱʵ����ʽ��
}

// ȡ�û��ڴ��еĳ�������ת������ʱʵ����ʽ��
// ���ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ó�����������Ч��ַ��math/ea.c����Ȼ���
// �û���������ȡ��Ӧ����ֵ��������Ϊ��ʱ������ʽ��������ʱ����ֵת������ʱʵ����math/convert.c����
// ������tmp - ת������ʱʵ�����ָ�룻info - info�ṹָ�룻code - ָ����롣
void get_long_int(temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);           // ȡָ���е���Ч��ֵַ��
	ti.a = get_fs_long((unsigned long *) addr);
	ti.b = 0;
	if (ti.sign = (ti.a < 0))       // ���Ǹ�������������ʱ��������λ��
		ti.a = - ti.a;          // ��ʱ������β��������Ϊ�޷�������
	int_to_real(&ti,tmp);           // ����ʱ����ת������ʱʵ����ʽ��
}

// ȡ�û��ڴ��е�64λ��������ת������ʱʵ����ʽ��
// ���ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ��64λ������������Ч��ַ��math/ea.c����
// Ȼ����û���������ȡ��Ӧ����ֵ��������Ϊ��ʱ������ʽ������ٰ���ʱ����ֵת������ʱʵ����math/convert.c����
// ������tmp - ת������ʱʵ�����ָ�룻info - info�ṹָ�룻code - ָ����롣
void get_longlong_int(temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);                           // ȡָ���е���Ч��ֵַ��
	ti.a = get_fs_long((unsigned long *) addr);     // ȡ�û�64λ��������
	ti.b = get_fs_long(1 + (unsigned long *) addr);
	if (ti.sign = (ti.b < 0))                       // ���Ǹ�����������ʱ��������λ��
		__asm__("notl %0 ; notl %1\n\t"         // ͬʱȡ����1�ͽ�λ������
			"addl $1,%0 ; adcl $0,%1"
			:"=r" (ti.a),"=r" (ti.b)
			:"0" (ti.a),"1" (ti.b));
	int_to_real(&ti,tmp);                           // ����ʱ����ת������ʱʵ����ʽ��
}

// ��һ��64λ����������N����10��
// �������������BCD����ֵת������ʱʵ����ʽ�����С������ǣ�N<<1 + N<<3��
#define MUL10(low,high) \
__asm__("addl %0,%0 ; adcl %1,%1\n\t" \
"movl %0,%%ecx ; movl %1,%%ebx\n\t" \
"addl %0,%0 ; adcl %1,%1\n\t" \
"addl %0,%0 ; adcl %1,%1\n\t" \
"addl %%ecx,%0 ; adcl %%ebx,%1" \
:"=a" (low),"=d" (high) \
:"0" (low),"1" (high):"cx","bx")

// 64λ�ӷ���
// ��32λ���޷�����val�ӵ�64λ��<high,low>�С�
#define ADD64(val,low,high) \
__asm__("addl %4,%0 ; adcl $0,%1":"=r" (low),"=r" (high) \
:"0" (low),"1" (high),"r" ((unsigned long) (val)))

// ȡ�û��ڴ��е�BCD����ֵ��ת������ʱʵ����ʽ��
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ��BCD��������Ч��ַ��math/ea.c����
// Ȼ����û���������ȡ10�ֽ���ӦBCD��ֵ������1�ֽ����ڷ��ţ���ͬʱת������ʱ������ʽ��������ʱ����ֵת������ʱʵ����
// ������tmp - ת������ʱʵ�����ָ�룻info - info�ṹָ�룻code - ָ����롣
void get_BCD(temp_real * tmp, struct info * info, unsigned short code)
{
	int k;
	char * addr;
	temp_int i;
	unsigned char c;

// ȡ��BCD����ֵ�����ڴ���Ч��ַ��Ȼ������1��BCD���ֽڣ������Чλ����ʼ����
// ��ȡ��BCD����ֵ�ķ���λ����������ʱ�����ķ���λ��Ȼ���9�ֽڵ�BCD��ֵת������ʱ������ʽ��������ʱ����ֵת������ʱ
// ʵ����
	addr = ea(info,code);                   // ȡ��Ч��ַ��
	addr += 9;                              // ָ�����һ������10�����ֽڡ�
	i.sign = 0x80 & get_fs_byte(addr--);    // ȡ���з���λ��
	i.a = i.b = 0;
	for (k = 0; k < 9; k++) {               // ת������ʱ������ʽ��
		c = get_fs_byte(addr--);
		MUL10(i.a, i.b);
		ADD64((c>>4), i.a, i.b);
		MUL10(i.a, i.b);
		ADD64((c&0xf), i.a, i.b);
	}
	int_to_real(&i,tmp);                    // ת������ʱʵ����ʽ��
}

// ���������Զ̣������ȣ�ʵ����ʽ���浽�û��������С�
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ñ���������Ч��ַaddr��Ȼ�����ʱʵ
// ����ʽ�Ľ��ת���ɶ�ʵ����ʽ���洢����Ч��ַaddr����
// ������tmp - ��ʱʵ����ʽ���ֵ��info - info�ṹָ�룻code - ָ����롣
void put_short_real(const temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	short_real sr;

	addr = ea(info,code);                           // ȡ��Ч��ַ��
	verify_area(addr,4);                            // Ϊ��������֤������ڴ档
	temp_to_short(tmp,&sr);                         // ���ת���ɶ�ʵ����ʽ��
	put_fs_long(sr,(unsigned long *) addr);         // �洢���ݵ��û��ڴ�����
}

// ���������Գ���˫���ȣ�ʵ����ʽ���浽�û��������С�
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ñ���������Ч��ַaddr��Ȼ�����ʱ
// ʵ����ʽ�Ľ��ת���ɳ�ʵ����ʽ�����洢����Ч��ַaddr����
// ������tmp - ��ʱʵ����ʽ���ֵ��info - info�ṹָ�룻code - ָ����롣
void put_long_real(const temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	long_real lr;

	addr = ea(info,code);                           // ȡ��Ч��ַ��
	verify_area(addr,8);                            // Ϊ��������֤������ڴ档
	temp_to_long(tmp,&lr);                          // ���ת���ɳ�ʵ����ʽ��
	put_fs_long(lr.a, (unsigned long *) addr);      // �洢���ݵ��û��ڴ�����
	put_fs_long(lr.b, 1 + (unsigned long *) addr);
}

// ������������ʱʵ����ʽ���浽�û��������С�
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ñ���������Ч��ַaddr��Ȼ�����
// ʱʵ���洢����Ч��ַaddr����
// ������tmp - ��ʱʵ����ʽ���ֵ��info - info�ṹָ�룻code - ָ����롣
void put_temp_real(const temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;

	addr = ea(info,code);                           // ȡ��Ч��ַ��
	verify_area(addr,10);                           // Ϊ��������֤������ڴ档
	put_fs_long(tmp->a, (unsigned long *) addr);    // �洢���ݵ��û��ڴ�����
	put_fs_long(tmp->b, 1 + (unsigned long *) addr);
	put_fs_word(tmp->exponent, 4 + (short *) addr);
}

// ���������Զ�������ʽ���浽�û��������С�
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ñ���������Ч��ַaddr��Ȼ�����
// ʱʵ����ʽ�Ľ��ת������ʱ������ʽ������Ǹ�����������������λ�������������浽�û��ڴ��С�
// ������tmp - ��ʱʵ����ʽ���ֵ��info - info�ṹָ�룻code - ָ����롣
void put_short_int(const temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);           // ȡ��Ч��ַ��
	real_to_int(tmp,&ti);           // ת������ʱ������ʽ��
	verify_area(addr,2);            // ��֤�����洢�ڴ档
	if (ti.sign)                    // ���з���λ����ȡ����ֵ��
		ti.a = -ti.a;
	put_fs_word(ti.a,(short *) addr);       // �洢���û��������С�
}

// ���������Գ�������ʽ���浽�û��������С�
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ñ���������Ч��ַaddr��Ȼ�����ʱ
// ʵ����ʽ�Ľ��ת������ʱ������ʽ������Ǹ�����������������λ�������������浽�û��ڴ��С�
// ������tmp - ��ʱʵ����ʽ���ֵ��info - info�ṹָ�룻code - ָ����롣
void put_long_int(const temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);                           // ȡ��Ч��ֵַ��
	real_to_int(tmp,&ti);                           // ת������ʱ������ʽ��
	verify_area(addr,4);                            // ��֤�����洢�ڴ档
	if (ti.sign)                                    // ���з���λ����ȡ����ֵ��
		ti.a = -ti.a;
	put_fs_long(ti.a,(unsigned long *) addr);       // �洢���û��������С�
}

// ����������64λ������ʽ���浽�û��������С�
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ñ���������Ч��ַaddr��Ȼ�����ʱ
// ʵ����ʽ�Ľ��ת������ʱ������ʽ������Ǹ�����������������λ�������������浽�û��ڴ��С�
// ������tmp - ��ʱʵ����ʽ���ֵ��info - info�ṹָ�룻code - ָ����롣
void put_longlong_int(const temp_real * tmp,
	struct info * info, unsigned short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);           // ȡ��Ч��ַ��
	real_to_int(tmp,&ti);           // ת������ʱ������ʽ��
	verify_area(addr,8);            // ��֤�洢����
	if (ti.sign)                    // ���Ǹ�������ȡ����1��
		__asm__("notl %0 ; notl %1\n\t"
			"addl $1,%0 ; adcl $0,%1"
			:"=r" (ti.a),"=r" (ti.b)
			:"0" (ti.a),"1" (ti.b));
	put_fs_long(ti.a,(unsigned long *) addr);       // �洢���û��������С�
	put_fs_long(ti.b,1 + (unsigned long *) addr);
}

// �޷�����<high, low>����10����������rem�С�
#define DIV10(low,high,rem) \
__asm__("divl %6 ; xchgl %1,%2 ; divl %6" \
	:"=d" (rem),"=a" (low),"=b" (high) \
	:"0" (0),"1" (high),"2" (low),"c" (10))

// ����������BCD���ʽ���浽�û��������С�
// �ú������ȸ��ݸ���ָ�������Ѱַģʽ�ֽ��е����ݺ�info�ṹ�е�ǰ�Ĵ����е����ݣ�ȡ�ñ���������Ч��ַaddr������֤��
// ��10�ֽ�BCD����û��ռ䡣Ȼ�����ʱʵ����ʽ�Ľ��ת����BCD���ʽ�����ݲ����浽�û��ڴ��С�����Ǹ�����������ߴ洢��
// �ڵ������Чλ��
// ������tmp - ��ʱʵ����ʽ���ֵ��info - info�ṹָ�룻code - ָ����롣
void put_BCD(const temp_real * tmp,struct info * info, unsigned short code)
{
	int k,rem;
	char * addr;
	temp_int i;
	unsigned char c;

	addr = ea(info,code);                   // ȡ��Ч��ַ��
	verify_area(addr,10);                   // ��֤�洢�ռ�������
	real_to_int(tmp,&i);                    // ת������ʱ������ʽ��
	if (i.sign)                             // ���Ǹ����������÷����ֽ������Чλ��
		put_fs_byte(0x80, addr+9);
	else                                    // ��������ֽ�����Ϊ0��
		put_fs_byte(0, addr+9);
	for (k = 0; k < 9; k++) {               // ��ʱ����ת����BCD�벢���档
		DIV10(i.a,i.b,rem);
		c = rem;
		DIV10(i.a,i.b,rem);
		c += rem<<4;
		put_fs_byte(c,addr++);
	}
}