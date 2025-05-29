#!/usr/libexec/flua
--[[
/*
 * Copyright (c) 2020-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
]]--
-- ex: et sw=4:

local ifcfg = require('ifconfig').open()
local ucl = require('ucl')

local ifaces = ifcfg:foreach_iface(function(_, iface, ifs)
    local name = iface:name()
    ifs[name] = {
        flags = iface:flags(),
        media = ifcfg:get_media(name),
        status = ifcfg:get_status(name),
        capabilities = ifcfg:get_capabilities(name),
        groups = ifcfg:get_groups(name),
        metric = ifcfg:get_metric(name),
        mtu = ifcfg:get_mtu(name),
        nd6 = ifcfg:get_nd6(name),
        bridge_status = ifcfg:get_bridge_status(name),
        lagg_status = ifcfg:get_lagg_status(name),
        laggdev = ifcfg:get_laggport_laggdev(name),
        addresses = ifcfg:foreach_ifaddr(iface, function(_, addr, addrs)
            table.insert(addrs, ifcfg:addr_info(addr))
            return addrs
        end, {}),
        sfp = (function()
            local info = ifcfg:get_sfp_info(name)
            return info and {
                info = info,
                vendor_info = ifcfg:get_sfp_vendor_info(name),
                status = ifcfg:get_sfp_status(name),
                -- Not generally interesting,
                -- but for completeness:
                -- dump = tostring(ifcfg:get_sfp_dump(name)),
            }
        end)(),
    }
    return ifs
end, {})

print(ucl.to_json(ifaces))
