SHLIB_NAME=	bsddialog.so
SRCS+=	lua_bsddialog.c
CFLAGS+=${FLUA_CFLAGS}
LDADD+=	-lprivatebsddialog
MAN=	bsddialog.3lua

.include "../Makefile.inc"

CFLAGS+=-I${SRCTOP}/contrib/bsddialog/lib
HEADER=	${SRCTOP}/contrib/bsddialog/lib/bsddialog.h
VER!=	awk -F'"' '/LIBBSDDIALOG_VERSION/ { print $$2 }' ${HEADER}
CFLAGS+=-DLIBBSDDIALOG_VERSION_MAJOR=${VER:S/./ /:[1]}

.include <bsd.lib.mk>
