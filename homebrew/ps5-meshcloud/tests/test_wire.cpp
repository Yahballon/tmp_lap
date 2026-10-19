// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "wire.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace meshcloud;
int main() {
 const std::uint64_t now=1800000000;
 const Operation every[]={Operation::Discovery,Operation::Search,
 Operation::Listing,Operation::Metadata,Operation::Upload,Operation::Download,
 Operation::Publish,Operation::Replication,Operation::Relay};
 VerifiedMembership unverified{};
 VerifiedMembership proven{minimum_bytes,minimum_bytes,now+100,true,true,1};
 for(const auto op:every) {
  wire::Frame msg{op,{1,2,3,4}};
  std::vector<std::uint8_t> encoded;
  assert(wire::serialize(msg,encoded));
  wire::Frame received{};
  assert(!wire::authenticated_parse(encoded,unverified,now,received));
  assert(wire::authenticated_parse(encoded,proven,now,received));
  assert(received.operation==op&&received.payload==msg.payload);
  auto malformed=encoded;malformed[4]=9;
  assert(!wire::authenticated_parse(malformed,proven,now,received));
  malformed=encoded;malformed[6]=1;
  assert(!wire::authenticated_parse(malformed,proven,now,received));
  malformed=encoded;malformed[5]=99;
  assert(!wire::authenticated_parse(malformed,proven,now,received));
  malformed=encoded;malformed[11]+=1;
  assert(!wire::authenticated_parse(malformed,proven,now,received));
  malformed=encoded;malformed.pop_back();
  assert(!wire::authenticated_parse(malformed,proven,now,received));
 }
 auto failed=proven;failed.authenticated_proof=false;
 wire::Frame read{Operation::Download,{7}};
 std::vector<std::uint8_t> encoded;
 assert(wire::serialize(read,encoded));
 assert(!wire::authenticated_parse(encoded,failed,now,read));
 failed=proven;failed.expires_at=now;
 assert(!wire::authenticated_parse(encoded,failed,now,read));
 wire::Frame max{Operation::Upload,std::vector<std::uint8_t>(wire::max_payload_bytes,17)};
 assert(wire::serialize(max,encoded));
 wire::Frame result{};
 assert(wire::authenticated_parse(encoded,proven,now,result));
 max.payload.push_back(0);
 assert(!wire::serialize(max,encoded));
 std::cout<<"Member-only framing, malformed message rejection and payload bounds passed\n";
}
