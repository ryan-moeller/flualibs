SRCTOP?= /usr/src
.export SRCTOP

SUBDIR= \
	fileno \
	libfetch \
	libifconfig \
	libkldstat \
	libkqueue \
	libmd \
	libmq \
	libroken \
	libxor \

.if exists(${SRCTOP}/contrib/bsddialog/lib/bsddialog.h)
SUBDIR+=libbsddialog
.endif
.if exists(/usr/include/libnvpair.h)
SUBDIR+=libbe
.endif

MANDOC_CMD=	sh ../manlint.sh

.include <bsd.subdir.mk>
