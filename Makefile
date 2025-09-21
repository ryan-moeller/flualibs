# NOTE: Must make cleandepend when changing SRCTOP between builds.
SRCTOP?= /usr/src
.export SRCTOP

SUBDIR= \
	b64 \
	fileno \
	kldstat \
	kqueue \
	libfetch \
	libifconfig \
	libmd \
	libmq \
	sendfile \
	sysctl \
	xor \

.if exists(${SRCTOP}/contrib/bsddialog/lib/bsddialog.h)
SUBDIR+=libbsddialog
.endif
.if exists(/usr/include/libnvpair.h)
SUBDIR+=libbe
.endif

MANDOC_CMD=	sh ../manlint.sh

.include <bsd.subdir.mk>
