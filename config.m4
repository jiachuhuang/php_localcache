dnl $Id$
dnl config.m4 for extension loc

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(loc, for loc support,
Make sure that the comment is aligned:
[  --with-loc             Include loc support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(loc, whether to enable loc support,
dnl Make sure that the comment is aligned:
dnl [  --enable-loc           Enable loc support])

if test "$PHP_LOC" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-loc -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/loc.h"  # you most likely want to change this
  dnl if test -r $PHP_LOC/$SEARCH_FOR; then # path given as parameter
  dnl   LOC_DIR=$PHP_LOC
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for loc files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       LOC_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$LOC_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the loc distribution])
  dnl fi

  dnl # --with-loc -> add include path
  PHP_ADD_INCLUDE($LOC_DIR/c_cache)

  dnl # --with-loc -> check for lib and symbol presence
  dnl LIBNAME=loc # you may want to change this
  dnl LIBSYMBOL=loc # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LOC_DIR/$PHP_LIBDIR, LOC_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_LOCLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong loc lib version or lib not found])
  dnl ],[
  dnl   -L$LOC_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(LOC_SHARED_LIBADD)

  LOC_FILES="loc.c c_cache/c_shared_allocator.c c_cache/c_shared_mmap.c c_cache/c_storage.c"

  PHP_NEW_EXTENSION(loc, ${LOC_FILES}, $ext_shared)
fi
