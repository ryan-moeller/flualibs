local fetch = require('fetch')

local server = 'http://localhost:8080'

local function field(name, value)
	return {name=name, value=value}
end

local function print_fields(prefix, fields)
	for _, field in ipairs(fields) do
		print(('>%s %s: %s'):format(prefix, field.name, field.value))
	end
end

local function skip_test_case(case)
	print('=== SKIP:', case.desc)
end

local function test_case(case)
	print('=== TEST:', case.desc)
	local f, stat, res_headers, res_trailers = assert(fetch.xrequest(
		server..case.path,
		case.method,
		case.flags,
		case.req_headers,
		case.req_trailers,
		case.res_headers,
		case.res_trailers,
		case.req_body
	))
	print('Response size:', stat.size)
	print('Response headers:', res_headers)
	if res_headers then
		print_fields('H', res_headers)
	end
	print('Response trailers (before body read):', res_trailers)
	if res_trailers then
		print_fields('T', res_trailers)
	end
	print('Response body:', f, ('%s'):format(type(f)))
	for line in f:lines() do
		io.write('>B ', line, '\n')
	end
	if case.test_trailers_before then
		print('Result of fetch.trailers(f) before f:close():')
		local t1 = fetch.trailers(f)
		print('t1:', t1)
		assert(t1 == res_trailers, 'fetch.trailers failed')
		local t1_len = type(t1) == 'table' and #t1 or 0
		local t2 = fetch.trailers(f)
		print('t2:', t2)
		local t2_len = type(t2) == 'table' and #t1 or 0
		assert(t2 == res_trailers, 'fetch.trailers unrepeatable')
		assert(t1_len == t2_len, 'fetch.trailers re-added fields')
	end
	f:close()
	print('Response trailers:', res_trailers)
	if res_trailers then
		print_fields('T', res_trailers)
	end
	if case.test_trailers_after then
		print('Result of fetch.trailers(f) after f:close():')
		local t1 = fetch.trailers(f)
		print('t1:', t1)
		assert(not t1, 'fetch.trailers failed after (1)')
		local t2 = fetch.trailers(f)
		print('t2:', t2)
		assert(not t2, 'fetch.trailers failed after (2)')
	end
	print('=== OK\n')
end

test_case {
	desc = '1. Basic POST with headers and body, no trailers expected',
	path = '/post',
	method = 'POST',
	req_headers = {
		field('X-Test-Header', 'LuaFetch'),
		field('Content-Type', 'text/plain'),
	},
	res_headers = {},
	res_trailers = {},
	req_body = 'Hello from Lua fetch.xrequest!',
}
test_case {
	desc = '2. GET with response trailers',
	path = '/trailers?foo=bar&baz=qux',
	method = 'GET',
	res_headers = {},
	res_trailers = {},
	test_trailers_before = true,
	test_trailers_after = true,
}
test_case {
	desc = '3. GET with response trailers (response fields ignored)',
	path = '/trailers?foo=bar&baz=qux',
	method = 'GET',
	test_trailers_before = true,
}
test_case {
	desc = '4. GET /trailers with no query (empty trailers)',
	path = '/trailers',
	method = 'GET',
	res_headers = {},
	res_trailers = {},
	test_trailers_before = true,
	test_trailers_after = true,
}
test_case {
	desc = '5. GET /get (no trailers, no res_trailers passed)',
	path = '/get',
	method = 'GET',
	res_headers = {},
	test_trailers_after = true,
}
test_case {
	desc = '6. Streaming request body with trailers',
	path = '/post',
	method = 'POST',
	req_headers = {
		field('Content-Type', 'text/plain'),
	},
	req_body = {
		read = function(self, _n)
			if not self.eof then
				self.eof = true
				return 'Streaming body!\n'
			end
			return ''
		end,
	},
	req_trailers = {
		field('X-Stream-Trailer', 'trailer'),
	},
	res_headers = {},
	res_trailers = {},
	test_trailers_after = true,
}
test_case {
	desc = '7. HEAD request, no body or trailers',
	path = '/headers',
	method = 'HEAD',
	res_headers = {},
	res_trailers = {},
	test_trailers_after = true,
}
test_case {
	desc = '8. Function as streaming request body (plain function)',
	path = '/post',
	method = 'POST',
	req_headers = {
		field('Content-Type', 'text/plain'),
	},
	req_body = (function()
		local chunks = {
			'chunk one\n',
			'chunk two\n',
			'chunk three\n',
		}
		local i = 0
		return function(_n)
			i = i + 1
			return chunks[i] or ''
		end
	end)(),
	res_headers = {},
	res_trailers = {},
	test_trailers_after = true,
}
test_case {
	desc = '9. Request body as table with read and seek methods',
	path = '/post',
	method = 'POST',
	req_headers = {
		field('Content-Type', 'text/plain'),
	},
	req_body = {
		data = 'Hello, seekable body!',
		pos = 1,
		read = function(self, n)
			local start_pos = self.pos
			local end_pos = math.min(self.pos + n - 1, #self.data)
			if start_pos > #self.data then
				return '' -- EOF
			end
			local chunk = self.data:sub(start_pos, end_pos)
			self.pos = end_pos + 1
			return chunk
		end,
		seek = function(self, whence, offset)
			if whence == 'set' then
				self.pos = offset + 1
			elseif whence == 'cur' then
				self.pos = self.pos + offset
			elseif whence == 'end' then
				self.pos = #self.data + offset + 1
			else
				return nil, 'invalid whence'
			end
			if self.pos < 1 then
				self.pos = 1
			end
			if self.pos > #self.data + 1 then
				self.pos = #self.data + 1
			end
			return self.pos - 1
		end,
	},
	res_headers = {},
	res_trailers = {},
	test_trailers_after = true,
}
