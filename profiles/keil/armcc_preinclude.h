#define __align(x)
#define __ALIGNOF(x) __alignof__(x)
#define __asm(x)
//#define __asm __asm__
#define __forceinline
#define __global_reg(x)
#define __inline
#define __INTADDR(x) x
#define __int64 long long
#define __irq
#define __packed
#define __pure
#define __smc(x)
#define __softfp
#define __svc(x) x
#define __svc_indirect(x)
#define __svc_indirect_r7(x)
#define __value_in_regs
#define __weak
#define __writeonly
#define __declspec(x)

extern void __breakpoint(int val);
extern void __cdp(unsigned int coproc, unsigned int opcode1, unsigned int opcode2);
extern void __clrex(void);
extern unsigned char __clz(unsigned int val);
extern unsigned int __current_pc(void);
extern unsigned int __current_sp(void);
extern int __disable_fiq(void); extern int __disable_irq(void);
extern void __dsb(unsigned int);
extern void __enable_fiq(void);
extern void __enable_irq(void);
extern double __fabs(double val);
extern float __fabsf(float val);
extern void __force_stores(void);
extern unsigned int __ldrex(volatile void *ptr);
extern unsigned long long __ldrexd(volatile void *ptr);
extern unsigned int __ldrt(const volatile void *ptr);
extern void __memory_changed(void);
extern void __nop(void);
extern void __pld(const void *ptr, ...);
extern void __pldw(const void *ptr, ...);
extern void __pli(const void *ptr, ...);
extern void __promise(int expr);
extern int __qadd(int val1, int val2);
extern int __qdbl(int val);
extern int __qsub(int val1, int val2);
extern unsigned int __qsub16(unsigned int val1, unsigned int val2);
extern unsigned int __rbit(unsigned int val);
extern unsigned int __rev(unsigned int val);
extern unsigned int __return_address(void);
extern unsigned int __ror(unsigned int val, unsigned int shift);
extern void __schedule_barrier(void);
extern int __semihost(int val, const void *ptr);
extern void __sev(void);
extern double __sqrt(double val);
extern float __sqrtf(float val);
extern int __ssat(int val, unsigned int sat);
extern int __strex(unsigned int val, volatile void *ptr);
extern int __strexd(unsigned long long val, volatile void *ptr);
extern void __strt(unsigned int val, volatile void *ptr);
extern unsigned int __swp(unsigned int val, volatile void *ptr);
extern int __usat(unsigned int val, unsigned int sat);
extern void __wfe(void);
extern void __wfi(void);
extern void __yield(void);
extern unsigned int __vfp_status(unsigned int val1, unsigned int val2);
