SHLIB_NAME=	bsddialog.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_bsddialog.c

CFLAGS+= \
	-I${SRCTOP}/contrib/bsddialog/lib \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lprivatebsddialog

MAN=	bsddialog.3lua

.include <bsd.lib.mk>
