// SPDX-License-Identifier: GPL-3.0-or-later
#include "wire.hpp"
#include <algorithm>
namespace meshcloud::wire {
namespace {
bool operation_code(Operation op,std::uint8_t& code) {
 switch(op) {
 case Operation::Discovery: code=1;return true;
 case Operation::Search: code=2;return true;
 case Operation::Listing: code=3;return true;
 case Operation::Metadata: code=4;return true;
 case Operation::Upload: code=5;return true;
 case Operation::Download: code=6;return true;
 case Operation::Publish: code=7;return true;
 case Operation::Replication: code=8;return true;
 case Operation::Relay: code=9;return true;
 }
 return false;
}
bool decode_operation(std::uint8_t code,Operation& op) {
 switch(code) {
 case 1: op=Operation::Discovery;return true;
 case 2: op=Operation::Search;return true;
 case 3: op=Operation::Listing;return true;
 case 4: op=Operation::Metadata;return true;
 case 5: op=Operation::Upload;return true;
 case 6: op=Operation::Download;return true;
 case 7: op=Operation::Publish;return true;
 case 8: op=Operation::Replication;return true;
 case 9: op=Operation::Relay;return true;
 default:return false;
 }
}
}
bool serialize(const Frame& frame,std::vector<std::uint8_t>& out) {
 out.clear();
 std::uint8_t op=0;
 if(!operation_code(frame.operation,op)||frame.payload.size()>max_payload_bytes)
  return false;
 std::vector<std::uint8_t> result(header_bytes+frame.payload.size(),0);
 result[0]='M';result[1]='C';result[2]='P';result[3]='1';
 result[4]=1;result[5]=op;
 const std::uint32_t n=static_cast<std::uint32_t>(frame.payload.size());
 result[8]=static_cast<std::uint8_t>((n>>24)&255);
 result[9]=static_cast<std::uint8_t>((n>>16)&255);
 result[10]=static_cast<std::uint8_t>((n>>8)&255);
 result[11]=static_cast<std::uint8_t>(n&255);
 std::copy(frame.payload.begin(),frame.payload.end(),result.begin()+header_bytes);
 out.swap(result);return true;
}
bool authenticated_parse(const std::vector<std::uint8_t>& data,
 const VerifiedMembership& session,std::uint64_t now,Frame& decoded) {
 decoded={};
 // Fail closed before interpreting even metadata, CID, search and relay frames.
 if(data.size()<header_bytes||data.size()>header_bytes+max_payload_bytes)
  return false;
 if(data[0]!='M'||data[1]!='C'||data[2]!='P'||data[3]!='1'||
    data[4]!=1||data[6]!=0||data[7]!=0)return false;
 Operation op{};
 if(!decode_operation(data[5],op))return false;
 if(!authorize(op,session,now).allowed)return false;
 const auto len=(static_cast<std::uint32_t>(data[8])<<24) |
   (static_cast<std::uint32_t>(data[9])<<16) |
   (static_cast<std::uint32_t>(data[10])<<8) |
   static_cast<std::uint32_t>(data[11]);
 if(len>max_payload_bytes || len!=data.size()-header_bytes)return false;
 Frame result;
 result.operation=op;
 result.payload.assign(data.begin()+header_bytes,data.end());
 decoded=std::move(result);return true;
}
} // namespace meshcloud::wire
