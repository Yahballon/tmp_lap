// SPDX-License-Identifier: GPL-3.0-or-later
#include "ticket_issue.hpp"
#include "audit_net.hpp"
#include "p2p_config.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <openssl/pem.h>
namespace fs=std::filesystem;
using namespace meshcloud;
namespace {
using Key=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
Key private_key(const fs::path& path){
 BIO* b=BIO_new_file(path.string().c_str(),"r");
 if(!b)return {nullptr,EVP_PKEY_free};
 EVP_PKEY* k=PEM_read_bio_PrivateKey(b,nullptr,
   [](char*,int,int,void*)->int {return 0;},nullptr);
 BIO_free(b);
 if(!k||EVP_PKEY_base_id(k)!=EVP_PKEY_ED25519) {
  if(k)EVP_PKEY_free(k);
  return {nullptr,EVP_PKEY_free};
 }
 return {k,EVP_PKEY_free};
}
std::uint64_t now_seconds(){
 const auto t=std::chrono::duration_cast<std::chrono::seconds>(
  std::chrono::system_clock::now().time_since_epoch()).count();
 return t>0?static_cast<std::uint64_t>(t):0;
}
int hex(char c){
 if(c>='0'&&c<='9')return c-'0';
 if(c>='a'&&c<='f')return c-'a'+10;
 if(c>='A'&&c<='F')return c-'A'+10;
 return -1;
}
bool auditor_keys(const fs::path& path,admission::TrustPolicy& trust){
 trust={};std::error_code ec;
 if(fs::symlink_status(path,ec).type()!=fs::file_type::regular||ec)return false;
 std::ifstream in(path,std::ios::binary);
 std::string line;
 while(std::getline(in,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.empty())continue;
  if(line.size()!=64||trust.pinned_auditors.size()>=32)return false;
  admission::PublicKey key{};
  for(int i=0;i<32;i++){
   const auto a=hex(line[2*i]),b=hex(line[2*i+1]);
   if(a<0||b<0)return false;
   key[static_cast<std::size_t>(i)]=static_cast<unsigned char>((a<<4)|b);
  }
  if(key==admission::PublicKey{} ||
     std::find(trust.pinned_auditors.begin(),trust.pinned_auditors.end(),key)!=
       trust.pinned_auditors.end())return false;
  trust.pinned_auditors.push_back(key);
 }
 trust.quorum=3;
 return in.eof()&&trust.pinned_auditors.size()>=3;
}
bool request_file(const fs::path& path,ticket_issue::Request& req){
 std::vector<unsigned char> bytes;
 return ticket_issue::load_file(path,200,bytes) &&
    ticket_issue::decode_request(bytes,req) &&
    ticket_issue::verify_request(req,now_seconds());
}
void help(){
 std::cout<<"MeshCloud Ticket v0.11 by YAHBALLON - independently verified 13 GiB\n"
 <<"  meshcloud-ticket request <node-identity.pem> <request.mctr>\n"
 <<"  meshcloud-ticket inspect <request.mctr> (prints PUBLIC identity; never private key)\n"
 <<"  meshcloud-ticket attest <auditor.pem> <node-LAN-IP> <independent-ledger.mcal> <request.mctr> <signature.mctp>\n"
 <<"  meshcloud-ticket assemble <request.mctr> <trusted-auditors.keys> <member.ticket> <sig1.mctp> <sig2.mctp> <sig3.mctp> [sig4...]\n"
 <<"All storage proofs require real pinned remote TLS, 13 GiB data and 3 distinct trusted auditors.\n"
 <<"No passwords, local fake tickets, guest modes or embedded signing keys.\n";
}
}
int main(int argc,char** argv) {
 if(argc<2){help();return 2;}
 const std::string operation=argv[1];
 const auto now=now_seconds();
 if(operation=="inspect"&&argc==3){
  ticket_issue::Request req{};
  if(!request_file(argv[2],req)){
   std::cerr<<"Invalid/expired signed ticket request\n";return 1;
  }
  constexpr char digits[]="0123456789abcdef";
  std::cout<<"Node public Ed25519 key: ";
  for(const auto byte:req.ticket.node_identity)
   std::cout<<digits[byte>>4]<<digits[byte&15];
  std::cout<<"\nRequired contribution: "<<req.ticket.offered_bytes
   <<" bytes (13 GiB), request at "<<req.ticket.issued_at
   <<", membership expires "<<req.ticket.expires_at<<"\n";
  return 0;
 }
 if(operation=="request"&&argc==4){
  auto node=private_key(argv[2]);
  ticket_issue::Request req{};
  std::vector<unsigned char> bytes;std::string error;
  if(!node||!ticket_issue::request(node.get(),now,req)||
     !ticket_issue::encode_request(req,bytes)||
     !ticket_issue::write_new(argv[3],bytes,error)){
   std::cerr<<"Could not create node-signed 13 GiB request: "<<error<<"\n";return 1;
  }
  std::cout<<"Node-signed request created; valid to start audits for 10 minutes. "
             "All auditors MUST attest against this SAME file.\n";
  return 0;
 }
 if(operation=="attest"&&argc==7){
  auto auditor=private_key(argv[2]);
  ticket_issue::Request req{};
  storage_audit::Ledger ledger{};
  std::string error;
  if(!auditor||!request_file(argv[5],req)||
     !storage_audit::load_ledger(argv[4],ledger)||
     ledger.node!=req.ticket.node_identity||ledger.bytes!=minimum_bytes) {
   std::cerr<<"Invalid independently provisioned 13 GiB ledger/request/auditor key\n";
   return 1;
  }
  storage_audit::VerifiedSample proof;
  if(!audit_net::challenge_once(argv[3],audit_net::port,auditor.get(),
          req.ticket.node_identity,ledger,proof,error)){
   std::cerr<<"Remote independent storage challenge DENIED: "<<error<<"\n";
   return 1;
  }
  ticket_issue::Partial signed_proof{};
  std::vector<unsigned char> bytes;
  if(!ticket_issue::attest(auditor.get(),req,ledger,proof,now_seconds(),signed_proof)||
     !ticket_issue::encode_partial(signed_proof,bytes)||
     !ticket_issue::write_new(argv[6],bytes,error)){
   std::cerr<<"Cannot sign partial auditor ticket: "<<error<<"\n";return 1;
  }
  std::cout<<"Authenticated remote 13 GiB audit PASSED; produced one independently signed partial.\n";
  return 0;
 }
 if(operation=="assemble"&&argc>=8&&argc<=37){
  ticket_issue::Request req{};
  admission::TrustPolicy pinned{};
  if(!request_file(argv[2],req)||!auditor_keys(argv[3],pinned)){
   std::cerr<<"Ticket request invalid/expired or no 3 pinned independent auditor keys\n";return 1;
  }
  std::vector<ticket_issue::Partial> attestations;
  for(int i=5;i<argc;i++){
   std::vector<unsigned char> data;ticket_issue::Partial part{};
   if(!ticket_issue::load_file(argv[i],160,data)||
      !ticket_issue::decode_partial(data,part)){
    std::cerr<<"Malformed auditor partial file\n";return 1;
   }
   attestations.push_back(part);
  }
  admission::MemberTicket ticket{};
  std::vector<std::uint8_t> binary;
  std::string error;
  if(!ticket_issue::assemble(req,attestations,pinned,now_seconds(),ticket)||
     !p2p::serialize_member_ticket(ticket,binary)||
     !ticket_issue::write_new(argv[4],binary,error)){
   std::cerr<<"Quorum DENIED: need 3 trusted unique valid signatures on SAME node request. "
               <<error<<"\n";return 1;
  }
  std::cout<<"Complete 3-of-5 Ed25519 ticket built for verified 13 GiB membership. "
              "Copy member.ticket to the matching node config; peer nodes must trust the same auditor keys.\n";
  return 0;
 }
 help();return 2;
}
