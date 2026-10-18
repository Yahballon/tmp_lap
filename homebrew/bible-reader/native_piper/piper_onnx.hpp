// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Genuine native Piper/ONNX Runtime CPU inference adapter.
// Still experimental: excluded from production Bible Reader until PS5 validation.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
struct OrtApi;
struct OrtEnv;
struct OrtSession;
struct OrtSessionOptions;
struct OrtMemoryInfo;
namespace bible_piper {
class NativeInference {
public:
 NativeInference()=default;
 ~NativeInference(){close();}
 NativeInference(const NativeInference&)=delete;
 NativeInference& operator=(const NativeInference&)=delete;
 bool open(const char* modelPath,std::string& error);
 bool synthesize(const std::vector<std::int64_t>& phonemeIds,
                 std::vector<std::int16_t>& pcm,std::string& error);
 void close();
 bool ready() const { return api_&&session_; }
private:
 bool success(void* status,std::string& error);
 const OrtApi* api_=nullptr;
 OrtEnv* env_=nullptr;
 OrtSessionOptions* opts_=nullptr;
 OrtSession* session_=nullptr;
 OrtMemoryInfo* mem_=nullptr;
};
} // namespace bible_piper
