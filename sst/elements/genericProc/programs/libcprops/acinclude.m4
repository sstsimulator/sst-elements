AC_DEFUN([CP_CHECK_PTHREADMUTEXRECURSIVE], [
AC_MSG_CHECKING([whether PTHREAD_MUTEX_RECURSIVE is supported])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define GNU_SOURCE
#include <pthread.h>

int main(void)
{
int rc;
pthread_mutex_t m;
pthread_mutexattr_t a;

if ((rc = pthread_mutexattr_init(&a))) return rc;
if ((rc = pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE)))
return rc;

if ((rc = pthread_mutex_init(&m, &a))) return rc;
if ((rc = pthread_mutex_lock(&m))) return rc;
if ((rc = pthread_mutex_trylock(&m))) return rc;
pthread_mutex_unlock(&m);
pthread_mutex_unlock(&m);
return 0;
}
]])],[recursive_mutex=yes
AC_MSG_RESULT(yes)
AC_DEFINE([CP_HAS_PTHREAD_MUTEX_RECURSIVE], [1], [define if PTHREAD_MUTEX_RECURSIVE is supported])
], [
AC_MSG_RESULT(no)
],[
AC_MSG_RESULT(assumed no)
])
AS_IF([test "x$recursive_mutex" = "x"], [
AC_MSG_CHECKING([whether PTHREAD_MUTEX_RECURSIVE_NP is supported])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define GNU_SOURCE
#include <pthread.h>

int main(void) 
{
    int rc;
    pthread_mutex_t m;
    pthread_mutexattr_t a;

    if ((rc = pthread_mutexattr_init(&a))) return rc;
    if ((rc = pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE_NP)))
        return rc;

    if ((rc = pthread_mutex_init(&m, &a))) return rc;
    if ((rc = pthread_mutex_lock(&m))) return rc;
    if ((rc = pthread_mutex_trylock(&m))) return rc;
    pthread_mutex_unlock(&m);
    pthread_mutex_unlock(&m);

    return 0;
}
]])],[
recursive_mutex=yes
AC_MSG_RESULT(yes)
AC_DEFINE([CP_HAS_PTHREAD_MUTEX_RECURSIVE_NP], [1], [define if PTHREAD_MUTEX_RECURSIVE_NP is supported])
],[
AC_MSG_RESULT(no)
],[
AC_MSG_RESULT(assumed no)
])
])
])


AC_DEFUN([AX_FUNC_WHICH_GETHOSTBYNAME_R], [

    AC_LANG_PUSH(C)
    AC_MSG_CHECKING([how many arguments gethostbyname_r() takes])

    AC_CACHE_VAL(ac_cv_func_which_gethostbyname_r, [

################################################################

ac_cv_func_which_gethostbyname_r=unknown

#
# ONE ARGUMENT (sanity check)
#

# This should fail, as there is no variant of gethostbyname_r() that takes
# a single argument. If it actually compiles, then we can assume that
# netdb.h is not declaring the function, and the compiler is thereby
# assuming an implicit prototype. In which case, we're out of luck.
#
AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            (void)gethostbyname_r(name) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=no)

#
# SIX ARGUMENTS
# (e.g. Linux)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            struct hostent ret, *retp;
            char buf@<:@1024@:>@;
            int buflen = 1024;
            int my_h_errno;
            (void)gethostbyname_r(name, &ret, buf, buflen, &retp, &my_h_errno) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=six)

fi

#
# FIVE ARGUMENTS
# (e.g. Solaris)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            struct hostent ret;
            char buf@<:@1024@:>@;
            int buflen = 1024;
            int my_h_errno;
            (void)gethostbyname_r(name, &ret, buf, buflen, &my_h_errno) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=five)

fi

#
# THREE ARGUMENTS
# (e.g. AIX, HP-UX, Tru64)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            struct hostent ret;
            struct hostent_data data;
            (void)gethostbyname_r(name, &ret, &data) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=three)

fi

################################################################

]) dnl end AC_CACHE_VAL

case "$ac_cv_func_which_gethostbyname_r" in
    three)
    AC_MSG_RESULT([three])
    AC_DEFINE([HAVE_FUNC_GETHOSTBYNAME_R_3],[1],[define if gethostbyname takes 3 arguments])
    ;;

    five)
    AC_MSG_RESULT([five])
    AC_DEFINE([HAVE_FUNC_GETHOSTBYNAME_R_5],[1],[define if gethostbyname takes 5 arguments])
    ;;

    six)
    AC_MSG_RESULT([six])
    AC_DEFINE([HAVE_FUNC_GETHOSTBYNAME_R_6],[1],[define if gethostbyname takes 6 arguments])
    ;;

    no)
    AC_MSG_RESULT([cannot find function declaration in netdb.h])
    ;;

    unknown)
    AC_MSG_RESULT([can't tell])
    ;;

    *)
    AC_MSG_ERROR([internal error])
    ;;
esac

AC_LANG_POP(C)

]) dnl end AC_DEFUN


AC_DEFUN([CP_CHECK_SSL],
[AC_MSG_CHECKING(if ssl is wanted)
AC_ARG_WITH(ssl,
[  --with-ssl enable ssl [will check /usr/local/ssl
                            /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr ]
],
[   AC_MSG_RESULT(yes)
    for dir in $withval /usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr; do
        ssldir="$dir"
        if test -f "$dir/include/openssl/ssl.h"; then
            found_ssl="yes";
            CFLAGS="$CFLAGS -I$ssldir/include/openssl -DHAVE_SSL";
            CXXFLAGS="$CXXFLAGS -I$ssldir/include/openssl -DHAVE_SSL";
            break;
        fi
        if test -f "$dir/include/ssl.h"; then
            found_ssl="yes";
            CFLAGS="$CFLAGS -I$ssldir/include/ -DHAVE_SSL";
            CXXFLAGS="$CXXFLAGS -I$ssldir/include/ -DHAVE_SSL";
            break
        fi
    done
    if test x_$found_ssl != x_yes; then
        AC_MSG_ERROR(Cannot find ssl libraries)
    else
        printf "OpenSSL found in $ssldir\n";
        LIBS="$LIBS -lssl -lcrypto";
        LDFLAGS="$LDFLAGS -L$ssldir/lib";
        HAVE_SSL=yes
    fi
    AC_SUBST(HAVE_SSL)
],
[
    AC_MSG_RESULT(no)
])
])dnl


dnl License
dnl Copyright 2008 Mateusz Loskot <mateusz@loskot.net>
dnl Copying and distribution of this macro, with or without modification, are permitted in any medium without royalty provided the copyright notice and this notice are preserved.
AC_DEFUN([AX_LIB_MYSQL],
[
    AC_ARG_WITH([mysql],
        [AC_HELP_STRING([--with-mysql=@<:@ARG@:>@],
            	        [use MySQL client library @<:@default=yes@:>@, optionally specify path to mysql_config])],
        [AS_IF([test "$withval" = "no"],
               [want_mysql="no"],[
	 	 AS_IF([test "$withval" = "yes"],
               [want_mysql="yes"],[
		 AS_IF([test -d "$withval"],
               [want_mysql="yes"
			    MYSQL_CONFIG="$withval"],
			   [want_mysql="maybe"])])])
		], [want_mysql="maybe"]
    )

    MYSQL_CFLAGS=""
    MYSQL_LDFLAGS=""
    MYSQL_VERSION=""

    dnl
    dnl Check MySQL libraries (libpq)
    dnl

    AS_IF([test "$want_mysql" = "yes" -o "$want_postgresql" = "maybe"],[
        AS_IF([test -z "$MYSQL_CONFIG" -o test],
              [AC_PATH_PROG([MYSQL_CONFIG], [mysql_config], [no])])

		AS_IF([test ! -x "$MYSQL_CONFIG"], [
			AS_IF([test "$want_mysql" = "yes"],
				  [AC_MSG_ERROR([$MYSQL_CONFIG does not exist or is not an executable])])
			MYSQL_CONFIG="no"
			found_mysql="no"])

        AS_IF([test "$MYSQL_CONFIG" != "no"],[
            MYSQL_CFLAGS="`$MYSQL_CONFIG --cflags`"
            MYSQL_LDFLAGS="`$MYSQL_CONFIG --libs`"
            MYSQL_VERSION=`$MYSQL_CONFIG --version`

            AC_DEFINE([HAVE_MYSQL], [1],
                [Define to 1 if MySQL libraries are available])

            found_mysql="yes"
			],[
            found_mysql="no"
			])
    ])

    dnl
    dnl Check if required version of MySQL is available
    dnl


    mysql_version_req=ifelse([$1], [], [], [$1])

    AS_IF([test "$found_mysql" = "yes" -a -n "$mysql_version_req"],[
        AC_MSG_CHECKING([if MySQL version is >= $mysql_version_req])

        dnl Decompose required version string of MySQL
        dnl and calculate its number representation
        mysql_version_req_major=`expr $mysql_version_req : '\([[0-9]]*\)'`
        mysql_version_req_minor=`expr $mysql_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_req_micro=`expr $mysql_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$mysql_version_req_micro" = "x"; then
            mysql_version_req_micro="0"
        fi

        mysql_version_req_number=`expr $mysql_version_req_major \* 1000000 \
                                   \+ $mysql_version_req_minor \* 1000 \
                                   \+ $mysql_version_req_micro`

        dnl Decompose version string of installed MySQL
        dnl and calculate its number representation
        mysql_version_major=`expr $MYSQL_VERSION : '\([[0-9]]*\)'`
        mysql_version_minor=`expr $MYSQL_VERSION : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_micro=`expr $MYSQL_VERSION : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        AS_IF([test "x$mysql_version_micro" = "x"],
              [mysql_version_micro="0"])

        mysql_version_number=`expr $mysql_version_major \* 1000000 \
                                   \+ $mysql_version_minor \* 1000 \
                                   \+ $mysql_version_micro`

        mysql_version_check=`expr $mysql_version_number \>\= $mysql_version_req_number`
        AS_IF([test "$mysql_version_check" = "1"],
              [AC_MSG_RESULT([yes])],
              [AC_MSG_RESULT([no])])
    ])

    AC_SUBST([MYSQL_VERSION])
    AC_SUBST([MYSQL_CFLAGS])
    AC_SUBST([MYSQL_LDFLAGS])
])


dnl License
dnl Copyright 2008 Mateusz Loskot <mateusz@loskot.net>
dnl Copying and distribution of this macro, with or without modification, are permitted in any medium without royalty provided the copyright notice and this notice are preserved.
AC_DEFUN([AX_LIB_POSTGRESQL],
[
    AC_ARG_WITH([postgresql],
        AC_HELP_STRING([--with-postgresql=@<:@ARG@:>@],
            [use PostgreSQL library @<:@default=yes@:>@, optionally specify path to pg_config]
        ),
        [
        AS_IF([test "$withval" = "no"],
			  [want_postgresql="no"],[
        AS_IF([test "$withval" = "yes"],
              [want_postgresql="yes"],[
		AS_IF([test -d "$withval"],
			  [want_postgresql="yes"
			   PG_CONFIG="$withval"],
			  [want_postgresql="maybe"])])])
        ], [want_postgresql="maybe"]
    )

    POSTGRESQL_CFLAGS=""
    POSTGRESQL_LDFLAGS=""
    POSTGRESQL_VERSION=""

    dnl
    dnl Check PostgreSQL libraries (libpq)
    dnl

    AS_IF([test "$want_postgresql" = "yes" -o "$want_postgresql" = "maybe"],[

        AS_IF([test -z "$PG_CONFIG" -o test],
              [AC_PATH_PROG([PG_CONFIG], [pg_config], [])])

        AS_IF([test ! -x "$PG_CONFIG"], [
            AS_IF([test "$want_postgresql" = "yes"],
				  [AC_MSG_ERROR([$PG_CONFIG does not exist or it is not an exectuable file])])
            PG_CONFIG="no"
            found_postgresql="no"])

        AS_IF([test "$PG_CONFIG" != "no"],[
            POSTGRESQL_CFLAGS="-I`$PG_CONFIG --includedir`"
            POSTGRESQL_LDFLAGS="-L`$PG_CONFIG --libdir` -lpq"

            POSTGRESQL_VERSION=`$PG_CONFIG --version | sed -e 's#PostgreSQL ##'`

            AC_DEFINE([HAVE_POSTGRESQL], [1],
                [Define to 1 if PostgreSQL libraries are available])

            found_postgresql="yes"
			],[
            found_postgresql="no"
			])
    ])

    dnl
    dnl Check if required version of PostgreSQL is available
    dnl


    postgresql_version_req=ifelse([$1], [], [], [$1])

    AS_IF([test "$found_postgresql" = "yes" -a -n "$postgresql_version_req"],[
        AC_MSG_CHECKING([if PostgreSQL version is >= $postgresql_version_req])

        dnl Decompose required version string of PostgreSQL
        dnl and calculate its number representation
        postgresql_version_req_major=`expr $postgresql_version_req : '\([[0-9]]*\)'`
        postgresql_version_req_minor=`expr $postgresql_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        postgresql_version_req_micro=`expr $postgresql_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        AS_IF([test "x$postgresql_version_req_micro" = "x"],
              [postgresql_version_req_micro="0"])

        postgresql_version_req_number=`expr $postgresql_version_req_major \* 1000000 \
                                   \+ $postgresql_version_req_minor \* 1000 \
                                   \+ $postgresql_version_req_micro`

        dnl Decompose version string of installed PostgreSQL
        dnl and calculate its number representation
        postgresql_version_major=`expr $POSTGRESQL_VERSION : '\([[0-9]]*\)'`
        postgresql_version_minor=`expr $POSTGRESQL_VERSION : '[[0-9]]*\.\([[0-9]]*\)'`
        postgresql_version_micro=`expr $POSTGRESQL_VERSION : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        AS_IF([test "x$postgresql_version_micro" = "x"],
              [postgresql_version_micro="0"])

        postgresql_version_number=`expr $postgresql_version_major \* 1000000 \
                                   \+ $postgresql_version_minor \* 1000 \
                                   \+ $postgresql_version_micro`

        postgresql_version_check=`expr $postgresql_version_number \>\= $postgresql_version_req_number`
        AS_IF([test "$postgresql_version_check" = "1"],
              [AC_MSG_RESULT([yes])],
              [AC_MSG_RESULT([no])])
    ])

    AC_SUBST([POSTGRESQL_VERSION])
    AC_SUBST([POSTGRESQL_CFLAGS])
    AC_SUBST([POSTGRESQL_LDFLAGS])
])

dnl ----------------------------------------------------------- -*- autoconf -*-
dnl Licensed to the Apache Software Foundation (ASF) under one or more
dnl contributor license agreements.  The ASF licenses this file to You
dnl under the Apache License, Version 2.0 (the "License"); you may not
dnl use this file except in compliance with the License. You may obtain a copy of the License at
dnl
dnl    http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and limitations under the License.
dnl
dnl check_addrinfo.m4 - checks support for addrinfo structure
dnl
dnl This macro determines if the platform supports the addrinfo structure.
dnl If this platform supports addrinfo, defines HAVE_STRUCT_ADDRINFO.
dnl

AC_DEFUN([CP_CHECK_ADDRINFO], [
	AC_MSG_CHECKING([whether struct addrinfo is defined])
	AC_TRY_COMPILE(
		[ #include <stdio.h>
		  #ifdef HAVE_UNISTD_H
		  # include <unistd.h>
		  #endif
		  #include <sys/types.h>
		  #include <sys/socket.h>
		  #include <netdb.h>
		],
		[
		  do {
		    struct addrinfo a;
			(void) a.ai_flags;
		  } while(0)
		],
		[
		  AC_MSG_RESULT(yes)
		  AC_DEFINE([HAVE_STRUCT_ADDRINFO], [1], [define if you have struct addrinfo])
		],
		[
		  AC_MSG_RESULT(no)
		])
])
