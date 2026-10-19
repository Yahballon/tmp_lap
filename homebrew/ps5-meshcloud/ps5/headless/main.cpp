// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 native text/log controlled node: no OpenGL dependency.
// Use the same security and transport core as the graphical UI.
#include "core.hpp"
#include "storage_audit.hpp"
#include "admission.hpp"
#include "provision.hpp"
#include "audit_net.hpp"
#include "ticket_issue.hpp"
#include "p2p_config.hpp"
#include "p2p_tls.hpp"
#include "peer_discovery.hpp"
#include "exchange.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
namespace {
namespace fs=std::filesystem;
const fs::path root="/data/PS5MeshCloud";
std::string bind_ip;
std::string service="setup";
void log(const std::string& text) {
 std::error_code ec;
 fs::create_directories(root,ec);
 std::ofstream file(root/"node.log",std::ios::app);
 if(file)file<<"MeshCloud v0.12 PS5: "<<text<<"\n";
 std::fprintf(stderr,"MeshCloud v0.12: %s\n",text.c_str());
}
bool read_config() {
 std::ifstream in(root/"node.conf");
 if(!in)return true;
 std::string line;
 while(std::getline(in,line)){
  if(line.size()>100)continue;
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  const auto split=line.find('=');
  if(split==std::string::npos)continue;
  const auto key=line.substr(0,split);
  const auto value=line.substr(split+1);
  if(key=="mode")service=value;
  else if(key=="bind")bind_ip=value;
 }
 return service=="setup"||service=="provision"||
        service=="audit"||service=="p2p";
}
bool identity() {
 const fs::path keyfile=root/"p2p"/"identity.pem";
 std::error_code ec;
 fs::create_directories(keyfile.parent_path(),ec);
 if(ec)return false;
 const auto status=fs::symlink_status(keyfile,ec);
 if(status.type()==fs::file_type::regular && !ec)return true;
 if(status.type()!=fs::file_type::not_found)return false;
 EVP_PKEY_CTX* context=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);
 EVP_PKEY* key=nullptr;
 bool ok=context&&EVP_PKEY_keygen_init(context)==1&&
         EVP_PKEY_keygen(context,&key)==1;
 if(context)EVP_PKEY_CTX_free(context);
 if(!ok){if(key)EVP_PKEY_free(key);return false;}
 const int fd=open(keyfile.c_str(),O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0600);
 if(fd<0){EVP_PKEY_free(key);return false;}
 FILE* stream=fdopen(fd,"w");
 if(!stream){close(fd);unlink(keyfile.c_str());EVP_PKEY_free(key);return false;}
 BIO* bio=BIO_new_fp(stream,BIO_NOCLOSE);
 ok=bio&&PEM_write_bio_PrivateKey(bio,key,nullptr,nullptr,0,nullptr,nullptr)==1 &&
    BIO_flush(bio)==1&&fflush(stream)==0&&fsync(fd)==0;
 if(bio)BIO_free(bio);
 fclose(stream);
 if(ok) {
  meshcloud::admission::PublicKey pub{};
  std::size_t n=pub.size();
  ok=EVP_PKEY_get_raw_public_key(key,pub.data(),&n)==1&&n==pub.size();
  if(ok) {
   constexpr char digits[]="0123456789abcdef";
   std::ofstream output(root/"p2p"/"node-public.hex",std::ios::trunc);
   if(!output)ok=false;
   else {
    for(unsigned char c:pub)output<<digits[c>>4]<<digits[c&15];
    output<<"\n";output.flush();ok=output.good();
   }
  }
 }
 EVP_PKEY_free(key);
 if(!ok)unlink(keyfile.c_str());
 return ok;
}
EVP_PKEY* load_private(){
 BIO* bio=BIO_new_file((root/"p2p"/"identity.pem").string().c_str(),"r");
 if(!bio)return nullptr;
 EVP_PKEY* key=PEM_read_bio_PrivateKey(bio,nullptr,
      [](char*,int,int,void*)->int{return 0;},nullptr);
 BIO_free(bio);
 if(key&&EVP_PKEY_base_id(key)==EVP_PKEY_ED25519)return key;
 if(key)EVP_PKEY_free(key);
 return nullptr;
}
int digit(char c){
 if(c>='0'&&c<='9')return c-'0';
 if(c>='a'&&c<='f')return c-'a'+10;
 if(c>='A'&&c<='F')return c-'A'+10;
 return -1;
}
bool pins(std::vector<meshcloud::admission::PublicKey>& out){
 out.clear();std::ifstream file(root/"p2p"/"auditors.keys");
 if(!file)return false;
 std::string line;
 while(std::getline(file,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty())continue;
  if(line.size()!=64||out.size()>=32)return false;
  meshcloud::admission::PublicKey key{};
  for(unsigned i=0;i<32;i++){
   auto a=digit(line[i*2]),b=digit(line[i*2+1]);
   if(a<0||b<0)return false;
   key[i]=static_cast<unsigned char>((a<<4)|b);
  }
  if(key==meshcloud::admission::PublicKey{} ||
     std::find(out.begin(),out.end(),key)!=out.end())return false;
  out.push_back(key);
 }
 return file.eof()&&out.size()>=3;
}
void sleep_brief(){std::this_thread::sleep_for(std::chrono::seconds(2));}
}
int main(){
 log("Started headless secure P2P node (graphics-free fallback)");
 if(!identity()){log("LOCKED: could not safely establish Ed25519 identity");return 1;}
 if(!read_config()){log("LOCKED: unknown node.conf mode");return 1;}
 if(service=="setup"){
  log("Initial node identity generated/loaded. Public key: p2p/node-public.hex");
  log("Set mode=provision or mode=audit in /data/PS5MeshCloud/node.conf and restart.");
  log("Never put node identity.pem or auditor private keys on an untrusted peer.");
  return 0;
 }
 if(bind_ip.empty()||bind_ip=="0.0.0.0"){
  log("LOCKED: configure bind=YOUR_PS5_PRIVATE_IPV4 in node.conf");
  return 1;
 }
 std::vector<meshcloud::admission::PublicKey> trusted;
 if(!pins(trusted)){log("LOCKED: pin >=3 independent audit public keys first");return 1;}
 if(service=="provision"){
  EVP_PKEY* key=load_private();
  if(!key){log("LOCKED: invalid node key");return 1;}
  std::error_code ec;fs::create_directories(root/"audit",ec);
  if(ec){EVP_PKEY_free(key);return 1;}
  std::string error;
  log("Auditor-only TLS provisioning listening on TCP 39720; no CID access.");
  const bool success=meshcloud::provision::receive_once(
      bind_ip,meshcloud::provision::port,key,trusted,
      root/"audit"/"node-random.bin",root/"audit"/"ledger.mcal",
      meshcloud::minimum_bytes,error,
      [](std::uint64_t done,std::uint64_t total){
       if(total)log("Verified 13 GiB sentinel: "+std::to_string(done*100/total)+"%");
      });
  EVP_PKEY_free(key);
  log(success?"COMPLETE: all 13 GiB verified and durably committed":
              "DENIED: "+error);
  return success?0:1;
 }
 if(service=="audit"){
  meshcloud::storage_audit::Ledger ledger{};
  if(!meshcloud::storage_audit::load_ledger(root/"audit"/"ledger.mcal",ledger)||
      ledger.bytes!=meshcloud::minimum_bytes){
   log("LOCKED: require complete 13 GiB independent audit dataset");return 1;
  }
  EVP_PKEY* key=load_private();
  if(!key)return 1;
  log("Auditor-only challenge listener TCP 39719 (no member ticket/guest CIDs).");
  while(true){
   std::string error;
   if(meshcloud::audit_net::serve_one(bind_ip,meshcloud::audit_net::port,key,
       trusted,root/"audit"/"node-random.bin",ledger,error))
    log("Auditor's independent challenge answered over pinned TLS");
   else log("Audit request denied or timeout: "+error);
   sleep_brief();
  }
 }
 if(service=="p2p"){
  meshcloud::p2p::Config configuration;
  std::string error;
  if(!configuration.load(root/"p2p",error)){
   log("LOCKED: 3-of-5 independent member.ticket not valid: "+error);
   return 1;
  }
  meshcloud::storage_audit::Ledger ledger{};
  std::error_code ec;
  const auto data=root/"audit"/"node-random.bin";
  if(!meshcloud::storage_audit::load_ledger(root/"audit"/"ledger.mcal",ledger)||
     ledger.bytes!=meshcloud::minimum_bytes||
     !fs::is_regular_file(data,ec)||ec||fs::file_size(data,ec)!=meshcloud::minimum_bytes){
   log("LOCKED: no 13 GiB durable dataset on disk");return 1;
  }
  meshcloud::exchange::SharedShardStore store(root/"shared",meshcloud::minimum_bytes);
  if(!store.open()){log("LOCKED: shared ciphertext store cannot open");return 1;}
  meshcloud::exchange::PeerGateway gateway(store);
  meshcloud::discovery::AddressBook addr(root/"p2p"/"addrman");
  if(!addr.open()){log("LOCKED: peer address book failed");return 1;}
  log("Mutual TLS1.3 / pinned peer / 3-auditor ticket P2P listener TCP 39718");
  while(true){
   if(meshcloud::p2p::listen_once(bind_ip,meshcloud::p2p::default_port,
        configuration.node(),gateway,error,nullptr,&addr))
    log("Completed authorized encrypted CID request");
   else log("Member-only P2P denied or timeout: "+error);
   sleep_brief();
  }
 }
 log("LOCKED: invalid mode");return 1;
}
