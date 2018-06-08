// Linux0.12 ��ӡ��Ϣ���
#include <stdarg.h>

#include <linux/kernel.h>

// ����vsprintf()������linux/kernel/vsprintf.c��
extern int vsprintf(char * buf, const char * fmt, va_list args);

static char log_buf[1024];                       // ��ʾ����ʱ��������

static unsigned short cur_log_level = LOG_INFO_TYPE;

// linux0.12 kernel log function
void Log(unsigned short log_level, const char *fmt, ...)
{
    if (log_level >= cur_log_level) {
        va_list args;                           // va_listʵ������һ���ַ�ָ������.

        // ���в�������ʼ����.Ȼ��ʹ�ø�ʽ��fmt�������б�args�����buf��.����ֵi��������ַ����ĳ���.�����в��������������.�����ÿ���̨��ʾ
        // ������������ʾ�ַ���.
        va_start(args, fmt);
        vsprintf(log_buf, fmt, args);
        va_end(args);
        console_print(log_buf);                 // chr_drv/console.c
    }
}

