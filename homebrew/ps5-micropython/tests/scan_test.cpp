// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/scripts_scan.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>
int main(){
 using namespace pyscan;
 for(const char* bad:{"helloworld.txt","../x.py","h.py/../","evil\\name.py",
                      ".hidden.py","too?.py","a.py/"})assert(!valid_name(bad));
 assert(valid_name("helloworld.py"));
 char d[]="/tmp/ps5py-scan-XXXXXX";
 assert(mkdtemp(d));
 const std::string dir=d;
 auto pre=scan(dir);
 assert(pre.folder_opened && pre.files.empty());
 const std::string hello=dir+"/helloworld.py";
 {std::ofstream f(hello);f<<"print('Hi PS5!')\n";}
 {std::ofstream f(dir+"/empty.py");}
 {std::ofstream f(dir+"/big.py");f<<std::string(kMaxScriptBytes+1,'x');}
 {std::ofstream f(dir+"/notpython.txt");f<<"abc";}
 assert(symlink(hello.c_str(),(dir+"/shortcut.py").c_str())==0);
 auto result=scan(dir);
 assert(result.folder_opened);
 assert(result.candidates==4);
 assert(result.files.size()==1 && result.files[0].name=="helloworld.py");
 assert(result.empty==1 && result.oversize==1 && result.unsafe==1);
 assert(result.status().find("SCAN: 1 local")!=std::string::npos);
 assert(read_script(hello,kMaxScriptBytes)=="print('Hi PS5!')\n");
 assert(read_script(dir+"/big.py",kMaxScriptBytes).empty());
 const auto bad=scan(dir+"/missing");
 assert(!bad.folder_opened && bad.directory_errno!=0);
 assert(unlink(hello.c_str())==0);
 assert(unlink((dir+"/empty.py").c_str())==0);
 assert(unlink((dir+"/big.py").c_str())==0);
 assert(unlink((dir+"/notpython.txt").c_str())==0);
 assert(unlink((dir+"/shortcut.py").c_str())==0);
 assert(rmdir(dir.c_str())==0);
 return 0;
}
