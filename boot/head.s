/*
 *  linux/boot/head.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  head.s contains the 32-bit startup code.
 *
 * NOTE!!! Startup happens at absolute address 0x00000000, which is also where
 * the page directory will exist. The startup code will be overwritten by
 * the page directory.
 */
/*
 *
 * head.s����32λ�������롣
 *
 * ע��!!! 32λ���������ǴӾ��Ե�ַ0x00000000��ʼ��,����Ҳͬ����ҳĿ¼�����ڵĵط�,���������������뽫��ҳĿ¼���ǵ�.
 *
 */
.text
.globl idt,gdt,pg_dir,tmp_floppy_area
pg_dir:# ҳĿ¼������������.
 # �ٴ�ע��!!!�����Ѿ�����32λ����ģʽ,��������$0x10�����ǰѵ�ַ0x10װ������μĴ���,��������ʵ��ȫ�ֶ����������е�ƫ��ֵ,���߸�׼ȷ
 # ��˵��һ�������������ѡ���.����$0x10�ĺ�����������Ȩ��0(λ0-1=0),ѡ��ȫ����������(λ2=0),ѡ����е�2��(λ3-15=2).������ָ����е�
 # ���ݶ���������.
 # ����Ĵ��뺬����:����ds,es,fs,gsΪsetup.s�й�������ݶ�(ȫ�ֶ����������2��)��ѡ���=0x10,������ջ������stack_startָ���user_stack
 # ������,Ȼ��ʹ�ñ�������涨������ж����������ȫ�ֶ���������.��ȫ�ֶ��������г�ʼ������setup.s�еĻ���һ��,�����޳���8MB�޸ĳ���16MB.
 # stack_start������kernel/sched.c��.����ָ��user_stack����ĩ�˵�һ����ָ��.������������ʹ�õ�ջ,���ҳ�Ϊϵͳջ.�����ƶ�������0ִ��(
 # init/main.c��)�Ժ��ջ�ͱ���������0������1��ͬʹ�õ��û�ջ��.
.globl startup_32
startup_32:
	movl $0x10, %eax					# ����GNU���,ÿ��ֱ�Ӳ�����Ҫ��'$'��ʼ,�����ʾ��ַ.ÿ���Ĵ�������Ҫ��'$'��ͷ,eax��ʾ��32λ��ax�Ĵ���.
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	lss stack_start, %esp				# ��ʾstack_start->ss:esp,����ϵͳ��ջ.stack_start������kernel/sched.c��.
	call setup_idt						# ���������ж����������ӳ���.
	call setup_gdt						# ��������ȫ�����������ӳ���.
	movl $0x10, %eax					# reload all the segment registers
	mov %ax, %ds						# after changing gdt. CS was already
	mov %ax, %es						# reloaded in 'setup_gdt'
	mov %ax, %fs						# ��Ϊ�޸���gdt,������Ҫ����װ�����еĶμĴ���.CS����μĴ����Ѿ���setup_gdt�����¼��ع���.
	mov %ax, %gs
	# ���ڶ��������еĶ��޳���setup.s�е�8MB�ĳ��˱��������õ�16MB,��������ٴζ����жμĴ���ִ�м��ز����Ǳ����.����,ͨ��ʹ��bochs���ٹ۲�,�������
	# CS�ٴ�ִ�м���,��ô��ִ�е�movl $0x10,%eaxʱCS����β��ɼ������е��޳�����8MB.��������Ӧ�����¼���CS.��������setup.s�е��ں˴�����������뱾
	# �������������õĴ�������������˶��޳��������ಿ����ȫһ��,8MB���޳����ں˳�ʼ���׶β���������,�������Ժ��ں�ִ�й����жμ���תʱ�����¼���CS.���
	# ����û�м�������û���ó������.��Ը�����,Ŀǰ�ں��о���call setup_gdt֮�������һ������תָ��:'ljmp $(__KERNEL_CS),$1f',��ת��movl $0x10,$eax
	# ��ȷ��CSȷʵ�����¼���.

	lss stack_start, %esp

	 # ����������ڲ���A20��ַ���Ƿ��Ѿ�����.���õķ��������ڴ��ַ0x000000��д������һ����ֵ,Ȼ���ڴ��ַ0x100000(1M)���Ƿ�Ҳ�������ֵ.���һֱ��ͬ��
	 # ��,��һֱ�Ƚ���ȥ,����ѭ��,����.��ʾ��ַA20��û��ѡͨ,����ں˾Ͳ���ʹ��1MB�����ڴ�.
	xorl %eax, %eax
1:	incl %eax							# check that A20 really IS enabled
	 # '1:'��һ���ֲ����Ź��ɵı��.����ɷ��ź��һ��ð�����.��ʱ�÷��ű�ʾ�λ�ü����ĵ�ǰֵ,��������Ϊָ��Ĳ�����.�ֲ��������ڰ����������ͱ����Ա��ʱ
	 # ʹ��һЩ����.����10���ֲ�������,���������������ظ�ʹ��.��Щ������ʹ������'0','1',...,'9'������.Ϊ�˶���һ���ֲ�����,��ѱ��д��'N:'��ʽ(����N��ʾ
	 # һ������).Ϊ��������ǰ���������������,��Ҫд��'Nb',����N�Ƕ�����ʱʹ�õ�����.Ϊ������һ���ֲ���ŵ���һ������,��Ҫ���'Nf',����N��10��ǰ������
	 # ֮һ.����'b'��ʾ"���(backwards)",'f'��ʾ"��ǰ(forwards)".�ڻ������ĳһ��,�������������/��ǰ����10�����.
	movl %eax, 0x000000					# loop forever if it isn't
	cmpl %eax, 0x100000
	je 1b								# '1b'��ʾ�����ת�����1ȥ.����'5f'���ʾ��ǰ��ת�����5ȥ.
/*
 * NOTE! 486 should set bit 16, to check for write-protect in supervisor
 * mode. Then it would be unnecessary with the "verify_area()"-calls.
 * 486 users probably want to set the NE (#5) bit also, so as to use
 * int 16 for math errors.
 */
/*
 * ע��!��������γ�����,486Ӧ�ý���16��λ,�Լ���ڳ����û�ģʽ�µ�д����,�˺�"verify_area()"���þͲ���Ҫ��.486���û�ͨ��Ҳ���뽫NE(#5)��λ,�Ա����ѧ
 * Э�������ĳ���ʹ��int 16.
 *
 */
 # ����ԭע�����ᵽ��486CPU��CR0��������λ16��д������־WP,���ڽ�ֹ�����û����ĳ�����һ���û�ֻ��ҳ���н���д����.�ñ�־��Ҫ���ڲ���ϵͳ�ڴ����½���ʱʵ��д
 # �︴�Ʒ���.
 # ������γ������ڼ����ѧЭ������оƬ�Ƿ����.�������޸Ŀ��ƼĴ���CR0,�ڼ������Э�������������ִ��һ��Э������ָ��,�������Ļ���˵��Э������оƬ������,��Ҫ
 # ����CR0�е�Э����������λEM(λ2),����λЭ���������ڱ�־MP(λ1).

	movl %cr0, %eax						# check math chip
	andl $0x80000011, %eax				# Save PG,PE,ET
	/* "orl $0x10020,%eax" here for 486 might be good */
	orl $2, %eax						# set MP
	movl %eax, %cr0
	call check_x87
	jmp after_page_tables

/*
 * We depend on ET to be correct. This checks for 287/387.
 */
/*
 * ����������ET��־����ȷ�������287/387�������.
 *
 */
# ����fninit��fstsw����ѧЭ������(80287/80387)��ָ��.
# fninit��Э������������ʼ������,�����Э����������һ��ĩ����ǰ����Ӱ����Ѻ�״̬,�����������ΪĬ��ֵ,���״̬�ֺ����и���ջʽ�Ĵ���.�ǵȴ���ʽ������ָ��(fninit)��
# ����Э��������ִֹ�е�ǰ����ִ�е��κ���ǰ����������.fstswָ��ȡЭ��������״̬��.���ϵͳ�д���Э�������Ļ�,��ô��ִ����fninitָ�����״̬�ֵ��ֽڿ϶�Ϊ0.

check_x87:
	fninit								# ��Э������������ʼ������.
	fstsw %ax							# ȡЭ������״̬�ֵ�ax�Ĵ�����.
	cmpb $0, %al						# ��ʼ��״̬��Ӧ��Ϊ0,����˵��Э������������.
	je 1f								/* no coprocessor: have to set bits */
	movl %cr0, %eax						# �����������ǰ��ת�����1��,�����дcr0.
	xorl $6, %eax						/* reset MP, set EM */
	movl %eax, %cr0
	ret

# ������һ���������ָʾ��.�京����ָ�洢�߽�������."2"��ʾ�����Ĵ�������ݵ�ƫ��λ�õ�������ֵַ���2λΪ���λ��(2^2),����4�ֽڷ�ʽ�����ڴ��ַ.��������GNU asֱ��
# д�������ֵ����2�Ĵη�ֵ��.ʹ�ø�ָʾ����Ŀ����Ϊ�����32λCPU�����ڴ��д�������ݵ��ٶȺ�Ч��.
# ����������ֽ�ֵ��80287Э������ָ��fsetpm�Ļ�����.�������ǰ�80287����Ϊ����ģʽ.
# 80387�����ָ��,���ҽ���Ѹ�ָ����ǿղ���.

.align 2
1:	.byte 0xDB,0xE4		/* fsetpm for 287, ignored by 387 */	# 287Э��������.
	ret

/*
 *  setup_idt
 *
 *  sets up a idt with 256 entries pointing to
 *  ignore_int, interrupt gates. It then loads
 *  idt. Everything that wants to install itself
 *  in the idt-table may do so themselves. Interrupts
 *  are enabled elsewhere, when we can be relatively
 *  sure everything is ok. This routine will be over-
 *  written by the page tables.
 */
/*
 * ��������������ж����������ӳ���setup_idt
 *
 * ���ж���������idt���óɾ���256����,����ָ��ignore_int�ж���.Ȼ������ж���������Ĵ���(lidtָ��).����ʵ�õ��ж����Ժ��ٰ�װ.�������������ط���Ϊһ�ж�����ʱ�ٿ���
 * �ж�.���ӳ��򽫻ᱻҳ���ǵ�.
 */
# �ж����������е�����ȻҲ��8�ֽ����,�����ʽ��ȫ�ֱ��еĲ�ͬ,����Ϊ��������.����0-1,6-7�ֽ���ƫ����,2-3�ֽ���ѡ���,4-5�ֽ���һЩ��־.��������,��256��.eax����������
# ��4�ֽ�,edx���и�4�ֽ�.�ں������ĳ�ʼ�������л��滻��װ��Щ����ʵ�õ��ж���������.

setup_idt:
	lea ignore_int, %edx			# ��ignore_int����Ч��ַ(ƫ��ֵ)ֵ->eax�Ĵ���
	movl $0x00080000, %eax			# ��ѡ���0x0008����eax�ĸ�16λ��.
	movw %dx, %ax					/* selector = 0x0008 = cs */	# ƫ��ֵ�ĵ�16λ����eax�ĵ�16λ��.��ʱeax��������������4�ֽڵ�ֵ.
	movw $0x8E00, %dx				/* interrupt gate - dpl=0, present */	# ��ʱedx��������������4�ֽڵ�ֵ.

	lea idt, %edi					# idt���ж���������ĵ�ַ.
	mov $256, %ecx
rp_sidt:
	movl %eax, (%edi)				# �����ж����������������.
	movl %edx, 4(%edi)
	addl $8, %edi					# ediָ�������һ��.
	dec %ecx
	jne rp_sidt
	lidt idt_descr					# �����ж���������Ĵ���ֵ.
	ret

/*
 *  setup_gdt
 *
 *  This routines sets up a new gdt and loads it.
 *  Only two entries are currently built, the same
 *  ones that were built in init.s. The routine
 *  is VERY complicated at two whole lines, so this
 *  rather long comment is certainly needed :-).
 *  This routine will beoverwritten by the page tables.
 */
/*
 * ����ȫ������������setup_gdt
 * ����ӳ�������һ���µ�ȫ����������gdt,������.���ӳ��򽫱�ҳ���ǵ�.
 *
 */
setup_gdt:
	lgdt gdt_descr					# ����ȫ����������Ĵ���(���������ú�)
	ret

/*
 * I put the kernel page tables right after the page directory,
 * using 4 of them to span 16 Mb of physical memory. People with
 * more than 16MB will have to expand this.
 */
/*
 * Linus���ں˵��ڴ�ҳ��ֱ�ӷ���ҳĿ¼֮��,ʹ����4������Ѱַ16MB�������ڴ�.������ж���16MB���ڴ�,����Ҫ��������������޸�.
 *
 */
 # ÿ��ҳ��Ϊ4KB�ֽ�(1ҳ�ڴ�ҳ��),��ÿ��ҳ������Ҫ4���ֽ�,���һ��ҳ�����Դ��1024������.���һ��ҳ����Ѱַ4KB�ĵ�ַ�ռ�,��һ��ҳ��Ϳ���Ѱַ
 # 4MB�������ڴ�.
 # ҳ����ĸ�ʽΪ:���ǰ0-11λ���һЩ��־,�����Ƿ����ڴ���(Pλ0),��д���(R/Wλ1),��ͨ���ǳ����û�ʹ��(U/Sλ2),�Ƿ��޸Ĺ���(�Ƿ�����)(Dλ6)��;
 # �����λ12-31��ҳ���ַ,����ָ��һҳ�ڴ��������ʼ��ַ.

.org 0x1000							# ��ƫ��0x1000����ʼ�ĵ�1��ҳ��(ƫ��0��ʼ�������ҳ��Ŀ¼).
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000							# ����������ڴ����ݿ��ƫ��0x5000����ʼ.
/*
 * tmp_floppy_area is used by the floppy-driver when DMA cannot
 * reach to a buffer-block. It needs to be aligned, so that it isn't
 * on a 64kB border.
 */
/*
 * ��DMA(ֱ�Ӵ洢������)���ܷ��ʻ����ʱ,�����tmp_floppy_area�ڴ��Ϳɹ�������������ʹ��.���ַ��Ҫ�������,�����Ͳ����Խ64KB�߽�.
 */

tmp_floppy_area:
	.fill 1024,1,0					# ������1024��,ÿ��1B,�����ֵ0.

 # �����⼸����ջ��������Ϊ��ת��init/main.c�е�main()������׼������.pushl $L6ָ����ջ��ѹ�뷵�ص�ַ,��pushl $main��ѹ����main()��������
 # �ĵ�ַ.��head.s���ִ��retָ��ʱ�ͻᵯ��main()�ĵ�ַ,���ѿ���Ȩת�Ƶ�init/main.c������.
 # ǰ��3����ջ0ֵӦ�÷ֱ��ʾenvp,argvָ���argc��ֵ,��main()û���õ�.

after_page_tables:
	pushl $0						# These are the parameters to main :-)
	pushl $0						# ��Щ�ǵ���main����Ĳ���(ָinit/main.c).
	pushl $0						# ���е�'$'���ű�ʾ����һ������������.
	pushl $L6						# return address for main, if it decides to.
	pushl $main						# 'main'�Ǳ�������main���ڲ���ʾ����.
	jmp setup_paging				# ��ת��setup_paging
L6:
	jmp L6							# main should never return here, but
									# just in case, we know what happens.
									# main������Բ�Ӧ�÷��ص�����.����Ϊ���Է���һ,��������˸����.�������Ǿ�֪������ʲô������.

/* This is the default interrupt "handler" :-) */
/* ������Ĭ�ϵ��ж�"�������" */

int_msg:
	.asciz "Unknown interrupt\n\r"	# �����ַ���"ĩ֪�ж�(�س�����)".

.align 2							# ��4�ֽڷ�ʽ�����ڴ��ַ.
ignore_int:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds						# ������ע��!!ds,es,fs,gs����Ȼ��16λ�Ĵ���,����ջ����Ȼ����32λ����ʽ��ջ,����Ҫռ��4���ֽڵĶ�ջ�ռ�.
	push %es
	push %fs
	movl $0x10, %eax				# ���ö�ѡ���(ʹds,es,fsָ��gdt���е����ݶ�).
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	pushl $int_msg					# �ѵ���printk�����Ĳ���ָ��(��ַ)��ջ.ע��!��int_msgǰ����'$',���ʾ��int_msg�����ĳ���('Unkn')��ջ.
	call printk						# �ú�����/kernel/printk.c��.
	popl %eax
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret							# �жϷ���(���жϵ���ʱѹ��ջ��CPU��־�Ĵ���(32λ)ֵҲ����).


/*
 * Setup_paging
 *
 * This routine sets up paging by setting the page bit
 * in cr0. The page tables are set up, identity-mapping
 * the first 16MB. The pager assumes that no illegal
 * addresses are produced (ie >4Mb on a 4Mb machine).
 *
 * NOTE! Although all physical memory should be identity
 * mapped by this routine, only the kernel page functions
 * use the >1Mb addresses directly. All "normal" functions
 * use just the lower 1Mb, or the local data space, which
 * will be mapped to some other place - mm keeps track of
 * that.
 *
 * For those with more memory than 16 Mb - tough luck. I've
 * not got it, why should you :-) The source is here. Change
 * it. (Seriously - it shouldn't be too difficult. Mostly
 * change some constants etc. I left it at 16Mb, as my machine
 * even cannot be extended past that (ok, but it was cheap :-)
 * I've tried to show which constants to change by having
 * some kind of marker at them (search for "16Mb"), but I
 * won't guarantee that's all :-( )
 */
/*
 * ����ӳ���ͨ�����ÿ��ƼĴ���cr0�ı�־(PGλ31)���������ڴ�ķ�ҳ������,�����ø���ҳ���������,�Ժ��ӳ��ǰ16MB�������ڴ�.��ҳ���ٶ�
 * ��������Ƿ��ĵ�ַӳ��(Ҳ����ֻ��4MB�Ļ��������ó�����4MB���ڴ��ַ).
 *
 * ע��!�������е������ַ��Ӧ��������ӳ�����к��ӳ��,��ֻ���ں�ҳ���������ֱ��ʹ��>1MB�ĵ�ַ.����"��ͨ"������ʹ�õ���1MB�ĵ�ַ�ռ�,
 * ������ʹ�þֲ����ݿռ�,�õ�ַ�ռ佫��ӳ�䵽����һЩ�ط�ȥ--mm(�ڴ�������)�������Щ�µ�.
 *
 */
 # ����Ӣ��ע�͵�2�εĺ�����ָ�ڻ��������ڴ��д���1MB���ڴ�ռ���Ҫ���������ڴ���.���ڴ����ռ���mmģ�����.���漰ҳ��ӳ�����.�ں���������
 # ��������������ָ��һ��(��ͨ)����.��Ҫʹ�����ڴ�����ҳ��,����Ҫʹ��get_free_page()�Ⱥ�����ȡ.��Ϊ���ڴ������ڴ�ҳ���ǹ�����Դ,������
 # ����ͳһ�����Ա�����Դ���ú;���.
 #
 # ���ڴ������ַ0x0����ʼ���1ҳҳĿ¼���4ҳҳ��.ҳĿ¼����ϵͳ���н��̹��õ�,�������4ҳҳ���������ں�ר��,����һһӳ�����Ե�ַ��ʼ16MB
 # �ռ䷶Χ�������ڴ���.�����µĽ���,ϵͳ�������ڴ���Ϊ������ҳ����ҳ��.����,1ҳ�ڴ泤����4096�ֽ�.

.align 2								# ��4�ֽڷ�ʽ�����ڴ��ַ�߽�.
setup_paging:							# ���ȶ�5ҳ�ڴ�(1ҳĿ¼+4ҳҳ��)����.
	movl $1024 * 5, %ecx				/* 5 pages - pg_dir+4 page tables */
	xorl %eax, %eax
	xorl %edi, %edi						/* pg_dir is at 0x000 */	# ҳĿ¼��0x0000��ַ��ʼ
	cld;rep;stosl						# eax���ݴ浽es:edi��ָ�ڴ�λ�ô�,��edi��4.

	 # ����4������ҳĿ¼���е���,��Ϊ����(�ں�)����4��ҳ������ֻ������4��.
	 # ҳĿ¼��Ľṹ��ҳ����Ľṹһ��,4���ֽ�Ϊ1��.
	 # ����"$pg0+7"��ʾ:0x00001007,��ҳĿ¼���еĵ�1��.
	 # ���1��ҳ�����ڵĵ�ַ=0x00001007 & 0xfffff000=0x1000;
	 # ��1��ҳ������Ա�־=0x00001007 & 0x00000fff = 0x07,��ʾ��ҳ����,�û��ɶ�д.
	movl $pg0 + 7, pg_dir				/* set present bit/user r/w */
	movl $pg1 + 7, pg_dir + 4			/*  --------- " " --------- */
	movl $pg2 + 7, pg_dir + 8			/*  --------- " " --------- */
	movl $pg3 + 7, pg_dir + 12			/*  --------- " " --------- */

	 # ����6����д4��ҳ���������������,����:4(ҳ��)*1024(��/ҳ��)=4096��(0-0xfff),����ӳ�������ڴ�4096*4KB = 16MB.
	 # ÿ���������:��ǰ����ӳ��������ڴ��ַ + ��ҳ�ı�־(�����Ϊ7).
	 # ʹ�õķ����Ǵ����һ��ҳ������һ�ʼ������˳����д.һ��ҳ������һ����ҳ���е�λ����1023*4 = 4092.������һҳ�����һ���λ�þ���$pg3+4092.

	movl $pg3 + 4092, %edi				# edi->���һҳ�����һ��.
	movl $0xfff007, %eax				/*  16Mb - 4096 + 7 (r/w user,p) */
										# ���һ���Ӧ�����ڴ�ҳ�ĵ�ַ��0xfff000,�������Ա�־7,��Ϊxfff007.
	std									# ����λ��λ,ediֵ�ݼ�(4�ֽ�).
1:	stosl								/* fill pages backwards - more efficient :-) */
	subl $0x1000, %eax					# ÿ���һ��,�����ֵַ��0x1000.
	jge 1b								# ���С��0��˵��ȫ��д����.
	cld
	 # ����ҳĿ¼�����ַ�Ĵ���cr3��ֵ,ָ��ҳĿ¼��.cr3�б������ҳĿ¼��������ַ.
	xorl %eax, %eax						/* pg_dir is at 0x0000 */		# ҳĿ¼����0x0000��.
	movl %eax, %cr3						/* cr3 - page directory start */
	 # ��������ʹ�÷�ҳ����(cr0��PG��־,λ31)
	movl %cr0, %eax
	orl $0x80000000, %eax				# ����PG��־.
	movl %eax, %cr0						/* set paging (PG) bit */
	ret									/* this also flushes prefetch-queue */

# �ڸı��ҳ�����־��Ҫ��ʹ��ת��ָ��ˢ��Ԥȡָ�����, �����õ��Ƿ���ָ��ret.
# �÷���ָ�����һ�������ǽ�pushl $mainѹ���ջ�е�main����ĵ�ַ����,����ת��/init/main.c����ȥ����.�����򵽴˾�����������.

.align 2								# ��4�ֽڷ�ʽ�����ڴ��ַ�߽�.
.word 0									# �����ȿճ�2�ֽ�,����.long _idt�ĳ�����4�ֽڶ����.

 # �����Ǽ����ж���������Ĵ���idtr��ָ��lidtҪ���6�ֽڲ�����.ǰ2�ֽ���idt����޳�,��4�ֽ���idt�������Ե�ַ�ռ��е�32λ����ַ.
idt_descr:
	.word 256 * 8 - 1					# idt contains 256 entries
	.long idt
.align 2
.word 0

 # �����Ǽ���ȫ����������Ĵ���gdtr��ָ��lgdtҪ���6�ֽڲ�����.ǰ2�ֽ���gdt����޳�,��4�ֽ���gdt������Ի���ַ.����ȫ�ֱ�������Ϊ
 # 2KB�ֽ�(0x7ff����),��Ϊÿ8�ֽ����һ����������,���Ա��й�����256��.����gdt��ȫ�ֱ��ڱ������е�ƫ��λ��.

gdt_descr:
	.word 256 * 8 - 1					# so does gdt (not that that's any
	.long gdt							# magic number, but it works for me :^)

	.align 8							# ��8(2^3)�ֽڷ�ʽ�����ڴ��ַ�߽�.
idt:	.fill 256, 8, 0					# idt is uninitialized	# 256��,ÿ��8�ֽ�,��0.

 # ȫ�ֱ�,ǰ4��ֱ��ǿ���(����),�����������,���ݶ�������,ϵͳ���ö�������,����ϵͳ���ö���������û�����ô�,Linus��ʱ���������ϵͳ����
 # ����ר�ŷ�����������Ķ���.
 # ͬ��Ԥ����252��Ŀռ�,���ڷ�������������ľֲ�������(LDT)�Ͷ�Ӧ������״̬��TSS��������.
 # (0-nul, 1-cs, 2-ds, 3-syscall, 4-TSS0, 5-LDT0, 6-TSS1, 7-LDT1, 8-TSS2 etc...)
gdt:
	.quad 0x0000000000000000			/* NULL descriptor */
	.quad 0x00c09a0000000fff			/* 16Mb */		# 0x08,�ں˴������󳤶�16MB.
	.quad 0x00c0920000000fff			/* 16Mb */		# 0x10,�ں����ݶ���󳤶�16MB.
	.quad 0x0000000000000000			/* TEMPORARY - don't use */
	.fill 252, 8, 0						/* space for LDT's and TSS's etc */	# Ԥ���ռ�.

