// SPDX-License-Identifier: GPL-3.0-or-later
#include "ticket_issue.hpp"
#include "core.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <system_error>
#include <openssl/evp.h>
#include <openssl/rand.h>
namespace meshcloud::ticket_issue {
namespace {
constexpr std::size_t request_length=4+32+16+8*4+4+64;
constexpr std::size_t partial_length=4+32+32+64;
constexpr char owner_tag[]="MeshCloud/owner-ticket-request/v1";
void add32(std::vector<unsigned char>& b,std::uint32_t x) {
 for(unsigned i=0;i<4;i++)b.push_back(static_cast<unsigned char>((x>>(8*i))&255));
}
void add64(std::vector<unsigned char>& b,std::uint64_t x) {
 for(unsigned i=0;i<8;i++)b.push_back(static_cast<unsigned char>((x>>(8*i))&255));
}
bool read32(const std::vector<unsigned char>& b,std::size_t& at,std::uint32_t& x) {
 if(at>b.size()||b.size()-at<4)return false;
 x=0;for(unsigned i=0;i<4;i++)x|=static_cast<std::uint32_t>(b[at++])<<(8*i);
 return true;
}
bool read64(const std::vector<unsigned char>& b,std::size_t& at,std::uint64_t& x) {
 if(at>b.size()||b.size()-at<8)return false;
 x=0;for(unsigned i=0;i<8;i++)x|=static_cast<std::uint64_t>(b[at++])<<(8*i);
 return true;
}
template<std::size_t N>
bool take(const std::vector<unsigned char>& b,std::size_t& at,std::array<unsigned char,N>& out) {
 if(at>b.size()||b.size()-at<N)return false;
 std::copy(b.begin()+static_cast<std::ptrdiff_t>(at),
           b.begin()+static_cast<std::ptrdiff_t>(at+N),out.begin());at+=N;
 return true;
}
template<std::size_t N>
void put(std::vector<unsigned char>& b,const std::array<unsigned char,N>& x) {
 b.insert(b.end(),x.begin(),x.end());
}
std::vector<unsigned char> owner_message(const Request& r) {
 auto bytes=admission::audit_transcript(r.ticket);
 std::vector<unsigned char> msg(owner_tag,owner_tag+sizeof(owner_tag)-1);
 msg.insert(msg.end(),bytes.begin(),bytes.end());
 return msg;
}
bool sign(EVP_PKEY* key,const std::vector<unsigned char>& msg,
          admission::Signature& out) {
 out={};if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519)return false;
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();if(!ctx)return false;
 std::size_t n=out.size();
 const bool ok=EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1 &&
  EVP_DigestSign(ctx,out.data(),&n,msg.data(),msg.size())==1 && n==out.size();
 EVP_MD_CTX_free(ctx);
 if(!ok)out={};
 return ok;
}
bool verify(const admission::PublicKey& key,const admission::Signature& sig,
            const std::vector<unsigned char>& msg) {
 EVP_PKEY* pub=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,key.data(),key.size());
 if(!pub)return false;
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();if(!ctx){EVP_PKEY_free(pub);return false;}
 const bool yes=EVP_DigestVerifyInit(ctx,nullptr,nullptr,nullptr,pub)==1 &&
  EVP_DigestVerify(ctx,sig.data(),sig.size(),msg.data(),msg.size())==1;
 EVP_MD_CTX_free(ctx);EVP_PKEY_free(pub);return yes;
}
std::array<unsigned char,32> hash(const Request& request){
 std::array<unsigned char,32> digest{};
 const auto msg=owner_message(request);
 unsigned n=0;
 if(EVP_Digest(msg.data(),msg.size(),digest.data(),&n,EVP_sha256(),nullptr)!=1||n!=32)
  digest={};
 return digest;
}
bool shape(const admission::MemberTicket& t,std::uint64_t now) {
 return t.node_identity!=admission::PublicKey{} &&
    t.ticket_nonce!=std::array<unsigned char,16>{} &&
    t.protocol_version==1&&t.offered_bytes==minimum_bytes &&
    t.audited_bytes==minimum_bytes && t.auditors.empty() &&
    t.issued_at>0&&t.issued_at<=now && now-t.issued_at<=600 &&
    t.expires_at>now && t.expires_at>t.issued_at &&
    t.expires_at-t.issued_at==3600;
}
}
bool request(EVP_PKEY* key,std::uint64_t now,Request& out){
 out={};
 if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519||
    now==0||now>UINT64_MAX-3600)return false;
 Request value{};
 std::size_t n=value.ticket.node_identity.size();
 if(EVP_PKEY_get_raw_public_key(key,value.ticket.node_identity.data(),&n)!=1||
    n!=value.ticket.node_identity.size()||
    RAND_bytes(value.ticket.ticket_nonce.data(),16)!=1)return false;
 value.ticket.offered_bytes=minimum_bytes;
 value.ticket.audited_bytes=minimum_bytes;
 value.ticket.issued_at=now;value.ticket.expires_at=now+3600;
 if(!sign(key,owner_message(value),value.owner_proof))return false;
 out=std::move(value);return true;
}
bool verify_request(const Request& r,std::uint64_t now){
 return shape(r.ticket,now)&&
  verify(r.ticket.node_identity,r.owner_proof,owner_message(r));
}
bool encode_request(const Request& r,std::vector<unsigned char>& out){
 out.clear();const auto& t=r.ticket;
 if(!t.auditors.empty())return false;
 out.insert(out.end(),{'M','C','T','R'});
 put(out,t.node_identity);put(out,t.ticket_nonce);
 add64(out,t.offered_bytes);add64(out,t.audited_bytes);
 add64(out,t.issued_at);add64(out,t.expires_at);
 add32(out,t.protocol_version);put(out,r.owner_proof);
 return out.size()==request_length;
}
bool decode_request(const std::vector<unsigned char>& raw,Request& out){
 out={};
 if(raw.size()!=request_length||
    !std::equal(raw.begin(),raw.begin()+4,"MCTR"))return false;
 Request r{};auto& t=r.ticket;std::size_t at=4;
 if(!take(raw,at,t.node_identity)||!take(raw,at,t.ticket_nonce)||
    !read64(raw,at,t.offered_bytes)||!read64(raw,at,t.audited_bytes)||
    !read64(raw,at,t.issued_at)||!read64(raw,at,t.expires_at)||
    !read32(raw,at,t.protocol_version)||!take(raw,at,r.owner_proof)||
    at!=raw.size())return false;
 out=std::move(r);return true;
}
bool encode_partial(const Partial& part,std::vector<unsigned char>& out){
 out={'M','C','T','P'};put(out,part.request_sha256);
 put(out,part.signer);put(out,part.signature);
 return out.size()==partial_length;
}
bool decode_partial(const std::vector<unsigned char>& raw,Partial& out){
 out={};if(raw.size()!=partial_length||
    !std::equal(raw.begin(),raw.begin()+4,"MCTP"))return false;
 std::size_t at=4;Partial part{};
 if(!take(raw,at,part.request_sha256)||!take(raw,at,part.signer)||
    !take(raw,at,part.signature)||at!=raw.size())return false;
 out=part;return true;
}
bool attest(EVP_PKEY* auditor,const Request& r,
    const storage_audit::Ledger& ledger,storage_audit::VerifiedSample& proof,
    std::uint64_t now,Partial& out){
 out={};
 if(!verify_request(r,now)||ledger.node!=r.ticket.node_identity||
    ledger.bytes!=minimum_bytes ||now<r.ticket.issued_at||
    now>=r.ticket.expires_at)return false;
 admission::AuditorSignature sig{};
 if(!storage_audit::sign_ticket_after_proof(auditor,ledger,r.ticket,proof,sig))
  return false;
 Partial result{};
 result.signer=sig.signer;result.signature=sig.signature;
 result.request_sha256=hash(r);
 if(!verify_partial(r,result))return false;
 out=result;return true;
}
bool verify_partial(const Request& r,const Partial& part){
 const auto h=hash(r);
 return h!=std::array<unsigned char,32>{}&&part.request_sha256==h&&
  part.signer!=r.ticket.node_identity &&
  verify(part.signer,part.signature,admission::audit_transcript(r.ticket));
}
bool assemble(const Request& r,const std::vector<Partial>& parts,
 const admission::TrustPolicy& trusted,std::uint64_t now,
 admission::MemberTicket& result){
 result={};
 if(!verify_request(r,now)||trusted.quorum<3||
    trusted.pinned_auditors.size()<trusted.quorum||
    trusted.pinned_auditors.size()>32||parts.size()<trusted.quorum||
    parts.size()>32)return false;
 for(std::size_t i=0;i<trusted.pinned_auditors.size();i++){
  const auto& key=trusted.pinned_auditors[i];
  if(key==admission::PublicKey{}||key==r.ticket.node_identity)return false;
  for(std::size_t j=i+1;j<trusted.pinned_auditors.size();j++)
   if(key==trusted.pinned_auditors[j])return false;
 }
 std::vector<admission::PublicKey> accepted;
 admission::MemberTicket ticket=r.ticket;
 for(const auto& p:parts){
  if(std::find(trusted.pinned_auditors.begin(),trusted.pinned_auditors.end(),p.signer)==
        trusted.pinned_auditors.end()||!verify_partial(r,p)||
     std::find(accepted.begin(),accepted.end(),p.signer)!=accepted.end())
   return false;
  accepted.push_back(p.signer);
  ticket.auditors.push_back({p.signer,p.signature});
 }
 if(accepted.size()<trusted.quorum)return false;
 result=std::move(ticket);return true;
}
bool load_file(const std::filesystem::path& path,std::size_t max,
    std::vector<unsigned char>& out){
 out.clear();
 std::error_code ec;
 if(std::filesystem::symlink_status(path,ec).type()!=
       std::filesystem::file_type::regular||ec)return false;
 const auto n=std::filesystem::file_size(path,ec);
 if(ec||n==0||n>max)return false;
 std::ifstream input(path,std::ios::binary);if(!input)return false;
 out.resize(static_cast<std::size_t>(n));
 input.read(reinterpret_cast<char*>(out.data()),static_cast<std::streamsize>(out.size()));
 return input.good() ||
    (input.eof()&&input.gcount()==static_cast<std::streamsize>(out.size()));
}
bool write_new(const std::filesystem::path& path,const std::vector<unsigned char>& bytes,
    std::string& error){
 error.clear();std::error_code ec;
 auto status=std::filesystem::symlink_status(path,ec);
 if(status.type()!=std::filesystem::file_type::not_found ||
    (ec&&ec!=std::errc::no_such_file_or_directory)) {
  error="Refusing existing output or symlink";return false;
 }
 // This CLI is a staging utility; the destination directory must be private
 // and not writable by untrusted users. Never accept an untrusted shared path.
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 if(!out){error="Could not create output";return false;}
 out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
 out.flush();if(out.good())return true;
 error="Could not write output";return false;
}
} // namespace meshcloud::ticket_issue
