-- Copyright (c) 2015-2023, Regents of the University of California.
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
-- @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
-- @author Zipeng Wang
-- @author Qianshan Yu

-- inspect.lua (https://github.com/kikito/inspect.lua) can be used for debugging.
-- See more at http://stackoverflow.com/q/15175859/2150331
-- local inspect = require('inspect')

-- NDN protocol
ndn = Proto("ndn", "Named Data Networking (NDN)")

-- TODO with NDNLPv2 processing:
-- * mark field "unknown" when the field is recognized but the relevant feature is disabled
-- * colorize "unknown field"
-- * for a field that appears out-of-order, display "out-of-order field " in red

-----------------------------------------------------
-----------------------------------------------------
-- Field formatting helpers

-- @return TLV-VALUE portion of a TLV block
function getValue(b)
   return b.tvb(b.offset + b.typeLen + b.lengthLen, b.length)
end

function getNonNegativeInteger(b)
   if b.length == 1 or b.length == 2 or b.length == 4 or b.length == 8 then
      return getValue(b):uint64()
   end
   return UInt64.max()
end

function getUriFromSha256DigestComponent(b)
   local s = ""
   if b.type == 1 then
      s = "sha256digest="
   elseif b.type == 2 then
      s = "params-sha256="
   else
      assert(false)
   end

   for i = 0, (b.length - 1) do
      local byte = b.tvb(b.offset + b.typeLen + b.lengthLen + i, 1)
      s = s .. string.format("%02x", byte:uint())
   end
   return s
end

function getUriFromNameComponent(b)
   if b.type == 1 or b.type == 2 then
      return getUriFromSha256DigestComponent(b)
   end

   local s = ""
   if b.type ~= 8 then
      s = string.format("%d=", b.type)
   end
   hasNonPeriod = false
   for i = 0, (b.length - 1) do
      local byte = b.tvb(b.offset + b.typeLen + b.lengthLen + i, 1)
      local ch = byte:uint()
      hasNonPeriod = hasNonPeriod or ch ~= 0x2E
      if (ch >= 0x41 and ch <= 0x5A) or (ch >= 0x61 and ch <= 0x7A) or (ch >= 0x30 and ch <= 0x39) or ch == 0x2D or ch == 0x2E or ch == 0x5F or ch == 0x7E then
         s = s .. byte:string()
      else
         s = s .. string.format("%%%02X", ch)
      end
   end
   if not hasNonPeriod then
      s = s .. "..."
   end
   return s
end

function getUriFromName(b)
   if b.elements == nil then
      return "/"
   end

   components = {}
   for i, comp in pairs(b.elements) do
      table.insert(components, getUriFromNameComponent(comp))
   end
   return "/" .. table.concat(components, "/")
end

function getUriFromFinalBlockId(b)
   if b.elements == nil then
      return "/"
   end
   return getUriFromNameComponent(b.elements[1])
end

function getNackReasonDetail(b)
   assert(b.type == 801)
   local code = getNonNegativeInteger(b)

   if code == UInt64(0) then return "None"
   elseif code == UInt64(50) then return "Congestion"
   elseif code == UInt64(100) then return "Duplicate"
   elseif code == UInt64(150) then return "NoRoute"
   else return tostring(code)
   end
end

function getCachePolicyDetail(b)
   assert(b.type == 821)
   local code = getNonNegativeInteger(b)

   if code == UInt64(0) then return "None"
   elseif code == UInt64(1) then return "NoCache"
   else return tostring(code) .. " (unknown)"
   end
end

function getNonce(b)
   assert(b.type == 10)
   if b.length ~= 4 then
      return "invalid (should have 4 octets)"
   end
   return getValue(b):uint()
end

function getHopLimit(b)
   assert(b.type == 34)
   if b.length ~= 1 then
      return "invalid (should have 1 octet)"
   end
   return getValue(b):uint()
end

function getTrue(block)
   return "Yes"
end

local AppPrivateBlock1 = 100
local AppPrivateBlock2 = 800
local AppPrivateBlock3 = 1000

function canIgnoreTlvType(t)
   if t < AppPrivateBlock2 or t >= AppPrivateBlock3 then
      return false
   else
      if math.fmod(t, 2) == 1 then
         return true
      else
         return false
      end
   end
end

function getGenericBlockInfo(block)
   local name = ""

   -- TODO: Properly format informational message based type value reservations
   -- (https://docs.named-data.net/NDN-packet-spec/current/types.html#reserved-ranges)
   if (block.type <= AppPrivateBlock1) then
      name = "Unrecognized from the reserved range " .. 0 .. "-" .. AppPrivateBlock1 .. ""
   elseif (AppPrivateBlock1 < block.type and block.type < AppPrivateBlock2) then
      name = "Unrecognized from the reserved range " .. (AppPrivateBlock1 + 1) .. "-" .. (AppPrivateBlock2 - 1) .. ""
   elseif (AppPrivateBlock2 <= block.type and block.type <= AppPrivateBlock3) then
      if canIgnoreTlvType(block.type) then
         name = "Unknown field (ignored)"
      else
         name = "Unknown field"
      end
   else
      name = "RESERVED_3"
   end

   return name .. ", Type: " .. block.type .. ", Length: " .. block.length
end

-----------------------------------------------------
-----------------------------------------------------

local NDN_DICT = {
   -- Name and name components
   [7]  = {name = "Name"                           , field = ProtoField.string("ndn.name", "Name")                                    , value = getUriFromName},
   [1]  = {name = "ImplicitSha256DigestComponent"  , field = ProtoField.string("ndn.implicit_sha256", "ImplicitSha256DigestComponent"), value = getUriFromNameComponent},
   [2]  = {name = "ParametersSha256DigestComponent", field = ProtoField.string("ndn.params_sha256", "ParametersSha256DigestComponent"), value = getUriFromNameComponent},
   [8]  = {name = "GenericNameComponent"           , field = ProtoField.string("ndn.namecomponent", "GenericNameComponent")           , value = getUriFromNameComponent},

   -- Interest and its sub-elements in Packet Format v0.3
   [5]  = {name = "Interest"                     , summary = true},
   [33] = {name = "CanBePrefix"                  , field = ProtoField.string("ndn.canbeprefix", "CanBePrefix")                     , value = getTrue},
   [18] = {name = "MustBeFresh"                  , field = ProtoField.string("ndn.mustbefresh", "MustBeFresh")                     , value = getTrue},
   [30] = {name = "ForwardingHint"               , summary = true},
   [10] = {name = "Nonce"                        , field = ProtoField.uint32("ndn.nonce", "Nonce", base.HEX)                       , value = getNonce},
   [12] = {name = "InterestLifetime"             , field = ProtoField.uint64("ndn.lifetime", "InterestLifetime", base.DEC)         , value = getNonNegativeInteger},
   [34] = {name = "HopLimit"                     , field = ProtoField.uint8("ndn.hoplimit", "HopLimit", base.DEC)                  , value = getHopLimit},
   [36] = {name = "ApplicationParameters"        , field = ProtoField.bytes("ndn.app_params", "ApplicationParameters")},

   -- Data and its sub-elements in Packet Format v0.3
   [6]  = {name = "Data"                         , summary = true},
   [20] = {name = "MetaInfo"                     , summary = true},
   [24] = {name = "ContentType"                  , field = ProtoField.uint64("ndn.contenttype", "ContentType", base.DEC)           , value = getNonNegativeInteger},
   [25] = {name = "FreshnessPeriod"              , field = ProtoField.uint64("ndn.freshness", "FreshnessPeriod", base.DEC)         , value = getNonNegativeInteger},
   [26] = {name = "FinalBlockId"                 , field = ProtoField.string("ndn.finalblock", "FinalBlockId")                     , value = getUriFromFinalBlockId},
   [21] = {name = "Content"                      , field = ProtoField.bytes("ndn.content", "Content")},
   [22] = {name = "SignatureInfo"                , summary = true},
   [27] = {name = "SignatureType"                , field = ProtoField.uint64("ndn.sigtype", "SignatureType", base.DEC)             , value = getNonNegativeInteger},
   [28] = {name = "KeyLocator"                   , summary = true},
   [29] = {name = "KeyDigest"                    , field = ProtoField.bytes("ndn.keydigest", "KeyDigest")},
   [23] = {name = "SignatureValue"               , field = ProtoField.bytes("ndn.sigvalue", "SignatureValue")},

   -- NDNLPv2 headers
   [80] = {name = "Fragment"                     },
   [81] = {name = "Sequence"                     , field = ProtoField.uint64("ndn.sequence", "Sequence", base.DEC)                 , value = getNonNegativeInteger},
   [82] = {name = "FragIndex"                    , field = ProtoField.uint64("ndn.fragindex", "FragIndex", base.DEC)               , value = getNonNegativeInteger},
   [83] = {name = "FragCount"                    , field = ProtoField.uint64("ndn.fragcount", "FragCount", base.DEC)               , value = getNonNegativeInteger},
   [98] = {name = "PitToken"                     , field = ProtoField.bytes("ndn.pit_token", "PitToken")},
   [100] = {name = "LpPacket"                    , summary = true},
   [800] = {name = "Nack"                        , summary = true},
   [801] = {name = "NackReason"                  , field = ProtoField.string("ndn.nack_reason", "NackReason")                      , value = getNackReasonDetail},
   [812] = {name = "IncomingFaceId"              , field = ProtoField.uint64("ndn.incoming_face", "IncomingFaceId", base.DEC)      , value = getNonNegativeInteger},
   [816] = {name = "NextHopFaceId"               , field = ProtoField.uint64("ndn.nexthop_face", "NextHopFaceId", base.DEC)        , value = getNonNegativeInteger},
   [820] = {name = "CachePolicy"                 , summary = true},
   [821] = {name = "CachePolicyType"             , field = ProtoField.string("ndn.cache_policy", "CachePolicyType")                , value = getCachePolicyDetail},
   [832] = {name = "CongestionMark"              , field = ProtoField.uint64("ndn.congestion_mark", "CongestionMark", base.DEC)    , value = getNonNegativeInteger},
   [836] = {name = "Ack"                         , field = ProtoField.uint64("ndn.ack", "Ack", base.DEC)                           , value = getNonNegativeInteger},
   [840] = {name = "TxSequence"                  , field = ProtoField.uint64("ndn.txseq", "TxSequence", base.DEC)                  , value = getNonNegativeInteger},
}

-- -- Add protofields in NDN protocol
ndn.fields = {}
for key, value in pairs(NDN_DICT) do
   table.insert(ndn.fields, value.field)
end

ndn.fields.bin = ProtoField.bytes("ndn.bin", "Binary")
ndn.fields.len = ProtoField.uint32("ndn.len", "Packet Length")
ndn.fields.type = ProtoField.string("ndn.type", "Packet Type")

-----------------------------------------------------
-----------------------------------------------------

-- block
-- .tvb
-- .offset
-- .type
-- .typeLen
-- .length
-- .lengthLen
-- .size = .typeLen + .lengthLen + .length

function addInfo(block, root) -- may be add additional context later
   local info = NDN_DICT[block.type]

   if (info == nil) then
      info = {}
      info.value = getGenericBlockInfo
      -- color
   end

   local treeInfo
   if (info.value == nil) then

      if (info.field ~= nil) then
         treeInfo = root:add(info.field, block.tvb(block.offset, block.size))
      else
         treeInfo = root:add(block.tvb(block.offset, block.size), info.name)
      end

      treeInfo:append_text(", Type: " .. block.type .. ", Length: " .. block.length)
   else
      block.value = info.value(block)

      if (info.field ~= nil) then
         treeInfo = root:add(info.field, block.tvb(block.offset, block.size), block.value)
      else
         treeInfo = root:add(block.tvb(block.offset, block.size), block.value)
      end
   end
   block.root = treeInfo
   return block.root
end

function addSummary(block)
   if (block.elements == nil) then
      return
   end

   local info = NDN_DICT[block.type]
   if (info == nil or info.summary == nil) then
      return
   end

   local summary = {}

   for k, subblock in pairs(block.elements) do
      if (subblock.value ~= nil) then
         local info = NDN_DICT[subblock.type]
         if (info ~= nil) then
            table.insert(summary, info.name .. ": " .. subblock.value)
         end
      end
   end

   if (#summary > 0) then
      block.summary = table.concat(summary, ", ")
      if (block.value == nil) then
         block.value = block.summary
      end
      block.root:append_text(", " .. block.summary)
   end
end

-----------------------------------------------------
-----------------------------------------------------

function readVarNumber(tvb, offset)
   if offset >= tvb:len() then
      return 0, 0
   end

   local firstOctet = tvb(offset, 1):uint()
   if (firstOctet < 253) then
      return firstOctet, 1
   elseif (firstOctet == 253) and (offset + 3 < tvb:len()) then
      return tvb(offset + 1, 2):uint(), 3
   elseif (firstOctet == 254) and (offset + 5 < tvb:len()) then
      return tvb(offset + 1, 4):uint(), 5
   elseif (firstOctet == 255) and (offset + 9 < tvb:len()) then
      return tvb(offset + 1, 8):uint64(), 9
   end

   return 0, 0
end

function getBlock(tvb, offset)
   local block = {}
   block.tvb = tvb
   block.offset = offset

   block.type, block.typeLen = readVarNumber(block.tvb, block.offset)
   if block.typeLen == 0 then
      return nil
   end

   block.length, block.lengthLen = readVarNumber(block.tvb, block.offset + block.typeLen)
   if block.lengthLen == 0 then
      return nil
   end

   block.size = block.typeLen + block.lengthLen + block.length

   return block
end

function canBeValidNdnPacket(block)
   if ((block.type == 5 or block.type == 6 or block.type == 100) and block.length <= 8800) then
      return true
   else
      return false
   end
end

function findNdnPacket(tvb)
   offset = 0

   while offset + 2 < tvb:len() do
      local block = getBlock(tvb, offset)

      if (block ~= nil) and canBeValidNdnPacket(block) then
         return block
      end

      offset = offset + 1
   end

   return nil
end

function getSubBlocks(block)
   local valueLeft = block.length
   local subBlocks = {}

   while valueLeft > 0 do
      local offset = block.offset + block.typeLen + block.lengthLen + (block.length - valueLeft)
      local child = getBlock(block.tvb, offset)
      if child == nil then
         return nil
      end

      valueLeft = valueLeft - child.size
      table.insert(subBlocks, child)
   end

   if (valueLeft == 0) then
      return subBlocks
   else
      return nil
   end
end

-----------------------------------------------------
-----------------------------------------------------

-- NDN protocol dissector function
function ndn.dissector(tvb, pInfo, root) -- Tvb, Pinfo, TreeItem

   if (tvb:len() ~= tvb:reported_len()) then
      return 0 -- ignore partially captured packets
      -- this can/may be re-enabled only for unfragmented UDP packets
   end

   local ok, block = pcall(findNdnPacket, tvb)
   if (not ok) then
      return 0
   end

   if (block == nil or block.offset == nil) then
      -- no valid NDN packets found
      return 0
   end

   local nBytesLeft = tvb:len() - block.offset
   -- print (pInfo.number .. ":: Found block: " .. block.type .. " of length " .. block.size .. " bytesLeft: " .. nBytesLeft)

   local pktType = ""
   local pktName = ""

   while (block.size <= nBytesLeft) do
      -- Create TreeItems
      block.tree = root:add(ndn, tvb(block.offset, block.size))

      local queue = {block}
      while (#queue > 0) do
         local block = queue[1]
         table.remove(queue, 1)

         block.elements = getSubBlocks(block)
         local subtree = addInfo(block, block.tree)

         if (block.elements ~= nil) then
            for i, subBlock in pairs(block.elements) do
               subBlock.tree = subtree
               table.insert(queue, subBlock)
            end
         end
      end

      -- Make summaries
      local queue = {block}
      while (#queue > 0) do
         local block = queue[1]
         if (block.visited ~= nil or block.elements == nil) then
            -- try to make summary
            table.remove(queue, 1)

            addSummary(block)
         else
            for i, subBlock in pairs(block.elements) do
               table.insert(queue, 1, subBlock)
            end
            block.visited = true
         end

         -- prepare information to fill info column
         if block.type == 5 and pktType ~= "Nack" then
            pktType = "Interest"
         elseif block.type == 6 then
            pktType = "Data"
         elseif block.type == 800 then
            pktType = "Nack"
         end

         if block.type == 7 then
            pktName = getUriFromName(block)
         end
      end

      local info = NDN_DICT[block.type]
      if (info ~= nil) then
         if (block.summary ~= nil) then
            block.tree:append_text(", " .. NDN_DICT[block.type].name .. ", " .. block.summary)
         else
            block.tree:append_text(", " .. NDN_DICT[block.type].name)
         end
      end

      nBytesLeft = nBytesLeft - block.size

      if (nBytesLeft > 0) then
         ok, block = pcall(getBlock, tvb, tvb:len() - nBytesLeft)
         if (not ok or block == nil or not canBeValidNdnPacket(block)) then
            break
         end
      end

      -- These fields can be used to get NDN-packet level information using tshark,
      -- for example when using the dissector only for reassembling NDN packets from TCP
      block.tree:add(ndn.fields.bin, tvb(0, tvb:len())):set_hidden()
      block.tree:add(ndn.fields.len, tvb:len()):set_hidden()
      block.tree:add(ndn.fields.type, pktType):set_hidden()
   end -- while(block.size <= nBytesLeft)

   pInfo.cols.protocol = tostring(pInfo.cols.protocol) .. " (" .. ndn.name .. ")"
   pInfo.cols.info = pktType .. " " .. pktName

   if (nBytesLeft > 0 and block ~= nil and block.size ~= nil and block.size > nBytesLeft) then
      pInfo.desegment_offset = tvb:len() - nBytesLeft

      -- Originally, I set desegment_len to the exact lenght, but it mysteriously didn't work for TCP
      -- pInfo.desegment_len = block.size -- this will not work to desegment TCP streams
      pInfo.desegment_len = DESEGMENT_ONE_MORE_SEGMENT
   end
end

local udpDissectorTable = DissectorTable.get("udp.port")
udpDissectorTable:add("6363", ndn)
udpDissectorTable:add("56363", ndn)

local tcpDissectorTable = DissectorTable.get("tcp.port")
tcpDissectorTable:add("6363", ndn)

local websocketDissectorTable = DissectorTable.get("ws.port")
-- websocketDissectorTable:add("9696", ndn)
websocketDissectorTable:add("1-65535", ndn)

local ethernetDissectorTable = DissectorTable.get("ethertype")
ethernetDissectorTable:add(0x8624, ndn)

local pppDissectorTable = DissectorTable.get("ppp.protocol")
pppDissectorTable:add(0x0077, ndn)

io.stderr:write("NDN dissector successfully loaded\n")
