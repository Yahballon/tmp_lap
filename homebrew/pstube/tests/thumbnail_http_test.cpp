// SPDX-License-Identifier: GPL-3.0-or-later
#include "transfer.hpp"
#include <atomic>
#include <cstdio>
#include <iostream>
#include <string>
int main(){
 const std::string out="/tmp/pstube-thumbnail-probe.jpg";
 std::atomic<int> progress{0};
 std::string error;
 bool success=pstube::fetch_mp4("https://i.ytimg.com/vi/dQw4w9WgXcQ/mqdefault.jpg",out,progress,error);
 if(!success){std::cerr<<"HTTPS thumbnail fetch FAILED: "<<error<<"\n";return 1;}
 FILE* f=fopen(out.c_str(),"rb");
 unsigned char bytes[3]{};
 bool ok=f&&fread(bytes,1,3,f)==3&&bytes[0]==255&&bytes[1]==216&&bytes[2]==255;
 if(f)fclose(f);
 remove(out.c_str());
 std::cout<<"HTTPS thumbnail fetch: "<<(ok?"real JPEG received":"invalid image")<<" progress "<<progress<<"%\n";
 return ok?0:2;
}