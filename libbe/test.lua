be = require('be')
ucl = require('ucl')

handle = assert(be.init())
bootenvs = assert(handle:get_bootenv_props())
print(ucl.to_json(bootenvs))
