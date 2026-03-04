local util = require('util')

local quantity = '123456789K'
local number = assert(util.expand_number(quantity))
print("number:", number)
local human = assert(util.humanize_number(#quantity, number, "",
    util.HN_AUTOSCALE, util.HN_NOSPACE))
print("human:", human)
assert(quantity == human)
