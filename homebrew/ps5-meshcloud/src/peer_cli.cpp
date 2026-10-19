// SPDX-License-Identifier: GPL-3.0-or-later
// Direct, symmetric mesh peer CLI. No public discovery or unaudited access.
#include "p2p_config.hpp"
#include "p2p_tls.hpp"
#include "core.hpp"
#include "cid.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
namespace fs=std::filesystem;
namespace {
bool port_of(const std::string& text,std::uint16_t& port) {
 if(text.empty()||text.find_first_not_of("0123456789")!=std::string::npos)
  return false;
 try {
  const auto parsed=std::stoul(text);
  if(parsed==0||parsed>65535)return false;
  port=static_cast<std::uint16_t>(parsed);
  return true;
 }catch(...){return false;}
}
bool read_block(const fs::path& path,std::vector<std::uint8_t>& bytes){
 std::error_code ec;
 if(!fs::is_regular_file(path,ec)||ec)return false;
 const auto count=fs::file_size(path,ec);
 if(ec||count<36||count>meshcloud::exchange::encrypted_block_limit)return false;
 std::ifstream in(path,std::ios::binary);
 if(!in)return false;
 bytes.resize(static_cast<std::size_t>(count));
 in.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
 return in.gcount()==static_cast<std::streamsize>(bytes.size())&&!in.bad();
}
bool write_no_overwrite(const fs::path& path,const std::vector<std::uint8_t>& bytes){
 std::error_code ec;
 if(fs::exists(path,ec)||ec)return false;
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 if(!out)return false;
 out.write(reinterpret_cast<const char*>(bytes.data()),
           static_cast<std::streamsize>(bytes.size()));
 out.flush();return out.good();
}
void usage(){
 std::cout<<"MeshCloud P2P v0.7 by YAHBALLON (audited members only)\n"
 <<"listen <config-directory> <shared-store> <bind-IPv4> [port]\n"
 <<"put <config-directory> <remote-IPv4> <encrypted.mcb> [port]\n"
 <<"get <config-directory> <remote-IPv4> <CIDv1> <output.mcb> [port]\n\n"
 <<"config: identity.pem, member.ticket, auditors.keys, peers.keys.\n"
 <<"Requires 3 independently validated 10-GiB auditor signatures. NO guest mode.\n"
 <<"Default TCP port 39718; only encrypted MCB1 chunks. NO filenames/keys sent.\n";
}
}
int main(int argc,char** argv){
 if(argc<2){usage();return 2;}
 const std::string op=argv[1];
 const bool server=op=="listen";
 if((server&&(argc<5||argc>6))||
    ((op=="put")&&(argc<5||argc>6))||
    ((op=="get")&&(argc<6||argc>7)) ||
    (op!="listen"&&op!="put"&&op!="get")) {usage();return 2;}
 std::uint16_t port=meshcloud::p2p::default_port;
 const int port_at=server||op=="put"?5:6;
 if(argc>port_at&&!port_of(argv[port_at],port)){
  std::cerr<<"Invalid TCP port\n";return 2;
 }
 meshcloud::p2p::Config config;
 std::string err;
 if(!config.load(argv[2],err)) {
  std::cerr<<"P2P admission LOCKED: "<<err<<"\n";return 1;
 }
 if(server) {
  std::error_code ec;
  const fs::path root=argv[3];
  fs::create_directories(root,ec);
  if(ec){std::cerr<<"Cannot create shared store\n";return 1;}
  const auto eligibility=meshcloud::preflight(root,meshcloud::minimum_bytes);
  if(!eligibility.eligible) {
   std::cerr<<"P2P DENIED: at least 10 GiB + safety margin required: "
            <<eligibility.message<<"\n";return 1;
  }
  meshcloud::exchange::SharedShardStore store(root,meshcloud::minimum_bytes);
  if(!store.open()){std::cerr<<"Store error: "<<store.error()<<"\n";return 1;}
  meshcloud::exchange::PeerGateway gateway(store);
  std::cout<<"P2P peer listening on "<<argv[4]<<":"<<port
           <<" (one mutually authenticated operation)\n";
  if(!meshcloud::p2p::listen_once(argv[4],port,config.node(),gateway,err)) {
   std::cerr<<"Peer operation not admitted: "<<err<<"\n";return 1;
  }
  std::cout<<"Authenticated peer request completed\n";return 0;
 }
 const std::string peer_ip=argv[3];
 std::vector<std::uint8_t> wire,response;
 std::string id;
 if(op=="put") {
  std::vector<std::uint8_t> ciphertext;
  if(!read_block(argv[4],ciphertext)){
   std::cerr<<"Only regular encrypted .mcb chunks up to 4 MiB are accepted\n";
   return 1;
  }
  id=meshcloud::cid::from_bytes(ciphertext);
  if(!meshcloud::exchange::make_put(id,ciphertext,wire)) {
   std::cerr<<"Invalid MCB1 ciphertext envelope or CID\n";return 1;
  }
 } else {
  id=argv[4];
  if(!meshcloud::exchange::make_get(id,wire)){
   std::cerr<<"Invalid raw SHA-256 CIDv1\n";return 1;
  }
  std::error_code ec;
  if(fs::exists(argv[5],ec)||ec){
   std::cerr<<"Refusing to overwrite existing output file\n";return 1;
  }
 }
 if(!meshcloud::p2p::dial_once(peer_ip,port,config.node(),wire,response,err)){
  std::cerr<<"P2P DENIED/FAILED: "<<err<<"\n";return 1;
 }
 if(op=="put") {
  if(response.size()!=13 || response[5]!=4 || response[12]!=0){
   std::cerr<<"Peer rejected encrypted chunk\n";return 1;
  }
  std::cout<<"Encrypted CID uploaded to peer: "<<id<<"\n";return 0;
 }
 meshcloud::wire::Frame frame;
 // The TLS layer already validated the peer's signed member session.
 // Enforce CID and ciphertext SHA-256 again before writing ANY disk output.
 if(response.size()<12+59+36 || response[5]!=6) {
  std::cerr<<"Peer did not return an encrypted fragment\n";return 1;
 }
 const std::string remote_cid(response.begin()+12,response.begin()+12+59);
 std::vector<std::uint8_t> ciphertext(response.begin()+12+59,response.end());
 if(remote_cid!=id||!meshcloud::cid::matches(id,ciphertext) ||
    ciphertext.size()>meshcloud::exchange::encrypted_block_limit ||
    !write_no_overwrite(argv[5],ciphertext)){
  std::cerr<<"CID mismatch, invalid size or could not write destination\n";
  return 1;
 }
 std::cout<<"Authenticated encrypted fragment retrieved: "<<id<<"\n";
}
