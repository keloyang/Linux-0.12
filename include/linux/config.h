/*
 * �ں�����ͷ�ļ�.����ʹ�õļ����������ͺ�Ӳ������(HD_TYPE)��ѡ��.
 */
#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * Defines for what uname() should return
 */
/*
 * ����uname()����Ӧ�÷��ص�ֵ.
 */
#define UTS_SYSNAME "Linux"
#define UTS_NODENAME "(none)"	              /* set by sethostname() */
#define UTS_RELEASE "0"		                  /* patchlevel */
#define UTS_VERSION "0.12"
#define UTS_MACHINE "i386"	                  /* hardware type */

/* Don't touch these, unless you really know what your doing. */
/* �벻Ҫ�����޸����涨��ֵ��������֪���Լ����ڸ�ʲô�� */
#define DEF_INITSEG	0x9000	                  /* �����������򽫱��ƶ����Ķ�ֵ	*/
#define DEF_SYSSEG	0x1000	                  /* �������������ϵͳģ����ص��ڴ�Ķ�ֵ.	*/
#define DEF_SETUPSEG	0x9020	              /* setup���������ڴ��λ��.	*/
#define DEF_SYSSIZE	0x3000	                  /* �ں�ϵͳģ��Ĭ��������(16�ֽ�=1��)	*/

/*
 * The root-device is no longer hard-coded. You can change the default
 * root-device by changing the line ROOT_DEV = XXX in boot/bootsect.s
 */
/*
 * ���ļ�ϵͳ�豸�Ѳ�����Ӳ�������.ͨ���޸�boot/bootsect.s�ļ�����ROOT_DEV=XXX,����Ըı���豸��Ĭ������ֵ.
 */

/*
 * The keyboard is now defined in kernel/chr_dev/keyboard.S
 */
/*
 * ���ڼ������ͱ�����kernel/chr_dev/keyboard.S�����ж���.
 */

/*
 * Normally, Linux can get the drive parameters from the BIOS at
 * startup, but if this for some unfathomable reason fails, you'd
 * be left stranded. For this case, you can define HD_TYPE, which
 * contains all necessary info on your harddisk.
 *
 * The HD_TYPE macro should look like this:
 *
 * #define HD_TYPE { head, sect, cyl, wpcom, lzone, ctl}
 *
 * In case of two harddisks, the info should be sepatated by
 * commas:
 *
 * #define HD_TYPE { h,s,c,wpcom,lz,ctl },{ h,s,c,wpcom,lz,ctl }
 */
/*
 * ͨ��,Linux�ܹ�������ʱ��BIOS�л�ȡ�������Ĳ���,����������δ֪ԭ���û�еõ���Щ����ʱ,��ʹ���������޲�.�����������,
 * ����Զ���HD_TYPE,���а���Ӳ�̵�������Ϣ.
 *
 * HD_TYPE��Ӧ����������������ʽ:
 *
 * #define HD_TYPE { head, sect, cyl, wpcom, lzone, ctl}
 *
 * ����������Ӳ�̵����,������Ϣ���ö��ŷֿ�:
 *
 * #define HD_TYPE { h,s,c,wpcom,lz,ctl },{ h,s,c,wpcom,lz,ctl }
 */
/*
 This is an example, two drives, first is type 2, second is type 3:

#define HD_TYPE { 4,17,615,300,615,8 }, { 6,17,615,300,615,0 }

 NOTE: ctl is 0 for all drives with heads<=8, and ctl=8 for drives
 with more than 8 heads.

 If you want the BIOS to tell what kind of drive you have, just
 leave HD_TYPE undefined. This is the normal thing to do.
*/
/*
 * ������һ������,����Ӳ��,��1��������2,��2��������3:
 *
 * #define HD_TYPE { 4,17,615,300,615,8 }, { 6,17,615,300,615,0 }
 *
 * ע:��Ӧ����Ӳ��,�����ͷ��<=8,��ctl����0,����ͷ������8��,��ctl=8.
 *
 * ���������BIOS����Ӳ�̵�����,��ôֻ�費����HD_TYPE.����Ĭ�ϲ���.
 */
#endif

