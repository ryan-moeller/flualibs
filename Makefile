SRCTOP?= /usr/src
.export SRCTOP

SUBDIR= \
	fileno \
	libbsddialog \
	libfetch \
	libkldstat \
	libkqueue \
	libmd \
	libmq \
	libroken \
	libxor \

.if exists(/usr/include/libnvpair.h)
SUBDIR+=libbe
.endif

MANDOC_CMD=	sh ../manlint.sh

.include <bsd.subdir.mk>
