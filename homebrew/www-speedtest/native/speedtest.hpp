// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
namespace speedtest {
struct Snapshot {
  bool running=false, completed=false;
  int phase=0; // 0 idle, 1 latency, 2 download, 3 upload, 4 done
  double ping=0, jitter=0, down=0, up=0, progress=0, liveMbps=0;
  std::vector<double> downGraph, upGraph;
  std::string status="Ready. Press X to start a real network test.";
};
struct Server { const char* name; const char* flag; const char* host; const char* base; }; 
const Server* servers();
int serverCount();
int selectedServer();
bool selectServer(int index);
bool start();
Snapshot snapshot();
} 
