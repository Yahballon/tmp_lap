// SPDX-License-Identifier: GPL-3.0-or-later
#include "storage_audit.hpp"
#include "core.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <system_error>
#include <openssl/evp.h>
#include <openssl/rand.h>
namespace meshcloud::storage_audit {
namespace {
namespace fs=std::filesystem;
constexpr std::uint64_t max_blocks=(meshcloud::maximum_bytes+block_bytes-1)/block_bytes;
constexpr std::size_t header_size=4+1+32+16+8+4+4;
bool ordinary(const fs::path& p){
 std::error_code ec;
 return fs::symlink_status(p,ec).type()==fs::file_type::regular&&!ec;
}
bool absent(const fs::path& p){
 std::error_code ec;
 const auto status=fs::symlink_status(p,ec);
 return status.type()==fs::file_type::not_found &&
        (!ec||ec==std::errc::no_such_file_or_directory);
}
bool valid_node(const admission::PublicKey& p){return p!=admission::PublicKey{};}
void w32(std::ostream& f,std::uint32_t n){for(int i=0;i<4;i++)f.put(static_cast<char>((n>>(8*i))&255));}
void w64(std::ostream& f,std::uint64_t n){for(int i=0;i<8;i++)f.put(static_cast<char>((n>>(8*i))&255));}
bool r32(std::istream& f,std::uint32_t& n){
 n=0;for(int i=0;i<4;i++){const auto c=f.get();if(c<0)return false;n|=static_cast<std::uint32_t>(c)<<(8*i);}return true;
}
bool r64(std::istream& f,std::uint64_t& n){
 n=0;for(int i=0;i<8;i++){const auto c=f.get();if(c<0)return false;n|=static_cast<std::uint64_t>(c)<<(8*i);}return true;
}
bool hash(const unsigned char* data,std::size_t size,Digest& digest){
 unsigned n=0;return EVP_Digest(data,size,digest.data(),&n,EVP_sha256(),nullptr)==1&&n==32;
}
bool block_count(std::uint64_t bytes,std::uint64_t& blocks){
 if(bytes==0||bytes>max_blocks*static_cast<std::uint64_t>(block_bytes))return false;
 blocks=(bytes+block_bytes-1)/block_bytes;
 return blocks>=challenge_samples && blocks<=max_blocks;
}
std::size_t block_length(const Ledger& ledger,std::size_t index){
 const auto offset=static_cast<std::uint64_t>(index)*block_bytes;
 return static_cast<std::size_t>(std::min<std::uint64_t>(block_bytes,ledger.bytes-offset));
}
std::uint32_t unbiased_random(std::uint32_t max,bool& okay){
 std::uint32_t value=0;
 const auto reject=static_cast<std::uint32_t>(0U-max)%max;
 for(unsigned attempt=0;attempt<50;attempt++){
  if(RAND_bytes(reinterpret_cast<unsigned char*>(&value),sizeof(value))!=1){okay=false;return 0;}
  if(value>=reject)return value%max;
 }
 okay=false;return 0;
}
}
bool provision(const fs::path& data,const fs::path& commitment,
 const admission::PublicKey& node,std::uint64_t bytes,std::string& error){
 error.clear();std::uint64_t count=0;std::error_code ec;
 if(!valid_node(node)||!block_count(bytes,count)){error="Invalid audit node or dataset length";return false;}
 if(!absent(data)||!absent(commitment) ||
    data.lexically_normal()==commitment.lexically_normal()){
  error="Will not overwrite existing audit files";return false;
 }
 Nonce unique{};
 if(RAND_bytes(unique.data(),static_cast<int>(unique.size()))!=1){error="CSPRNG failed";return false;}
 std::ofstream file(data,std::ios::binary|std::ios::trunc);
 if(!file){error="Could not open node storage file";return false;}
 std::ofstream auditor(commitment,std::ios::binary|std::ios::trunc);
 if(!auditor){file.close();fs::remove(data,ec);error="Could not create auditor commitment";return false;}
 auditor.write("MCAL",4);auditor.put(1);
 auditor.write(reinterpret_cast<const char*>(node.data()),32);
 auditor.write(reinterpret_cast<const char*>(unique.data()),16);
 w64(auditor,bytes);w32(auditor,block_bytes);w32(auditor,static_cast<std::uint32_t>(count));
 std::vector<unsigned char> buffer(block_bytes);
 bool okay=auditor.good();
 for(std::uint64_t index=0;okay&&index<count;index++){
  const auto length=static_cast<std::size_t>(
    std::min<std::uint64_t>(block_bytes,bytes-index*block_bytes));
  Digest digest{};
  okay=RAND_bytes(buffer.data(),static_cast<int>(length))==1 &&
       hash(buffer.data(),length,digest);
  if(!okay)break;
  file.write(reinterpret_cast<const char*>(buffer.data()),static_cast<std::streamsize>(length));
  auditor.write(reinterpret_cast<const char*>(digest.data()),static_cast<std::streamsize>(digest.size()));
  okay=file.good()&&auditor.good();
 }
 std::fill(buffer.begin(),buffer.end(),0);
 file.flush();auditor.flush();okay=okay&&file.good()&&auditor.good();
 file.close();auditor.close();
 if(!okay){fs::remove(data,ec);fs::remove(commitment,ec);error="Random audit provision failed";return false;}
 return true;
}
bool load_ledger(const fs::path& path,Ledger& out){
 out={};if(!ordinary(path))return false;
 std::error_code ec;const auto size=fs::file_size(path,ec);
 if(ec||size<header_size||size>header_size+max_blocks*32)return false;
 std::ifstream in(path,std::ios::binary);if(!in)return false;
 char header[5]{};in.read(header,5);
 if(in.gcount()!=5||header[0]!='M'||header[1]!='C'||header[2]!='A'||
    header[3]!='L'||header[4]!=1)return false;
 Ledger temp{};
 in.read(reinterpret_cast<char*>(temp.node.data()),32);
 in.read(reinterpret_cast<char*>(temp.dataset_id.data()),16);
 std::uint32_t size_per_block=0,count=0;
 if(!in||!r64(in,temp.bytes)||!r32(in,size_per_block)||!r32(in,count))return false;
 std::uint64_t expected=0;
 if(!valid_node(temp.node)||size_per_block!=block_bytes||
    !block_count(temp.bytes,expected)||count!=expected||
    size!=header_size+expected*32)return false;
 temp.block_hashes.resize(count);
 for(auto& digest:temp.block_hashes){
  in.read(reinterpret_cast<char*>(digest.data()),static_cast<std::streamsize>(digest.size()));
  if(!in)return false;
 }
 out=std::move(temp);return true;
}
bool make_challenge(const Ledger& ledger,std::uint64_t now,Challenge& out){
 out={};std::uint64_t expected=0;
 if(!valid_node(ledger.node)||!now||
    now>std::numeric_limits<std::uint64_t>::max()-challenge_lifetime||
    !block_count(ledger.bytes,expected)||
    ledger.block_hashes.size()!=expected)return false;
 Challenge result{};
 result.node=ledger.node;result.dataset_id=ledger.dataset_id;
 result.deadline=now+challenge_lifetime;
 if(RAND_bytes(result.challenge_id.data(),static_cast<int>(result.challenge_id.size()))!=1)
  return false;
 bool okay=true;
 while(result.indices.size()<challenge_samples){
  const auto index=unbiased_random(static_cast<std::uint32_t>(expected),okay);
  if(!okay)return false;
  if(std::find(result.indices.begin(),result.indices.end(),index)==result.indices.end())
   result.indices.push_back(index);
 }
 out=std::move(result);return true;
}
bool answer(const fs::path& path,const Challenge& challenge,Answer& out){
 out={};if(!ordinary(path)||challenge.indices.size()!=challenge_samples)return false;
 std::error_code ec;const auto length=fs::file_size(path,ec);
 if(ec||length==0)return false;
 std::ifstream file(path,std::ios::binary);if(!file)return false;
 Answer reply{};reply.challenge_id=challenge.challenge_id;
 for(const auto index:challenge.indices){
  const auto offset=static_cast<std::uint64_t>(index)*block_bytes;
  if(offset>=length)return false;
  const auto size=static_cast<std::size_t>(std::min<std::uint64_t>(block_bytes,length-offset));
  file.seekg(static_cast<std::streamoff>(offset),std::ios::beg);
  if(!file)return false;
  std::vector<unsigned char> bytes(size);
  file.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(size));
  if(file.gcount()!=static_cast<std::streamsize>(size))return false;
  reply.sampled_bytes.push_back(std::move(bytes));
 }
 out=std::move(reply);return true;
}
bool verify(const Ledger& ledger,const Challenge& challenge,const Answer& answer,
 std::uint64_t now,bool& consumed,VerifiedSample& verified){
 verified.accepted_=false; // fail closed: stale success never survives a failed check
 if(consumed||now>challenge.deadline||
    challenge.deadline<now||challenge.deadline-now>challenge_lifetime||
    challenge.indices.size()!=challenge_samples||
    answer.sampled_bytes.size()!=challenge_samples||
    challenge.challenge_id!=answer.challenge_id||
    challenge.dataset_id!=ledger.dataset_id||challenge.node!=ledger.node||
    ledger.block_hashes.empty())return false;
 consumed=true;
 for(std::size_t i=0;i<challenge_samples;i++){
  const auto index=challenge.indices[i];
  if(index>=ledger.block_hashes.size()||
     std::find(challenge.indices.begin(),
       challenge.indices.begin()+static_cast<std::ptrdiff_t>(i),index)!=
       challenge.indices.begin()+static_cast<std::ptrdiff_t>(i))return false;
  const auto& sample=answer.sampled_bytes[i];
  if(sample.size()!=block_length(ledger,index))return false;
  Digest digest{};
  if(!hash(sample.data(),sample.size(),digest)||digest!=ledger.block_hashes[index])return false;
 }
 verified.accepted_=true;
 verified.node_=ledger.node;verified.dataset_id_=ledger.dataset_id;
 verified.bytes_=ledger.bytes;verified.verified_at_=now;
 return true;
}
bool sign_ticket_after_proof(EVP_PKEY* key,const Ledger& ledger,
 const admission::MemberTicket& ticket,VerifiedSample& proven,
 admission::AuditorSignature& result){
 result={};
 if(!proven.accepted_||proven.node_!=ledger.node||
    proven.dataset_id_!=ledger.dataset_id||proven.bytes_!=ledger.bytes||
    ticket.issued_at<proven.verified_at_||
    ticket.issued_at-proven.verified_at_>60||
    !key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519||
    ledger.bytes!=minimum_bytes||ledger.node!=ticket.node_identity||
    ticket.audited_bytes!=minimum_bytes||ticket.audited_bytes>ledger.bytes||
    ticket.offered_bytes!=minimum_bytes||ticket.expires_at<=ticket.issued_at||
    ticket.expires_at-ticket.issued_at>3600||ticket.protocol_version!=1||
    ticket.ticket_nonce==Nonce{})return false;
 std::size_t len=result.signer.size();
 if(EVP_PKEY_get_raw_public_key(key,result.signer.data(),&len)!=1||
    len!=result.signer.size())return false;
 const auto transcript=admission::audit_transcript(ticket);
 EVP_MD_CTX* context=EVP_MD_CTX_new();if(!context)return false;
 len=result.signature.size();
 const bool okay=EVP_DigestSignInit(context,nullptr,nullptr,nullptr,key)==1 &&
    EVP_DigestSign(context,result.signature.data(),&len,
                   transcript.data(),transcript.size())==1 &&
    len==result.signature.size();
 EVP_MD_CTX_free(context);
 if(!okay)result={};
 else proven.accepted_=false; // ONE signed ticket per verified challenge
 return okay;
}
} // namespace meshcloud::storage_audit
