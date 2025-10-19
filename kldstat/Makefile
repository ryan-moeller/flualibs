SHLIB_NAME=	kldstat.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_kldstat.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	kldstat.3lua

.include <bsd.lib.mk>
