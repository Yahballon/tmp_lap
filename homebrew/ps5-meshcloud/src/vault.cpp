// SPDX-License-Identifier: GPL-3.0-or-later
#include "vault.hpp"
#include "cid.hpp"
#include "stream.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>
#include <vector>
#include <openssl/evp.h>
#include <openssl/rand.h>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#endif

namespace meshcloud {
namespace {
namespace fs=std::filesystem;
constexpr unsigned char magic[4]={'M','C','B','1'};
constexpr std::size_t prefix_size=36;
constexpr char aad_domain[]="MeshCloud/file-chunk/v1";

bool random_bytes(unsigned char* p,std::size_t n) {
 return n<=static_cast<std::size_t>(std::numeric_limits<int>::max()) &&
        RAND_bytes(p,static_cast<int>(n))==1;
}
std::string hex(const unsigned char* p,std::size_t n) {
 static constexpr char alphabet[]="0123456789abcdef";
 std::string out;out.resize(n*2);
 for(std::size_t i=0;i<n;i++){out[2*i]=alphabet[p[i]>>4];out[2*i+1]=alphabet[p[i]&15];}
 return out;
}
bool unhex(const std::string& s,unsigned char* out,std::size_t n) {
 if(s.size()!=n*2)return false;
 for(std::size_t i=0;i<n;i++) {
  auto digit=[](char x)->int {
   if(x>='0'&&x<='9')return x-'0';
   if(x>='a'&&x<='f')return x-'a'+10;
   return -1;
  };
  const int a=digit(s[2*i]),b=digit(s[2*i+1]);
  if(a<0||b<0)return false;
  out[i]=static_cast<unsigned char>((a<<4)|b);
 }
 return true;
}
std::string unique_suffix() {
 unsigned char random[16]{};
 if(!random_bytes(random,sizeof(random)))return {};
 return hex(random,sizeof(random));
}
bool safe_directory(const fs::path& p) {
 std::error_code ec;
 auto status=fs::symlink_status(p,ec);
 return !ec&&status.type()==fs::file_type::directory;
}
bool write_new_file(const fs::path& path,const std::vector<unsigned char>& bytes) {
 if(fs::exists(path))return false;
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 if(!out)return false;
 out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
 out.flush();return out.good();
}
bool seal(const std::vector<unsigned char>& plaintext,const FileKey& key,
          const std::array<unsigned char,16>& salt,std::uint64_t sequence,
          std::vector<unsigned char>& blob) {
 blob.clear();if(plaintext.size()>chunk_bytes)return false;
 blob.resize(prefix_size+plaintext.size());
 std::copy(magic,magic+4,blob.begin());
 if(!random_bytes(blob.data()+4,12))return false;
 const auto len=static_cast<std::uint32_t>(plaintext.size());
 for(int i=0;i<4;i++)blob[32+i]=static_cast<unsigned char>((len>>(8*i))&255);
 std::array<unsigned char,24> context{};
 std::copy(salt.begin(),salt.end(),context.begin());
 for(int i=0;i<8;i++)context[16+i]=static_cast<unsigned char>((sequence>>(8*i))&255);
 EVP_CIPHER_CTX* evp=EVP_CIPHER_CTX_new();if(!evp)return false;
 int written=0,extra=0,ignored=0;bool ok=false;
 do {
  if(EVP_EncryptInit_ex(evp,EVP_aes_256_gcm(),nullptr,nullptr,nullptr)!=1)break;
  if(EVP_CIPHER_CTX_ctrl(evp,EVP_CTRL_GCM_SET_IVLEN,12,nullptr)!=1)break;
  if(EVP_EncryptInit_ex(evp,nullptr,nullptr,key.data(),blob.data()+4)!=1)break;
  if(EVP_EncryptUpdate(evp,nullptr,&ignored,
        reinterpret_cast<const unsigned char*>(aad_domain),sizeof(aad_domain)-1)!=1)break;
  if(EVP_EncryptUpdate(evp,nullptr,&ignored,context.data(),static_cast<int>(context.size()))!=1)break;
  if(EVP_EncryptUpdate(evp,nullptr,&ignored,blob.data()+32,4)!=1)break;
  if(!plaintext.empty()&&EVP_EncryptUpdate(evp,blob.data()+prefix_size,&written,
        plaintext.data(),static_cast<int>(plaintext.size()))!=1)break;
  if(EVP_EncryptFinal_ex(evp,blob.data()+prefix_size+written,&extra)!=1)break;
  if(static_cast<std::size_t>(written+extra)!=plaintext.size())break;
  if(EVP_CIPHER_CTX_ctrl(evp,EVP_CTRL_GCM_GET_TAG,16,blob.data()+16)!=1)break;
  ok=true;
 }while(false);
 EVP_CIPHER_CTX_free(evp);if(!ok)blob.clear();return ok;
}
bool unseal(const std::vector<unsigned char>& blob,const FileKey& key,
            const std::array<unsigned char,16>& salt,std::uint64_t sequence,
            std::vector<unsigned char>& plaintext) {
 plaintext.clear();
 if(blob.size()<prefix_size||blob.size()>prefix_size+chunk_bytes ||
    !std::equal(magic,magic+4,blob.begin()))return false;
 std::uint32_t length=0;
 for(int i=0;i<4;i++)length|=static_cast<std::uint32_t>(blob[32+i])<<(8*i);
 if(length!=blob.size()-prefix_size)return false;
 std::array<unsigned char,24> context{};
 std::copy(salt.begin(),salt.end(),context.begin());
 for(int i=0;i<8;i++)context[16+i]=static_cast<unsigned char>((sequence>>(8*i))&255);
 std::vector<unsigned char> tmp(length);
 EVP_CIPHER_CTX* evp=EVP_CIPHER_CTX_new();if(!evp)return false;
 int written=0,extra=0,ignored=0;bool ok=false;
 do {
  if(EVP_DecryptInit_ex(evp,EVP_aes_256_gcm(),nullptr,nullptr,nullptr)!=1)break;
  if(EVP_CIPHER_CTX_ctrl(evp,EVP_CTRL_GCM_SET_IVLEN,12,nullptr)!=1)break;
  if(EVP_DecryptInit_ex(evp,nullptr,nullptr,key.data(),blob.data()+4)!=1)break;
  if(EVP_DecryptUpdate(evp,nullptr,&ignored,
        reinterpret_cast<const unsigned char*>(aad_domain),sizeof(aad_domain)-1)!=1)break;
  if(EVP_DecryptUpdate(evp,nullptr,&ignored,context.data(),static_cast<int>(context.size()))!=1)break;
  if(EVP_DecryptUpdate(evp,nullptr,&ignored,blob.data()+32,4)!=1)break;
  if(length&&EVP_DecryptUpdate(evp,tmp.data(),&written,
      blob.data()+prefix_size,static_cast<int>(length))!=1)break;
  if(EVP_CIPHER_CTX_ctrl(evp,EVP_CTRL_GCM_SET_TAG,16,
      const_cast<unsigned char*>(blob.data()+16))!=1)break;
  if(EVP_DecryptFinal_ex(evp,tmp.data()+written,&extra)!=1)break;
  if(static_cast<std::uint32_t>(written+extra)!=length)break;
  ok=true;
 }while(false);
 EVP_CIPHER_CTX_free(evp);if(ok)plaintext.swap(tmp);return ok;
}
bool read_cid_block(const fs::path& path,const std::string& cid,std::vector<unsigned char>& data) {
 std::error_code ec;
 const auto status=fs::symlink_status(path,ec);
 if(ec||status.type()!=fs::file_type::regular)return false;
 const auto size=fs::file_size(path,ec);
 if(ec||size<prefix_size||size>prefix_size+chunk_bytes)return false;
 std::ifstream in(path,std::ios::binary);
 if(!in)return false;
 data.resize(static_cast<std::size_t>(size));
 in.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(data.size()));
 return in.gcount()==static_cast<std::streamsize>(data.size())&&!in.bad()&&cid::matches(cid,data);
}
} // namespace

bool ProcessLock::acquire(const fs::path& path) {
 if(native_!=-1)return true;
#ifdef _WIN32
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
                         OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return false;
 OVERLAPPED overlap{};
 if(!LockFileEx(file,LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,
                0,1,0,&overlap)) {
  CloseHandle(file);return false;
 }
 native_=reinterpret_cast<std::intptr_t>(file);
#else
 const int fd=::open(path.c_str(),O_RDWR|O_CREAT|O_NOFOLLOW,0600);
 if(fd<0)return false;
 if(flock(fd,LOCK_EX|LOCK_NB)!=0){::close(fd);return false;}
 native_=static_cast<std::intptr_t>(fd);
#endif
 return true;
}
ProcessLock::~ProcessLock() {
 if(native_==-1)return;
#ifdef _WIN32
 HANDLE file=reinterpret_cast<HANDLE>(native_);
 OVERLAPPED overlap{};
 UnlockFileEx(file,0,1,0,&overlap);
 CloseHandle(file);
#else
 const int fd=static_cast<int>(native_);
 flock(fd,LOCK_UN);
 ::close(fd);
#endif
}

bool make_key(FileKey& key){return random_bytes(key.data(),key.size());}
bool save_key(const fs::path& path,const FileKey& key) {
 const std::string encoded=hex(key.data(),key.size())+"\n";
#ifdef _WIN32
 // CREATE_NEW prevents accidental overwrite of another key, including collisions.
 // The parent directory must have owner-only ACLs: Windows inherits the directory DACL.
 HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;
 const bool ok=WriteFile(file,encoded.data(),static_cast<DWORD>(encoded.size()),
                         &written,nullptr)!=0 &&
               written==encoded.size() && FlushFileBuffers(file)!=0;
 CloseHandle(file);
 if(!ok)DeleteFileW(path.c_str());
 return ok;
#else
 // No symlink following and creation-only mode 0600, BEFORE sensitive bytes are written.
 const int fd=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(fd<0)return false;
 std::size_t offset=0;bool ok=true;
 while(offset<encoded.size()) {
  const ssize_t n=::write(fd,encoded.data()+offset,encoded.size()-offset);
  if(n<=0){ok=false;break;}
  offset+=static_cast<std::size_t>(n);
 }
 if(ok)ok=fsync(fd)==0;
 if(close(fd)!=0)ok=false;
 if(!ok)unlink(path.c_str());
 return ok;
#endif
}
bool load_key(const fs::path& path,FileKey& key) {
 std::ifstream in(path,std::ios::binary);if(!in)return false;
 std::string line,extra;
 if(!std::getline(in,line))return false;
 if(std::getline(in,extra))return false;
 FileKey tmp{};if(!unhex(line,tmp.data(),tmp.size()))return false;
 key=tmp;return true;
}
LocalVault::LocalVault(fs::path root)
 :root_(std::move(root)),catalog_(root_/"catalog.sqlite"){}
bool LocalVault::open() {
 std::error_code ec;
 if(!fs::exists(root_,ec))fs::create_directories(root_,ec);
 if(ec||!safe_directory(root_)){error_="Invalid vault directory";return false;}
 if(!process_lock_.acquire(root_/".meshcloud.lock")) {
  error_="Vault is already active in another process or filesystem locking unavailable";return false;
 }
 for(const auto& sub:{"blocks","manifests"}) {
  const auto p=root_/sub;
  if(!fs::exists(p,ec))fs::create_directory(p,ec);
  if(ec||!safe_directory(p)){error_="Invalid vault subdirectory";return false;}
 }
 if(!catalog_.open()){error_=catalog_.error();return false;}
 return true;
}
bool LocalVault::write_block(const std::string& cid,const std::vector<unsigned char>& data) {
 if(!cid::validate(cid)||!cid::matches(cid,data))return false;
 const auto final=root_/"blocks"/(cid+".mcb");
 if(fs::exists(final)) {
  std::vector<unsigned char> existing;
  return read_cid_block(final,cid,existing);
 }
 const auto suffix=unique_suffix();if(suffix.empty())return false;
 const auto temp=root_/"blocks"/(".partial-"+suffix);
 if(!write_new_file(temp,data)) {
  std::error_code ec;fs::remove(temp,ec);return false;
 }
 std::error_code ec;
 fs::rename(temp,final,ec);
 if(ec){fs::remove(temp,ec);return false;}
 return true;
}
bool LocalVault::read_block(const std::string& cid,std::vector<unsigned char>& data) {
 if(!cid::validate(cid))return false;
 return read_cid_block(root_/"blocks"/(cid+".mcb"),cid,data);
}
bool LocalVault::import_file(const fs::path& source,std::uint64_t now,
                             const FileKey& key,std::string& id) {
 id.clear();std::error_code ec;
 if(!fs::is_regular_file(source,ec)||ec){error_="Source is not a regular file";return false;}
 std::ifstream in(source,std::ios::binary);
 if(!in){error_="Source cannot be opened";return false;}
 std::array<unsigned char,16> salt{};
 if(!random_bytes(salt.data(),salt.size())){error_="Secure random generation failed";return false;}
 const auto suffix=unique_suffix();
 if(suffix.empty())return false;
 const auto temp=root_/"manifests"/(".partial-"+suffix);
 std::ofstream manifest(temp,std::ios::binary|std::ios::trunc);
 if(!manifest){error_="Cannot create manifest";return false;}
 manifest<<"MCM1\n";
 manifest<<hex(salt.data(),salt.size())<<"\n";
 // File size line: use known filesystem stat, then verify streamed size matches to reject races.
 const auto size=fs::file_size(source,ec);
 if(ec||size>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
  error_="File size unsupported by local filesystem/metadata format";manifest.close();fs::remove(temp,ec);return false;
 }
 manifest<<size<<"\n";
 std::vector<std::string> block_ids;
 std::vector<unsigned char> plain(chunk_bytes),sealed;
 std::uint64_t total=0;
 bool good=true;
 while(true) {
  in.read(reinterpret_cast<char*>(plain.data()),static_cast<std::streamsize>(plain.size()));
  const auto got=in.gcount();
  if(got<0||in.bad()||(in.fail()&&!in.eof())){good=false;break;}
  if(got==0)break;
  if(static_cast<std::uint64_t>(got)>size-total){good=false;break;}
  std::vector<unsigned char> chunk(plain.begin(),plain.begin()+got);
  if(!seal(chunk,key,salt,static_cast<std::uint64_t>(block_ids.size()),sealed)) {good=false;break;}
  const auto block_id=cid::from_bytes(sealed);
  if(block_id.empty()||!write_block(block_id,sealed)){good=false;break;}
  manifest<<block_id<<"\n";
  block_ids.push_back(block_id);
  total+=static_cast<std::uint64_t>(got);
  if(!manifest.good()){good=false;break;}
  if(in.eof())break;
 }
 manifest.flush();good=good&&manifest.good()&&total==size;
 manifest.close();
 if(!good){error_="Streaming import failed or input changed";fs::remove(temp,ec);return false;}
 std::string manifest_id;std::uint64_t length=0;
 if(!cid::from_file(temp,manifest_id,length)||manifest_id.empty()) {
  error_="Manifest hashing failed";fs::remove(temp,ec);return false;
 }
 const auto dst=root_/"manifests"/(manifest_id+".mcm");
 if(fs::exists(dst)){error_="Duplicate manifest";fs::remove(temp,ec);return false;}
 fs::rename(temp,dst,ec);
 if(ec){error_="Could not finalize manifest";fs::remove(temp,ec);return false;}
 CatalogEntry e{manifest_id,total,now,now,now+retention_seconds,0};
 if(now>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())-retention_seconds ||
    !catalog_.register_file(e,block_ids)) {
  error_="Could not register completed local import";fs::remove(dst,ec);return false;
 }
 id=manifest_id;return true;
}
bool LocalVault::export_file(const std::string& id,const fs::path& target,
                             const FileKey& key,std::uint64_t now) {
 CatalogEntry e{};
 if(!cid::validate(id)||!catalog_.find(id,e)){error_="Unknown CID";return false;}
 if(now>=e.expires_at){error_="File expired";return false;}
 std::vector<std::string> indexed;
 if(!catalog_.blocks(id,indexed)){error_="Invalid shard catalog";return false;}
 const auto manifest_path=root_/"manifests"/(id+".mcm");
 std::string check;std::uint64_t manifest_size=0;
 if(!cid::from_file(manifest_path,check,manifest_size)||check!=id) {
  error_="Manifest CID mismatch";return false;
 }
 std::ifstream manifest(manifest_path,std::ios::binary);
 std::string magic_line,salt_line,size_line;
 if(!std::getline(manifest,magic_line)||magic_line!="MCM1"||
    !std::getline(manifest,salt_line)||!std::getline(manifest,size_line)) {
  error_="Malformed manifest";return false;
 }
 std::array<unsigned char,16> salt{};
 if(!unhex(salt_line,salt.data(),salt.size())||size_line!=std::to_string(e.bytes)) {
  error_="Invalid manifest metadata";return false;
 }
 std::error_code ec;
 if(fs::exists(target,ec)||ec){error_="Output already exists";return false;}
 const auto suffix=unique_suffix();if(suffix.empty())return false;
 const auto temp=target.string()+".mesh-part-"+suffix;
 std::ofstream out(temp,std::ios::binary|std::ios::trunc);
 if(!out){error_="Cannot create output";return false;}
 bool good=true;std::uint64_t total=0;
 for(std::size_t index=0;index<indexed.size();index++) {
  std::string block_id;
  if(!std::getline(manifest,block_id)||block_id!=indexed[index]) {good=false;break;}
  std::vector<unsigned char> sealed,plain;
  if(!read_block(block_id,sealed) ||
     !unseal(sealed,key,salt,static_cast<std::uint64_t>(index),plain) ||
     plain.size()>e.bytes-total){good=false;break;}
  out.write(reinterpret_cast<const char*>(plain.data()),static_cast<std::streamsize>(plain.size()));
  if(!out.good()){good=false;break;}
  total+=static_cast<std::uint64_t>(plain.size());
 }
 std::string extra;
 if(std::getline(manifest,extra))good=false;
 good=good&&total==e.bytes;
 out.flush();good=good&&out.good();
 out.close();
 if(!good){error_="Wrong key, corrupted block or incomplete restore";fs::remove(temp,ec);return false;}
 if(fs::exists(target,ec)){fs::remove(temp,ec);error_="Output created concurrently";return false;}
 fs::rename(temp,target,ec);
 if(ec){fs::remove(temp,ec);error_="Could not finalize output";return false;}
 if(!catalog_.mark_completed_download(id,now)) {
  error_="Restore succeeded but retention refresh failed";return false;
 }
 return true;
}
bool LocalVault::prune(std::uint64_t now,std::uint64_t& expired_files,
                       std::uint64_t& deleted_blocks) {
 expired_files=0;deleted_blocks=0;
 std::vector<std::string> ids;
 if(!catalog_.purge_expired(now,ids)){error_="Catalog expiry sweep failed";return false;}
 expired_files=ids.size();
 std::error_code ec;
 for(const auto& id:ids)fs::remove(root_/"manifests"/(id+".mcm"),ec);
 // The exclusive process lock guarantees that a .partial-* object cannot be
 // written concurrently by another MeshCloud process at collection time.
 // Remove interrupted-upload temporary files, not arbitrary unknown filenames.
 for(const auto& dirname:{"blocks","manifests"}) {
  for(const auto& entry:fs::directory_iterator(root_/dirname,ec)) {
   if(ec){error_="Cannot scan incomplete uploads";return false;}
   const auto name=entry.path().filename().string();
   if(name.size()!=41||name.rfind(".partial-",0)!=0)continue;
   const auto suffix=name.substr(9);
   if(suffix.find_first_not_of("0123456789abcdef")!=std::string::npos)continue;
   if(!fs::is_regular_file(entry.symlink_status()))continue;
   fs::remove(entry.path(),ec);
   if(ec){error_="Cannot delete interrupted upload";return false;}
  }
 }
 for(const auto& entry:fs::directory_iterator(root_/"blocks",ec)) {
  if(ec){error_="Block scan failed";return false;}
  const auto path=entry.path();
  const auto name=path.filename().string();
  if(name.size()<5||name.substr(name.size()-4)!=".mcb")continue;
  const auto block_id=name.substr(0,name.size()-4);
  if(!cid::validate(block_id))continue;
  bool referenced=true;
  if(!catalog_.block_is_referenced(block_id,referenced)){error_="Reference check failed";return false;}
  if(!referenced && fs::remove(path,ec))deleted_blocks++;
  if(ec){error_="Could not delete unreferenced block";return false;}
 }
 return true;
}
} // namespace meshcloud
