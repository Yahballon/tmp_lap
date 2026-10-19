// SPDX-License-Identifier: GPL-3.0-or-later
#include "erasure.hpp"
#include "cid.hpp"
#include <algorithm>
#include <utility>
namespace meshcloud::erasure {
namespace {
// GF(256), primitive polynomial x^8+x^4+x^3+x^2+1 (0x11d).
std::uint8_t mul(std::uint8_t a,std::uint8_t b) noexcept {
 unsigned product=0;
 for(unsigned bit=0;bit<8;bit++) {
  if(b&1u)product^=a;
  const unsigned carry=a&0x80u;
  a=static_cast<std::uint8_t>(a<<1);
  if(carry)a^=0x1du;
  b=static_cast<std::uint8_t>(b>>1);
 }
 return static_cast<std::uint8_t>(product);
}
std::uint8_t power(std::uint8_t x,unsigned e) noexcept {
 std::uint8_t result=1;
 while(e) {
  if(e&1u)result=mul(result,x);
  x=mul(x,x);
  e>>=1;
 }
 return result;
}
std::uint8_t inverse(std::uint8_t x) noexcept {
 return x?power(x,254):0;
}
bool invert(const std::vector<std::vector<std::uint8_t>>& source,
            std::vector<std::vector<std::uint8_t>>& output) {
 const std::size_t k=source.size();
 if(k==0)return false;
 std::vector<std::vector<std::uint8_t>> matrix(
  k,std::vector<std::uint8_t>(2*k,0));
 for(std::size_t r=0;r<k;r++) {
  if(source[r].size()!=k)return false;
  std::copy(source[r].begin(),source[r].end(),matrix[r].begin());
  matrix[r][k+r]=1;
 }
 for(std::size_t column=0;column<k;column++) {
  std::size_t pivot=column;
  while(pivot<k && matrix[pivot][column]==0)pivot++;
  if(pivot==k)return false;
  if(pivot!=column)std::swap(matrix[pivot],matrix[column]);
  const std::uint8_t inv=inverse(matrix[column][column]);
  for(std::size_t c=0;c<2*k;c++)
   matrix[column][c]=mul(matrix[column][c],inv);
  for(std::size_t r=0;r<k;r++) {
   if(r==column)continue;
   const std::uint8_t factor=matrix[r][column];
   if(!factor)continue;
   for(std::size_t c=0;c<2*k;c++)
    matrix[r][c]^=mul(factor,matrix[column][c]);
  }
 }
 output.assign(k,std::vector<std::uint8_t>(k));
 for(std::size_t r=0;r<k;r++)
  std::copy(matrix[r].begin()+static_cast<std::ptrdiff_t>(k),
            matrix[r].end(),output[r].begin());
 return true;
}
}
Codec::Codec(std::size_t k,std::size_t m):k_(k),m_(m){}
bool Codec::valid() const noexcept {
 return k_>=1 && k_<=32 && m_>=1 && m_<=254 && k_+m_<=255;
}
std::uint8_t Codec::coefficient(std::size_t row,std::size_t col) const noexcept {
 // First k rows = identity (systematic); others = Cauchy matrix.
 // x=(k + parity_row), y=col; all coordinates are distinct in GF256.
 if(row<k_)return static_cast<std::uint8_t>(row==col?1:0);
 const auto x=static_cast<std::uint8_t>(row);
 const auto y=static_cast<std::uint8_t>(col);
 return inverse(static_cast<std::uint8_t>(x^y));
}
bool Codec::encode(const std::vector<Bytes>& data,std::vector<Bytes>& shards) const {
 shards.clear();
 if(!valid() || data.size()!=k_ || data[0].empty() ||
    data[0].size()>max_shard_bytes)return false;
 const auto length=data[0].size();
 for(const auto& fragment:data)if(fragment.size()!=length)return false;
 std::vector<Bytes> output=data;
 output.resize(k_+m_,Bytes(length,0));
 for(std::size_t r=k_;r<k_+m_;r++) {
  auto& parity=output[r];
  for(std::size_t col=0;col<k_;col++) {
   const auto factor=coefficient(r,col);
   for(std::size_t i=0;i<length;i++)
    parity[i]^=mul(factor,data[col][i]);
  }
 }
 shards.swap(output);
 return true;
}
bool Codec::recover(const std::vector<std::optional<Bytes>>& available,
                    const std::vector<std::string>& authenticated_cids,
                    std::vector<Bytes>& original_data) const {
 original_data.clear();
 if(!valid() || available.size()!=shard_count() ||
    authenticated_cids.size()!=shard_count())return false;
 // Validate all content claims before giving a fragment any influence.
 std::vector<std::size_t> selected;
 std::size_t len=0;
 for(std::size_t i=0;i<shard_count();i++) {
  if(!cid::validate(authenticated_cids[i]))return false;
  if(!available[i].has_value())continue;
  const auto& fragment=*available[i];
  if(fragment.empty()||fragment.size()>max_shard_bytes)continue;
  if(!cid::matches(authenticated_cids[i],fragment))continue;
  if(!len)len=fragment.size();
  if(fragment.size()!=len)continue;
  selected.push_back(i);
  if(selected.size()==k_)break;
 }
 if(selected.size()!=k_)return false;
 std::vector<std::vector<std::uint8_t>> rows(
  k_,std::vector<std::uint8_t>(k_));
 for(std::size_t r=0;r<k_;r++)
  for(std::size_t col=0;col<k_;col++)
   rows[r][col]=coefficient(selected[r],col);
 std::vector<std::vector<std::uint8_t>> inverse_rows;
 if(!invert(rows,inverse_rows))return false;
 std::vector<Bytes> restored(k_,Bytes(len,0));
 for(std::size_t r=0;r<k_;r++) {
  for(std::size_t source=0;source<k_;source++) {
   const auto factor=inverse_rows[r][source];
   const auto& chunk=*available[selected[source]];
   for(std::size_t i=0;i<len;i++)
    restored[r][i]^=mul(factor,chunk[i]);
  }
 }
 // Reject a solution that doesn't match the original systematic shard CIDs.
 for(std::size_t r=0;r<k_;r++)
  if(!cid::matches(authenticated_cids[r],restored[r]))return false;
 original_data.swap(restored);
 return true;
}
} // namespace meshcloud::erasure
