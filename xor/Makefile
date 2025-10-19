SHLIB_NAME=	xor.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_xor.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	xor.3lua

.include <bsd.lib.mk>
