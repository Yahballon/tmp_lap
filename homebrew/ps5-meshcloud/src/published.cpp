// SPDX-License-Identifier: GPL-3.0-or-later
#include "published.hpp"
#include "cid.hpp"
#include "vault.hpp"
#include <algorithm>
#include <limits>

namespace meshcloud::published {
namespace {
void u32(std::vector<unsigned char>& out,std::uint32_t n) {
 for(unsigned i=0;i<4;i++)out.push_back(static_cast<unsigned char>((n>>(8*i))&255));
}
void u64(std::vector<unsigned char>& out,std::uint64_t n) {
 for(unsigned i=0;i<8;i++)out.push_back(static_cast<unsigned char>((n>>(8*i))&255));
}
bool shape(const Page& p) {
 using i64=std::numeric_limits<std::int64_t>;
 if(!cid::validate(p.file_manifest_cid) || p.file_manifest_cid.size()!=59 ||
    p.page_count==0 || p.index>=p.page_count ||
    p.chunks.size()>max_chunks_per_page ||
    p.created_at==0 || p.issued_at<p.created_at ||
    p.issued_at>static_cast<std::uint64_t>(i64::max())-page_retention_seconds ||
    p.expires_at!=p.issued_at+page_retention_seconds ||
    p.ciphertext_bytes>static_cast<std::uint64_t>(i64::max()))return false;
 if(p.chunks.empty() && !(p.ciphertext_bytes==0 &&
      p.page_count==1 && p.index==0))return false;
 std::uint64_t sum=0;
 for(const auto& chunk:p.chunks) {
  if(chunk.cid.size()!=59 || !cid::validate(chunk.cid) ||
     chunk.bytes<36 || chunk.bytes>chunk_bytes+36 ||
     chunk.bytes>p.ciphertext_bytes-sum)return false;
  sum+=chunk.bytes;
 }
 if(sum>p.ciphertext_bytes)return false;
 if(p.publisher==admission::PublicKey{})return false;
 return true;
}
}
std::vector<unsigned char> transcript(const Page& p) {
 constexpr char domain[]="MeshCloud/published-ciphertext-page/v1";
 std::vector<unsigned char> bytes;
 bytes.reserve(120+p.chunks.size()*63);
 bytes.insert(bytes.end(),domain,domain+sizeof(domain)-1);
 bytes.insert(bytes.end(),p.publisher.begin(),p.publisher.end());
 bytes.insert(bytes.end(),p.file_manifest_cid.begin(),p.file_manifest_cid.end());
 u64(bytes,p.created_at);
 u64(bytes,p.issued_at);
 u64(bytes,p.expires_at);
 u64(bytes,p.ciphertext_bytes);
 u32(bytes,p.index);
 u32(bytes,p.page_count);
 u32(bytes,static_cast<std::uint32_t>(p.chunks.size()));
 for(const auto& chunk:p.chunks) {
  bytes.insert(bytes.end(),chunk.cid.begin(),chunk.cid.end());
  u32(bytes,chunk.bytes);
 }
 return bytes;
}
bool sign(Page& p,EVP_PKEY* key) {
 p.signature.fill(0);
 if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519)return false;
 std::size_t count=p.publisher.size();
 if(EVP_PKEY_get_raw_public_key(key,p.publisher.data(),&count)!=1 ||
    count!=p.publisher.size() || !shape(p))return false;
 const auto msg=transcript(p);
 auto* context=EVP_MD_CTX_new();
 if(!context)return false;
 std::size_t signature_size=p.signature.size();
 const bool ok=EVP_DigestSignInit(context,nullptr,nullptr,nullptr,key)==1 &&
  EVP_DigestSign(context,p.signature.data(),&signature_size,
                 msg.data(),msg.size())==1 &&
  signature_size==p.signature.size();
 EVP_MD_CTX_free(context);
 if(!ok)p.signature.fill(0);
 return ok;
}
bool verify(const Page& p,const admission::PublicKey& trusted,
            std::uint64_t now) {
 if(!shape(p) || p.publisher!=trusted || now<p.issued_at ||
    now>=p.expires_at || p.created_at>now)return false;
 auto* key=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,
              p.publisher.data(),p.publisher.size());
 if(!key)return false;
 auto* ctx=EVP_MD_CTX_new();
 if(!ctx){EVP_PKEY_free(key);return false;}
 const auto msg=transcript(p);
 const bool valid=EVP_DigestVerifyInit(ctx,nullptr,nullptr,nullptr,key)==1 &&
   EVP_DigestVerify(ctx,p.signature.data(),p.signature.size(),
                     msg.data(),msg.size())==1;
 EVP_MD_CTX_free(ctx);EVP_PKEY_free(key);
 return valid;
}
bool verify_complete(const std::vector<Page>& pages,
                     const admission::PublicKey& trusted,std::uint64_t now) {
 if(pages.empty()||pages.size()>static_cast<std::size_t>(
      std::numeric_limits<std::uint32_t>::max()))return false;
 const auto& first=pages[0];
 if(first.index!=0 || first.page_count!=pages.size())return false;
 std::uint64_t total=0;
 for(std::size_t i=0;i<pages.size();i++) {
  const auto& p=pages[i];
  if(p.index!=i || p.page_count!=pages.size() ||
     p.publisher!=first.publisher ||
     p.file_manifest_cid!=first.file_manifest_cid ||
     p.created_at!=first.created_at || p.issued_at!=first.issued_at ||
     p.expires_at!=first.expires_at ||
     p.ciphertext_bytes!=first.ciphertext_bytes ||
     !verify(p,trusted,now))return false;
  for(const auto& c:p.chunks) {
   if(c.bytes>first.ciphertext_bytes-total)return false;
   total+=c.bytes;
  }
 }
 return total==first.ciphertext_bytes;
}
bool flatten(const std::vector<Page>& pages,
             const admission::PublicKey& trusted,
             std::uint64_t now,std::vector<ChunkRef>& out) {
 out.clear();
 if(!verify_complete(pages,trusted,now))return false;
 for(const auto& p:pages)
  out.insert(out.end(),p.chunks.begin(),p.chunks.end());
 return true;
}
} // namespace meshcloud::published
