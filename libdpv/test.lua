local dpv = require('dpv')

assert(dpv({options=dpv.options.TEST_MODE}, {
	{name="null", path="/dev/null"},
	{name="zero", path="/dev/zero"},
}))
