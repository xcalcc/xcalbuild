/*
 * __xvsa_common.h
 * common config with xvsa for preprocessing
 *
 */
#ifndef __XVSA_COMMON_H__
#define __XVSA_COMMON_H__

#define __XVSA__ 1

/*
 * not support _FloatXYZ types, define them to float/double/long double
 */
typedef float _Float32;
typedef double _Float32x;
typedef double _Float64;
typedef long double _Float64x;
typedef __float128  _Float128;
typedef __float128  _Float128x;

/*
 * undef __OPTIMIZE__ to bypass a few unsupported builtin:
 * __builtin_va_arg_pack, __builtin_va_arg_pack_len
 */
#ifdef __OPTIMIZE__
# undef __OPTIMIZE__
#endif

/*
 * define __builtin_launder(x) to (x) because it's unsupported
 */
#define __builtin_launder(x) (x)

/*
 * define _Atomic to be empty
 */
#if !defined(__APPLE__)
# define _Atomic
#endif

/*
 * define __gnu_printf__ to be printf
 */
#define __gnu_printf__ printf


/*
 * define __has_feature(x)
 */
#if !defined(__APPLE__)
# define __has_feature(x)       __has_feature_##x
# define __has_builtin(x)       __has_builtin_##x
# define __has_extension(x)     __has_extension_##x
# define __building_module(x)   __building_module_##x
#endif

#define __has_feature_modules   0

#define __has_builtin___dmb     0
#define __has_builtin___dmb     0
#define __has_builtin___dsb     0
#define __has_builtin___isb     0
#define __has_builtin___wfi     0
#define __has_builtin___wfe     0
#define __has_builtin___sev     0
#define __has_builtin___sevl    0
#define __has_builtin___yield   0

#define __has_extension_gnu_asm 1

#define __building_module__Builtin_intrinsics 0
#define __has_builtin___make_integer_seq 1

#endif  /* __XVSA_COMMON_H__ */
