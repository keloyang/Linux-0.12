/*
 * linux/include/linux/math_emu.h
 *
 * (C) 1991 Linus Torvalds
 */
#ifndef _LINUX_MATH_EMU_H
#define _LINUX_MATH_EMU_H

#include <linux/sched.h>

struct info {
	long ___math_ret;               // math_emulate()�����ߣ�int 7�����ص�ַ��
	long ___orig_eip;               // ��ʱ����ԭEIP�ĵط���
	long ___edi;                    // �쳣�ж�int 7���������ջ�ļĴ�����
	long ___esi;
	long ___ebp;
	long ___sys_call_ret;           // �ж�7����ʱ��ȥִ��ϵͳ���õķ��ش�����롣
	long ___eax;                    // ���²��֣�18--30�У���ϵͳ����ʱջ�нṹ��ͬ��
	long ___ebx;
	long ___ecx;
	long ___edx;
	long ___orig_eax;               // �粻��ϵͳ���ö��������ж�ʱ����ֵΪ-1��
	long ___fs;
	long ___es;
	long ___ds;
	long ___eip;                    // 26--30����CPU�Զ���ջ��
	long ___cs;
	long ___eflags;
	long ___esp;
	long ___ss;
};

#define EAX (info->___eax)
#define EBX (info->___ebx)
#define ECX (info->___ecx)
#define EDX (info->___edx)
#define ESI (info->___esi)
#define EDI (info->___edi)
#define EBP (info->___ebp)
#define ESP (info->___esp)
#define EIP (info->___eip)
#define ORIG_EIP (info->___orig_eip)
#define EFLAGS (info->___eflags)
#define DS (*(unsigned short *) &(info->___ds))
#define ES (*(unsigned short *) &(info->___es))
#define FS (*(unsigned short *) &(info->___fs))
#define CS (*(unsigned short *) &(info->___cs))
#define SS (*(unsigned short *) &(info->___ss))

void __math_abort(struct info *, unsigned int);

#define math_abort(x,y) \
(((void (*)(struct info *,unsigned int)) __math_abort)((x),(y)))

/*
 * Gcc forces this stupid alignment problem: I want to use only two longs
 * for the temporary real 64-bit mantissa, but then gcc aligns out the
 * structure to 12 bytes which breaks things in math_emulate.c. Shit. I
 * want some kind of "no-alignt" pragma or something.
 */
/*
 * Gccǿ�ƶ����������޴����������ʹ������longȥ��ʾ��ʱʵ��64λ����Ч��������
 */
// ��ʱʵ���ṹ
typedef struct {
	long a,b;               // ��Ч����
	short exponent;         // ָ����
} temp_real;

// ��ʱʵ���ṹ�������ж��룩
typedef struct {
	short m0,m1,m2,m3;      // ��Ч����
	short exponent;         // ָ����
} temp_real_unaligned;

#define real_to_real(a,b) \
((*(long long *) (b) = *(long long *) (a)),((b)->exponent = (a)->exponent))

// ��ʵ�ͽṹ��
typedef struct {
	long a,b;
} long_real;

typedef long short_real;        // ��ʵ�͡�

typedef struct {
	long a,b;
	short sign;
} temp_int;                     // ��ʱ���͡�

// Э������״̬�����ݽṹ��
struct swd {
	int ie:1;               // ��Ч������
	int de:1;               // �ǹ�񻯡�
	int ze:1;               // ���㡣
	int oe:1;               // �������
	int ue:1;               // �������
	int pe:1;               // ���ȡ�
	int sf:1;               // ջ�����־��
	int ir:1;               // 
	int c0:1;               // ����λ��
	int c1:1;
	int c2:1;
	int top:3;              // ����ָ����ǰ�ĸ�80λ�Ĵ���λ��ջ����
	int c3:1;
	int b:1;                // æ��
};

#define I387 (current->tss.i387)        // ��ǰ����ṹ�б������ѧЭ�������ṹ��
#define SWD (*(struct swd *) &I387.swd) // Э������״̬�֡�
#define ROUNDING ((I387.cwd >> 10) & 3) // ȡ����������������ֶΡ�
#define PRECISION ((I387.cwd >> 8) & 3) // ȡ�������о��ȿ����ֶΡ�

#define BITS24	0
#define BITS53	2
#define BITS64	3

#define ROUND_NEAREST	0               // ���뵽�����ż����
#define ROUND_DOWN	1               // �������ޡ�
#define ROUND_UP	2               // ���������ޡ�
#define ROUND_0		3               // ��Ч��

#define CONSTZ   (temp_real_unaligned) {0x0000,0x0000,0x0000,0x0000,0x0000}     // ����0.0��
#define CONST1   (temp_real_unaligned) {0x0000,0x0000,0x0000,0x8000,0x3FFF}     // ��ʱʵ��1.0��
#define CONSTPI  (temp_real_unaligned) {0xC235,0x2168,0xDAA2,0xC90F,0x4000}     // ����Pi��
#define CONSTLN2 (temp_real_unaligned) {0x79AC,0xD1CF,0x17F7,0xB172,0x3FFE}     // ����Loge(2)��
#define CONSTLG2 (temp_real_unaligned) {0xF799,0xFBCF,0x9A84,0x9A20,0x3FFD}     // ����Log10(2)��
#define CONSTL2E (temp_real_unaligned) {0xF0BC,0x5C17,0x3B29,0xB8AA,0x3FFF}     // ����Log2(e)��
#define CONSTL2T (temp_real_unaligned) {0x8AFE,0xCD1B,0x784B,0xD49A,0x4000}     // ����Log2(10)��

// ��״̬�ֿ��������쳣λ��
#define set_IE() (I387.swd |= 1)        // ��Ч������
#define set_DE() (I387.swd |= 2)        // �ǹ�񻯡�
#define set_ZE() (I387.swd |= 4)        // ���㡣
#define set_OE() (I387.swd |= 8)        // �������
#define set_UE() (I387.swd |= 16)       // �������
#define set_PE() (I387.swd |= 32)       // ���ȡ�

// ����궨����������״̬����������־λC0��C1��C2��C3��
#define set_C0() (I387.swd |= 0x0100)
#define set_C1() (I387.swd |= 0x0200)
#define set_C2() (I387.swd |= 0x0400)
#define set_C3() (I387.swd |= 0x4000)
/* ea.c */

char * ea(struct info * __info, unsigned short __code);

/* convert.c */

void short_to_temp(const short_real * __a, temp_real * __b);
void long_to_temp(const long_real * __a, temp_real * __b);
void temp_to_short(const temp_real * __a, short_real * __b);
void temp_to_long(const temp_real * __a, long_real * __b);
void real_to_int(const temp_real * __a, temp_int * __b);
void int_to_real(const temp_int * __a, temp_real * __b);

/* get_put.c */

void get_short_real(temp_real *, struct info *, unsigned short);
void get_long_real(temp_real *, struct info *, unsigned short);
void get_temp_real(temp_real *, struct info *, unsigned short);
void get_short_int(temp_real *, struct info *, unsigned short);
void get_long_int(temp_real *, struct info *, unsigned short);
void get_longlong_int(temp_real *, struct info *, unsigned short);
void get_BCD(temp_real *, struct info *, unsigned short);
void put_short_real(const temp_real *, struct info *, unsigned short);
void put_long_real(const temp_real *, struct info *, unsigned short);
void put_temp_real(const temp_real *, struct info *, unsigned short);
void put_short_int(const temp_real *, struct info *, unsigned short);
void put_long_int(const temp_real *, struct info *, unsigned short);
void put_longlong_int(const temp_real *, struct info *, unsigned short);
void put_BCD(const temp_real *, struct info *, unsigned short);

/* add.c */

void fadd(const temp_real *, const temp_real *, temp_real *);

/* mul.c */

void fmul(const temp_real *, const temp_real *, temp_real *);

/* div.c */

void fdiv(const temp_real *, const temp_real *, temp_real *);

/* compare.c */

void fcom(const temp_real *, const temp_real *);        // ���渡��ָ��FTST��
void fucom(const temp_real *, const temp_real *);
void ftst(const temp_real *);

#endif

