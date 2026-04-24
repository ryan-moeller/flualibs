SHLIB_NAME=	core.so
SRCS+=	lua_zfs_core.c
CFLAGS+=${ZFS_CFLAGS} ${FLUA_CFLAGS}
LDADD+=	-lnvpair -lpthread -lzfs_core -lspl
MAN=	zfs.core.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
