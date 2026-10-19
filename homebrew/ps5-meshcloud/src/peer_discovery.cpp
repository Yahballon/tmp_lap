// SPDX-License-Identifier: GPL-3.0-or-later
#include "peer_discovery.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <system_error>
#include <openssl/evp.h>
#include <sqlite3.h>
namespace meshcloud::discovery {
namespace {
namespace fs=std::filesystem;
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
constexpr std::size_t header_size=5;
constexpr std::size_t signed_length=record_bytes-64;
constexpr char domain[]="MeshCloud/peer-address/lan-v1";
Statement statement(sqlite3* db,const char* query) {
 sqlite3_stmt* p=nullptr;
 if(sqlite3_prepare_v2(db,query,-1,&p,nullptr)!=SQLITE_OK)
  return {nullptr,sqlite3_finalize};
 return {p,sqlite3_finalize};
}
bool private_ipv4(const std::array<std::uint8_t,4>& a) {
 return (a[0]==10) || (a[0]==172&&a[1]>=16&&a[1]<=31) ||
  (a[0]==192&&a[1]==168) || a[0]==127;
}
bool shape(const SignedAddress& a) {
 return private_ipv4(a.ipv4) && a.port>=1024 &&
  a.identity!=admission::PublicKey{} && a.published>0 &&
  a.published<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())-record_lifetime &&
  a.expires>a.published && a.expires-a.published<=record_lifetime;
}
void write64(std::vector<std::uint8_t>& out,std::uint64_t n) {
 for(unsigned i=0;i<8;i++)out.push_back(static_cast<std::uint8_t>((n>>(8*i))&255));
}
bool read64(const std::uint8_t* p,std::uint64_t& n) {
 n=0;for(unsigned i=0;i<8;i++)n|=static_cast<std::uint64_t>(p[i])<<(8*i);
 return true;
}
std::vector<std::uint8_t> transcript(const SignedAddress& a) {
 std::vector<std::uint8_t> data;
 data.reserve(sizeof(domain)-1+signed_length);
 data.insert(data.end(),domain,domain+sizeof(domain)-1);
 data.insert(data.end(),{'M','C','A','1'});
 data.insert(data.end(),a.identity.begin(),a.identity.end());
 data.insert(data.end(),a.ipv4.begin(),a.ipv4.end());
 data.push_back(static_cast<std::uint8_t>((a.port>>8)&255));
 data.push_back(static_cast<std::uint8_t>(a.port&255));
 write64(data,a.published);write64(data,a.expires);
 return data;
}
std::string path_utf8(const fs::path& p) {
 const auto bytes=p.u8string();
 return std::string(bytes.begin(),bytes.end());
}
}
bool parse_lan_ipv4(const std::string& s,std::array<std::uint8_t,4>& out) {
 out={};if(s.size()<7||s.size()>15)return false;
 std::size_t pos=0;
 for(std::size_t i=0;i<4;i++) {
  const auto begin=pos;
  unsigned n=0;
  while(pos<s.size()&&s[pos]>='0'&&s[pos]<='9') {
   if(pos-begin>=3)return false;
   n=n*10+static_cast<unsigned>(s[pos]-'0');
   if(n>255)return false;
   pos++;
  }
  if(pos==begin || (pos-begin>1&&s[begin]=='0'))return false;
  out[i]=static_cast<std::uint8_t>(n);
  if(i==3){if(pos!=s.size())return false;}
  else if(pos>=s.size()||s[pos++]!='.')return false;
 }
 return private_ipv4(out);
}
std::string ipv4_text(const SignedAddress& p) {
 return std::to_string(p.ipv4[0])+"."+std::to_string(p.ipv4[1])+"."+
        std::to_string(p.ipv4[2])+"."+std::to_string(p.ipv4[3]);
}
bool sign(SignedAddress& record,EVP_PKEY* key) {
 record.signature.fill(0);
 if(!key||EVP_PKEY_base_id(key)!=EVP_PKEY_ED25519)return false;
 std::size_t n=record.identity.size();
 if(EVP_PKEY_get_raw_public_key(key,record.identity.data(),&n)!=1 ||
    n!=record.identity.size()||!shape(record))return false;
 const auto data=transcript(record);
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();if(!ctx)return false;
 std::size_t bytes=record.signature.size();
 const bool ok=EVP_DigestSignInit(ctx,nullptr,nullptr,nullptr,key)==1 &&
  EVP_DigestSign(ctx,record.signature.data(),&bytes,data.data(),data.size())==1 &&
  bytes==record.signature.size();
 EVP_MD_CTX_free(ctx);
 if(!ok)record.signature.fill(0);
 return ok;
}
bool verify(const SignedAddress& a,std::uint64_t now) {
 if(!shape(a) || now>=a.expires || a.published>now+300 ||
    a.signature==admission::Signature{})return false;
 const auto data=transcript(a);
 EVP_PKEY* key=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,
                  a.identity.data(),a.identity.size());
 if(!key)return false;
 EVP_MD_CTX* ctx=EVP_MD_CTX_new();
 if(!ctx){EVP_PKEY_free(key);return false;}
 const bool ok=EVP_DigestVerifyInit(ctx,nullptr,nullptr,nullptr,key)==1 &&
    EVP_DigestVerify(ctx,a.signature.data(),a.signature.size(),
                     data.data(),data.size())==1;
 EVP_MD_CTX_free(ctx);EVP_PKEY_free(key);
 return ok;
}
bool encode_record(const SignedAddress& a,std::vector<std::uint8_t>& out) {
 out.clear();
 if(!shape(a))return false;
 const auto data=transcript(a);
 out.insert(out.end(),data.end()-(static_cast<std::ptrdiff_t>(signed_length)),data.end());
 out.insert(out.end(),a.signature.begin(),a.signature.end());
 return out.size()==record_bytes;
}
bool decode_record(const std::uint8_t* data,std::size_t length,SignedAddress& r) {
 r={};
 if(!data||length!=record_bytes||data[0]!='M'||data[1]!='C'||
    data[2]!='A'||data[3]!='1')return false;
 std::copy(data+4,data+36,r.identity.begin());
 std::copy(data+36,data+40,r.ipv4.begin());
 r.port=static_cast<std::uint16_t>((static_cast<unsigned>(data[40])<<8)|data[41]);
 read64(data+42,r.published);read64(data+50,r.expires);
 std::copy(data+58,data+122,r.signature.begin());
 return shape(r);
}
bool encode_gossip(const std::vector<SignedAddress>& records,
                   std::vector<std::uint8_t>& out) {
 out.clear();if(records.size()>max_gossip_records)return false;
 out.insert(out.end(),{'M','C','G','1',static_cast<std::uint8_t>(records.size())});
 for(const auto& a:records) {
  std::vector<std::uint8_t> data;
  if(!encode_record(a,data)){out.clear();return false;}
  out.insert(out.end(),data.begin(),data.end());
 }
 return true;
}
bool decode_gossip(const std::vector<std::uint8_t>& in,
                   std::vector<SignedAddress>& out) {
 out.clear();
 if(in.size()<header_size || in.size()>header_size+max_gossip_records*record_bytes ||
   in[0]!='M'||in[1]!='C'||in[2]!='G'||in[3]!='1' ||
   in[4]>max_gossip_records ||
   in.size()!=header_size+static_cast<std::size_t>(in[4])*record_bytes)return false;
 for(std::size_t i=0;i<in[4];i++) {
  SignedAddress entry{};
  if(!decode_record(in.data()+header_size+i*record_bytes,record_bytes,entry)){
   out.clear();return false;
  }
  out.push_back(entry);
 }
 return true;
}
AddressBook::AddressBook(fs::path directory):directory_(std::move(directory)){}
AddressBook::~AddressBook(){if(db_)sqlite3_close(db_);}
bool AddressBook::open() {
 if(db_)return true;
 std::error_code ec;
 if(!fs::exists(directory_,ec))fs::create_directories(directory_,ec);
 if(ec||fs::symlink_status(directory_,ec).type()!=fs::file_type::directory ||ec){
  error_="Invalid peer database directory";return false;
 }
#ifndef _WIN32
 fs::permissions(directory_,fs::perms::owner_all,fs::perm_options::replace,ec);
 if(ec){error_="Cannot secure peer database directory";return false;}
#endif
 const auto file=path_utf8(directory_/"addrman.sqlite");
 if(sqlite3_open_v2(file.c_str(),&db_,
     SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,nullptr)!=SQLITE_OK) {
  error_="Cannot open addrman sqlite file";
  if(db_){sqlite3_close(db_);db_=nullptr;}return false;
 }
 sqlite3_busy_timeout(db_,5000);
 const char* ddl="PRAGMA journal_mode=WAL;"
   "CREATE TABLE IF NOT EXISTS peers("
   "identity BLOB PRIMARY KEY,record BLOB NOT NULL,"
   "published INTEGER NOT NULL,expires INTEGER NOT NULL);"
   "CREATE INDEX IF NOT EXISTS peers_expiry ON peers(expires);";
 char* failure=nullptr;
 if(sqlite3_exec(db_,ddl,nullptr,nullptr,&failure)!=SQLITE_OK){
  error_=failure?failure:"SQLite addrman initialization failed";
  sqlite3_free(failure);sqlite3_close(db_);db_=nullptr;return false;
 }
 return true;
}
bool AddressBook::expire(std::uint64_t now) {
 if(!db_||now>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
  return false;
 auto s=statement(db_,"DELETE FROM peers WHERE expires<=?1");
 return s && sqlite3_bind_int64(s.get(),1,static_cast<sqlite3_int64>(now))==SQLITE_OK
          && sqlite3_step(s.get())==SQLITE_DONE;
}
bool AddressBook::add(const SignedAddress& a,std::uint64_t now) {
 if(!db_||!verify(a,now)||!expire(now))return false;
 std::vector<std::uint8_t> bytes;
 if(!encode_record(a,bytes))return false;
 auto exists=statement(db_,"SELECT published FROM peers WHERE identity=?1");
 if(!exists||sqlite3_bind_blob(exists.get(),1,a.identity.data(),32,SQLITE_TRANSIENT)!=SQLITE_OK)
  return false;
 const int row=sqlite3_step(exists.get());
 if(row==SQLITE_ROW) {
  if(static_cast<std::uint64_t>(sqlite3_column_int64(exists.get(),0))>=a.published)
   return true; // Replay of a known record never refreshes the lease.
 }else if(row==SQLITE_DONE){
  exists.reset();
  auto cap=statement(db_,"SELECT COUNT(*) FROM peers");
  if(!cap||sqlite3_step(cap.get())!=SQLITE_ROW ||
     sqlite3_column_int64(cap.get(),0)>=static_cast<sqlite3_int64>(max_known_peers))
   return false;
 }else return false;
 exists.reset();
 auto insert=statement(db_,"INSERT INTO peers(identity,record,published,expires)"
  "VALUES(?1,?2,?3,?4) ON CONFLICT(identity) DO UPDATE SET "
  "record=excluded.record,published=excluded.published,expires=excluded.expires "
  "WHERE excluded.published>peers.published");
 return insert &&
    sqlite3_bind_blob(insert.get(),1,a.identity.data(),32,SQLITE_TRANSIENT)==SQLITE_OK &&
    sqlite3_bind_blob(insert.get(),2,bytes.data(),static_cast<int>(bytes.size()),SQLITE_TRANSIENT)==SQLITE_OK &&
    sqlite3_bind_int64(insert.get(),3,static_cast<sqlite3_int64>(a.published))==SQLITE_OK &&
    sqlite3_bind_int64(insert.get(),4,static_cast<sqlite3_int64>(a.expires))==SQLITE_OK &&
    sqlite3_step(insert.get())==SQLITE_DONE;
}
bool AddressBook::list(std::uint64_t now,std::vector<SignedAddress>& out,std::size_t limit) {
 out.clear();
 if(!db_||limit>max_known_peers||!expire(now))return false;
 auto query=statement(db_,"SELECT record FROM peers ORDER BY RANDOM() LIMIT ?1");
 if(!query || sqlite3_bind_int64(query.get(),1,static_cast<sqlite3_int64>(limit))!=SQLITE_OK)
  return false;
 int state=SQLITE_OK;
 while((state=sqlite3_step(query.get()))==SQLITE_ROW){
  const auto len=sqlite3_column_bytes(query.get(),0);
  const auto* bytes=static_cast<const std::uint8_t*>(sqlite3_column_blob(query.get(),0));
  SignedAddress a{};
  if(len!=static_cast<int>(record_bytes)||!decode_record(bytes,record_bytes,a)||
     !verify(a,now)){out.clear();return false;}
  out.push_back(a);
 }
 if(state!=SQLITE_DONE){out.clear();return false;}
 return true;
}
bool authorized_getaddr(const std::vector<std::uint8_t>& request,
    const admission::PublicKey& authenticated_peer,AddressBook& book,
    std::uint64_t now,std::vector<std::uint8_t>& reply) {
 reply.clear();
 std::vector<SignedAddress> supplied;
 if(!decode_gossip(request,supplied)||supplied.size()>1)return false;
 if(!supplied.empty()) {
  if(supplied[0].identity!=authenticated_peer||!book.add(supplied[0],now))
   return false;
 }
 std::vector<SignedAddress> known;
 if(!book.list(now,known,max_gossip_records))return false;
 return encode_gossip(known,reply);
}
bool authorized_addr_reply(const std::vector<std::uint8_t>& response,
    AddressBook& book,std::uint64_t now) {
 std::vector<SignedAddress> entries;
 if(!decode_gossip(response,entries))return false;
 // Verify ALL signatures/scope/expiry before importing ANY record.
 for(const auto& a:entries)if(!verify(a,now))return false;
 for(const auto& a:entries)if(!book.add(a,now))return false;
 return true;
}
} // namespace meshcloud::discovery
