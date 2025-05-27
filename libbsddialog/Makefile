SHLIB_NAME=	bsddialog.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_bsddialog.c

CFLAGS+= \
	-I${SRCTOP}/contrib/bsddialog/lib \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

HEADER=	${SRCTOP}/contrib/bsddialog/lib/bsddialog.h
VER!=	awk -F'"' '/LIBBSDDIALOG_VERSION/ { print $$2 }' ${HEADER}
CFLAGS+=	-DLIBBSDDIALOG_VERSION_MAJOR=${VER:S/./ /:[1]}

LDADD+=	-lprivatebsddialog

MAN=	bsddialog.3lua

.include <bsd.lib.mk>
