#ifndef CTL_OPT_X86_H__
#define CTL_OPT_X86_H__

/*
 * Small x86 tuning hooks shared by AVX2 hot paths.
 *
 * CTL_X86_ASM can be set to 0 to disable inline assembly while keeping the
 * same C/AVX2 implementation. The assembly hook is intentionally tiny: the
 * compiler still schedules the AVX2 arithmetic, while the handwritten
 * prefetch keeps the streaming dot-product and butterfly loops fed.
 */

#ifndef CTL_X86_ASM
#define CTL_X86_ASM   1
#endif

#if CTL_X86_ASM && (defined __GNUC__ || defined __clang__) \
	&& (defined __i386__ || defined __x86_64__)
#define CTL_HAVE_X86_ASM   1
static inline void
ctl_asm_prefetch_ro(const void *p)
{
	__asm__ __volatile__("prefetcht0 (%0)" : : "r"(p));
}
#else
#define CTL_HAVE_X86_ASM   0
static inline void
ctl_asm_prefetch_ro(const void *p)
{
	(void)p;
}
#endif

#endif
