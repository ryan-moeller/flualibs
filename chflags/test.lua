local chflags = require('chflags')

assert(chflags.chflags('/COPYRIGHT', chflags.UF_ARCHIVE))
