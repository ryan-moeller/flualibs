-- Test server for HTTP/1.1
--
-- Invoked by socat via test-server.sh

local httpd <const> = require('httpd')

local server <const> = httpd.create_server()

local function body_parts(req)
	local parts <const> = {
		('Method: %s\n'):format(req.method),
		('Path: %s\n'):format(req.path),
	}
	local function dump_fields(section)
		for name, field in pairs(section) do
			for _, value in ipairs(field.raw) do
				table.insert(parts,
				    ('\t%s: %s\n'):format(name, value))
			end
		end
	end
	table.insert(parts, 'Params:\n')
	for param, values in pairs(req.params) do
		table.insert(parts,
		    ('\t%s: %s\n'):format(param, table.concat(values, ', ')))
	end
	table.insert(parts, 'Headers:\n')
	dump_fields(req.headers)
	if type(req.body) == 'function' then
		table.insert(parts, 'Body: [[\n')
		for chunk in req.body() do
			table.insert(parts, chunk)
		end
		table.insert(parts, ']]\n')
		table.insert(parts, 'Trailers:\n')
		dump_fields(req.trailers)
	elseif req.body then
		table.insert(parts, ('Body: [[%s]]\n'):format(req.body))
	end
	return parts
end

server:add_route('GET', '^/get$', function(req)
	local parts <const> = body_parts(req)
	return {
		status = 200,
		reason = 'OK',
		headers = {
			['Content-Type'] = 'text/plain',
			['X-Test-Endpoint'] = req.path,
		},
		body = table.concat(parts),
	}
end)

server:add_route('GET', '^/trailers$', function(req)
	local parts <const> = body_parts(req)
	local trailer_names <const> = {}
	for name in pairs(req.params) do
		table.insert(trailer_names, name)
	end
	return {
		status = 200,
		reason = 'OK',
		headers = {
			['Content-Type'] = 'text/plain',
			['X-Test-Endpoint'] = req.path,
			['Transfer-Encoding'] = 'chunked',
			['Trailer'] = table.concat(trailer_names, ','),
		},
		body = function(sock)
			for _, part in ipairs(parts) do
				httpd.write_chunk(sock, part)
			end
			httpd.write_trailers(sock, req.params)
		end,
	}
end)

server:add_route('POST', '^/post$', function(req)
	local parts <const> = body_parts(req)
	return {
		status = 201,
		reason = 'Created',
		headers = {
			['Content-Type'] = 'text/plain',
			['X-Test-Endpoint'] = req.path,
		},
		body = table.concat(parts),
	}
end)

server:add_route('HEAD', '^/headers$', function(req)
	local headers <const> = {
		['Content-Type'] = 'text/plain',
		['X-Test-Endpoint'] = req.path,
	}
	for name, values in pairs(req.params) do
		-- Skip headers we're already using.
		if not headers[name] then
			headers[name] = values
		end
	end
	return {
		status = 200,
		reason = 'OK',
		headers = headers,
		-- no body
	}
end)

server:run(httpd.TRACE)
