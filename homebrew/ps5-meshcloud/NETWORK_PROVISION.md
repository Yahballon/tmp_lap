# MeshCloud v0.12 — 13 GiB provisioning over authenticated P2P LAN

**13 GiB per node** remains the shared-storage minimum and hard cap; no file-size
limit is introduced for source files. This milestone replaces the one-time manual
copy of auditor-generated random proof data with a true, stream-based network
transfer. It is not a public storage upload endpoint.

## Independent bootstrap, no guest mode

The three transport channels are deliberately independent:

| Port | Operation | Authentication |
|---|---|---|
| TCP 39718 | Shared encrypted CID GET/PUT/getaddr | 3 independently signed, current 13 GiB member.ticket plus pinned mutual TLS1.3 |
| TCP 39719 | Remote random block proof challenge | Pre-pinned independent auditor mutual TLS1.3; ONLY 48 random proof blocks |
| TCP 39720 | Initial 13 GiB random data + auditor ledger transfer | Pre-pinned independent auditor mutual TLS1.3; ONLY recipient's sentinel dataset |

**Node onboarding cannot require a ticket that does not exist yet.** On ports
39719 and 39720 an authenticated *pre-approved auditor key* replaces membership,
but absolutely no regular P2P, CIDs, search or user file metadata can cross.

## Initial PS5 ↔ PC LAN setup

1. On PS5 Network, press **R3** to generate a private Ed25519 identity on the PS5
   and PUBLIC signed request \`/data/PS5MeshCloud/p2p/request.mctr\`.
   Do not copy the identity.pem off the console. On PC run
   \`meshcloud-ticket inspect request.mctr\` to obtain its public Ed25519 key.
2. Provision the **exact node identity**, locally on independent auditor PC:
   \`meshcloud-audit provision-for <64-char-PS5-pubkey> C:\Audit\random.bin C:\Audit\ledger.mcal 13\`.
   This writes 13 GiB of cryptographically random source bytes on the auditor PC
   and a ~6.5 MiB ledger. The source must remain securely held on that PC
   until transfer completes. The exact same auditor ledger should be
   authentically distributed to the 3 independent verifiers, not replaced by
   self-issued node commitments.
3. Install the three auditors' PUBLIC Ed25519 keys into
   \`/data/PS5MeshCloud/p2p/auditors.keys\`, one lowercase 64-char hex per line.
   \`/data/PS5MeshCloud/p2p/network.conf\` must define
   \`bind=<PS5-private-LAN-IPv4>\`. Protect this directory from other apps/users.
4. PS5 Network: press **L3**. It accepts one direct TCP 39720 connection from
   *one of the already pinned independent auditors*. On the independent PC run:

   \`\`\`powershell
   meshcloud-provision.exe send C:\Audit\auditor1.pem <PS5-public-hex> 192.168.1.50 C:\Audit\random.bin C:\Audit\ledger.mcal
   \`\`\`

   Replace the IP with the **actual PS5 LAN IP**. No open-wildcard listening,
   private key transmission or public Internet port-forwarding.

5. On the PS5, every 64-KiB block is SHA-256 checked against the received ledger
   while it is streamed directly to a new \`.pending\` file. The application
   refuses existing filenames, insufficient free space, unknown auditors,
   mismatching node identities, bad ledger structure, partial transfer or
   corrupt blocks. **Only after all 13 GiB are verified and fsynced** does it
   move staged files into
   \`/data/PS5MeshCloud/audit/node-random.bin\` and \`ledger.mcal\`. If the
   final ACK is lost, a fully completed dataset may exist even when the PC
   reports a transport failure; inspect before retrying.

6. PS5 Network: press **L2** separately for each of the three trusted
   verifiers. Each verifier challenges random blocks on TCP 39719 and runs
   \`meshcloud-ticket attest\` with its own Ed25519 key and trusted ledger,
   yielding its own \`.mctp\` signature. All must sign the SAME short-lived
   node-signed \`request.mctr\`; create a fresh request on PS5 if expired.
7. \`meshcloud-ticket assemble\` validates the 3 distinct pinned
   auditor signatures and writes \`member.ticket\`. Copy only that completed
   ticket to the PS5's private p2p config. The existing encrypted CID P2P
   path then performs its own mutual TLS + exporter-bound ticket proof.

## Desktop provisioning receiver

Instead of PS5 L3 you may run this on a Linux/Windows/macOS node PC:

\`\`\`powershell
meshcloud-provision.exe recv C:\Node\identity.pem C:\Node\auditors.keys 192.168.1.20 C:\Node\audit\node-random.bin C:\Node\audit\ledger.mcal
\`\`\`

The destination parent folder must already exist. The source auditor uses
the corresponding \`meshcloud-provision send ...\` command. Port 39720 must
be permitted through your **private LAN** firewall. Neither side may use
\`0.0.0.0\` as a listener address.

## Testing and limits

The CI tests the exact 64-KiB streaming TLS protocol using a **4 MiB
transport-only fixture**. This is deliberately not enough to issue a ticket,
and the production CLI/native PS5 UI accept **only 13 GiB**. The CI exercises
successful transfer, SHA-256 random proof readback, outsider auditor rejection
and refusal to overwrite existing data; it also runs under PS5
\`-fno-exceptions\` constraints.

The current transfer must run to completion in one session. **Resume,
throughput/progress reporting, out-of-band auditor governance,
unattended three-auditor renewals, full file-level 90-day redundancy
and direct PS5 hardware verification are not yet complete.**
This prototype uses the full 13 GiB as audit sentinel data; the eventual
shared-storage design must replace it with audited encrypted user fragments
without exceeding the 13 GiB contribution. Do not claim the current
sentinel occupies space available for user-file storage.
