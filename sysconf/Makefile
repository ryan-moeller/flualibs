SHLIB_NAME=	sysconf.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_sysconf.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	sysconf.3lua

.include <bsd.lib.mk>
