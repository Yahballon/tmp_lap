// SPDX-License-Identifier: GPL-3.0-or-later
#include "core.hpp"
#include "cid.hpp"
#include <fstream>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <string>
int main(int argc,char** argv) {
 std::string directory="."; std::uint64_t quota_gib=10;std::string cid_file;
 for(int i=1;i<argc;i++) {
  std::string arg=argv[i];
  if(arg=="--help" || arg=="-h") {
   std::cout<<"MeshCloud by YAHBALLON v0.1.0 pre-alpha\n"
    <<"Usage: meshcloud [--storage PATH] [--quota-gib N]\n"
    <<"Members must provide 10 GiB verified storage; NETWORK DISABLED pending audited proof protocol.\n";
   return 0;
  }
  if(arg=="--cid-file" && i+1<argc) cid_file=argv[++i];
  else if(arg=="--storage" && i+1<argc) directory=argv[++i];
  else if(arg=="--quota-gib" && i+1<argc) {
   std::string s=argv[++i];
   if(s.empty() || s.find_first_not_of("0123456789")!=std::string::npos) {
    std::cerr<<"Invalid quota\n";return 2;
   }
   try {quota_gib=std::stoull(s);}catch(...) {std::cerr<<"Invalid quota\n";return 2;}
   if(quota_gib>1000000) {std::cerr<<"Quota out of range\n";return 2;}
  } else {std::cerr<<"Unknown or incomplete argument: "<<arg<<"\n";return 2;}
 }
 if(!cid_file.empty()) {
  std::ifstream input(cid_file,std::ios::binary);
  if(!input) {std::cerr<<"Cannot read file\n";return 2;}
  std::vector<std::uint8_t> data;
  char buffer[65536];
  while(input) {
   input.read(buffer,sizeof(buffer));const auto n=input.gcount();
   if(n>0) {
    if(data.size()+(std::size_t)n>32ULL*1024*1024) {
     std::cerr<<"CID diagnostic limited to files <= 32 MiB in pre-alpha\n";return 2;
    }
    data.insert(data.end(),buffer,buffer+n);
   }
  }
  if(input.bad()) {std::cerr<<"File read failed\n";return 2;}
  std::cout<<meshcloud::cid::from_bytes(data)<<"\n";
  return 0;
 }
 auto r=meshcloud::preflight(directory,quota_gib*1024ULL*1024*1024);
 std::cout<<"MeshCloud by YAHBALLON | Desktop Node Preflight\n"
  <<"Required minimum: 10 GiB | Offered: "<<quota_gib<<" GiB\n"
  <<"Free disk: "<<(r.free_bytes/1024/1024)<<" MiB\n"
  <<"Disk readiness: "<<(r.eligible?"PASS":"DENIED")<<" - "<<r.message<<"\n"
  <<"Membership: DENIED (cryptographic remote storage attestation not implemented)\n"
  <<"Discovery/Search/Upload/Download: DISABLED\n";
 return r.eligible?0:1;
}
