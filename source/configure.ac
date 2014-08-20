#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([skypemorph], [0.1], [http://uwaterloo.ca])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/Packetizer.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_INSTALL

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h netdb.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memset select socket])

# Checks for cyassl headers
CYASSL_CPPFLAGS=
AC_ARG_WITH([cyassl-include],
            [AS_HELP_STRING([--with-cyassl-include=INCLUDE_DIR],
              [specify cyassl header location])],
            [CYASSL_CPPFLAGS="-I$withval/cyassl -I$withval"],
            [AC_MSG_ERROR([no cyassl include path given])])

OLD_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $CYASSL_CPPFLAGS"

AC_CHECK_HEADER([cyassl/ctaocrypt/random.h], 
                [AM_CPPFLAGS="$AM_CPPFLAGS $CYASSL_CPPFLAGS"],
                [AC_MSG_ERROR([cyassl/ctaocrypt/random.h cannot be found])], 
                [])
AC_CHECK_HEADER([cyassl/ctaocrypt/arc4.h], 
                [AM_CPPFLAGS="$AM_CPPFLAGS $CYASSL_CPPFLAGS"],
                [AC_MSG_ERROR([cyassl/ctaocrypt/arc4.h cannot be found])], 
                [])
AC_CHECK_HEADER([cyassl/ctaocrypt/aes.h], 
                [AM_CPPFLAGS="$AM_CPPFLAGS $CYASSL_CPPFLAGS"],
                [AC_MSG_ERROR([cyassl/ctaocrypt/aes.h cannot be found])], 
                [])

CPPFLAGS=$OLD_CPPFLAGS
AC_SUBST([CYASSL_CPPFLAGS])

# Checks for libraries.
CYASSL_LDFLAGS=
AC_ARG_WITH([cyassl-lib],
            [AS_HELP_STRING([--with-cyassl-lib=LIB_DIR],
              [specify cyassl lib location])],
            [CYASSL_LDFLAGS=-L"$withval"],
            [AC_MSG_ERROR([no cyassl lib path given])])
AC_SUBST([CYASSL_LDFLAGS])

OLD_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS $CYASSL_LDFLAGS"
OLD_LIBS=$LIBS
LIBS=
AC_CHECK_LIB([cyassl], [AesCtrEncrypt], [], [AC_MSG_ERROR(libcyassl with AES CTR mode support was not found\
 Please use the latest git repo https://github.com/cyassl/cyassl.git to get AES CTR support)])
CYASSL_LIBS=$LIBS
LIBS=$OLD_LIBS
LDFLAGS=$OLD_LDFLAGS
AC_SUBST([CYASSL_LIBS])

# C++ from now on 
# so push C++ as the default language  

AC_LANG_SAVE

AC_LANG_PUSH([C++])

AC_ARG_WITH([public-api], AS_HELP_STRING([--with-public-api], [Building with Skype public api]))
AC_ARG_WITH([tproxy], AS_HELP_STRING([--with-tproxy], [Building with TProxy (needs root level access)]))
AC_ARG_WITH([openwrt], AS_HELP_STRING([--with-openwrt], [Building for OpenWRT platforms]))



AS_IF([test "x$with_openwrt" = "xyes"],
	 [EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DOPENWRT_ENABLED"] 
)

AS_IF([test "x$with_tproxy" = "xyes"],
        [TPROXY_CPPFLAGS="-DTPROXY_ENABLED"
	EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS $TPROXY_CPPFLAGS"]
)



AM_CONDITIONAL(SKYPE_PAPI, [test "x$with_public_api" = "xyes"])
case $host in
    *mingw32* | *cygwin* )
    
    if test "x$with_public_api" != "xyes"; then
		AC_MSG_ERROR([with-public-api option required for cross comilation])]
	fi

    if test "x$with_openwrt" != "xyes"; then
		AC_MSG_ERROR([with-openwrt option required for cross comilation])]
	fi

	EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DSKYPE_PAPI"
	# Define Windows version
	WINVER="-D_WIN32_WINNT=0x0501"
	CPPFLAGS="$CPPFLAGS $WINVER"	
	
	WIN32_LIBS="-lws2_32 -lmswsock"
	AC_SUBST([WIN32_LIBS])
	
	 # Add boost include directory
	BOOST_CPPFLAGS=
	AC_ARG_WITH([boost-include],
            [AS_HELP_STRING([--with-boost-include=INCLUDE_DIR],
              [specify boost header location])],
            [BOOST_CPPFLAGS="-I$withval"],
            [AC_MSG_ERROR([boost include path is required for a cross compilation])])
    AC_SUBST([BOOST_CPPFLAGS])        
    
    # Checks for libraries.
	BOOST_LDFLAGS=
	AC_ARG_WITH([boost-lib],
				[AS_HELP_STRING([--with-boost-lib=LIB_DIR],
				[specify boost lib location])],
				[BOOST_LDFLAGS=-L"$withval"],
				[AC_MSG_ERROR([boost library path is required for a cross compilation])])
	AC_SUBST([BOOST_LDFLAGS])
            
	BOOST_LIBS="-lboost_system-mt -lboost_thread_win32-mt"
	AC_SUBST([BOOST_LIBS])
	;;
	*)
	AS_IF([test "x$with_public_api" = "xyes"],
		[PKG_CHECK_MODULES([GLIB], [glib-2.0]) 
		PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1]) 
		EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DSKYPE_PAPI"] 
	)
	PAPI_LIBS="$GLIB_LIBS $DBUS_GLIB_LIBS"
	AC_SUBST([PAPI_LIBS])
	PAPI_CFLAGS="$GLIB_CFLAGS $DBUS_GLIB_CFLAGS"
	AC_SUBST([PAPI_CFLAGS])
	
	# Check for boost
	m4_include([m4/ax_boost_base.m4])
	m4_include([m4/ax_boost_asio.m4])
	m4_include([m4/ax_boost_system.m4])
	m4_include([m4/ax_boost_thread.m4])
	
	
	AX_BOOST_BASE([1.35.0], 
				[], 
				[AC_MSG_ERROR([boost library cannot be found])])

	AX_BOOST_ASIO
	AX_BOOST_SYSTEM
	AX_BOOST_THREAD
esac




if test "x$with_tproxy" = "xyes"; then
	if test "x$with_openwrt" = "xyes"; then
		AC_MSG_ERROR([Cannot use tproxy and openwrt options at the same time])
	fi
fi

AS_IF([test "x$with_tproxy" = "xyes"],
        [TPROXY_CPPFLAGS="-DTPROXY_ENABLED"
	EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS $TPROXY_CPPFLAGS"]
)

AM_CONDITIONAL(TPROXY_ENABLED, [test "x$with_tproxy" = "xyes"])
AC_SUBST([TPROXY_CPPFLAGS])

#For PKG_CHECK_MODULES to work proparly one needs to call `aclocal -l $PKG_DIR` where PKG_DIR is the directory of pkg.m4
#For more info see http://myitcorner.com/blog/?p=207





# Check for TR1 or C++11 features 
AC_CHECK_HEADER(tr1/unordered_map,[EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DHAVE_CXXTR1"],[])
AC_CHECK_HEADER(unordered_map,[EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DHAVE_CXX11"],[])

AC_SUBST([EXTRA_CPPFLAGS])







#SkypeKit section
if test "x$with_public_api" != "xyes"; then
# Checks for skype headers
	SKYPEKIT_TREE=
	AC_ARG_WITH([skypekit-tree],
        	    [AS_HELP_STRING([--with-skypekit-tree],
              	[specify skypekit tree location])],
            	[SKYPEKIT_TREE=$withval],
            	[AC_MSG_ERROR([no skypekit tree path given])])
	SKYPE_CPPFLAGS="-I$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/src/api \
	 -I$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/src/ipc \
	 -I$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/src/types \
	 -I$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/src/platform/threading \
	 -I$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/src/platform/transport \
	 -I$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/src/platform/transport/tcp \
	 -I$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/src/platform/threading/pthread \
	 -I$SKYPEKIT_TREE/ipc/cpp \
	 -I$SKYPEKIT_TREE/ipc/cpp/platform/se \
	 -I$SKYPEKIT_TREE/ipc/cpp/ssl/cyassl/include \
	 -I$SKYPEKIT_TREE/ipc/cpp/ssl/cyassl/ctaocrypt/include \
	 -I$SKYPEKIT_TREE/ipc/cpp/ssl/cyassl/include/openssl"
	AC_SUBST([SKYPE_CPPFLAGS])

OLD_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $SKYPE_CPPFLAGS"
AC_CHECK_HEADER([skype-embedded_2.h], 
                [AM_CPPFLAGS="$AM_CPPFLAGS $SKYPE_CPPFLAGS"],
                [AC_MSG_ERROR([skype-embedded_2.h cannot be found])], 
                [])
CPPFLAGS=$OLD_CPPFLAGS


# Checks for libraries.
SKYPE_LDFLAGS=
AC_ARG_WITH([skype-lib],
            [AS_HELP_STRING([--with-skype-lib],
              [specify skype lib location])],
            [SKYPE_LDFLAGS=-L"$withval"],
            [AC_MSG_NOTICE([no skype lib path given, use the default location])
            SKYPE_LDFLAGS=-L"$SKYPEKIT_TREE/interfaces/skype/cpp_embedded/build"])

SKYPE_LIBS=

AC_SUBST([SKYPE_LDFLAGS])
AC_SUBST([SKYPE_LIBS])

fi



AC_LANG_POP([C++])

AC_LANG_RESTORE

# end of C++ checks


DEBUG_CFLAGS="-g -DDEBUG"
AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug], [Enable debugging support (default: enabled)])],
    [ if test "x$enableval" != "xno"; then
        AM_CFLAGS="$DEBUG_CFLAGS $AM_CFLAGS"
      else
        AM_CFLAGS="$AM_CFLAGS -DNDEBUG"
      fi],
    [AM_CFLAGS="-DNDEBUG"]
    )
AC_SUBST([AM_CFLAGS])

EXTRA_CFLAGS=

# Various GCC warnings that should never fire for release quality code
GCCWARNINGS="-Wall -fno-strict-aliasing -W -Wfloat-equal -Wundef           \
  -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wchar-subscripts      \
  -Wcomment -Wformat=2 -Wwrite-strings -Wmissing-declarations              \
  -Wredundant-decls -Wswitch-enum -Winit-self -Wmissing-field-initializers \
  -Waddress -Wmissing-noreturn -Wnormalized=id -Wstrict-overflow=1         \
  -Wextra -Warray-bounds -Wstack-protector -Wformat -Wformat-security      \
  -Wswitch-default"



AC_ARG_ENABLE(gcc-lots-o-warnings,
   [AS_HELP_STRING([--enable-gcc-lots-o-warnings], 
                   [Enable lots of gcc warnings (default: disabled)])], 
   [ if test "x$enableval" != "xno"; then
       EXTRA_CFLAGS="$EXTRA_CFLAGS $GCCWARNINGS"
     else
       EXTRA_CFLAGS=$EXTRA_CFLAGS
     fi
   ])


AC_ARG_ENABLE(gcc-hardening,
   [AS_HELP_STRING([--enable-gcc-hardening], [Enable compiler security checks (default: disabled)])], 
   [ if test "x$enableval" != "xno"; then
       EXTRA_CFLAGS="$EXTRA_CFLAGS -D_FORTIFY_SOURCE=2 -fstack-protector-all"
       EXTRA_CFLAGS="$EXTRA_CFLAGS -fwrapv -fPIE -Wstack-protector"
       EXTRA_CFLAGS="$EXTRA_CFLAGS --param ssp-buffer-size=1"
       LDFLAGS="$LDFLAGS -pie"
     fi
   ])
AC_SUBST([EXTRA_CFLAGS])

AC_ARG_ENABLE(linker-hardening,
   [AS_HELP_STRING([--enable-linker-hardening], [Enable linker security fixups (default: disabled)])], 
   [ if test "x$enableval" != "xno"; then
       LDFLAGS="$LDFLAGS -z relro -z now"
     fi
   ])

AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_CONFIG_FILES([Makefile
 src/Makefile])
AC_OUTPUT
