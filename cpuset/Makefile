SHLIB_NAME=	cpuset.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_cpuset.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	cpuset.3lua

.include <bsd.lib.mk>
