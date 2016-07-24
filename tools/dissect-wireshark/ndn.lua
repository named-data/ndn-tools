-- Copyright (c) 2015-2016,  Regents of the University of California.
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


-- inspect.lua (https://github.com/kikito/inspect.lua) can be used for debugging.
-- See more at http://stackoverflow.com/q/15175859/2150331
-- local inspect = require('inspect')

-- NDN protocol
ndn = Proto("ndn", "Named Data Networking (NDN)")

-----------------------------------------------------
-----------------------------------------------------
-- Field formatting helpers

-- Borrowed from http://lua-users.org/wiki/StringRecipes
function escapeString(str)
   if (str) then
      str = string.gsub(str, "\n", "\r\n")
      str = string.gsub(str, "([^%w %-%_%.%~])",
                        function (c) return string.format ("%%%02X", string.byte(c)) end)
      str = string.gsub(str, " ", "+")
   end
   return str
end

function getUriFromNameComponent(block)
   -- @todo Implement proper proper URL escaping
   return block.tvb(block.offset + block.typeLen + block.lengthLen, block.length):string()
end

function getUriFromName(nameBlock)
   if (nameBlock.elements == nil) then
      return ""
   else
      components = {}
      for i, block in pairs(nameBlock.elements) do
         table.insert(components, getUriFromNameComponent(block))
      end

      return "/" .. table.concat(components, "/")
   end
end

function getNonNegativeInteger(b)
   if (b.length == 1) then
      return b.tvb(b.offset + b.typeLen + b.lengthLen, 1):uint()
   elseif (b.length == 2) then
      return b.tvb(b.offset + b.typeLen + b.lengthLen, 2):uint()
   elseif (b.length == 4) then
      return b.tvb(b.offset + b.typeLen + b.lengthLen, 4):uint()
   -- Something strange with uint64, not supporting it for now
   -- elseif (b.length == 8) then
   --    return b.tvb(b.offset + b.typeLen + b.lengthLen, 8):uint64()
   else
      return 0xFFFFFFFF;
   end
end

function getUriFromExclude(block)
   -- @todo
   return ""
end

function getTrue(block)
   return "Yes"
end

local AppPrivateBlock1 = 128
local AppPrivateBlock2 = 32767

function getGenericBlockInfo(block)
   local name = ""

   if (block.type < AppPrivateBlock1) then
      name = "RESERVED_1"
   elseif (AppPrivateBlock1 <= block.type and block.type < 253) then
      name = "APP_TAG_1"
   elseif (253 <= block.type and block.type < AppPrivateBlock2) then
      name = "RESERVED_3"
   else
      name = "APP_TAG_3"
   end

   return name .. ", Type: " .. block.type .. ", Length: " .. block.length
end

-----------------------------------------------------
-----------------------------------------------------

local NDN_DICT = {
   -- Interest or Data packets
   [5]  = {name = "Interest"                     , summary = true},
   [6]  = {name = "Data"                         , summary = true},

   -- Name field
   [7]  = {name = "Name"                         , field = ProtoField.string("ndn.name", "Name")                                   , value = getUriFromName},
   [1]  = {name = "ImplicitSha256DigestComponent", field = ProtoField.string("ndn.implicitsha256", "ImplicitSha256DigestComponent"), value = getUriFromNameComponent},
   [8]  = {name = "NameComponent"                , field = ProtoField.string("ndn.namecomponent", "NameComponent")                 , value = getUriFromNameComponent},

   -- Sub-fields of Interest packet
   [9]  = {name = "Selectors"                    , summary = true},
   [10] = {name = "Nonce"                        , field = ProtoField.bytes("ndn.nonce", "Nonce")},
   [12] = {name = "InterestLifetime"             , field = ProtoField.uint32("ndn.interestlifetime", "InterestLifetime", base.DEC) , value = getNonNegativeInteger},

   -- Sub-fields of Interest/Selector field
   [13] = {name = "MinSuffixComponents"          , field = ProtoField.uint32("ndn.minsuffix", "MinSuffixComponents")               , value = getNonNegativeInteger},
   [14] = {name = "MaxSuffixComponents"          , field = ProtoField.uint32("ndn.maxsuffix", "MaxSuffixComponents")               , value = getNonNegativeInteger},
   [15] = {name = "PublisherPublicKeyLocator"    , summary = true},
   [16] = {name = "Exclude"                      , field = ProtoField.string("ndn.exclude", "Exclude")                             , value = getUriFromExclude},
   [17] = {name = "ChildSelector"                , field = ProtoField.uint32("ndn.childselector", "ChildSelector", base.DEC)       , value = getNonNegativeInteger},
   [18] = {name = "MustBeFresh"                  , field = ProtoField.string("ndn.mustbefresh", "MustBeFresh")                     , value = getTrue},
   [19] = {name = "Any"                          , field = ProtoField.string("ndn.any", "Any")                                     , value = getTrue},

   -- Sub-fields of Data packet
   [20] = {name = "MetaInfo"                     , summary = true},
   [21] = {name = "Content"                      , field = ProtoField.string("ndn.content", "Content")},
   [22] = {name = "SignatureInfo"                , summary = true},
   [23] = {name = "SignatureValue"               , field = ProtoField.bytes("ndn.signaturevalue", "SignatureValue")},

   -- Sub-fields of Data/MetaInfo field
   [24] = {name = "ContentType"                  , field = ProtoField.uint32("ndn.contenttype", "Content Type", base.DEC)          , value = getNonNegativeInteger},
   [25] = {name = "FreshnessPeriod"              , field = ProtoField.uint32("ndn.freshnessperiod", "FreshnessPeriod", base.DEC)   , value = getNonNegativeInteger},
   [26] = {name = "FinalBlockId"                 , field = ProtoField.string("ndn.finalblockid", "FinalBlockId")                   , value = getUriFromNameComponent},

   -- Sub-fields of Data/Signature field
   [27] = {name = "SignatureType"                , field = ProtoField.uint32("ndn.signaturetype", "SignatureType", base.DEC)       , value = getNonNegativeInteger},
   [28] = {name = "KeyLocator"                   , summary = true},
   [29] = {name = "KeyDigest"                    , field = ProtoField.bytes("ndn.keydigest", "KeyDigest")},

   -- Other fields
   [30] = {name = "LinkPreference"               , field = ProtoField.uint32("ndn.link_preference", "LinkPreference", base.DEC)    , value = getNonNegativeInteger},
   [31] = {name = "LinkDelegation"               , summary = true},
   [32] = {name = "SelectedDelegation"           , field = ProtoField.uint32("ndn.selected_delegation", "SelectedDelegation", base.DEC), value = getNonNegativeInteger},
}


-- -- Add protofields in NDN protocol
ndn.fields = {
}
for key, value in pairs(NDN_DICT) do
   table.insert(ndn.fields, value.field)
end


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
         treeInfo = root:add(block.tvb(block.offset, block.size), info.name)
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
   if ((block.type == 5 or block.type == 6) and block.length <= 8800) then
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
      end

      local info = NDN_DICT[block.type]
      if (info ~= nil) then
         block.tree:append_text(", " .. NDN_DICT[block.type].name .. ", " .. block.summary)
      end

      nBytesLeft = nBytesLeft - block.size

      if (nBytesLeft > 0) then
         ok, block = pcall(getBlock, tvb, tvb:len() - nBytesLeft)
         if (not ok or not canBeValidNdnPacket(block)) then
            break
         end
      end
   end

   pInfo.cols.protocol = tostring(pInfo.cols.protocol) .. " (" .. ndn.name .. ")"

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

io.stderr:write("ndn.lua is successfully loaded\n")
