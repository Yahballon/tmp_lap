// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "core.hpp"
#include "cid.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
int main() {
 using namespace meshcloud;
 const std::uint64_t now=1800000000ULL;
 const Operation all[]={Operation::Discovery,Operation::Search,Operation::Listing,
   Operation::Metadata,Operation::Upload,Operation::Download,Operation::Publish,
   Operation::Replication,Operation::Relay};
 VerifiedMembership m{};
 for(const auto op:all)assert(!authorize(op,m,now).allowed);
 m={minimum_bytes-1,minimum_bytes,now+60,true,true,1};
 for(const auto op:all)assert(authorize(op,m,now).denial==Denial::Quota);
 m.claimed_bytes=minimum_bytes;m.audited_bytes=minimum_bytes-1;
 for(const auto op:all)assert(authorize(op,m,now).denial==Denial::StorageProof);
 m.audited_bytes=minimum_bytes;
 m.claimed_bytes=maximum_bytes+1;
 for(const auto op:all)assert(authorize(op,m,now).denial==Denial::Quota);
 m.claimed_bytes=minimum_bytes;
 m.audited_bytes=maximum_bytes+1;
 for(const auto op:all)assert(authorize(op,m,now).denial==Denial::StorageProof);
 m.audited_bytes=minimum_bytes;m.authenticated_proof=false;
 for(const auto op:all)assert(!authorize(op,m,now).allowed);
 m.authenticated_proof=true;m.authenticated_identity=false;
 for(const auto op:all)assert(!authorize(op,m,now).allowed);
 m.authenticated_identity=true;m.expires_at=now-1;
 for(const auto op:all)assert(!authorize(op,m,now).allowed);
 m.expires_at=now+3601;
 for(const auto op:all)assert(!authorize(op,m,now).allowed);
 m.expires_at=now+60;
 for(const auto op:all)assert(authorize(op,m,now).allowed);
 assert(!preflight(".",minimum_bytes-1).eligible);
 assert(!preflight(".",maximum_bytes+1).eligible);
 const std::vector<std::uint8_t> empty{};
 const auto e=cid::from_bytes(empty);
 assert(e=="bafkreihdwdcefgh4dqkjv67uzcmw7ojee6xedzdetojuzjevtenxquvyku");
 assert(cid::validate(e)&&cid::matches(e,empty));
 assert(!cid::validate(e.substr(0,e.size()-1)));
 auto bad=e;bad[3]='Z';assert(!cid::validate(bad));
 const std::vector<std::uint8_t> a={'a','b','c'},b={'a','b','d'};
 const auto id=cid::from_bytes(a);
 assert(cid::validate(id)&&cid::matches(id,a)&&!cid::matches(id,b));
 std::cout<<"MeshCloud membership restrictions and CID tests passed\n";
}
