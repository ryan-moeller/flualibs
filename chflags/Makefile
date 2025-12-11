SHLIB_NAME=	chflags.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_chflags.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	chflags.3lua

.include <bsd.lib.mk>
