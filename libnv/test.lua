local nv = require('nv')

local nvl = nv.create()

nvl:add_bool("simple", true)
nvl:add_string("hello", "world")
nvl:fdump(io.stdout)
