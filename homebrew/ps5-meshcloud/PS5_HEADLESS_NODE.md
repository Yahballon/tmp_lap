# MeshCloud PS5 Native Network Node — v0.12 headless build

**This is a real PS5-native network-service build, not the graphical MeshCloud UI.**
The current upstream BlackBearReloaded OpenGL release endpoint returns HTTP 404
in GitHub Actions, so the original GUI build must remain marked FAILED. This
separate fallback uses the SAME audited C++ core, signed PS5 title packaging and
OpenSSL/SQLite SDK but no graphics libraries. It is explicitly named
"MeshCloud PS5 Network Node (HEADLESS)" so nobody mistakes it for a GUI fix.

## Install and startup

The installable artifact is \`PPSA99718.zip\` from GitHub Actions run
"Build PS5 MeshCloud Network Node (no OpenGL)." Like the graphic variant it
uses PPSA99718 (do not install the two variants simultaneously).

On first launch it creates, with exclusive 0600 permissions:
\`/data/PS5MeshCloud/p2p/identity.pem\` (NEVER SHARE) and
\`/data/PS5MeshCloud/p2p/node-public.hex\` (safe to copy).
It logs to \`/data/PS5MeshCloud/node.log\`. On an unconfigured first boot it
exits without opening any port.

To enable one explicit operating mode, create
\`/data/PS5MeshCloud/node.conf\` containing exactly:

\`\`\`ini
mode=provision
bind=192.168.1.50
\`\`\`

Use **the actual PS5 private LAN IPv4**, not the example. Restart the app after
editing its configuration. In \`provision\` mode, it accepts one authenticated
13-GiB auditor transfer on TCP 39720, checks every 64-KiB block and exits
when complete. You must have at least 15 GiB available for first provision.

Other modes: \`mode=audit\` answers authenticated independent random challenge
requests on TCP 39719 using its previously provisioned exact 13 GiB dataset,
and \`mode=p2p\` starts a mutual-TLS member-only encrypted CID listener on
TCP 39718. The audit and P2P modes continue until the application is closed.

For any network mode, install \`p2p/auditors.keys\` with **three or more
independent, pre-approved, distinct Ed25519 auditor public keys**. P2P mode
additionally needs \`member.ticket\`, \`peers.keys\`, and a successful independent
3-of-5 ticket issued for this PS5 identity. No guest/CID/file access is possible
from the provisioning/audit ports.

To obtain an auditor source dataset, run on a trusted PC:
\`meshcloud-audit provision-for <node-public-hex> <source.bin> <ledger.mcal> 13\`
then \`meshcloud-provision send ...\`. See [NETWORK_PROVISION.md](NETWORK_PROVISION.md).

## Limits and testing

A green **PS5 native compiler/linker/ZIP CI** does NOT demonstrate hardware
runtime success. Test on your own jailbroken PS5 and send its node.log for
further fixes. Native graphics UI is not present and has its own blocked SDK
build. This fallback remains a pre-alpha: no NAT traversal, public DHT,
fully independent auditor deployment, audited replacement of sentinel with
actual user ciphertext, resume after interrupted 13 GiB transfers, or proven
continuous multi-node replication. Data transfer requires explicitly pinned
local LAN peers and external valid 13 GiB storage proofs.

The audit sentinel consumes the entire 13 GiB; in this version the shared
fragment store is not integrated into that same budget. Therefore **do not
treat headless mode=p2p as proof that storage accounting is production safe**.
Perform tests only with trusted peers and enough spare disk space.
