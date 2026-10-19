// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "core.hpp"
#include <array>
#include <cstdint>
#include <vector>
namespace meshcloud::admission {
// Fixed-size verified challenge-response material.
// All auditors must be independently pinned trusted public keys. This DOES
// NOT itself produce remote proof of 10 GiB: the auditors must actually
// challenge persisted capacity before issuing signatures.
using PublicKey=std::array<unsigned char,32>;
using Signature=std::array<unsigned char,64>;
using Challenge=std::array<unsigned char,32>;
struct AuditorSignature {
 PublicKey signer{};
 Signature signature{};
};
struct MemberTicket {
 PublicKey node_identity{};
 std::array<unsigned char,16> ticket_nonce{};
 std::uint64_t offered_bytes=0;
 std::uint64_t audited_bytes=0;
 std::uint64_t issued_at=0;
 std::uint64_t expires_at=0;
 std::uint32_t protocol_version=1;
 std::vector<AuditorSignature> auditors;
};
struct TrustPolicy {
 std::vector<PublicKey> pinned_auditors;
 std::size_t quorum=3;
};
// Canonical little-endian fixed-width bytes, with unique protocol separator;
// sign only these bytes when an auditor has PROVED actual remote 10 GiB.
std::vector<unsigned char> audit_transcript(const MemberTicket& ticket);
// Session transcript binds ticket to fresh verifier-chosen nonce AND a
// non-zero authenticated secure-channel exporter (e.g. TLS 1.3 exporter).
std::vector<unsigned char> session_transcript(const MemberTicket& ticket,
      const Challenge& challenge,const Challenge& transport_exporter);
// Fail closed. No trust bootstrap, local quota guess, or member self-signing.
bool verify_member(const MemberTicket& ticket,const TrustPolicy& trusted,
    const Challenge& verifier_nonce,const Challenge& transport_exporter,
    const Signature& node_session_signature,std::uint64_t now,
    VerifiedMembership& verified);
// Non-forgeable in-process session handle for network dispatch. Callers
// cannot synthesize one by setting public membership booleans.
class AuthenticatedPeer {
public:
 AuthenticatedPeer()=default;
 bool permitted(Operation op,std::uint64_t now,const Challenge& nonce,
                const Challenge& exporter) const noexcept;
private:
 friend bool authenticate_session(const MemberTicket&,const TrustPolicy&,
     const Challenge&,const Challenge&,const Signature&,std::uint64_t,
     AuthenticatedPeer&);
 VerifiedMembership membership_{};
 Challenge challenge_{};
 Challenge exporter_{};
 bool authenticated_=false;
};
bool authenticate_session(const MemberTicket&,const TrustPolicy&,
    const Challenge& nonce,const Challenge& exporter,const Signature& session_signature,
    std::uint64_t now,AuthenticatedPeer& out);
} // namespace meshcloud::admission
