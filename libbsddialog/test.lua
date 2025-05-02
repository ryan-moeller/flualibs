local ucl = require('ucl')
local bsddialog = require('bsddialog')

local function menuitem(fields)
	return {
		prefix = fields[1],
		on = fields[2],
		depth = fields[3],
		name = fields[4],
		desc = fields[5],
		bottomdesc = fields[6],
	}
end

bsddialog.init()

local conf = bsddialog.initconf()
conf.title = "menu"
conf.text.escape = true
local items = {
	menuitem{"I",   true,  0, "Name 1", "Desc 1", "Bottom Desc 1"},
	menuitem{"II",  false, 0, "Name 2", "Desc 2", "Bottom Desc 2"},
	menuitem{"III", true,  0, "Name 3", "Desc 3", "Bottom Desc 3"},
	menuitem{"IV",  false, 0, "Name 4", "Desc 4", "Bottom Desc 4"},
	menuitem{"V",   true,  0, "Name 5", "Desc 5", "Bottom Desc 5"},
}
res = table.pack(bsddialog.menu(conf, [[\ZsExample\ZS]], 15, 30, 5, items, 0))

theme = bsddialog.get_theme()

bsddialog._end()

print(table.unpack(res))
print(ucl.to_json(theme))
