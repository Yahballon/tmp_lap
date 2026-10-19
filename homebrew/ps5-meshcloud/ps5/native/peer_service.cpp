// SPDX-License-Identifier: GPL-3.0-or-later
#include "peer_service.hpp"
#include "p2p_tls.hpp"
#include "p2p_config.hpp"
#include "peer_discovery.hpp"
#include "cid.hpp"
#include "core.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <chrono>
#include <vector>
namespace meshps5 {
namespace {
namespace fs=std::filesystem;
const fs::path home="/data/PS5MeshCloud";
struct Network {
 std::string bind,remote,cid,advertise;
};
Network settings() {
 Network net;
 std::ifstream config(home/"p2p"/"network.conf");
 std::string line;
 while(std::getline(config,line)) {
  if(line.size()>160)continue;
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  const auto sep=line.find('=');
  if(sep==std::string::npos)continue;
  const auto key=line.substr(0,sep),value=line.substr(sep+1);
  if(key=="bind")net.bind=value;
  else if(key=="advertise")net.advertise=value;
  else if(key=="peer")net.remote=value;
  else if(key=="cid")net.cid=value;
 }
 return net;
}
bool read_chunk(const fs::path& path,std::vector<std::uint8_t>& out) {
 std::error_code ec;
 if(!fs::is_regular_file(path,ec)||ec)return false;
 const auto size=fs::file_size(path,ec);
 if(ec||size<36||size>meshcloud::exchange::encrypted_block_limit)return false;
 std::ifstream input(path,std::ios::binary);
 if(!input)return false;
 out.resize(static_cast<std::size_t>(size));
 input.read(reinterpret_cast<char*>(out.data()),static_cast<std::streamsize>(out.size()));
 return input.gcount()==static_cast<std::streamsize>(out.size())&&!input.bad();
}
bool write_new(const fs::path& path,const std::vector<std::uint8_t>& bytes) {
 std::error_code ec;
 if(fs::exists(path,ec)||ec)return false;
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 if(!out)return false;
 out.write(reinterpret_cast<const char*>(bytes.data()),
           static_cast<std::streamsize>(bytes.size()));
 out.flush();return out.good();
}
}
PeerService::~PeerService(){
 if(worker_.joinable())worker_.join();
}
void PeerService::report(std::string message) {
 std::lock_guard<std::mutex> guard(mutex_);
 status_=std::move(message);
}
std::string PeerService::status() const {
 std::lock_guard<std::mutex> guard(mutex_);
 return status_;
}
bool PeerService::launch(Action action,std::string path) {
 if(busy_.exchange(true))return false;
 if(worker_.joinable())worker_.join();
 worker_=std::thread([this,action,path=std::move(path)]{
  run(action,path);
  busy_=false;
 });
 report("P2P: secure peer job started");
 return true;
}
bool PeerService::listen(){return launch(Action::Listen);}
bool PeerService::send_local(const std::string& path){return launch(Action::Put,path);}
bool PeerService::fetch_cid(){return launch(Action::Get);}
bool PeerService::discover(){return launch(Action::Discover);}
void PeerService::run(Action action,const std::string& path) {
 meshcloud::p2p::Config config;
 std::string error;
 if(!config.load(home/"p2p",error)){
  report("P2P LOCKED: "+error);return;
 }
 const auto options=settings();
 meshcloud::discovery::AddressBook book(home/"p2p"/"addrman");
 if(!book.open()){report("P2P: cannot access signed peer database");return;}
 const auto preflight=meshcloud::preflight("/data",meshcloud::minimum_bytes);
 if(!preflight.eligible) {
  report("P2P LOCKED: no verified local 10 GiB + safety reserve");
  return;
 }
 if(action==Action::Listen) {
  meshcloud::exchange::SharedShardStore store(home/"shared",meshcloud::minimum_bytes);
  if(!store.open()){report("P2P: cannot open encrypted shared store");return;}
  meshcloud::exchange::PeerGateway gateway(store);
  if(!options.advertise.empty()){
   meshcloud::discovery::SignedAddress me{};
   if(!meshcloud::discovery::parse_lan_ipv4(options.advertise,me.ipv4)){
    report("P2P: invalid opt-in advertise LAN address");return;
   }
   me.port=meshcloud::p2p::default_port;
   const auto t=std::chrono::duration_cast<std::chrono::seconds>(
         std::chrono::system_clock::now().time_since_epoch()).count();
   if(t<=0){report("P2P: invalid time");return;}
   me.published=static_cast<std::uint64_t>(t);
   me.expires=me.published+meshcloud::discovery::record_lifetime;
   if(!meshcloud::discovery::sign(me,config.node().private_key) ||
      !book.add(me,me.published)){
    report("P2P: could not advertise signed endpoint");return;
   }
  }
  if(!meshcloud::p2p::listen_once(options.bind,meshcloud::p2p::default_port,
                                  config.node(),gateway,error,nullptr,&book)){
   report("P2P denied: "+error);return;
  }
  report("P2P: authenticated encrypted peer request completed");
  return;
 }
 std::string next_peer=options.remote;
 if(next_peer.empty()) {
  const auto t=std::chrono::duration_cast<std::chrono::seconds>(
       std::chrono::system_clock::now().time_since_epoch()).count();
  std::vector<meshcloud::discovery::SignedAddress> known;
  if(t>0 && book.list(static_cast<std::uint64_t>(t),known,16)){
   for(const auto& p:known){
    if(p.identity!=config.node().ticket.node_identity) {
     next_peer=meshcloud::discovery::ipv4_text(p);break;
    }
   }
  }
 }
 if(next_peer.empty()){
  report("P2P: configure initial peer=IPv4 or reconnect via cached signed addrman");return;
 }
 if(action==Action::Discover) {
  if(!meshcloud::p2p::discover_once(next_peer,meshcloud::p2p::default_port,
          config.node(),book,options.advertise,error)) {
   report("P2P getaddr denied: "+error);return;
  }
  std::vector<meshcloud::discovery::SignedAddress> known;
  const auto t=std::chrono::duration_cast<std::chrono::seconds>(
       std::chrono::system_clock::now().time_since_epoch()).count();
  if(t>=0 && book.list(static_cast<std::uint64_t>(t),known))
   report("P2P: "+std::to_string(known.size())+" signed LAN peers cached");
  else report("P2P: authenticated getaddr complete");
  return;
 }
 std::string cid;
 std::vector<std::uint8_t> request,response;
 if(action==Action::Put) {
  std::vector<std::uint8_t> encrypted;
  if(!read_chunk(path,encrypted)){
   report("P2P: select a local encrypted MCB1 block (max 4 MiB)");
   return;
  }
  cid=meshcloud::cid::from_bytes(encrypted);
  if(!meshcloud::exchange::make_put(cid,encrypted,request)){
   report("P2P: encrypted block/CID integrity check failed");return;
  }
 } else {
  cid=options.cid;
  if(!meshcloud::exchange::make_get(cid,request)){
   report("P2P: configure valid cid=b... in network.conf");return;
  }
 }
 if(!meshcloud::p2p::dial_once(next_peer,meshcloud::p2p::default_port,
                               config.node(),request,response,error)){
  report("P2P denied: "+error);return;
 }
 if(action==Action::Put) {
  if(response.size()!=13||response[5]!=4||response[12]!=0) {
   report("P2P: remote peer did not accept encrypted CID");return;
  }
  report("P2P: encrypted block sent (CID "+cid.substr(0,20)+"...)");
  return;
 }
 if(response.size()<12+59+36||response[5]!=6){
  report("P2P: encrypted chunk not found on remote peer");return;
 }
 const std::string got(response.begin()+12,response.begin()+12+59);
 std::vector<std::uint8_t> fragment(response.begin()+12+59,response.end());
 if(got!=cid||fragment.size()>meshcloud::exchange::encrypted_block_limit||
    !meshcloud::cid::matches(cid,fragment)){
  report("P2P: rejected invalid remote block hash");return;
 }
 std::error_code ec;
 fs::create_directories(home/"received",ec);
 if(ec||!write_new(home/"received"/(cid+".mcb"),fragment)){
  report("P2P: cannot save new verified encrypted block");return;
 }
 report("P2P: verified encrypted block received (CID "+cid.substr(0,20)+"...)");
}
} // namespace meshps5
