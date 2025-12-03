SHLIB_NAME=	dpv.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_dpv.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-ldpv

MAN=	dpv.3lua

.include <bsd.lib.mk>
