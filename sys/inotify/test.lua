local inotify = require('sys.inotify')
local ucl = require('ucl')

local mask_names = {
	'ACCESS',
	'MODIFY',
	'ATTRIB',
	'CLOSE_WRITE',
	'CLOSE_NOWRITE',
	'CLOSE',
	'OPEN',
	'MOVED_FROM',
	'MOVED_TO',
	'MOVE',
	'CREATE',
	'DELETE',
	'DELETE_SELF',
	'MOVE_SELF',
	'IGNORED',
	'Q_OVERFLOW',
	'UNMOUNT',
}
local event_masks = {}
for _, name in ipairs(mask_names) do
	event_masks[inotify[name]] = name
end
local function event_mask_name(event)
	local mask = event.mask
	local name = event_masks[mask & inotify.ALL_EVENTS] or event_masks[mask]
	if mask & inotify.ISDIR ~= 0 then
		name = name .. '+ISDIR'
	end
	return name
end

local ifd <close> = assert(inotify.init())
local wd = assert(ifd:add_watch('/tmp', inotify.ALL_EVENTS))
local n = 0
repeat
	local events = assert(ifd:read())
	for _, event in ipairs(events) do
		event.mask_name = event_mask_name(event)
		print(ucl.to_json(event))
		assert(event.wd == wd)
		n = n + 1
	end
until n >= 10 -- arbitrary break point
ifd:close() -- ensure <close> doesn't error after :close()
