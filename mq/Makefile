
SHLIB_NAME=	mq.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_mq.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lrt

# XXX: not perfect, but works for the CI
.if !exists(/usr/share/man/man4/mqueuefs.4.gz)
mantransform: .PHONY
	sed -i '' -e 's/mqueuefs 4/mqueuefs 5/' ${MAN}
all-man: mantransform
manlint: mantransform
.endif

MAN=	mq.3lua

.include <bsd.lib.mk>
