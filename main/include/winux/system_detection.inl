#ifndef __SYSTEM_DETECTION_INL__
#define __SYSTEM_DETECTION_INL__

/*
    The operating system, must be one of: (OS_<os-name>)
        DARWIN   - Any Darwin system
        MAC      - OS X and iOS
        OSX      - OS X
        IOS      - iOS
        MSDOS    - MS-DOS and Windows
        OS2      - OS/2
        OS2EMX   - XFree86 on OS/2 (not PM)
        WIN32    - Win32 (Windows 2000/XP/Vista/7 and Windows Server 2003/2008)
        WINCE    - WinCE (Windows CE 5.0)
        WINRT    - WinRT (Windows 8 Runtime)
        CYGWIN   - Cygwin
        SOLARIS  - Sun Solaris
        HPUX     - HP-UX
        ULTRIX   - DEC Ultrix
        LINUX    - Linux [has variants]
        FREEBSD  - FreeBSD [has variants]
        NETBSD   - NetBSD
        OPENBSD  - OpenBSD
        BSDI     - BSD/OS
        IRIX     - SGI Irix
        OSF      - HP Tru64 UNIX
        SCO      - SCO OpenServer 5
        UNIXWARE - UnixWare 7, Open UNIX 8
        AIX      - AIX
        HURD     - GNU Hurd
        DGUX     - DG/UX
        RELIANT  - Reliant UNIX
        DYNIX    - DYNIX/ptx
        QNX      - QNX [has variants]
        QNX6     - QNX RTP 6.1
        LYNX     - LynxOS
        BSD4     - Any BSD 4.4 system
        UNIX     - Any UNIX BSD/SYSV system
        ANDROID  - Android platform
        HAIKU    - Haiku

    The following operating systems have variants:
        LINUX    - both OS_LINUX and OS_ANDROID are defined when building for Android
                - only OS_LINUX is defined if building for other Linux systems
        QNX      - both OS_QNX and OS_BLACKBERRY are defined when building for Blackberry 10
                - only OS_QNX is defined if building for other QNX targets
        FREEBSD  - OS_FREEBSD is defined only when building for FreeBSD with a BSD userland
                - OS_FREEBSD_KERNEL is always defined on FreeBSD, even if the userland is from GNU

    The compiler platform, must be one of: (CL_<cl-name>)
        CYGWIN   - Cygwin
        MINGW    - MinGW compiler
        GCC      - GNU Compiler Collection
        VC       - Visual C++
        BC       - Borland C++
*/

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
#   define OS_DARWIN
#   define OS_BSD4
#   ifdef __LP64__
#       define OS_DARWIN64
#   else
#       define OS_DARWIN32
#   endif
#elif defined(ANDROID)
#   define OS_ANDROID
#   define OS_LINUX
#elif defined(__CYGWIN__)
#   define OS_CYGWIN
#elif !defined(SAG_COM) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#   define OS_WIN32
#   define OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#   if defined(WINCE) || defined(_WIN32_WCE)
#       define OS_WINCE
#   elif defined(WINAPI_FAMILY)
#       if defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
#           define OS_WINPHONE
#           define OS_WINRT
#       elif WINAPI_FAMILY==WINAPI_FAMILY_APP
#           define OS_WINRT
#       else
#           define OS_WIN32
#       endif
#   else
#       define OS_WIN32
#   endif
#elif defined(__sun) || defined(sun)
#   define OS_SOLARIS
#elif defined(hpux) || defined(__hpux)
#   define OS_HPUX
#elif defined(__ultrix) || defined(ultrix)
#   define OS_ULTRIX
#elif defined(sinix)
#   define OS_RELIANT
#elif defined(__native_client__)
#   define OS_NACL
#elif defined(__linux__) || defined(__linux)
#   define OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#   ifndef __FreeBSD_kernel__
#       define OS_FREEBSD
#   endif
#   define OS_FREEBSD_KERNEL
#   define OS_BSD4
#elif defined(__NetBSD__)
#   define OS_NETBSD
#   define OS_BSD4
#elif defined(__OpenBSD__)
#   define OS_OPENBSD
#   define OS_BSD4
#elif defined(__bsdi__)
#   define OS_BSDI
#   define OS_BSD4
#elif defined(__sgi)
#   define OS_IRIX
#elif defined(__osf__)
#   define OS_OSF
#elif defined(_AIX)
#   define OS_AIX
#elif defined(__Lynx__)
#   define OS_LYNX
#elif defined(__GNU__)
#   define OS_HURD
#elif defined(__DGUX__)
#   define OS_DGUX
#elif defined(__QNXNTO__)
#   define OS_QNX
#elif defined(_SEQUENT_)
#   define OS_DYNIX
#elif defined(_SCO_DS) /* SCO OpenServer 5 + GCC */
#   define OS_SCO
#elif defined(__USLC__) /* all SCO platforms + UDK or OUDK */
#   define OS_UNIXWARE
#elif defined(__svr4__) && defined(i386) /* Open UNIX 8 + GCC */
#   define OS_UNIXWARE
#elif defined(__INTEGRITY)
#   define OS_INTEGRITY
#elif defined(VXWORKS) /* there is no "real" VxWorks define - this has to be set in the mkspec! */
#   define OS_VXWORKS
#elif defined(__HAIKU__)
#   define OS_HAIKU
#elif defined(__MAKEDEPEND__)
#
#else
#   error "Eien has not been ported to this OS - see https://www.fastdo.net/"
#endif

#if defined(OS_WIN32) || defined(OS_WIN64) || defined(OS_WINCE) || defined(OS_WINRT)
#   define OS_WIN
#endif

#if defined(OS_DARWIN)
#   define OS_MAC
#   if defined(OS_DARWIN64)
#       define OS_MAC64
#   elif defined(OS_DARWIN32)
#       define OS_MAC32
#   endif
#   include <TargetConditionals.h>
#   if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#       define OS_IOS
#   elif defined(TARGET_OS_MAC) && TARGET_OS_MAC
#       define OS_OSX
#       define OS_MACX // compatibility synonym
#   endif
#endif

#if defined(OS_WIN)
#   undef OS_UNIX
#elif !defined(OS_UNIX)
#   define OS_UNIX
#endif

#ifdef OS_DARWIN
#   include <Availability.h>
#   include <AvailabilityMacros.h>
#
#   ifdef OS_OSX
#       if !defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_10_6
#           undef __MAC_OS_X_VERSION_MIN_REQUIRED
#           define __MAC_OS_X_VERSION_MIN_REQUIRED __MAC_10_6
#       endif
#       if !defined(MAC_OS_X_VERSION_MIN_REQUIRED) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_6
#           undef MAC_OS_X_VERSION_MIN_REQUIRED
#           define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_6
#       endif
#   endif
#
#   // Numerical checks are preferred to named checks, but to be safe we define the missing version names in case Programmer uses them.
#
#   if !defined(__MAC_10_7)
#       define __MAC_10_7 1070
#   endif
#   if !defined(__MAC_10_8)
#       define __MAC_10_8 1080
#   endif
#   if !defined(__MAC_10_9)
#       define __MAC_10_9 1090
#   endif
#   if !defined(__MAC_10_10)
#       define __MAC_10_10 101000
#   endif
#   if !defined(__MAC_10_11)
#       define __MAC_10_11 101100
#   endif
#   if !defined(MAC_OS_X_VERSION_10_7)
#       define MAC_OS_X_VERSION_10_7 1070
#   endif
#   if !defined(MAC_OS_X_VERSION_10_8)
#       define MAC_OS_X_VERSION_10_8 1080
#   endif
#   if !defined(MAC_OS_X_VERSION_10_9)
#       define MAC_OS_X_VERSION_10_9 1090
#   endif
#   if !defined(MAC_OS_X_VERSION_10_10)
#       define MAC_OS_X_VERSION_10_10 101000
#   endif
#   if !defined(MAC_OS_X_VERSION_10_11)
#       define MAC_OS_X_VERSION_10_11 101100
#   endif
#
#   if !defined(__IPHONE_4_3)
#       define __IPHONE_4_3 40300
#   endif
#   if !defined(__IPHONE_5_0)
#       define __IPHONE_5_0 50000
#   endif
#   if !defined(__IPHONE_5_1)
#       define __IPHONE_5_1 50100
#   endif
#   if !defined(__IPHONE_6_0)
#       define __IPHONE_6_0 60000
#   endif
#   if !defined(__IPHONE_6_1)
#       define __IPHONE_6_1 60100
#   endif
#   if !defined(__IPHONE_7_0)
#       define __IPHONE_7_0 70000
#   endif
#   if !defined(__IPHONE_7_1)
#       define __IPHONE_7_1 70100
#   endif
#   if !defined(__IPHONE_8_0)
#       define __IPHONE_8_0 80000
#   endif
#   if !defined(__IPHONE_8_1)
#       define __IPHONE_8_1 80100
#   endif
#   if !defined(__IPHONE_8_2)
#       define __IPHONE_8_2 80200
#   endif
#   if !defined(__IPHONE_8_3)
#       define __IPHONE_8_3 80300
#   endif
#   if !defined(__IPHONE_8_4)
#       define __IPHONE_8_4 80400
#   endif
#   if !defined(__IPHONE_9_0)
#       define __IPHONE_9_0 90000
#   endif
#endif

#ifdef __LSB_VERSION__
#   if __LSB_VERSION__ < 40
#       error "This version of the Linux Standard Base is unsupported"
#   endif
#endif

/* Compiler detect */
#if defined(__MINGW32__) || defined(__MINGW64__) /* MinGW compiler */
#   define CL_MINGW
#endif
#if defined(__CYGWIN__)
#   define CL_CYGWIN
#endif
#if defined(__BORLANDC__)
#   define CL_BC
#endif
#if defined(__GNUC__)
#   define CL_GCC
#endif
#if defined(_MSC_VER) && _MSC_VER > 0
#   define CL_VC
#endif

#endif // __SYSTEM_DETECTION_INL__
