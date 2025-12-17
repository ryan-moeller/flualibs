local extattr = require('sys.extattr')

local ns = extattr.NAMESPACE_USER
local name = "test"
local value = "Hello, FreeBSD!"

local f <close> = io.tmpfile()
assert(extattr.set(f, ns, name, value))
local names = assert(extattr.list(f, ns))
assert(#names == 1)
assert(names[1] == name)
local v = assert(extattr.get(f, ns, name))
assert(v == value)
assert(extattr.delete(f, ns, name))
