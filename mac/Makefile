SHLIB_NAME=	mac.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_mac.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	mac.3lua

.include <bsd.lib.mk>
