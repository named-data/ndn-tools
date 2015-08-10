-- Copyright (c) 2015,  Regents of the University of California.
--
-- This file is part of ndn-tools (Named Data Networking Essential Tools).
-- See AUTHORS.md for complete list of ndn-tools authors and contributors.
--
-- ndn-tools is free software: you can redistribute it and/or modify it under the terms
-- of the GNU General Public License as published by the Free Software Foundation,
-- either version 3 of the License, or (at your option) any later version.
--
-- ndn-tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
-- without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
-- PURPOSE.  See the GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License along with
-- ndn-tools, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
--
-- @author Qi Zhao       <https://www.linkedin.com/pub/qi-zhao/73/835/9a3>
-- @author Seunghyun Yoo <http://relue2718.com/>
-- @author Seungbae Kim  <https://sites.google.com/site/sbkimcv/>


-- inspect.lua (https://github.com/kikito/inspect.lua) can be used for debugging.
-- See more at http://stackoverflow.com/q/15175859/2150331
-- local inspect = require('inspect')

-- NDN protocol
p_ndnproto = Proto ("ndn", "Named Data Network (NDN)") -- to create a 'Proto' object

-- Type and Length fields
local f_packet_type = ProtoField.uint16("ndn.type", "Type", base.DEC_HEX)
local f_packet_size = ProtoField.uint16("ndn.length", "Length", base.DEC_HEX)

-- Interest or Data packets
local f_interest = ProtoField.string("ndn.interest", "Interest", FT_STRING)
local f_data = ProtoField.string("ndn.data", "Data", FT_STRING)

-- Name field
local f_name = ProtoField.string("ndn.name", "Name", FT_STRING)
local f_namecomponent = ProtoField.string("ndn.namecomponent", "Name Component", FT_STRING)
local f_implicitSHA = ProtoField.string("ndn.implicitsha", "Implicit SHA 256 Digest Component", FT_STRING)

-- Sub-fields of Interest packet
local f_interest_selector = ProtoField.string("ndn.selector", "Selector", FT_STRING)
local f_interest_nonce = ProtoField.uint16("ndn.nonce", "Nonce", base.DEC_HEX)
local f_interest_scope = ProtoField.string("ndn.scope", "Scope", FT_STRING)
local f_interest_interestlifetime = ProtoField.uint16("ndn.interestlifetime", "Interest Life Time", base.DEC_HEX)

-- Sub-fields of Interest/Selector field
local f_interest_selector_minsuffix = ProtoField.uint16("ndn.minsuffix", "Min Suffix Components", base.DEC_HEX)
local f_interest_selector_maxsuffix = ProtoField.uint16("ndn.maxsuffix", "Max Suffix Components", base.DEC_HEX)
local f_interest_selector_keylocator = ProtoField.string("ndn.keylocator", "Publisher Public Key Locator", FT_STRING)
local f_interest_selector_exclude = ProtoField.string("ndn.exclude", "Exclude", FT_STRING)
local f_interest_selector_childselector = ProtoField.uint16("ndn.childselector", "Child Selector", base.DEC_HEX)
local f_interest_selector_mustbefresh = ProtoField.string("ndn.mustbefresh", "Must Be Fresh", FT_STRING)
local f_interest_selector_any = ProtoField.string("ndn.any", "Any", FT_STRING)

-- Sub-fields of Data packet
local f_data_metainfo = ProtoField.string("ndn.metainfo", "Meta Info", FT_STRING)
local f_data_content = ProtoField.string("ndn.content", "Content", FT_STRING)
local f_data_signatureinfo = ProtoField.string("ndn.signatureinfo", "Signature Info", FT_STRING)
local f_data_signaturevalue = ProtoField.string("ndn.signaturevalue", "Signature Value", FT_STRING)

-- Sub-fields of Data/MetaInfo field
local f_data_metainfo_contenttype = ProtoField.uint16("ndn.contenttype", "Content Type", base.DEC_HEX)
local f_data_metainfo_freshnessperiod = ProtoField.uint16("ndn.freshnessperiod", "Freshness Period", base.DEC_HEX)
local f_data_metainfo_finalblockid = ProtoField.string("ndn.finalblockid", "Final Block ID", FT_STRING)

-- Sub-fields of Data/Signature field
local f_data_signature_signaturetype = ProtoField.uint16("ndn.signaturetype", "Signature Type", base.DEC_HEX)
local f_data_signature_keylocator = ProtoField.string("ndn.keylocator", "Key Locator", FT_STRING)
local f_data_signature_keydigest = ProtoField.string("ndn.keydigest", "Key Digest", FT_STRING)

-- Add protofields in NDN protocol
p_ndnproto.fields = {f_packet_type, f_packet_size, f_data, f_interest, f_name, f_namecomponent, f_implicitSHA, f_interest_selector, f_interest_nonce, f_interest_scope, f_interest_interestlifetime, f_interest_selector_mustbefresh, f_interest_selector_minsuffix, f_interest_selector_maxsuffix, f_interest_selector_keylocator, f_interest_selector_exclude, f_interest_selector_childselector, f_interest_selector_any, f_data_metainfo, f_data_content, f_data_signatureinfo, f_data_signaturevalue, f_data_metainfo_contenttype, f_data_metainfo_freshnessperiod, f_data_metainfo_finalblockid, f_data_signature_signaturetype, f_data_signature_keylocator, f_data_signature_keydigest}

-- ndntlv_info = { data: { field, type, string }, children: {} }

-- To handle the fragmented packets
-- type: map
-- * key: (host ip address, host port number)
-- * value: type: map
--          * key: packet number
--          * value: packet status
local pending_packets = {}
local CONST_STR_TRUNCATED = "TRUNCATED"
local CONST_STR_NDNTLV = "NDNTLV"
local GLOBAL_PACKET_INDEX = 0

function set_packet_status( packet_key, packet_number, status_key, status_value )
  if type( pending_packets[ packet_key ] ) ~= "table" then
    pending_packets[ packet_key ] = {}
  end
  if type( pending_packets[ packet_key ][ packet_number ] ) ~= "table" then
    pending_packets[ packet_key ][ packet_number ] = {}
  end
  pending_packets[ packet_key ][ packet_number ][ status_key ] = status_value
end

function get_packet_status( packet_key, packet_number, status_key )
  if type( pending_packets[ packet_key ] ) ~= "table" then
    return nil
  end
  if type( pending_packets[ packet_key ][ packet_number ] ) ~= "table" then
    return nil
  end
  return pending_packets[ packet_key ][ packet_number ][ status_key ]
end

function get_keys_from( table )
  local keyset = {}
  local n = 0
  for k, v in pairs( table ) do
    n = n + 1
    keyset[n] = k
  end
  return keyset
end

function dump_packet_status()
  --print(inspect(pending_packets))
end

function bytearray_to_int( raw_bytes, offset, length )
  local ret = 0
  for i = offset, offset + length - 1 do
    ret = ret * 256 + raw_bytes:get_index( i )
  end
  return ret
end

function deepcopy(orig)
    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in next, orig, nil do
            copy[deepcopy(orig_key)] = deepcopy(orig_value)
        end
        setmetatable(copy, deepcopy(getmetatable(orig)))
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

function parse_ndn_tlv( packet_key, packet_number, is_original, max_size, optional_params, ndntlv_info )
  local raw_bytes = nil
  local buf = nil
  local length = nil

  if ( is_original ) then
    buf = optional_params["buf"]
    length = buf:len()
  else
    raw_bytes = optional_params["raw_bytes"]
    length = raw_bytes:len()
  end

  local current_pos = 0
  local _size_num_including_header = 0

  local ret = true -- a result of a ndn-tlv parser
  local isFirst = false -- flag that is going to be enabled when the first buffer arrives [BUGGY]

  while ( current_pos < length ) do
    isFirst = ( current_pos == 0 )

    -- extract TYPE
    local _type_uint = nil
    if ( is_original ) then
      _type_uint = buf( current_pos, 1 ):uint()
    else
      _type_uint = bytearray_to_int( raw_bytes, current_pos, 1 )
    end

    -- print("type:" .. _type_uint)

    if ( isFirst ) then
      _size_num_including_header = _size_num_including_header + 1
    end
    current_pos = current_pos + 1

    -- extract SIZE
    local _size_num = nil
    if ( is_original ) then
      _size_num = buf( current_pos, 1 ):uint()
    else
      _size_num = bytearray_to_int( raw_bytes, current_pos, 1 )
    end

    if ( isFirst ) then
        _size_num_including_header = _size_num_including_header + 1
    end
    current_pos = current_pos + 1

    if ( _size_num == 253 ) then
      if ( is_original ) then
        _size_num = buf( current_pos, 2 ):uint()
      else
        _size_num = bytearray_to_int( raw_bytes, current_pos, 2 )
      end
      if ( isFirst ) then
        _size_num_including_header = _size_num_including_header + _size_num + 2
      end
      current_pos = current_pos + 2
    elseif ( _size_num == 254 ) then
      if ( is_original ) then
        _size_num = buf( current_pos, 4 ):uint()
      else
        _size_num = bytearray_to_int( raw_bytes, current_pos, 4 )
      end
      if ( isFirst ) then
        _size_num_including_header = _size_num_including_header + _size_num + 4
      end
      current_pos = current_pos + 4
    elseif ( _size_num == 255 ) then
      print("## error ## lua doesn't support 8 bytes of number variables.")
      if ( is_original ) then
        _size_num = buf( current_pos, 8 ):uint64() -- can lua number be larger than 32 bits? -- the type 'userdata'
      else
        _size_num = bytearray_to_int( raw_bytes, current_pos, 8 )
      end
      if ( isFirst ) then
        _size_num_including_header = _size_num_including_header + _size_num + 8
      end
      current_pos = current_pos + 8
    else
      if ( isFirst ) then
        _size_num_including_header = _size_num_including_header + _size_num
      end
    end

    -- subtree:add( f_packet_size, _size )
    local type_size_info = " (Type: " .. _type_uint .. ", Size: " .. _size_num .. ")"

    -- need to check which one should be used: either _size_num or _size_num_including_header
    if ( max_size ~= -1 and max_size < _size_num ) then
      if ( is_original ) then
        set_packet_status( packet_key, packet_number, "error", "The size of sub ndn-tlv packet can't exceed the parent's one." )
      end
      ret = false
      break
    end

    if ( isFirst ) then
      if ( is_original ) then
        set_packet_status( packet_key, packet_number, "expected_size", _size_num_including_header )
      end
    end

    if ( _type_uint == 18 ) then
      if ( is_original ) then
        set_packet_status( packet_key, packet_number, "error", "the type of field is 18 (but why is this an error?).")
      end
      return ret
    end

    if ( current_pos + _size_num > length ) then
      if ( is_original ) then
        set_packet_status( packet_key, packet_number, "status", CONST_STR_TRUNCATED)
      end
      ret = false
      break
    end

    local _payload = nil
    local new_optional_params = {}
    if ( is_original ) then
      _payload = buf( current_pos, _size_num )
      new_optional_params["buf"] = _payload
    else
      new_optional_params["raw_bytes"] = raw_bytes:subset( current_pos, _size_num )
    end
    current_pos = current_pos + _size_num

    local child_tree = nil

    if ( _type_uint == 5 ) then -- interest packet can contain sub NDN-TLV packets
      -- Interest packet
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_interest, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 6 ) then
      -- Data packet
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_data, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 7 ) then
      -- Name
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_name, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 8 ) then
      -- Name Component
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_namecomponent, _payload, _payload:string(ENC_UTF_8) .. type_size_info } )
      end
    elseif ( _type_uint == 1 ) then
      -- Implicit SHA 256 Digest Component
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_implicitSHA, _payload, _payload:string() .. type_size_info } )
      end
    elseif ( _type_uint == 9 ) then
      -- Selectors
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_interest_selector, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 10 ) then
      -- Nonce
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_nonce, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 11 ) then
      -- Scope
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_scope, _payload, _payload:string() .. type_size_info } )
      end
    elseif ( _type_uint == 12 ) then
      -- Interest Lifetime
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_interestlifetime, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 13 ) then
      -- Selectors / Min Suffix Components
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_selector_minsuffix, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 14 ) then
      -- Selectors / Max Suffix Components
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_selector_maxsuffix, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 15 ) then
      -- Selectors / Publish Key Locator
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_interest_selector_keylocator, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 16 ) then
      -- Selectors / Exclude
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_interest_selector_exclude, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 17 ) then
      -- Selectors / Child Selector
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_selector_childselector, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 18 ) then
      -- Selectors / Must be Fresh
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_selector_mustbefresh, _payload, _payload:string() .. type_size_info } )
      end
    elseif ( _type_uint == 19 ) then
      -- Selectors / Any
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_interest_selector_any, _payload, _payload:string() .. type_size_info } )
      end
    elseif ( _type_uint == 20 ) then
      -- MetaInfo
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_data_metainfo, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 21 ) then
      -- Content
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_data_content, _payload, _payload:string() .. type_size_info } )
      end
    elseif ( _type_uint == 22 ) then
      -- SignatureInfo
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_data_signatureinfo, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 23 ) then
      -- SignatureValue
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_data_signaturevalue, _payload, _payload:string() .. type_size_info } )
      end
    elseif ( _type_uint == 24 ) then
      -- MetaInfo / ContentType
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_data_metainfo_contenttype, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 25 ) then
      -- MetaInfo / FreshnessPeriod
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_data_metainfo_freshnessperiod, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 26 ) then
      -- MetaInfo / FinalBlockId
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_data_metainfo_finalblockid, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 27 ) then
      -- Signature / SignatureType
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_data_signature_signaturetype, _payload, _payload:uint(), nil, type_size_info } )
      end
    elseif ( _type_uint == 28 ) then
      -- Signature / KeyLocator
      if ( is_original ) then
        child_tree = add_subtree( ndntlv_info, { f_data_signature_keylocator, _payload, type_size_info } )
      end
      ret = ret and parse_ndn_tlv( packet_key, packet_number, is_original, _size_num, new_optional_params, child_tree )
    elseif ( _type_uint == 29 ) then
      -- Signature / KeyDigest
      if ( is_original ) then
        add_subtree( ndntlv_info, { f_data_signature_keydigest, _payload, _payload:string() .. type_size_info } );
      end
    else
      --print("## warning ## unhandled type_uint: ", _type_uint)
      ret = false
      -- if the packet seems to be a NDN packet, it would be better idea to add some warning messages in the subtress instead of returning false.
    end
  end
  return ret
end

function create_subtree_from( info, subtree )
  for k, v in pairs( info["children"] ) do
    local data = v["data"]
    if type(data) == "table" then
      local child_tree = subtree:add( unpack( data ) )
      create_subtree_from( v, child_tree )
    end
  end
end

function add_subtree( info, data )
  local child_tree = { ["data"] = data, ["children"] = {} }
  table.insert( info["children"], child_tree )
  return child_tree
end

function create_empty_ndntlv_info()
  return { ["data"] = nil, ["children"] = {} }
end

function parse_buffer_and_update( packet_key, packet_number, is_original, pkt, root, optional_params )
  -- TODO: need to set the maximum length
  local ndntlv_info = create_empty_ndntlv_info()
  local was_ndntlv_packet = parse_ndn_tlv( packet_key, packet_number, is_original, -1, optional_params, ndntlv_info )

  if was_ndntlv_packet then
    local buf = nil
    if ( is_original ) then
      buf = optional_params["buf"]
      set_packet_status( packet_key, packet_number, "ndntlv_info", ndntlv_info )
      set_packet_status( packet_key, packet_number, "status", CONST_STR_NDNTLV )
    else
      buf = ByteArray.tvb( optional_params["raw_bytes"], optional_params["tvb_name"] )
      ndntlv_info = create_empty_ndntlv_info()
      parse_ndn_tlv( packet_key, packet_number, true, -1, { ["buf"] = buf }, ndntlv_info )

      local used_packet_numbers = optional_params["used_packet_numbers"]

      for k,v in pairs(used_packet_numbers) do
        set_packet_status( packet_key, v, "ndntlv_info", ndntlv_info )
        set_packet_status( packet_key, v, "status", CONST_STR_NDNTLV )
      end
    end
  end

  -- print( packet_key .. "--" .. packet_number .. ".." .. tostring(was_ndntlv_packet) )

  -- It needs to check whether the packet type is NDN-TLV.
  local saved_ndntlv_info = get_packet_status( packet_key, packet_number, "ndntlv_info" )
  local parsed = get_packet_status( packet_key, packet_number, "parsed" )
  if saved_ndntlv_info ~= nil then
    pkt.cols.protocol = p_ndnproto.name -- set the protocol name to NDN
    if ( parsed ~= true ) then
      set_packet_status( packet_key, packet_number, "parsed", true )
      local subtree = root:add( p_ndnproto, buf ) -- create subtree for ndnproto
      create_subtree_from( saved_ndntlv_info, subtree )
    end
  end
end

-- # not efficient
-- # lua -- doesn't support the random access...?
function get_next_element( tbl, current_value )
  for k, v in pairs( tbl ) do
    if ( v > current_value ) then
      return v
    end
  end
  return current_value
end

-- # not efficient
function get_previous_element( tbl, current_value )
  local prev = current_value
  for k, v in pairs( tbl ) do
    if ( v < current_value ) then
      prev = v
    else
      break
    end
  end
  return prev
end

-- ndnproto dissector function
function p_ndnproto.dissector( buf, pkt, root )
  -- validate packet length is adequate, otherwise quit
  local length = buf:len()
  local packet_number = pkt.number -- an unique serial for each packet
  local packet_key = tostring(pkt.src) .. ":" .. tostring(pkt.src_port) .. ":" .. tostring(pkt.dst) .. ":" .. tostring(pkt.dst_port)
  print("## info ## packet[" .. packet_number .. "], length = " .. length )
  set_packet_status( packet_key, packet_number, "parsed", false )

  if length == 0 then
  else
    local raw_bytes = buf:range():bytes()
    parse_buffer_and_update( packet_key, packet_number, true, pkt, root, { ["buf"] = buf } )
    set_packet_status( packet_key, packet_number, "buffer", raw_bytes )

    local pending_packet_numbers = get_keys_from( pending_packets[ packet_key ] )
    for k, v in pairs( pending_packet_numbers ) do
      local pending_packet_number = v

      if ( pending_packet_number <= packet_number ) then

        local status = get_packet_status( packet_key, pending_packet_number, "status" )
        local expected_size = get_packet_status( packet_key, pending_packet_number, "expected_size" )
        local used_packet_numbers = {}

        if ( status == CONST_STR_TRUNCATED ) then
          local merged_temp_buf = ByteArray.new()
          local temp_packet_number = pending_packet_number
          local pending_packet_number_end = 0
          while (merged_temp_buf:len() < expected_size) do
            local temp_buf = get_packet_status( packet_key, temp_packet_number, "buffer" )
            if ( temp_buf == nil ) then
              break
            else
              merged_temp_buf:append( temp_buf )
              pending_packet_number_end = temp_packet_number

              table.insert( used_packet_numbers, temp_packet_number )
              temp_packet_number = get_next_element( pending_packet_numbers, temp_packet_number )
            end
          end
          if ( merged_temp_buf:len() >= expected_size ) then
            local merged_tvb_name = "Reassembled (" .. pending_packet_number .. "-" .. pending_packet_number_end .. ")"
            local merged_parser_option = {
              ["raw_bytes"] = merged_temp_buf,
              ["tvb_name"] = merged_tvb_name,
              ["pending_packet_number"] = pending_packet_number,
              ["pending_packet_number_end"] = pending_packet_number_end,
              ["used_packet_numbers"] = used_packet_numbers,
            }
            print(pending_packet_number .. ".." .. pending_packet_number_end)
            parse_buffer_and_update( packet_key, packet_number, false, pkt, root, merged_parser_option )
          end
        end
      end
    end
    --dump_packet_status()
  end
end

-- Initialization routine
function p_ndnproto.init()
end

local websocket_dissector_table = DissectorTable.get("ws.port")
websocket_dissector_table:add("1-65535", p_ndnproto)

local tcp_dissector_table = DissectorTable.get("tcp.port")
tcp_dissector_table:add("6363", p_ndnproto)

local udp_dissector_table = DissectorTable.get("udp.port")
udp_dissector_table:add("6363", p_ndnproto)

print("ndntlv.lua is successfully loaded.")

----------------------------------------------------------------------
-- helper functions
----------------------------------------------------------------------
function dump_buf(buf)
  print("buffer.length = "..buf:len())
  local tmp = ""
  for i=0, buf:len()-1 do
      if i % 16 == 0 then
          tmp = tmp .. string.format("%04d",i) .. " : "
      end
      tmp = tmp .. (buf:range(i,1).." ")
      if (i+1) % 16 == 0 then
        tmp = tmp .. ("\n")
      end
  end
  print(tmp)
end

function print_table(tbl, indent)
  if not indent then indent = 0 end
  for k, v in pairs(tbl) do
    formatting = string.rep("  ", indent) .. k .. ": "
    if type(v) == "table" then
      print(formatting)
      print_table(v, indent+1)
    elseif type(v) == 'boolean' then
      print(formatting , tostring(v))
    else
      print(formatting , v)
    end
  end
end
