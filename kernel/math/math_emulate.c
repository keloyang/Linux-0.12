/*
 * linux/kernel/math/math_emulate.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * Limited emulation 27.12.91 - mostly loads/stores, which gcc wants
 * even for soft-float, unless you use bruce evans' patches. The patches
 * are great, but they have to be re-applied for every version, and the
 * library is different for soft-float and 80387. So emulation is more
 * practical, even though it's slower.
 *
 * 28.12.91 - loads/stores work, even BCD. I'll have to start thinking
 * about add/sub/mul/div. Urgel. I should find some good source, but I'll
 * just fake up something.
 *
 * 30.12.91 - add/sub/mul/div/com seem to work mostly. I should really
 * test every possible combination.
 */
/*
 * ���淶Χ���޵ĳ���91.12.27 -���������һЩ����/�洢ָ�������ʹ����Bruce Evans�Ĳ������򣬷���ʹʹ�����ִ�и���
 * ���㣬gccҲ��Ҫ��Щָ�Bruce�Ĳ�������ǳ��ã���ÿ�λ�gcc�汾�㶼��������������򡣶��Ҷ����������ʵ�ֺ�80387����
 * ʹ�õĿ��ǲ�ͬ�ġ����ʹ�÷����Ǹ�Ϊʵ�ʵķ��������ܷ��淽��������
 * 
 * 91.12.24 - ����/�洢Э������ָ��������ˣ���ʹ��BCD���Ҳ��ʹ�á��ҽ���ʼ����ʵ��add/sub/mul/divָ�������Ӧ����
 * һЩ�õ����ϣ����������һ��ȷ���һЩ������
 * 
 * 91.12.30 - add/sub/mul/div/com��Щָ�������������ʹ���ˡ�����Ӧ�ò���ÿ��ָ����ܵ���ϲ�����
 */

/*
 * This file is full of ugly macros etc: one problem was that gcc simply
 * didn't want to make the structures as they should be: it has to try to
 * align them. Sickening code, but at least I've hidden the ugly things
 * in this one file: the other files don't need to know about these things.
 *
 * The other files also don't care about ST(x) etc - they just get addresses
 * to 80-bit temporary reals, and do with them as they please. I wanted to
 * hide most of the 387-specific things here.
 */
/*
 * ��������е�������Щ��Ť�ĺ꣺����֮һ��gcc���ǲ���ѽṹ��������Ӧ�ó�Ϊ�����ӣ�gcc��ͼ�Խṹ���ж��봦���������ᣬ
 * �����������Ѿ����������ŵĴ��붼��������ôһ���ļ����ˣ����������ļ�����Ҫ�˽���Щ��Ϣ��
 * 
 * �����ĳ���Ҳ����Ҫ֪��ST(x)��80387�ڲ��ṹ - ����ֻ��Ҫ�õ�80λ��ʱʵ���ĵ�ַ�Ϳ���������������뾡����������������
 * ��387ר����Ϣ��
 */

#include <linux/math_emu.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#define bswapw(x) __asm__("xchgb %%al,%%ah":"=a" (x):"0" ((short)x))    // ����2�ֽ�λ�á�
#define ST(x) (*__st((x)))                              // ȡ�����ST(x)�ۼ�����ֵ��
#define PST(x) ((const temp_real *) __st((x)))          // ȡ�����ST(x)�ۼ�����ָ�롣

/*
 * We don't want these inlined - it gets too messy in the machine-code.
 */
static void fpop(void);
static void fpush(void);
static void fxchg(temp_real_unaligned * a, temp_real_unaligned * b);
static temp_real_unaligned * __st(int i);

// ִ�и���ָ����档
// �ú������ȼ������I387�ṹ״̬�ּĴ������Ƿ���δ���ε��쳣��־��λ���������״̬����æ��־B�������á�Ȼ���ָ��ָ��
// ������������ȡ������ָ��EIP����2�ֽڸ���ָ�����code�����ŷ�������code���������京����д�����Բ�ͬ��������ֵ��Linus
// ʹ���˼�����ͬ��switch�������з��洦��
// ������info�ṹָ�롣
static void do_emu(struct info * info)
{
	unsigned short code;
	temp_real tmp;
	char * address;

// �ú������ȼ������I387�ṹ״̬�ּĴ������Ƿ���δ���ε��쳣��־��λ�����о�����״̬����æ��־B��λ15��������λB��
// ־��Ȼ�����ǰ�ָ��ָ�뱣���������ٿ���ִ�б������Ĵ����ǲ����û����롣���ǲ��ǣ��������ߵĴ����ѡ���������0x0f����
// ˵���ں����д���ʹ���˸���ָ���������ʾ������ָ���CS��EIPֵ����Ϣ���ں�����Ҫ��ѧ���桱��ͣ����
	if (I387.cwd & I387.swd & 0x3f)
		I387.swd |= 0x8000;             // ����æ��־B��
	else
		I387.swd &= 0x7fff;             // ��æ��־B��
	ORIG_EIP = EIP;                         // ���渡��ָ��ָ�롣
/* 0x0007 means user code space */
	if (CS != 0x000F) {                     // �����û�������ͣ����
		printk("math_emulate: %04x:%08x\n\r",CS,EIP);
		panic("Math emulation needed in kernel");
	}
// Ȼ������ȡ������ָ��EIP����2�ֽڸ���ָ�����code������Inter CPU�洢����ʱ�ǡ�Сͷ����Little endian����ǰ�ģ���ʱȡ��
// �Ĵ���������ָ���1����2�ֽ�˳��ߵ������������Ҫ����һ��code�������ֽڵ�˳��Ȼ�������ε���1�������ֽ��е�ESCλ����
// ����11011�������ŰѸ���ָ��ָ��EIP���浽TSS��i387�ṹ�е�fip�ֶ��У���CS���浽fcs�ֶ��У�ͬʱ����΢������ĸ���ָ��
// ����code�ŵ�fcs�ֶεĸ�16λ�С�������Щֵ��Ϊ���ڳ��ַ���Ĵ������쳣ʱ���������ʹ����ʵ��Э������һ�����д������
// ��EIPָ�����ĸ���ָ����������
	code = get_fs_word((unsigned short *) EIP);     // ȡ2�ֽڵĸ���ָ����������
	bswapw(code);                                   // �����ߵ��ֽڡ�
	code &= 0x7ff;                                  // ���δ����е�ESC�롣
	I387.fip = EIP;                                 // ����ָ��ָ�롣
	*(unsigned short *) &I387.fcs = CS;             // ��������ѡ�����
	*(1+(unsigned short *) &I387.fcs) = code;       // ������롣
	EIP += 2;                                       // ָ��ָ��ָ����һ���ֽڡ�
        switch (code) {
		case 0x1d0: /* fnop */          // �ղ���ָ��FNOP
			return;
		case 0x1d1: case 0x1d2: case 0x1d3:     // ��Чָ����롣���źţ��˳���
		case 0x1d4: case 0x1d5: case 0x1d6: case 0x1d7:
			math_abort(info,1<<(SIGILL-1));
		case 0x1e0:                     // FCHS - �ı�ST����λ����ST = -ST��
			ST(0).exponent ^= 0x8000;
			return;
		case 0x1e1:                     // FABS - ȡ����ֵ����ST = |ST|��
			ST(0).exponent &= 0x7fff;
			return;
		case 0x1e2: case 0x1e3:         // ��Чָ����롣���źţ��˳���
			math_abort(info,1<<(SIGILL-1));
		case 0x1e4:                     // FTST - ����TS��ͬʱ����״̬����Cn��
			ftst(PST(0));
			return;
		case 0x1e5:                     // FXAM - ���TSֵ��ͬʱ�޸�״̬����Cn��
			printk("fxam not implemented\n\r");     // δʵ�֡����ź��˳���
			math_abort(info,1<<(SIGILL-1));
		case 0x1e6: case 0x1e7:         // ��Чָ����롣���źţ��˳���
			math_abort(info,1<<(SIGILL-1));
		case 0x1e8:                     // FLD1 - ���س���1.0���ۼ���ST��
			fpush();
			ST(0) = CONST1;
			return;
		case 0x1e9:                     // FLDL2T - ���س���Log2(10)���ۼ���ST��
			fpush();
			ST(0) = CONSTL2T;
			return;
		case 0x1ea:                     // FLDL2E - ���س���Log2(e)���ۼ���ST��
			fpush();
			ST(0) = CONSTL2E;
			return;
		case 0x1eb:                     // FLDPI - ���س���Pi���ۼ���ST��
			fpush();
			ST(0) = CONSTPI;
			return;
		case 0x1ec:                     // FLDLG2 - ���س���Log10(2)���ۼ���ST��
			fpush();
			ST(0) = CONSTLG2;
			return;
		case 0x1ed:                     // FLDLN2 - ���س���Loge(2)���ۼ���ST��
			fpush();
			ST(0) = CONSTLN2;
			return;
		case 0x1ee:                     // FLDZ - ���س���0.0���ۼ���ST��
			fpush();
			ST(0) = CONSTZ;
			return;
		case 0x1ef:                     // ��Ч��δʵ�ַ���ָ����롣���źţ��˳���
			math_abort(info,1<<(SIGILL-1));
		case 0x1f0: case 0x1f1: case 0x1f2: case 0x1f3:
		case 0x1f4: case 0x1f5: case 0x1f6: case 0x1f7:
		case 0x1f8: case 0x1f9: case 0x1fa: case 0x1fb:
		case 0x1fc: case 0x1fd: case 0x1fe: case 0x1ff:
			printk("%04x fxxx not implemented\n\r",code + 0xc800);
			math_abort(info,1<<(SIGILL-1));
		case 0x2e9:                     // FUCOMPP - �޴���Ƚϡ�
			fucom(PST(1),PST(0));
			fpop(); fpop();
			return;
		case 0x3d0: case 0x3d1:         // FNOP - ��387������Ӧ����0x3e0��0x3e1��
			return;
		case 0x3e2:                     // FCLEX - ��״̬�����쳣��־��
			I387.swd &= 0x7f00;
			return;
		case 0x3e3:                     // FINIT - ��ʼ��Э��������
			I387.cwd = 0x037f;
			I387.swd = 0x0000;
			I387.twd = 0x0000;
			return;
		case 0x3e4:                     // FNOP - ��80387��
			return;
		case 0x6d9:                     // FCOMPP - ST(i)��ST�Ƚϣ���ջ�������Ρ�
			fcom(PST(1),PST(0));
			fpop(); fpop();
			return;
		case 0x7e0:                     // FSTSW AX - ���浱ǰ״̬�ֵ�AX�Ĵ����С�
			*(short *) &EAX = I387.swd;
			return;
	}
// ���濪ʼ�����2�ֽ����3λ��REG��ָ���11011,XXXXXXXX��REG��ʽ�Ĵ��롣
	switch (code >> 3) {
		case 0x18:                      // FADD ST, ST(i)��
			fadd(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x19:                      // FMUL ST, ST(i)��
			fmul(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x1a:                      // FCOM ST(i)��
			fcom(PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x1b:                      // FCOMP ST(i)��
			fcom(PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(0));
			fpop();
			return;
		case 0x1c:                      // FSUB ST, ST(i)��
			real_to_real(&ST(code & 7),&tmp);
			tmp.exponent ^= 0x8000;
			fadd(PST(0),&tmp,&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x1d:                      // FSUBR ST, ST(i)��
			ST(0).exponent ^= 0x8000;
			fadd(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x1e:                      // FDIV ST, ST(i)��
			fdiv(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x1f:                      // FDIVR ST, ST(i)��
			fdiv(PST(code & 7),PST(0),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x38:                      // FLD ST(i)��
			fpush();
			ST(0) = ST((code & 7)+1);
			return;
		case 0x39:                      // FXCH ST(i)��
			fxchg(&ST(0),&ST(code & 7));
			return;
		case 0x3b:                      // FSTP ST(i)��
			ST(code & 7) = ST(0);
			fpop();
			return;
		case 0x98:                      // FADD ST(i)�� ST��
			fadd(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			return;
		case 0x99:                      // FMUL ST(i)�� ST��
			fmul(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			return;
		case 0x9a:                      // FCOM ST(i)��
			fcom(PST(code & 7),PST(0));
			return;
		case 0x9b:                      // FCOMP ST(i)��
			fcom(PST(code & 7),PST(0));
			fpop();
			return;			
		case 0x9c:                      // FSUBR ST(i), ST��
			ST(code & 7).exponent ^= 0x8000;
			fadd(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			return;
		case 0x9d:                      // FSUB ST(i), ST��
			real_to_real(&ST(0),&tmp);
			tmp.exponent ^= 0x8000;
			fadd(PST(code & 7),&tmp,&tmp);
			real_to_real(&tmp,&ST(code & 7));
			return;
		case 0x9e:                      // FDIVR ST(i), ST��
			fdiv(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			return;
		case 0x9f:                      // FDIV ST(i), ST��
			fdiv(PST(code & 7),PST(0),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			return;
		case 0xb8:                      // FFREE ST(i), ST��δʵ�֡�
			printk("ffree not implemented\n\r");
			math_abort(info,1<<(SIGILL-1));
		case 0xb9:                      // FXCH ST(i)��
			fxchg(&ST(0),&ST(code & 7));
			return;
		case 0xba:                      // FST ST(i)��
			ST(code & 7) = ST(0);
			return;
		case 0xbb:                      // FSTP ST(i)��
			ST(code & 7) = ST(0);
			fpop();
			return;
		case 0xbc:                      // FUCOM ST(i)��
			fucom(PST(code & 7),PST(0));
			return;
		case 0xbd:                      // FUCOMP ST(i)��
			fucom(PST(code & 7),PST(0));
			fpop();
			return;
		case 0xd8:                      // FADDP ST(i), ST��
			fadd(PST(code & 7),PST(0),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			fpop();
			return;
		case 0xd9:                      // FMULP ST(i), ST��
			fmul(PST(code & 7),PST(0),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			fpop();
			return;
		case 0xda:                      // FCOMP ST(i)��
			fcom(PST(code & 7),PST(0));
			fpop();
			return;
		case 0xdc:                      // FSUBRP ST(i)��
			ST(code & 7).exponent ^= 0x8000;
			fadd(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			fpop();
			return;
		case 0xdd:                      // FSUBP ST(I), ST��
			real_to_real(&ST(0),&tmp);
			tmp.exponent ^= 0x8000;
			fadd(PST(code & 7),&tmp,&tmp);
			real_to_real(&tmp,&ST(code & 7));
			fpop();
			return;
		case 0xde:                      // FDIVRP ST(i), ST��
			fdiv(PST(0),PST(code & 7),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			fpop();
			return;
		case 0xdf:                      // FDIVP ST(i), ST��
			fdiv(PST(code & 7),PST(0),&tmp);
			real_to_real(&tmp,&ST(code & 7));
			fpop();
			return;
		case 0xf8:                      // FFREE ST(i)��δʵ�֡�
			printk("ffree not implemented\n\r");
			math_abort(info,1<<(SIGILL-1));
			fpop();
			return;
		case 0xf9:                      // FXCH ST(i)��
			fxchg(&ST(0),&ST(code & 7));
			return;
		case 0xfa:                      // FSTP ST(i)��
		case 0xfb:                      // FSTP ST(i)��
			ST(code & 7) = ST(0);
			fpop();
			return;
	}
// �����2���ֽ�λ7--6��MOD��λ2--0��R/M��ָ���11011,XXX��MOD��XXX��R/M��ʽ�Ĵ��롣MOD�ڸ��ӳ����д��������������
// �ô�������0xe7��0b11100111�����ε�MOD��
	switch ((code>>3) & 0xe7) {
		case 0x22:
//			put_short_real(PST(0),info,code);
                    panic("kernel/math/math_emulate.c->do_emu(),394");
			return;
		case 0x23:                      // FSTP - ���浥����ʵ������ʵ������
			put_short_real(PST(0),info,code);
			fpop();
			return;
		case 0x24:                      // FLDENV - ����Э������״̬�Ϳ��ƼĴ����ȡ�
			address = ea(info,code);
			for (code = 0 ; code < 7 ; code++) {
				((long *) & I387)[code] =
				   get_fs_long((unsigned long *) address);
				address += 4;
			}
			return;
		case 0x25:                      // FLDCW - ���ؿ����֡�
			address = ea(info,code);
			*(unsigned short *) &I387.cwd =
				get_fs_word((unsigned short *) address);
			return;
		case 0x26:                      // FSTENV - ����Э������״̬�Ϳ��ƼĴ����ȡ�
			address = ea(info,code);
			verify_area(address,28);
			for (code = 0 ; code < 7 ; code++) {
				put_fs_long( ((long *) & I387)[code],
					(unsigned long *) address);
				address += 4;
			}
			return;
		case 0x27:                      // FSTCW - �洢�����֡�
			address = ea(info,code);
			verify_area(address,2);
			put_fs_word(I387.cwd,(short *) address);
			return;
		case 0x62:                      // FIST - �洢����������
			put_long_int(PST(0),info,code);
			return;
		case 0x63:                      // FISTP - �洢����������
			put_long_int(PST(0),info,code);
			fpop();
			return;
		case 0x65:                      // FLD - ������չ����ʱ��ʵ����
			fpush();
			get_temp_real(&tmp,info,code);
			real_to_real(&tmp,&ST(0));
			return;
		case 0x67:                      // FSTP - ������չʵ����
			put_temp_real(PST(0),info,code);
			fpop();
			return;
		case 0xa2:                      // FST - ����˫����ʵ����
			put_long_real(PST(0),info,code);
			return;
		case 0xa3:                      // FSTP - �洢˫����ʵ����
			put_long_real(PST(0),info,code);
			fpop();
			return;
		case 0xa4:                      // FRSTOR - �ָ�����108�ֽڵļĴ������ݡ�
			address = ea(info,code);
			for (code = 0 ; code < 27 ; code++) {
				((long *) & I387)[code] =
				   get_fs_long((unsigned long *) address);
				address += 4;
			}
			return;
		case 0xa6:                      // FSAVE - ��������108�ֽڼĴ������ݡ�
			address = ea(info,code);
			verify_area(address,108);
			for (code = 0 ; code < 27 ; code++) {
				put_fs_long( ((long *) & I387)[code],
					(unsigned long *) address);
				address += 4;
			}
			I387.cwd = 0x037f;
			I387.swd = 0x0000;
			I387.twd = 0x0000;
			return;
		case 0xa7:                      // FSTSW - ����״̬״̬�֡�
			address = ea(info,code);
			verify_area(address,2);
			put_fs_word(I387.swd,(short *) address);
			return;
		case 0xe2:                      // FIST - �������������
			put_short_int(PST(0),info,code);
			return;
		case 0xe3:                      // FISTP - �������������
			put_short_int(PST(0),info,code);
			fpop();
			return;
		case 0xe4:                      // FBLD - ����BCD��������
			fpush();
			get_BCD(&tmp,info,code);
			real_to_real(&tmp,&ST(0));
			return;
		case 0xe5:                      // FILD - ���س���������
			fpush();
			get_longlong_int(&tmp,info,code);
			real_to_real(&tmp,&ST(0));
			return;
		case 0xe6:                      // FBSTP - ����BCD��������
			put_BCD(PST(0),info,code);
			fpop();
			return;
		case 0xe7:                      // BISTP - ���泤��������
			put_longlong_int(PST(0),info,code);
			fpop();
			return;
	}
// ���洦���2�ม��ָ����ȸ���ָ������λ10--9��MFֵȡָ�����͵�����Ȼ�����OPA��OPB�����ֵ���зֱ���������11011��
// MF��000,XXX��R/M��ʽ��ָ����롣
	switch (code >> 9) {
		case 0:                 // MF = 00����ʵ����32λ��������
			get_short_real(&tmp,info,code);
			break;
		case 1:                 // MF = 01����������32λ��������
			get_long_int(&tmp,info,code);
			break;
		case 2:                 // MF = 10����ʵ����64λʵ������
			get_long_real(&tmp,info,code);
			break;
		case 4:                 // MF = 11����������64λ��������Ӧ����case 3��
			get_short_int(&tmp,info,code);
	}
	switch ((code>>3) & 0x27) {
		case 0:                 // FADD��
			fadd(&tmp,PST(0),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 1:                 // FMUL��
			fmul(&tmp,PST(0),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 2:                 // FCOM��
			fcom(&tmp,PST(0));
			return;
		case 3:                 // FCOMP��
			fcom(&tmp,PST(0));
			fpop();
			return;
		case 4:                 // FSUB��
			tmp.exponent ^= 0x8000;
			fadd(&tmp,PST(0),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 5:                 // FSUBR��
			ST(0).exponent ^= 0x8000;
			fadd(&tmp,PST(0),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 6:                 // FDIV��
			fdiv(PST(0),&tmp,&tmp);
			real_to_real(&tmp,&ST(0));
			return;
		case 7:                 // FDIVR��
			fdiv(&tmp,PST(0),&tmp);
			real_to_real(&tmp,&ST(0));
			return;
	}
// ��������11011,XX,1,XX��000,R/M��ָ����롣
	if ((code & 0x138) == 0x100) {
			fpush();
			real_to_real(&tmp,&ST(0));
			return;
	}
// �����Ϊ��Чָ�
	printk("Unknown math-insns: %04x:%08x %04x\n\r",CS,EIP,code);
	math_abort(info,1<<(SIGFPE-1));
}

// CPU�쳣�ж�int 7���õ�80387����ӿں�����
// ����ǰ����û��ʹ�ù�Э��������������ʹ��Э��������־used_math��Ȼ���ʼ��80387�Ŀ����֡�״̬�ֺ������֡����ʹ���ж�
// int 7���ñ������ķ��ص�ַָ����Ϊ�������ø���ָ�����������do_emu()��
// ����____false��_orig_eip��
void math_emulate(long ___false)
{
	if (!current->used_math) {
		current->used_math = 1;
		I387.cwd = 0x037f;
		I387.swd = 0x0000;
		I387.twd = 0x0000;
	}
/* &___false points to info->___orig_eip, so subtract 1 to get info */
	do_emu((struct info *) ((&___false) - 1));
}

// ��ֹ���������
// ��������Чָ��������δʵ�ֵ�ָ�����ʱ���ú������Ȼָ������ԭEIP��������ָ���źŸ���ǰ���̡����ջָ��ָ���ж�
// int 7������̵��ñ������ķ��ص�ַ��ֱ�ӷ��ص��жϴ�������С�
void __math_abort(struct info * info, unsigned int signal)
{
	EIP = ORIG_EIP;
	current->signal |= signal;
	__asm__("movl %0,%%esp ; ret"::"g" ((long) info));
}

// �ۼ���ջ����������
// ��״̬��TOP�ֶ�ֵ��1������7ȡģ��
static void fpop(void)
{
	unsigned long tmp;

	tmp = I387.swd & 0xffffc7ff;
	I387.swd += 0x00000800;
	I387.swd &= 0x00003800;
	I387.swd |= tmp;
}

// �ۼ���ջ��ջ������
// ��״̬��TOP�ֶμ�1������7��������7ȡģ��
static void fpush(void)
{
	unsigned long tmp;

	tmp = I387.swd & 0xffffc7ff;
	I387.swd += 0x00003800;
	I387.swd &= 0x00003800;
	I387.swd |= tmp;
}

// ���������ۼ�����ֵ��
static void fxchg(temp_real_unaligned * a, temp_real_unaligned * b)
{
	temp_real_unaligned c;

	c = *a;
	*a = *b;
	*b = c;
}

// ȡST(i)���ڴ�ָ�롣
// ȡ״̬����TOP�ֶ�ֵ������ָ�����������ݼĴ����Ų�ȡģ����󷵻�ST(i)��Ӧ��ָ�롣
static temp_real_unaligned * __st(int i)
{
	i += I387.swd >> 11;            // ȡ״̬����TOP�ֶ�ֵ��
	i &= 7;
	return (temp_real_unaligned *) (i*10 + (char *)(I387.st_space));
}