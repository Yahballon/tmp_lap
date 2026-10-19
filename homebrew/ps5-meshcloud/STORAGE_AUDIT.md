# MeshCloud v0.9 – Independent storage challenge engine

**This is a real randomized storage test, not a self-attestation that automatically unlocks the P2P network.**

## What is implemented

A future independent auditor generates truly random, incompressible data using OpenSSL CSPRNG and provisions it as a streaming dataset on the candidate storage node. The auditor retains a compact SHA-256 hash ledger: **13 GiB of data produces roughly 6.5 MiB of hashes** (64-KiB blocks). A node must keep the full data to reliably answer unpredictable random-block challenges.

An auditor issues a fresh 16-byte random challenge nonce and selects 48 distinct blocks without modulo bias, with a 60-second time window. The node retrieves those exact blocks (about 3 MiB) and returns them. Verification hashes each returned block, checks the node and dataset IDs, accepts the challenge only once in the verifier session, and returns an **opaque VerifiedSample** on success. Modified answers, a mismatched dataset or node, duplicate challenge indices and expired responses fail closed.

An independent Ed25519 auditor MAY sign its part of a member-ticket quorum only after the sample verifier accepted the challenge and the auditor's actual randomly provisioned dataset covers **at least 13 GiB**. Tickets expire within one hour. A 4-MiB or 1-GiB demonstration cannot issue membership. The old 3-auditor quorum and private TLS exporter binding remain required at every network endpoint.

## Run a real local 13-GiB disk test

The v0.9 Windows/Linux/macOS archives include meshcloud-audit (meshcloud-audit.exe on Windows).

```powershell
# EXPLICIT: this writes 13 GiB of new noncompressible random bytes.
# Create the parent folders beforehand and ensure sufficient free disk space.
mkdir C:\MeshCloud\node
mkdir C:\MeshCloud\auditor
meshcloud-audit.exe local-check C:\MeshCloud\node\random-storage.bin C:\MeshCloud\auditor\hashes.mcal 13
```

It retains both files intentionally: the node's 13-GiB sample data, and the small auditor hash ledger. The tool generates a fresh random test identity and performs one randomized 48-block test. It will not overwrite existing files and requires 2 GiB additional disk headroom.

**A local test is NOT an independent proof of storage**. For authentic independent membership, one independently operated provisioning participant must supply a **single identity-specific 13 GiB random dataset** (NOT 13 GiB per auditor). Its committed hashes must be authenticated and distributed to at least three independently controlled auditors, each retaining the ledger and its own private signing key outside the claimant machine. Each auditor independently challenges the SAME dataset with fresh random positions and strict deadlines, verifies the response and separately signs the ticket. The present audit module is transport-neutral; **remote challenge transport, multi-auditor orchestration, challenge replay state across restarts and unattended ticket renewals are still unfinished**. Do not treat this local CLI as a bypass or valid cloud onboarding.

## Security limits and tradeoffs

- Sampling proves *probabilistic retrievability* of auditor-created incompressible data, not exclusive use of a local HDD. A remote storage farm with sufficiently fast links might still answer challenges.
- Audit data should not be derived from a seed known to the node: then it could regenerate the bytes instead of storing them.
- Initial random sentinel bytes consume real capacity. When the network holds encrypted user fragments, unused sentinels should be replaced by authenticated ciphertext with independently verifiable block commitments, so the same 13 GiB contribution actually stores useful content. **This substitution is not implemented yet.**
- Dataset uniqueness per node identity, independently governed auditor keys, anti-Sybil resource cost, persistence checks and network latency limits remain critical for a production design.
- This is not proof that an entire user file remains available. File 90-day expiry, content replication, and shard repair remain separate mechanisms.
- No public access is granted until 3 separately verified signatures are aggregated into a current member.ticket recognized by the TLS core.

## Next integration milestones

1. Remote encrypted **auditor↔node** random dataset provisioning and timed proof exchange (not open file GET).
2. Three genuinely independent trusted auditors, each retaining its hash ledger and signing a verified ticket; challenge/replay persistence.
3. Automatic ticket refresh every hour, revocation on repeated failure, and membership-aware peer discovery.
4. Multi-node erasure-coded file placement, periodic self-healing and 90-day file-level retention.

**Current v0.9 implementation is an audited-storage building block, NOT a production decentralized cloud.**

## Native PS5 / Windows independently pinned auditor TCP challenge

MeshCloud v0.10 introduces `meshcloud-audit-net` as a **dedicated independent audit-only TLS 1.3 transport**, default TCP **39719**. Two Ed25519 identities are pinned *before* data is requested. The node only answers one time-bounded randomized sample challenge. No member.ticket is needed for this **pre-admission** endpoint, but it never exposes CID, search, fragment GET/PUT or user file metadata. Unknown auditor identities fail closed.

A 13 GiB network storage contribution is a **hard cap and membership threshold**. A local candidate must have 15 GiB free to create the random sentinel data plus 2 GiB headroom. User-file streaming length is not capped to 13 GiB; the 13 GiB cap is per-node contributed storage, not a per-file size limit.

1. Create a node Ed25519 identity.pem (keep private), then on a trusted PC run `meshcloud-audit pubkey <node-identity.pem>` to print only the 64-char public key. DO NOT copy the private key to an untrusted auditor.
2. On the independent auditor PC, run `meshcloud-audit provision-for <node-pub-hex> <random-data.bin> <ledger.mcal> 13`; its CSPRNG generates the exact 13 GiB sentinel data bound to the node identity. The auditor RETAINS its ledger and securely provisions the data to the node. At present this **one-time 13 GiB provisioning/copy is out-of-band**, not part of the TLS audit API. The node receives its copy of the matching `ledger.mcal` and `random-data.bin` but cannot mint audit signatures.
3. Keep three **independent** auditor Ed25519 public keys on the node in `auditors.keys` and the node's private key in `identity.pem`. The PS5 expects the audit files at `/data/PS5MeshCloud/audit/node-random.bin` and `/data/PS5MeshCloud/audit/ledger.mcal`, the identity/keys at `/data/PS5MeshCloud/p2p/`. In Network, press **L2** to accept one pinned auditor request on 39719. The PC can also run `meshcloud-audit-net serve <node-identity.pem> <auditor-pub-hex> <data-file> <ledger.mcal> <bindIPv4>`.
4. An external auditor runs `meshcloud-audit-net check <auditor-private.pem> <its-independent-ledger.mcal> <node-IPv4>`. The caller checks 48 random SHA-256 committed 64-KiB blocks over mutual Ed25519 TLS; wrong pinned identity, wrong nonce/data, or timeout fails. The CLI prints PASS only after verification.

**Do not confuse a successful challenge with an issued member.ticket.** The native auditor response and CLI do not issue admission tickets automatically. The existing signing API can sign a ticket only from a 13-GiB verified proof; persistent replay records, three separate independent auditors, ticket aggregation/distribution, and secure network provisioning are still required for real unattended P2P onboarding. The 13-GiB sentinels are not yet replaced with verifiable user shards, so usable space for a production shared file store needs further accounting.


## v0.11: THREE independent auditors → usable member.ticket

The new \`meshcloud-ticket\` program turns successful **independent remote 13 GiB challenge results** into actual member-ticket signatures. There is NO test/guest/skip option. Audit and storage transfer occur on the dedicated pinned TLS port 39719; ordinary member-only CID exchange remains on port 39718. **Only the auditor operator holds its private key.**

Use a common, explicitly provisioned \`auditors.keys\` containing the exact public Ed25519 keys of at least three truly separate verifiers. The same trusted keys must be installed in every P2P node's configuration; a key made up by a new candidate will NOT be recognized by other nodes.

**Initial preparation (one-time):** Generate an Ed25519 identity for the node and a DIFFERENT identity on each independent auditor machine; keep all private keys on their own machines. For a PS5, install only the node identity.pem at \`/data/PS5MeshCloud/p2p/\`. External auditors independently retain the authoritative \`ledger.mcal\` for the node's random **13 GiB** dataset. The PS5 gets the same 13 GiB sentinels at \`/data/PS5MeshCloud/audit/node-random.bin\`. This initial dataset transfer is still OUT OF BAND; all three auditors inspect the same 13 GiB rather than requiring 39 GiB.

1. **Node signs exactly one short-lived request.** On PC: \`meshcloud-ticket request <node-identity.pem> <request.mctr>\`. On native PS5 v0.5: press **R3** in the Network page to produce \`/data/PS5MeshCloud/p2p/request.mctr\`; download that PUBLIC request file to the independent auditors. The file expires for new audits after 10 minutes. Create a fresh request after it expires.
2. **Each independently run auditor** must already have its own private signing key, and its own immutable source ledger, and must be allowed by the node's \`auditors.keys\`. For each auditor, turn on the node's audit responder (PS5: **L2**; PC: \`meshcloud-audit-net serve\`), then run \`meshcloud-ticket attest <auditor-private.pem> <node-LAN-IP> <independent-ledger.mcal> <same-request.mctr> <unique-sig.mctp>\`. This performs a real pinned TLS 1.3 randomized remote audit (48 blocks) before writing one cryptographically verified partial signature. Auditor keys are never copied to the node. As of v0.11 the responder accepts one challenge per L2; restart L2 between each independent auditor, or run a controlled PC listener for each.
3. **The node operator assembles a verified quorum:** \`meshcloud-ticket assemble <request.mctr> <auditors.keys> <member.ticket> <sig1.mctp> <sig2.mctp> <sig3.mctp>\`. The assembler checks the owner signature, time, 13 GiB amounts, at least three **different explicitly trusted** auditor public keys, every Ed25519 signature, and the binding of every partial to the same request. Any invalid/duplicate/untrusted/mismatched signer causes the entire assembly to fail.
4. Copy the resulting binary **member.ticket** to the matching node's private \`p2p/\` config. Subsequent direct P2P connections still require mutual TLS identity pinning, an unexpired signed membership ticket and a fresh TLS exporter challenge. The PC and PS5 then use the same format.

**Important operational limits:** This is a *controlled LAN pre-alpha*, not production public membership. The prototype neither provides remote 13 GiB provisioning nor continuously hosted auditors, durable issuance/replay ledgers, auto-ticket renewal, robust auditor governance, NAT traversal or end-to-end PS5 hardware confirmation. An individual successful 13-GiB proof and signature do not by themselves confer access: quorum signatures and all other live member checks are mandatory.


### Native PS5 key security and provisioning

On PS5 v0.5, **R3** first generates a NEW Ed25519 node identity directly on the console (exclusive file creation with private permissions) if none exists, then saves a signed `request.mctr`. The private identity NEVER needs to travel to the auditor or PC. Copy only the PUBLIC `request.mctr` file to a PC, then execute `meshcloud-ticket inspect <request.mctr>` to print the node's verified public key and use that key with `meshcloud-audit provision-for <public-key-hex> ... 13`. The audit request lasts 10 minutes; do not delete or replace identity.pem when creating a fresh request. If the previous request filename exists, archive it or remove it manually before asking R3 to issue a new one. Never distribute private auditor keys or private node identity files.
