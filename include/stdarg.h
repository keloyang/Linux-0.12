// stdarg.h�Ǳ�׼����ͷ�ļ�.���Ժ����ʽ������������б�.

#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;                  // ����va_list��һ���ַ�ָ������.

/* Amount of space required in an argument list for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used.  */
/* �������������ΪTYPE��arg�����б���Ҫ��Ŀռ�����.TYPEҲ������ʹ�ø����͵�һ�����ʽ*/

// ������䶨����ȡ�����TYPE���͵��ֽڳ���ֵ.��int����(4)�ı���.
#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

// ����������ʼ��ָ��AP,ʹ��ָ�򴫸������Ŀɱ������ĵ�һ������.
// �ڵ�һ�ε���va_arg��va_endǰ,�������ȵ���va_start��.����LASTARG�Ǻ������������ұ߲����ı�ʶ��,��'...'��ߵ�һ�ϱ�ʶ��.
// AP�ǿɱ���������ָ��,LASTARG�����һ��ָ������.&(LASTARG)����ȡ���ַ(����ָ��),���Ҹ�ָ�����ַ�����.����LASTARG�Ŀ��
// ֵ��AP���ǿɱ�������е�һ��������ָ��.�ú�û�з���ֵ.
// ����__builtin_saveregs()����gcc�Ŀ����libgcc2.c�ж����,���ڱ���Ĵ���.
#ifndef __sparc__
#define va_start(AP, LASTARG) 						\
 (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#else
#define va_start(AP, LASTARG) 						\
 (__builtin_saveregs (),						\
  AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))
#endif

// ����ú����ڱ����ú������һ����������.va_end�����޸�APʹ�������µ���va_start֮ǰ���ܱ�ʹ��.va_end������va_arg�������в���
// ���ٱ�����.
void va_end (va_list);		/* Defined in gnulib */         /* ��gnulib�ж���. */
#define va_end(AP)

// ����ĺ�������չ���ʽ,ʹ������һ�������ݲ���������ͬ�����ͺ�ֵ.
// ����ȱʡֵ,va_arg�������ַ�,�޷����ַ��͸�������.�ڵ�һ��ʹ��va_argʱ,�����ر��еĵ�һ������,������ÿ�ε��ö������ر��е���һ��
// ����.����ͨ���ȷ���AP,Ȼ��������ֵ��ָ����һ����ʵ�ֵ�.va_argʹ��TYPE����ɷ��ʺͶ�λ��һ��,ÿ����һ��va_arg,�����޸�AP��ָʾ
// ���е���һ����.
#define va_arg(AP, TYPE)						\
 (AP += __va_rounded_size (TYPE),					\
  *((TYPE *) (AP - __va_rounded_size (TYPE))))

#endif /* _STDARG_H */

