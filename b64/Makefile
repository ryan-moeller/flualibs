SHLIB_NAME=	b64.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_b64.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	b64.3lua

.include <bsd.lib.mk>
