#ifndef _HEAD_H
#define _HEAD_H

typedef struct desc_struct {		// �����˶������������ݽṹ.�ýṹ��˵��ÿ������������8���ֽڹ���,ÿ������������256��.
	unsigned long a,b;
} desc_table[256];

extern unsigned long pg_dir[1024];	// �ڴ�ҳĿ¼����.ÿ��Ŀ¼��Ϊ4�ֽ�.�������ַ0��ʼ.
extern desc_table idt,gdt;		    // �ж���������,ȫ����������.

#define GDT_NUL 0			// ȫ����������ĵ�0 ,����
#define GDT_CODE 1			// ��1��,���ں˴������������.
#define GDT_DATA 2			// ��2��,���ں����ݶ���������.
#define GDT_TMP 3			// ��3��,ϵͳ��������,Linuxû��ʹ��.

#define LDT_NUL 0			// ÿ���ֲ���������ĵ�0��,����.
#define LDT_CODE 1			// ��1��,���û�����������������.
#define LDT_DATA 2			// ��2��,���û��������ݶ���������.

#endif

