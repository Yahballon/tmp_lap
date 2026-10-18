// SPDX-License-Identifier: GPL-3.0-or-later
// VITS/Thorsten Medium inference with ONNX Runtime C API (not prerecorded).
#include "piper_onnx.hpp"
#include <onnxruntime_c_api.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
namespace bible_piper {
bool NativeInference::success(void* opaque,std::string& error){
 auto* state=static_cast<OrtStatus*>(opaque);
 if(!state)return true;
 error=api_?api_->GetErrorMessage(state):"ONNX Runtime initialization error";
 if(api_)api_->ReleaseStatus(state);
 return false;
}
void NativeInference::close(){
 if(api_){
  if(mem_)api_->ReleaseMemoryInfo(mem_);
  if(session_)api_->ReleaseSession(session_);
  if(opts_)api_->ReleaseSessionOptions(opts_);
  if(env_)api_->ReleaseEnv(env_);
 }
 mem_=nullptr;session_=nullptr;opts_=nullptr;env_=nullptr;api_=nullptr;
}
bool NativeInference::open(const char* modelPath,std::string& error){
 close();error.clear();
 if(!modelPath||!*modelPath){error="Empty Piper model path";return false;}
 const OrtApiBase* base=OrtGetApiBase();
 api_=base?base->GetApi(ORT_API_VERSION):nullptr;
 if(!api_){error="ONNX Runtime C API is missing";close();return false;}
 if(!success(api_->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"BibleReader",&env_),error)||
    !success(api_->CreateSessionOptions(&opts_),error)||
    !success(api_->SetIntraOpNumThreads(opts_,1),error)||
    !success(api_->SetInterOpNumThreads(opts_,1),error)||
    !success(api_->SetSessionGraphOptimizationLevel(opts_,ORT_DISABLE_ALL),error)||
    !success(api_->DisableMemPattern(opts_),error)||
    !success(api_->DisableCpuMemArena(opts_),error)||
    !success(api_->CreateSession(env_,modelPath,opts_,&session_),error)||
    !success(api_->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&mem_),error)){
  close();return false;
 }
 return true;
}
bool NativeInference::synthesize(const std::vector<std::int64_t>& ids,
                                std::vector<std::int16_t>& pcm,std::string& error){
 pcm.clear();error.clear();
 if(!ready()){error="Native Piper not initialized";return false;}
 if(ids.size()<4||ids.size()>8192){error="Piper input length out of bounds";return false;}
 // Thorsten Medium: input [1,N] int64, lengths [1] int64,
 // scales [3] float, output [1,1,S] float.
 auto tokens=ids;
 std::int64_t length=(std::int64_t)tokens.size();
 float scales[3]={0.667f,1.0f,0.8f};
 const std::int64_t tokenDims[2]={1,length},lengthDims[1]={1},scaleDims[1]={3};
 OrtValue* inputs[3]={nullptr,nullptr,nullptr};
 OrtValue* output=nullptr;
 OrtTensorTypeAndShapeInfo* shape=nullptr;
 bool ok=true;
 do{
  if(!success(api_->CreateTensorWithDataAsOrtValue(mem_,tokens.data(),
        tokens.size()*sizeof(std::int64_t),tokenDims,2,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64,&inputs[0]),error)||
     !success(api_->CreateTensorWithDataAsOrtValue(mem_,&length,
        sizeof(length),lengthDims,1,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64,&inputs[1]),error)||
     !success(api_->CreateTensorWithDataAsOrtValue(mem_,scales,
        sizeof(scales),scaleDims,1,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&inputs[2]),error)){ok=false;break;}
  const char* names[3]={"input","input_lengths","scales"};
  const char* outputs[1]={"output"};
  const OrtValue* values[3]={inputs[0],inputs[1],inputs[2]};
  if(!success(api_->Run(session_,nullptr,names,values,3,outputs,1,&output),error)||
     !success(api_->GetTensorTypeAndShape(output,&shape),error)){ok=false;break;}
  ONNXTensorElementDataType type=ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED;
  std::size_t count=0;
  if(!success(api_->GetTensorElementType(shape,&type),error)||
     !success(api_->GetTensorShapeElementCount(shape,&count),error)){ok=false;break;}
  if(type!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT||count<100||count>22050*120){
   error="Piper returned invalid PCM tensor";ok=false;break;
  }
  void* raw=nullptr;
  if(!success(api_->GetTensorMutableData(output,&raw),error)||!raw){
   if(error.empty())error="Piper tensor has no samples";
   ok=false;break;
  }
  const float* samples=static_cast<const float*>(raw);
  float peak=0.01f;
  for(std::size_t i=0;i<count;++i){
   if(!std::isfinite(samples[i])){error="Piper returned non-finite samples";ok=false;break;}
   peak=std::max(peak,std::abs(samples[i]));
  }
  if(!ok)break;
  const float gain=32767.f/peak;
  pcm.reserve(count);
  for(std::size_t i=0;i<count;++i){
   const float v=std::clamp(samples[i]*gain,-32768.f,32767.f);
   pcm.push_back((std::int16_t)v);
  }
 }while(false);
 if(shape)api_->ReleaseTensorTypeAndShapeInfo(shape);
 if(output)api_->ReleaseValue(output);
 for(auto*& input:inputs)if(input)api_->ReleaseValue(input);
 if(!ok)pcm.clear();
 return ok;
}
} // namespace bible_piper
