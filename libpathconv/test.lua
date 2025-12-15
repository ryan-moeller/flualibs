local pathconv = require('pathconv')
local abs2rel = pathconv.abs2rel
local rel2abs = pathconv.rel2abs

local sysdir = '/usr/src/sys'

assert(abs2rel(sysdir, '/usr/local/lib') == '../../src/sys')
assert(rel2abs('../../src/sys', '/usr/local/lib') == sysdir)

assert(abs2rel(sysdir, '/usr') == 'src/sys')
assert(rel2abs('src/sys', '/usr') == sysdir)

assert(abs2rel(sysdir, sysdir) == '.')
assert(rel2abs('.', sysdir) == sysdir)
