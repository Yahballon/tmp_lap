// SPDX-License-Identifier: GPL-3.0-or-later
// Direct, symmetric mesh peer CLI. No public discovery or unaudited access.
#include "p2p_config.hpp"
#include "peer_discovery.hpp"
#include "p2p_tls.hpp"
#include "core.hpp"
#include "cid.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
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
 <<"get <config-directory> <remote-IPv4> <CIDv1> <output.mcb> [port]\n"
 <<"listen-loop <config-directory> <shared-store> <bind-IPv4> [port]\n"
 <<"discover <config-directory> <remote-IPv4> [port]\n"
 <<"discover-auto <config-directory> [port] (cached signed endpoints)\n"
 <<"peers <config-directory> (persisted signed peer entries)\n\n"
 <<"config: identity.pem, member.ticket, auditors.keys, peers.keys.\n"
 <<"Requires 3 independently validated 13-GiB auditor signatures. NO guest mode.\n"
 <<"Default TCP port 39718; only encrypted MCB1 chunks. NO filenames/keys sent.\n";
}
}
int main(int argc,char** argv){
 if(argc<2){usage();return 2;}
 const std::string op=argv[1];
 if(op=="peers"||op=="discover"||op=="discover-auto") {
  const bool peers=op=="peers",auto_discovery=op=="discover-auto";
  if((peers&&argc!=3) || (auto_discovery&&(argc<3||argc>4)) ||
     (!peers&&!auto_discovery&&(argc<4||argc>5))) {
   usage();return 2;
  }
  std::uint16_t requested_port=meshcloud::p2p::default_port;
  const int port_arg=auto_discovery?3:4;
  if(!peers&&argc>port_arg&&!port_of(argv[port_arg],requested_port)){
   std::cerr<<"Invalid P2P discovery TCP port\n";return 2;
  }
  meshcloud::p2p::Config configuration;
  std::string error;
  if(!configuration.load(argv[2],error)){
   std::cerr<<"P2P membership LOCKED: "<<error<<"\n";return 1;
  }
  meshcloud::discovery::AddressBook book(fs::path(argv[2])/"addrman");
  if(!book.open()){std::cerr<<"Addrman database error: "<<book.error()<<"\n";return 1;}
  std::vector<meshcloud::discovery::SignedAddress> known;
  const auto seconds=std::chrono::duration_cast<std::chrono::seconds>(
       std::chrono::system_clock::now().time_since_epoch()).count();
  if(seconds<0||!book.list(static_cast<std::uint64_t>(seconds),known,
                           meshcloud::discovery::max_known_peers)){
   std::cerr<<"Stored peer list is invalid\n";return 1;
  }
  if(peers){
   std::cout<<"Known signed LAN endpoints: "<<known.size()<<"\n";
   for(const auto& p:known)
    std::cout<<"  "<<meshcloud::discovery::ipv4_text(p)<<":"<<p.port
             <<" signed, expires "<<p.expires<<"\n";
   return 0;
  }
  // Explicit opt-in. Absence of advertise.ipv4 NEVER exposes the LAN IP.
  std::string advertise;
  std::ifstream settings(fs::path(argv[2])/"advertise.ipv4");
  if(settings) {
   std::getline(settings,advertise);
   if(!advertise.empty()&&advertise.back()=='\r')advertise.pop_back();
  }
  if(!auto_discovery){
   if(!meshcloud::p2p::discover_once(argv[3],requested_port,
        configuration.node(),book,advertise,error)){
    std::cerr<<"Authenticated getaddr failed: "<<error<<"\n";return 1;
   }
  } else {
   bool found=false;
   for(const auto& known_peer:known){
    if(meshcloud::p2p::discover_once(meshcloud::discovery::ipv4_text(known_peer),
         known_peer.port,configuration.node(),book,advertise,error)){
     found=true;break;
    }
   }
   if(!found) {
    std::cerr<<"Could not contact any saved audited LAN peer: "<<error<<"\n";return 1;
   }
  }
  known.clear();
  book.list(static_cast<std::uint64_t>(seconds),known,
            meshcloud::discovery::max_known_peers);
  std::cout<<"Member-only getaddr completed; persisted "<<known.size()
           <<" signed peer endpoint(s) for next startup\n";
  return 0;
 }
 const bool loop=op=="listen-loop";
 const bool server=op=="listen"||loop;
 if((server&&(argc<5||argc>6))||
    ((op=="put")&&(argc<5||argc>6))||
    ((op=="get")&&(argc<6||argc>7)) ||
    (op!="listen"&&op!="listen-loop"&&op!="put"&&op!="get")) {usage();return 2;}
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
   std::cerr<<"P2P DENIED: at least 13 GiB + safety margin required: "
            <<eligibility.message<<"\n";return 1;
  }
  meshcloud::exchange::SharedShardStore store(root,meshcloud::minimum_bytes);
  if(!store.open()){std::cerr<<"Store error: "<<store.error()<<"\n";return 1;}
  meshcloud::exchange::PeerGateway gateway(store);
  meshcloud::discovery::AddressBook book(fs::path(argv[2])/"addrman");
  if(!book.open()) {
   std::cerr<<"Cannot open signed local peer database\n";return 1;
  }
  std::ifstream announce(fs::path(argv[2])/"advertise.ipv4");
  if(announce) {
   std::string ip;
   std::getline(announce,ip);
   if(!ip.empty()&&ip.back()=='\r')ip.pop_back();
   meshcloud::discovery::SignedAddress record{};
   const auto clock=std::chrono::duration_cast<std::chrono::seconds>(
       std::chrono::system_clock::now().time_since_epoch()).count();
   if(clock<=0 || !meshcloud::discovery::parse_lan_ipv4(ip,record.ipv4) ||
      (record.port=port,record.published=static_cast<std::uint64_t>(clock),
       record.expires=record.published+meshcloud::discovery::record_lifetime,
       !meshcloud::discovery::sign(record,config.node().private_key)) ||
      !book.add(record,record.published)){
    std::cerr<<"Refusing invalid opt-in advertise.ipv4\n";return 1;
   }
  }
  std::cout<<"Audited P2P peer on "<<argv[4]<<":"<<port
           <<" (TLS1.3, signed getaddr + encrypted fragments)\n";
  do {
   if(meshcloud::p2p::listen_once(argv[4],port,config.node(),gateway,
                                 err,nullptr,&book)){
    std::cout<<"Authenticated peer request completed\n";
   } else if(err!="Timed out waiting for peer") {
    std::cerr<<"Peer request denied: "<<err<<"\n";
    if(err.find("bind/listen")!=std::string::npos)return 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
   }
  } while(loop);
  return 0;
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
