SHLIB_NAME=	stat.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_stat.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	stat.3lua

.include <bsd.lib.mk>
