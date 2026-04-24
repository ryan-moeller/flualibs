DIRS=	FLUA_ZFS
FLUA_ZFS=	${LIBDIR}/flua/zfs

.include <bsd.dirs.mk>

SUBDIR=	core

.include <bsd.subdir.mk>

.if 0
SHLIB_NAME=	zfs.so
SRCS+=	lua_zfs.c
CFLAGS+=${ZFS_CFLAGS} ${FLUA_CFLAGS}
LDADD+=	-lnvpair -lpthread -lzfs_core -lzfs -lspl
MAN=	zfs.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
.endif
