// SPDX-License-Identifier: GPL-3.0-or-later
#include "admission.hpp"
#include <algorithm>
#include <limits>
#include <openssl/evp.h>
namespace meshcloud::admission {
namespace {
void append64(std::vector<unsigned char>& bytes,std::uint64_t v) {
 for(unsigned i=0;i<8;i++)bytes.push_back(static_cast<unsigned char>((v>>(8*i))&255));
}
void append32(std::vector<unsigned char>& bytes,std::uint32_t v) {
 for(unsigned i=0;i<4;i++)bytes.push_back(static_cast<unsigned char>((v>>(8*i))&255));
}
template<std::size_t N>
bool nonzero(const std::array<unsigned char,N>& value) {
 return std::any_of(value.begin(),value.end(),[](unsigned char x){return x!=0;});
}
bool verify_signature(const PublicKey& key,const Signature& signature,
                       const std::vector<unsigned char>& msg) {
 EVP_PKEY* public_key=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,key.data(),key.size());
 if(!public_key)return false;
 EVP_MD_CTX* context=EVP_MD_CTX_new();
 if(!context){EVP_PKEY_free(public_key);return false;}
 const bool ok=EVP_DigestVerifyInit(context,nullptr,nullptr,nullptr,public_key)==1 &&
  EVP_DigestVerify(context,signature.data(),signature.size(),msg.data(),msg.size())==1;
 EVP_MD_CTX_free(context);EVP_PKEY_free(public_key);
 return ok;
}
std::array<unsigned char,32> hash(const std::vector<unsigned char>& msg) {
 std::array<unsigned char,32> digest{};
 unsigned len=0;
 if(EVP_Digest(msg.data(),msg.size(),digest.data(),&len,EVP_sha256(),nullptr)!=1 || len!=32)
  digest.fill(0);
 return digest;
}
}
std::vector<unsigned char> audit_transcript(const MemberTicket& ticket) {
 constexpr char tag[]="MeshCloud/audit/v1";
 std::vector<unsigned char> out;
 out.reserve(sizeof(tag)+96);
 out.insert(out.end(),tag,tag+sizeof(tag)-1);
 out.insert(out.end(),ticket.node_identity.begin(),ticket.node_identity.end());
 out.insert(out.end(),ticket.ticket_nonce.begin(),ticket.ticket_nonce.end());
 append64(out,ticket.offered_bytes);
 append64(out,ticket.audited_bytes);
 append64(out,ticket.issued_at);
 append64(out,ticket.expires_at);
 append32(out,ticket.protocol_version);
 return out;
}
std::vector<unsigned char> session_transcript(const MemberTicket& ticket,
    const Challenge& nonce,const Challenge& exporter) {
 constexpr char tag[]="MeshCloud/session/v1";
 const auto audit_hash=hash(audit_transcript(ticket));
 std::vector<unsigned char> msg;
 msg.reserve(sizeof(tag)+audit_hash.size()+nonce.size()+exporter.size());
 msg.insert(msg.end(),tag,tag+sizeof(tag)-1);
 msg.insert(msg.end(),audit_hash.begin(),audit_hash.end());
 msg.insert(msg.end(),nonce.begin(),nonce.end());
 msg.insert(msg.end(),exporter.begin(),exporter.end());
 return msg;
}
bool verify_member(const MemberTicket& ticket,const TrustPolicy& trusted,
    const Challenge& nonce,const Challenge& exporter,
    const Signature& proof,std::uint64_t now,VerifiedMembership& verified) {
 verified={};
 // No bootstrap through self-reported disk values, zero channel binders,
 // duplicate auditors, untrusted keys or stale/replayed auditor tickets.
 if(ticket.protocol_version!=1 || !nonzero(ticket.node_identity) ||
    !nonzero(ticket.ticket_nonce) || !nonzero(nonce) || !nonzero(exporter) ||
    (ticket.offered_bytes<minimum_bytes || ticket.offered_bytes>maximum_bytes) ||
    (ticket.audited_bytes<minimum_bytes || ticket.audited_bytes>maximum_bytes) ||
    ticket.issued_at>now || now>=ticket.expires_at ||
    ticket.expires_at-ticket.issued_at>3600 ||
    trusted.quorum<3 || trusted.quorum>trusted.pinned_auditors.size() ||
    trusted.pinned_auditors.size()>32 || ticket.auditors.size()>32)
  return false;
 if(std::any_of(trusted.pinned_auditors.begin(),trusted.pinned_auditors.end(),
       [&](const auto& key){return key==ticket.node_identity || !nonzero(key);}))
  return false;
 for(std::size_t i=0;i<trusted.pinned_auditors.size();i++)
  for(std::size_t j=i+1;j<trusted.pinned_auditors.size();j++)
   if(trusted.pinned_auditors[i]==trusted.pinned_auditors[j])return false;
 const auto msg=audit_transcript(ticket);
 std::vector<PublicKey> accepted;
 for(const auto& a:ticket.auditors) {
  if(std::find(trusted.pinned_auditors.begin(),trusted.pinned_auditors.end(),
               a.signer)==trusted.pinned_auditors.end())continue;
  if(std::find(accepted.begin(),accepted.end(),a.signer)!=accepted.end())continue;
  if(!verify_signature(a.signer,a.signature,msg))continue;
  accepted.push_back(a.signer);
 }
 if(accepted.size()<trusted.quorum)return false;
 if(!verify_signature(ticket.node_identity,proof,session_transcript(ticket,nonce,exporter)))
  return false;
 verified.claimed_bytes=ticket.offered_bytes;
 verified.audited_bytes=ticket.audited_bytes;
 verified.expires_at=ticket.expires_at;
 verified.authenticated_identity=true;
 verified.authenticated_proof=true;
 verified.protocol=ticket.protocol_version;
 return authorize(Operation::Download,verified,now).allowed;
}
bool authenticate_session(const MemberTicket& ticket,const TrustPolicy& trusted,
    const Challenge& nonce,const Challenge& exporter,const Signature& signature,
    std::uint64_t now,AuthenticatedPeer& peer) {
 peer=AuthenticatedPeer{};
 VerifiedMembership accepted{};
 if(!verify_member(ticket,trusted,nonce,exporter,signature,now,accepted))return false;
 peer.membership_=accepted;
 peer.challenge_=nonce;
 peer.exporter_=exporter;
 peer.authenticated_=true;
 return true;
}
bool AuthenticatedPeer::permitted(Operation op,std::uint64_t now,
    const Challenge& nonce,const Challenge& exporter) const noexcept {
 return authenticated_ && nonce==challenge_ && exporter==exporter_ &&
        authorize(op,membership_,now).allowed;
}
} // namespace meshcloud::admission
