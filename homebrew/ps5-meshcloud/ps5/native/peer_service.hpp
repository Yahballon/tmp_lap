// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
namespace meshps5 {
class PeerService {
public:
 PeerService()=default;
 ~PeerService();
 PeerService(const PeerService&)=delete;
 PeerService& operator=(const PeerService&)=delete;
 // Button X: serve one authenticated peer request.
 bool listen();
 // Button Square: send a local encrypted .mcb chunk by CID.
 bool send_local(const std::string& source_file);
 // Button Triangle: retrieve the CID listed in p2p/network.conf.
 bool fetch_cid();
 // Explicit signed getaddr from configured verified LAN peer.
 bool discover();
 // L2: accept one independently pinned auditor before network admission.
 bool audit_listen();
 // R3: create node-signed 13GiB membership request without a ticket.
 bool make_member_request();
 // L3 on Network: receive 13GiB from a PRE-PINNED auditor only.
 bool receive_audit_dataset();
 bool busy() const noexcept {return busy_.load();}
 std::string status() const;
private:
 enum class Action { Listen, Put, Get, Discover, AuditListen, CreateTicketRequest, ReceiveAuditDataset };
 bool launch(Action action,std::string path={});
 void run(Action action,const std::string& path);
 void report(std::string status);
 std::atomic<bool> busy_{false};
 mutable std::mutex mutex_;
 std::string status_="P2P transport available; membership keys required";
 std::thread worker_;
};
} // namespace meshps5
