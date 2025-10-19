SHLIB_NAME=	sysctl.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_sysctl.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	sysctl.3lua

.include <bsd.lib.mk>
