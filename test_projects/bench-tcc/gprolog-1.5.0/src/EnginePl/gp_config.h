/* EnginePl/gp_config.h.  Generated from gp_config.h.in by configure.  */
/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : configuration                                                   *
 * File  : gp_config.h.in                                                  *
 * Descr.: general configuration file (handled by autoconf) - header file  *
 * Author: Daniel Diaz                                                     *
 *                                                                         *
 * Copyright (C) 1999-2021 Daniel Diaz                                     *
 *                                                                         *
 * This file is part of GNU Prolog                                         *
 *                                                                         *
 * GNU Prolog is free software: you can redistribute it and/or             *
 * modify it under the terms of either:                                    *
 *                                                                         *
 *   - the GNU Lesser General Public License as published by the Free      *
 *     Software Foundation; either version 3 of the License, or (at your   *
 *     option) any later version.                                          *
 *                                                                         *
 * or                                                                      *
 *                                                                         *
 *   - the GNU General Public License as published by the Free             *
 *     Software Foundation; either version 2 of the License, or (at your   *
 *     option) any later version.                                          *
 *                                                                         *
 * or both in parallel, as here.                                           *
 *                                                                         *
 * GNU Prolog is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received copies of the GNU General Public License and   *
 * the GNU Lesser General Public License along with this program.  If      *
 * not, see http://www.gnu.org/licenses/.                                  *
 *-------------------------------------------------------------------------*/

#ifndef _GP_CONFIG_H
#define _GP_CONFIG_H

/* Define if you have inttypes.h */
#define HAVE_INTTYPES_H 1

/* Define if you have sys/ioctl_compat.h */
/* #undef HAVE_SYS_IOCTL_COMPAT_H */

/* Define if you have sys/syginfo.h */
/* #undef HAVE_SYS_SIGINFO_H */

/* Define if you have termios.h */
#define HAVE_TERMIOS_H 1

/* Define if you have termio.h */
/* #undef HAVE_TERMIO_H */

/* Define if you have malloc.h */
#define HAVE_MALLOC_H 1

/* Define if you have endian.h */
#define HAVE_ENDIAN_H 1

/* Define if you have sys/endian.h */
/* #undef HAVE_SYS_ENDIAN_H */

/* Define if you have byteswap.h */
#define HAVE_BYTESWAP_H 1

/* Define if you have float.h */
#define HAVE_FLOAT_H 1

/* Define if you have the fgetc function */
#define HAVE_FGETC 1

/* Define if you have the sigsetjmp function */
#define HAVE_SIGSETJMP 1

/* NB: fpclassify is a macro, see BipsPl/arith_inl_c.c */
/* Define if you have the fpclass function */
/* #undef HAVE_FPCLASS */

/* Define if you have the _fpclass function (windows) */
/* #undef HAVE__FPCLASS */

/* Define if you have the isnan function */
#define HAVE_ISNAN 1

/* Define if you have the isinf function */
#define HAVE_ISINF 1

/* Define if you have the asinh function */
#define HAVE_ASINH 1

/* Define if you have the acosh function */
#define HAVE_ACOSH 1

/* Define if you have the atanh function */
#define HAVE_ATANH 1

/* Define if you have a working mmap system call */
#define HAVE_MMAP 1

/* Define if you have a working mprotect system call */
#define HAVE_MPROTECT 1

/* Define if you have a mallopt function */
#define HAVE_MALLOPT 1

/* Define if you have a htole32 function */
#define HAVE_DECL_HTOLE32 1

/* Define if you have a bswap_32 function */
#define HAVE_DECL_BSWAP_32 1

/* Define inline keyword */
/* #undef inline */

/* Define if you don't want to use machine registers */
/* #undef NO_USE_REGS */

/* Define if you don't want to use the ebp register on ix86 */
#define NO_USE_EBP 1

/* Define if you don't want to fast call on ix86 */
/* #undef NO_USE_FAST_CALL */

/* Define if you don't want to include line editor facility */
/* #undef NO_USE_LINEDIT */

/* Define if you don't want consult/1 launches pl2wam with pipe on its input */
/* #undef NO_USE_PIPED_STDIN_FOR_CONSULT */

/* Define if you don't want to use the win32 GUI console */
#define NO_USE_GUI_CONSOLE 1

/* Define if you don't want to include sockets facility */
/* #undef NO_USE_SOCKETS */

/* Define if you don't want to include the FD constraint solver */
/* #undef NO_USE_FD_SOLVER */




/* Define if the cpu is a mips32 */
/* #undef M_mips32 */

/* Define if the cpu is an alpha */
/* #undef M_alpha */

/* Define if the cpu is a sparc32 */
/* #undef M_sparc32 */

/* Define if the cpu is a sparc64 */
/* #undef M_sparc64 */

/* Define if the cpu is a ix86 */
/* #undef M_ix86 */

/* Define if the cpu is a ppc32 */
/* #undef M_ppc32 */

/* Define if the cpu is a x86-64 */
#define M_x86_64 1

/* Define if the cpu is a arm32 (armv7, armhf) */
/* #undef M_arm32 */

/* Define if the cpu is a arm4 (arch64) */
/* #undef M_arm64 */

/* Define if the OS is an SGI IRIX */
/* #undef M_irix */

/* Define if the OS is an DEC OSF1 */
/* #undef M_osf */

/* Define if the OS is a sunos */
/* #undef M_sunos */

/* Define if the OS is a solaris */
/* #undef M_solaris */

/* Define if the OS is a linux */
#define M_linux 1

/* Define if the OS is a darwin */
/* #undef M_darwin */

/* Define if the OS is a WinXX based on Cygwin */
/* #undef M_cygwin */

/* Define if the OS is a WIN32 */
/* #undef M_win32 */

/* Define if the OS is a WIN64 (in addition to M_win32) */
/* #undef M_win64 */

/* Define if the OS is a SCO */
/* #undef M_sco */

/* Define if the OS is a (Free/Open/Net)BSD */
/* #undef M_bsd */


/* Define if the system is an mips32/irix */
/* #undef M_mips32_irix */

/* Define if the system is an alpha/linux */
/* #undef M_alpha_linux */

/* Define if the system is an alpha/OSF1 */
/* #undef M_alpha_osf */

/* Define if the system is a ix86/linux */
/* #undef M_ix86_linux */

/* Define if the system is a ix86/sco */
/* #undef M_ix86_sco */

/* Define if the system is a ix86/freebsd or openbsd or netbsd */
/* #undef M_ix86_bsd */

/* Define if the system is a ix86/cygwin */
/* #undef M_ix86_cygwin */

/* Define if the system is a ix86/mingw */
/* #undef M_ix86_mingw */

/* Define if the system is a ix86/win32 */
/* #undef M_ix86_win32 */

/* Define if the system is a ix86/darwin */
/* #undef M_ix86_darwin */

/* Define if the system is a ix86/solaris */
/* #undef M_ix86_solaris */

/* Define if the system is a sparc32/solaris */
/* #undef M_sparc32_solaris */

/* Define if the system is a sparc32/sunos */
/* #undef M_sparc32_sunos */

/* Define if the system is a sparc32/bsd */
/* #undef M_sparc32_bsd */

/* Define if the system is a sparc64/bsd */
/* #undef M_sparc64_bsd */

/* Define if the system is a ppc32/linux */
/* #undef M_ppc32_linux */

/* Define if the system is a ppc32/darwin */
/* #undef M_ppc32_darwin */

/* Define if the system is a ppc32/bsd */
/* #undef M_ppc32_bsd */

/* Define if the system is a x86-64/linux */
#define M_x86_64_linux 1

/* Define if the system is a x86-64/solaris */
/* #undef M_x86_64_solaris */

/* Define if the system is a x86-64/bsd */
/* #undef M_x86_64_bsd */

/* Define if the system is a x86-64/cygwin */
/* #undef M_x86_64_cygwin */

/* Define if the system is a x86-64/mingw */
/* #undef M_x86_64_mingw */

/* Define if the system is a x86-64/darwin */
/* #undef M_x86_64_darwin */

/* Define if the system is a arm32/linux */
/* #undef M_arm32_linux */

/* Define if the system is a arm64/linux */
/* #undef M_arm64_linux */

/* Define if the system is a arm64/darwin (e.g. Apple arm M1/MacOS)*/
/* #undef M_arm64_darwin */


/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif


/* Constant definitions */

#define PROLOG_DIALECT "gprolog"
#define PROLOG_NAME "GNU Prolog"
#define PROLOG_VERSION "1.5.0"
#define PROLOG_DATE "Jun 19 2026"
#define PROLOG_COPYRIGHT "Copyright (C) 1999-2026 Daniel Diaz"

#define TOP_LEVEL "gprolog"
#define GPLC "gplc"
#define HEXGPLC "hexgplc"
#define ENV_VARIABLE "PL_PATH"

#define M_VENDOR "pc"
#define M_CPU "x86_64"
#define M_OS "linux-gnu"

#define CC "clang"
#define CFLAGS_PREFIX_REG "-ffixed-%s"
#define CFLAGS "-O3 -fomit-frame-pointer -Wno-char-subscripts"
#define CFLAGS_MACHINE " -fno-strict-aliasing -fcommon"
#define LDFLAGS ""
#define LDLIBS "-lm"
#define AS "as"
#define ASFLAGS "--64"
#define STRIP "strip"

#define ASM_SUFFIX ".s"
#define OBJ_SUFFIX ".o"
#define EXE_SUFFIX ""
#define CC_OBJ_NAME_OPT "-o "
#define CC_EXE_NAME_OPT "-o "

/* define if Windows HtmlHelp is used 1=static (-lhtmlhelp), 2=dynamic (DLL) */
/* #undef WITH_HTMLHELP */

#define DLL_W32GUICONS "w32guicons.dll"
#define LIB_LINEDIT "liblinedit.a"
#define LIB_ENGINE_PL "libengine_pl.a"
#define LIB_BIPS_PL "libbips_pl.a"
#define LIB_ENGINE_FD "libengine_fd.a"
#define LIB_BIPS_FD "libbips_fd.a"


#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_VOIDP 8

#define SIZEOF_PTR                 SIZEOF_VOIDP
#define WORD_SIZE                  (8 * SIZEOF_PTR)

/* Define if socklen_t is not defined */
/* #undef socklen_t */

/* Define if you have a working sigaction to detect SIGSEGV bad addr */
#define HAVE_WORKING_SIGACTION 1

/* Define if obj chain needs to reverse order of collected objects */
#define OBJ_CHAIN_REVERSE_ORDER 1

#include "arch_dep.h"
#include "pl_long.h"

#endif /* !_GP_CONFIG_H */
