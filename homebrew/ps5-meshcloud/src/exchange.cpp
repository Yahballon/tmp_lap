// SPDX-License-Identifier: GPL-3.0-or-later
#include "exchange.hpp"
#include "cid.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <limits>
#include <memory>
#include <system_error>
#include <openssl/rand.h>
#include <sqlite3.h>

namespace meshcloud::exchange {
namespace {
namespace fs=std::filesystem;
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
constexpr std::size_t blob_header=36;
bool safe_time(std::uint64_t now) {
 return now<=static_cast<std::uint64_t>(
   std::numeric_limits<sqlite3_int64>::max())-staging_ttl_seconds;
}
bool valid_envelope(const std::vector<std::uint8_t>& data) {
 if(data.size()<blob_header||data.size()>encrypted_block_limit)return false;
 if(data[0]!='M'||data[1]!='C'||data[2]!='B'||data[3]!='1')return false;
 const auto size=static_cast<std::uint32_t>(data[32]) |
   (static_cast<std::uint32_t>(data[33])<<8) |
   (static_cast<std::uint32_t>(data[34])<<16) |
   (static_cast<std::uint32_t>(data[35])<<24);
 return size==data.size()-blob_header;
}
bool valid_cid(const std::string& id) {
 return id.size()==cid_ascii_size && cid::validate(id);
}
Statement prepare(sqlite3* db,const char* sql) {
 sqlite3_stmt* result=nullptr;
 if(sqlite3_prepare_v2(db,sql,-1,&result,nullptr)!=SQLITE_OK)
  return {nullptr,sqlite3_finalize};
 return {result,sqlite3_finalize};
}
bool bind_cid(sqlite3_stmt* s,int position,const std::string& id) {
 return sqlite3_bind_text(s,position,id.data(),static_cast<int>(id.size()),
                          SQLITE_TRANSIENT)==SQLITE_OK;
}
std::string unique_hex() {
 unsigned char buf[16]{};
 if(RAND_bytes(buf,static_cast<int>(sizeof(buf)))!=1)return {};
 constexpr char digits[]="0123456789abcdef";
 std::string result(32,'0');
 for(std::size_t i=0;i<16;i++) {
  result[i*2]=digits[buf[i]>>4];result[i*2+1]=digits[buf[i]&15];
 }
 return result;
}
bool safe_folder(const fs::path& p) {
 std::error_code ec;
 return fs::symlink_status(p,ec).type()==fs::file_type::directory && !ec;
}
bool known_file(const fs::path& p) {
 std::error_code ec;
 return fs::symlink_status(p,ec).type()==fs::file_type::regular && !ec;
}
bool read_verified(const fs::path& path,const std::string& id,
                   std::vector<std::uint8_t>& content) {
 content.clear();
 if(!known_file(path))return false;
 std::error_code ec;
 const auto length=fs::file_size(path,ec);
 if(ec||length<blob_header||length>encrypted_block_limit)return false;
 std::ifstream in(path,std::ios::binary);
 if(!in)return false;
 std::vector<std::uint8_t> b(static_cast<std::size_t>(length));
 in.read(reinterpret_cast<char*>(b.data()),
         static_cast<std::streamsize>(b.size()));
 if(in.gcount()!=static_cast<std::streamsize>(b.size()) ||
    in.bad() || !valid_envelope(b) || !cid::matches(id,b))return false;
 content.swap(b);
 return true;
}
bool write_stage(const fs::path& path,const std::vector<std::uint8_t>& data) {
 if(fs::exists(path))return false;
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 if(!out)return false;
 out.write(reinterpret_cast<const char*>(data.data()),
           static_cast<std::streamsize>(data.size()));
 out.flush();return out.good();
}
fs::path block_path(const fs::path& root,const std::string& id) {
 return root/"blocks"/(id+".mcb");
}
bool no_overflow(std::uint64_t used,std::size_t bytes,std::uint64_t max) {
 return used<=max && static_cast<std::uint64_t>(bytes)<=max-used;
}
} // namespace

SharedShardStore::SharedShardStore(fs::path root,std::uint64_t limit)
 :directory_(std::move(root)),max_bytes_(limit){}
SharedShardStore::~SharedShardStore() {
 if(db_)sqlite3_close(db_);
}
bool SharedShardStore::sql(const char* command) {
 char* err=nullptr;
 const int rc=sqlite3_exec(db_,command,nullptr,nullptr,&err);
 if(rc==SQLITE_OK)return true;
 error_=err?err:sqlite3_errmsg(db_);
 sqlite3_free(err);return false;
}
bool SharedShardStore::open() {
 if(db_)return true;
 std::error_code ec;
 if(!fs::exists(directory_,ec))fs::create_directories(directory_,ec);
 if(ec||!safe_folder(directory_)){error_="Invalid exchange directory";return false;}
#ifndef _WIN32
 fs::permissions(directory_,fs::perms::owner_all,fs::perm_options::replace,ec);
 if(ec){error_="Cannot restrict exchange folder permissions";return false;}
#endif
 // Prevent two cooperating processes mutating the same objects/TTL catalog.
 if(!writer_lock_.acquire(directory_/".exchange.lock")) {
  error_="Exchange directory already locked";return false;
 }
 const auto blocks=directory_/"blocks";
 if(!fs::exists(blocks,ec))fs::create_directory(blocks,ec);
 if(ec||!safe_folder(blocks)){error_="Invalid encrypted block directory";return false;}
#ifndef _WIN32
 fs::permissions(blocks,fs::perms::owner_all,fs::perm_options::replace,ec);
 if(ec){error_="Cannot restrict encrypted shard folder permissions";return false;}
#endif
 if(max_bytes_==0 || max_bytes_>
     static_cast<std::uint64_t>(std::numeric_limits<sqlite3_int64>::max())) {
  error_="Invalid shard storage quota";return false;
 }
 const auto path=directory_/"shards.sqlite";
 const auto u8=path.u8string();
 const std::string path_utf8(u8.begin(),u8.end());
 if(sqlite3_open_v2(path_utf8.c_str(),&db_,
   SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,nullptr)!=SQLITE_OK) {
  error_=db_?sqlite3_errmsg(db_):"Could not open shard metadata";
  if(db_){sqlite3_close(db_);db_=nullptr;}
  return false;
 }
 sqlite3_busy_timeout(db_,5000);
 if(!sql("PRAGMA foreign_keys=ON;") ||
    !sql("PRAGMA journal_mode=WAL;") ||
    !sql("CREATE TABLE IF NOT EXISTS shards("
         "cid TEXT PRIMARY KEY, bytes INTEGER NOT NULL CHECK(bytes>0),"
         " expires INTEGER NOT NULL);") ||
    !sql("CREATE INDEX IF NOT EXISTS shard_expiry ON shards(expires);")) {
  sqlite3_close(db_);db_=nullptr;return false;
 }
 // Reconcile expired and interrupted shard writes on service startup.
 const auto seconds=std::chrono::duration_cast<std::chrono::seconds>(
     std::chrono::system_clock::now().time_since_epoch()).count();
 std::uint64_t removed=0;
 if(seconds<0 || !prune(static_cast<std::uint64_t>(seconds),removed)) {
  error_="Failed to reconcile exchange store on startup";
  sqlite3_close(db_);db_=nullptr;return false;
 }
 return true;
}
Result SharedShardStore::put(const std::string& id,
    const std::vector<std::uint8_t>& bytes,std::uint64_t now) {
 if(!db_)return Result::StorageFailure;
 if(!safe_time(now)||!valid_cid(id)||!valid_envelope(bytes)||
    !cid::matches(id,bytes))return Result::Malformed;
 auto existing=prepare(db_,"SELECT bytes,expires FROM shards WHERE cid=?1");
 if(!existing||!bind_cid(existing.get(),1,id))return Result::StorageFailure;
 const int row=sqlite3_step(existing.get());
 if(row==SQLITE_ROW) {
  // Re-sending a fragment does not refresh its 90-day staging lease.
  if(sqlite3_column_int64(existing.get(),1)>static_cast<sqlite3_int64>(now)) {
   std::vector<std::uint8_t> previous;
   return read_verified(block_path(directory_,id),id,previous)&&previous==bytes
    ?Result::Ok:Result::StorageFailure;
  }
  existing.reset();
  std::uint64_t deleted=0;
  if(!prune(now,deleted))return Result::StorageFailure;
  // The expired record and its ciphertext are now removed; accept a fresh PUT.
 } else if(row!=SQLITE_DONE) {
  return Result::StorageFailure;
 }
 existing.reset();
 auto used_query=prepare(db_,"SELECT COALESCE(SUM(bytes),0) FROM shards");
 if(!used_query||sqlite3_step(used_query.get())!=SQLITE_ROW)
  return Result::StorageFailure;
 const auto signed_used=sqlite3_column_int64(used_query.get(),0);
 if(signed_used<0)return Result::StorageFailure;
 const auto used=static_cast<std::uint64_t>(signed_used);
 used_query.reset();
 if(!no_overflow(used,bytes.size(),max_bytes_))return Result::QuotaExceeded;
 auto suffix=unique_hex();if(suffix.empty())return Result::StorageFailure;
 const auto temp=directory_/"blocks"/(".partial-"+suffix);
 const auto final=block_path(directory_,id);
 if(!write_stage(temp,bytes)) {
  std::error_code ec;fs::remove(temp,ec);return Result::StorageFailure;
 }
 std::error_code ec;
 if(fs::exists(final,ec)||ec) {
  fs::remove(temp,ec);return Result::StorageFailure;
 }
 fs::rename(temp,final,ec);
 if(ec){fs::remove(temp,ec);return Result::StorageFailure;}
 auto insert=prepare(db_,"INSERT INTO shards(cid,bytes,expires) VALUES(?1,?2,?3)");
 const bool success=insert && bind_cid(insert.get(),1,id) &&
  sqlite3_bind_int64(insert.get(),2,static_cast<sqlite3_int64>(bytes.size()))==SQLITE_OK &&
  sqlite3_bind_int64(insert.get(),3,
                      static_cast<sqlite3_int64>(now+staging_ttl_seconds))==SQLITE_OK &&
  sqlite3_step(insert.get())==SQLITE_DONE;
 insert.reset();
 if(!success){fs::remove(final,ec);return Result::StorageFailure;}
 return Result::Ok;
}
Result SharedShardStore::get(const std::string& id,std::uint64_t now,
    std::vector<std::uint8_t>& encrypted) {
 encrypted.clear();
 if(!db_)return Result::StorageFailure;
 if(!valid_cid(id)||!safe_time(now))return Result::Malformed;
 auto select=prepare(db_,"SELECT expires FROM shards WHERE cid=?1");
 if(!select||!bind_cid(select.get(),1,id))return Result::StorageFailure;
 const int step=sqlite3_step(select.get());
 if(step==SQLITE_DONE)return Result::NotFound;
 if(step!=SQLITE_ROW)return Result::StorageFailure;
 if(sqlite3_column_int64(select.get(),0)<=static_cast<sqlite3_int64>(now))
  return Result::NotFound;
 return read_verified(block_path(directory_,id),id,encrypted)
     ?Result::Ok:Result::StorageFailure;
}
bool SharedShardStore::prune(std::uint64_t now,std::uint64_t& removed) {
 removed=0;
 if(!db_||!safe_time(now))return false;
 // Catalog deletion first. Crash recovery reconciles orphaned files below.
 auto deletion=prepare(db_,"DELETE FROM shards WHERE expires<=?1");
 if(!deletion || sqlite3_bind_int64(deletion.get(),1,
       static_cast<sqlite3_int64>(now))!=SQLITE_OK ||
    sqlite3_step(deletion.get())!=SQLITE_DONE)return false;
 deletion.reset();
 std::error_code ec;
 for(const auto& entry:fs::directory_iterator(directory_/"blocks",ec)) {
  if(ec){error_="Failed scanning exchange blocks";return false;}
  const auto path=entry.path();
  const auto name=path.filename().string();
  if(!known_file(path))continue; // never follow symlinks
  bool orphan=false;
  if(name.size()==41 && name.rfind(".partial-",0)==0 &&
      name.substr(9).find_first_not_of("0123456789abcdef")==std::string::npos) {
   orphan=true;
  } else if(name.size()==cid_ascii_size+4 &&
            name.substr(cid_ascii_size)==".mcb" &&
            valid_cid(name.substr(0,cid_ascii_size))) {
   auto query=prepare(db_,"SELECT 1 FROM shards WHERE cid=?1");
   if(!query || !bind_cid(query.get(),1,name.substr(0,cid_ascii_size)))return false;
   const int step=sqlite3_step(query.get());
   if(step==SQLITE_DONE)orphan=true;
   else if(step!=SQLITE_ROW)return false;
  }
  if(orphan) {
   if(!fs::remove(path,ec)||ec){error_="Cannot reclaim expired shard";return false;}
   removed++;
  }
 }
 if(ec){error_="Cannot iterate shard directory";return false;}
 return true;
}
bool make_put(const std::string& id,const std::vector<std::uint8_t>& ciphertext,
              std::vector<std::uint8_t>& result) {
 result.clear();
 if(!valid_cid(id)||!valid_envelope(ciphertext)||
    !cid::matches(id,ciphertext))return false;
 wire::Frame frame{Operation::Upload,{}};
 frame.payload.reserve(cid_ascii_size+ciphertext.size());
 frame.payload.insert(frame.payload.end(),id.begin(),id.end());
 frame.payload.insert(frame.payload.end(),ciphertext.begin(),ciphertext.end());
 return wire::serialize(frame,result);
}
bool make_get(const std::string& id,std::vector<std::uint8_t>& result) {
 result.clear();
 if(!valid_cid(id))return false;
 wire::Frame frame{Operation::Download,{}};
 frame.payload.assign(id.begin(),id.end());
 return wire::serialize(frame,result);
}
bool PeerGateway::handle(const std::vector<std::uint8_t>& request,
    const admission::AuthenticatedPeer& peer,const admission::Challenge& nonce,
    const admission::Challenge& exporter,std::uint64_t now,
    std::vector<std::uint8_t>& response) {
 response.clear();
 wire::Frame decoded;
 if(!wire::authenticated_parse(request,peer,nonce,exporter,now,decoded))
  return false; // deny without even a status reply
 if(decoded.operation!=Operation::Upload && decoded.operation!=Operation::Download)
  return false;
 if(decoded.payload.size()<cid_ascii_size)return false;
 const std::string id(decoded.payload.begin(),
                      decoded.payload.begin()+static_cast<std::ptrdiff_t>(cid_ascii_size));
 if(!valid_cid(id))return false;
 Result result=Result::StorageFailure;
 wire::Frame reply{Operation::Metadata,{}};
 if(decoded.operation==Operation::Upload) {
  const std::vector<std::uint8_t> encrypted(
   decoded.payload.begin()+static_cast<std::ptrdiff_t>(cid_ascii_size),
   decoded.payload.end());
  if(!valid_envelope(encrypted)||!cid::matches(id,encrypted))return false;
  result=store_.put(id,encrypted,now);
 } else {
  if(decoded.payload.size()!=cid_ascii_size)return false;
  std::vector<std::uint8_t> ciphertext;
  result=store_.get(id,now,ciphertext);
  if(result==Result::Ok) {
   reply.operation=Operation::Download;
   reply.payload.assign(id.begin(),id.end());
   reply.payload.insert(reply.payload.end(),ciphertext.begin(),ciphertext.end());
   return wire::serialize(reply,response);
  }
 }
 reply.payload.push_back(static_cast<std::uint8_t>(result));
 return wire::serialize(reply,response);
}
bool read_get_response(const std::vector<std::uint8_t>& bytes,
     const std::string& requested_id,const admission::AuthenticatedPeer& peer,
     const admission::Challenge& nonce,const admission::Challenge& exporter,
     std::uint64_t now,std::vector<std::uint8_t>& verified) {
 verified.clear();
 if(!valid_cid(requested_id))return false;
 wire::Frame decoded;
 if(!wire::authenticated_parse(bytes,peer,nonce,exporter,now,decoded) ||
    decoded.operation!=Operation::Download ||
    decoded.payload.size()<cid_ascii_size+blob_header ||
    decoded.payload.size()>cid_ascii_size+encrypted_block_limit)return false;
 if(!std::equal(requested_id.begin(),requested_id.end(),decoded.payload.begin()))
  return false;
 std::vector<std::uint8_t> ciphertext(
  decoded.payload.begin()+static_cast<std::ptrdiff_t>(cid_ascii_size),
  decoded.payload.end());
 if(!valid_envelope(ciphertext)||!cid::matches(requested_id,ciphertext))
  return false;
 verified.swap(ciphertext);
 return true;
}
} // namespace meshcloud::exchange
