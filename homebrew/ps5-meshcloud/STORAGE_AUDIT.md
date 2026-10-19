# MeshCloud v0.9 – Independent storage challenge engine

**This is a real randomized storage test, not a self-attestation that automatically unlocks the P2P network.**

## What is implemented

A future independent auditor generates truly random, incompressible data using OpenSSL CSPRNG and provisions it as a streaming dataset on the candidate storage node. The auditor retains a compact SHA-256 hash ledger: **10 GiB of data produces roughly 5 MiB of hashes** (64-KiB blocks). A node must keep the full data to reliably answer unpredictable random-block challenges.

An auditor issues a fresh 16-byte random challenge nonce and selects 48 distinct blocks without modulo bias, with a 60-second time window. The node retrieves those exact blocks (about 3 MiB) and returns them. Verification hashes each returned block, checks the node and dataset IDs, accepts the challenge only once in the verifier session, and returns an **opaque VerifiedSample** on success. Modified answers, a mismatched dataset or node, duplicate challenge indices and expired responses fail closed.

An independent Ed25519 auditor MAY sign its part of a member-ticket quorum only after the sample verifier accepted the challenge and the auditor's actual randomly provisioned dataset covers **at least 10 GiB**. Tickets expire within one hour. A 4-MiB or 1-GiB demonstration cannot issue membership. The old 3-auditor quorum and private TLS exporter binding remain required at every network endpoint.

## Run a real local 10-GiB disk test

The v0.9 Windows/Linux/macOS archives include meshcloud-audit (meshcloud-audit.exe on Windows).

```powershell
# EXPLICIT: this writes 10 GiB of new noncompressible random bytes.
# Create the parent folders beforehand and ensure sufficient free disk space.
mkdir C:\MeshCloud\node
mkdir C:\MeshCloud\auditor
meshcloud-audit.exe local-check C:\MeshCloud\node\random-storage.bin C:\MeshCloud\auditor\hashes.mcal 10
```

It retains both files intentionally: the node's 10-GiB sample data, and the small auditor hash ledger. The tool generates a fresh random test identity and performs one randomized 48-block test. It will not overwrite existing files and requires 2 GiB additional disk headroom.

**A local test is NOT an independent proof of storage**. For authentic independent membership, one independently operated provisioning participant must supply a **single identity-specific 10 GiB random dataset** (NOT 10 GiB per auditor). Its committed hashes must be authenticated and distributed to at least three independently controlled auditors, each retaining the ledger and its own private signing key outside the claimant machine. Each auditor independently challenges the SAME dataset with fresh random positions and strict deadlines, verifies the response and separately signs the ticket. The present audit module is transport-neutral; **remote challenge transport, multi-auditor orchestration, challenge replay state across restarts and unattended ticket renewals are still unfinished**. Do not treat this local CLI as a bypass or valid cloud onboarding.

## Security limits and tradeoffs

- Sampling proves *probabilistic retrievability* of auditor-created incompressible data, not exclusive use of a local HDD. A remote storage farm with sufficiently fast links might still answer challenges.
- Audit data should not be derived from a seed known to the node: then it could regenerate the bytes instead of storing them.
- Initial random sentinel bytes consume real capacity. When the network holds encrypted user fragments, unused sentinels should be replaced by authenticated ciphertext with independently verifiable block commitments, so the same 10 GiB contribution actually stores useful content. **This substitution is not implemented yet.**
- Dataset uniqueness per node identity, independently governed auditor keys, anti-Sybil resource cost, persistence checks and network latency limits remain critical for a production design.
- This is not proof that an entire user file remains available. File 90-day expiry, content replication, and shard repair remain separate mechanisms.
- No public access is granted until 3 separately verified signatures are aggregated into a current member.ticket recognized by the TLS core.

## Next integration milestones

1. Remote encrypted **auditor↔node** random dataset provisioning and timed proof exchange (not open file GET).
2. Three genuinely independent trusted auditors, each retaining its hash ledger and signing a verified ticket; challenge/replay persistence.
3. Automatic ticket refresh every hour, revocation on repeated failure, and membership-aware peer discovery.
4. Multi-node erasure-coded file placement, periodic self-healing and 90-day file-level retention.

**Current v0.9 implementation is an audited-storage building block, NOT a production decentralized cloud.**
