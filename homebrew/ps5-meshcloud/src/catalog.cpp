// SPDX-License-Identifier: GPL-3.0-or-later
#include "catalog.hpp"
#include "cid.hpp"
#include <sqlite3.h>
#include <limits>
#include <memory>
namespace meshcloud {
namespace {
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db,const char* sql) {
 sqlite3_stmt* p=nullptr;
 if(sqlite3_prepare_v2(db,sql,-1,&p,nullptr)!=SQLITE_OK)return {nullptr,sqlite3_finalize};
 return {p,sqlite3_finalize};
}
bool bind_text(sqlite3_stmt* s,int n,const std::string& value) {
 return sqlite3_bind_text(s,n,value.c_str(),static_cast<int>(value.size()),SQLITE_TRANSIENT)==SQLITE_OK;
}
bool legal_time(std::uint64_t t) {
 return t<=static_cast<std::uint64_t>(std::numeric_limits<sqlite3_int64>::max())-retention_seconds;
}
}
Catalog::Catalog(std::filesystem::path path):path_(std::move(path)){}
Catalog::~Catalog(){if(db_)sqlite3_close(db_);}
bool Catalog::exec(const char* sql) {
 char* error=nullptr;
 const int rc=sqlite3_exec(db_,sql,nullptr,nullptr,&error);
 if(rc!=SQLITE_OK) {
  error_=error?error:sqlite3_errmsg(db_);
  sqlite3_free(error);return false;
 }
 return true;
}
bool Catalog::open() {
 if(db_)return true;
 const auto filename=path_.u8string();
 const std::string utf8(filename.begin(),filename.end());
 if(sqlite3_open_v2(utf8.c_str(),&db_,
    SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,nullptr)!=SQLITE_OK) {
  error_=db_?sqlite3_errmsg(db_):"Database open failed";
  if(db_){sqlite3_close(db_);db_=nullptr;}return false;
 }
 sqlite3_busy_timeout(db_,5000);
 if(!exec("PRAGMA foreign_keys=ON;") ||
    !exec("PRAGMA journal_mode=WAL;") ||
    !exec("CREATE TABLE IF NOT EXISTS files("
          "cid TEXT PRIMARY KEY,bytes INTEGER NOT NULL CHECK(bytes>=0),"
          "created_at INTEGER NOT NULL, last_success INTEGER NOT NULL,"
          "expires_at INTEGER NOT NULL, complete_count INTEGER NOT NULL DEFAULT 0);") ||
    !exec("CREATE TABLE IF NOT EXISTS file_blocks("
          "file_cid TEXT NOT NULL REFERENCES files(cid) ON DELETE CASCADE,"
          "sequence INTEGER NOT NULL CHECK(sequence>=0), block_cid TEXT NOT NULL,"
          "PRIMARY KEY(file_cid,sequence));") ||
    !exec("CREATE INDEX IF NOT EXISTS idx_blocks ON file_blocks(block_cid);") ||
    !exec("CREATE INDEX IF NOT EXISTS idx_expires ON files(expires_at);")) {
  sqlite3_close(db_);db_=nullptr;return false;
 }
 // Compatible migration for vault databases created by v0.2/v0.3.
 // Keep historical CIDs, timestamps and reference counts unchanged.
 auto columns=prepare(db_,"SELECT 1 FROM pragma_table_info('files') WHERE name='display_name'");
 if(!columns) {error_="Cannot inspect catalog schema";return false;}
 const int rc=sqlite3_step(columns.get());
 if(rc==SQLITE_DONE) {
  columns.reset();
  if(!exec("ALTER TABLE files ADD COLUMN display_name TEXT NOT NULL DEFAULT '';"))return false;
 }else if(rc!=SQLITE_ROW) {
  error_=sqlite3_errmsg(db_);return false;
 }
 return true;
}
bool Catalog::begin(){return exec("BEGIN IMMEDIATE;");}
bool Catalog::commit(){return exec("COMMIT;");}
void Catalog::rollback(){if(db_)sqlite3_exec(db_,"ROLLBACK;",nullptr,nullptr,nullptr);}
bool Catalog::register_file(const CatalogEntry& e,const std::vector<std::string>& chunks) {
 if(!db_||!cid::validate(e.cid)||!legal_time(e.created_at)||
    e.bytes>static_cast<std::uint64_t>(std::numeric_limits<sqlite3_int64>::max()) ||
    e.created_at!=e.last_successful_download || e.completed_downloads!=0 ||
    e.expires_at!=e.created_at+retention_seconds ||
    chunks.size()>static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()))
  return false;
 for(const auto& c:chunks)if(!cid::validate(c))return false;
 if(!begin())return false;
 auto file=prepare(db_,"INSERT INTO files(cid,bytes,created_at,last_success,expires_at,complete_count,display_name) "
                       "VALUES(?1,?2,?3,?4,?5,0,?6)");
 auto block=prepare(db_,"INSERT INTO file_blocks(file_cid,sequence,block_cid) VALUES(?1,?2,?3)");
 bool ok=file&&block;
 if(ok) {
  ok=bind_text(file.get(),1,e.cid) &&
   sqlite3_bind_int64(file.get(),2,static_cast<sqlite3_int64>(e.bytes))==SQLITE_OK &&
   sqlite3_bind_int64(file.get(),3,static_cast<sqlite3_int64>(e.created_at))==SQLITE_OK &&
   sqlite3_bind_int64(file.get(),4,static_cast<sqlite3_int64>(e.last_successful_download))==SQLITE_OK &&
   sqlite3_bind_int64(file.get(),5,static_cast<sqlite3_int64>(e.expires_at))==SQLITE_OK &&
   e.display_name.size()<=255 &&
   bind_text(file.get(),6,e.display_name) &&
   sqlite3_step(file.get())==SQLITE_DONE;
 }
 for(std::size_t i=0;ok&&i<chunks.size();i++) {
  sqlite3_reset(block.get());sqlite3_clear_bindings(block.get());
  ok=bind_text(block.get(),1,e.cid) &&
     sqlite3_bind_int64(block.get(),2,static_cast<sqlite3_int64>(i))==SQLITE_OK &&
     bind_text(block.get(),3,chunks[i]) &&
     sqlite3_step(block.get())==SQLITE_DONE;
 }
 file.reset();block.reset();
 if(ok&&commit())return true;
 error_=sqlite3_errmsg(db_);rollback();return false;
}
bool Catalog::find(const std::string& id,CatalogEntry& entry) {
 if(!db_||!cid::validate(id))return false;
 auto s=prepare(db_,"SELECT cid,bytes,created_at,last_success,expires_at,complete_count,display_name "
                    "FROM files WHERE cid=?1");
 if(!s||!bind_text(s.get(),1,id)||sqlite3_step(s.get())!=SQLITE_ROW)return false;
 entry.cid=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),0));
 entry.bytes=static_cast<std::uint64_t>(sqlite3_column_int64(s.get(),1));
 entry.created_at=static_cast<std::uint64_t>(sqlite3_column_int64(s.get(),2));
 entry.last_successful_download=static_cast<std::uint64_t>(sqlite3_column_int64(s.get(),3));
 entry.expires_at=static_cast<std::uint64_t>(sqlite3_column_int64(s.get(),4));
 entry.completed_downloads=static_cast<std::uint64_t>(sqlite3_column_int64(s.get(),5));
 const auto* name=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),6));
 entry.display_name=name?name:"";
 return true;
}
bool Catalog::list_live(std::uint64_t now,std::vector<CatalogEntry>& result,std::size_t limit) {
 result.clear();
 if(!db_||!legal_time(now)||limit==0||limit>10000)return false;
 auto statement=prepare(db_,"SELECT cid,bytes,created_at,last_success,expires_at,"
                           "complete_count,display_name FROM files WHERE expires_at>?1 "
                           "ORDER BY created_at DESC,cid LIMIT ?2");
 if(!statement ||
    sqlite3_bind_int64(statement.get(),1,static_cast<sqlite3_int64>(now))!=SQLITE_OK ||
    sqlite3_bind_int64(statement.get(),2,static_cast<sqlite3_int64>(limit))!=SQLITE_OK)
  return false;
 int rc=SQLITE_OK;
 while((rc=sqlite3_step(statement.get()))==SQLITE_ROW) {
  CatalogEntry entry;
  const auto* id=reinterpret_cast<const char*>(sqlite3_column_text(statement.get(),0));
  const auto* name=reinterpret_cast<const char*>(sqlite3_column_text(statement.get(),6));
  if(!id||!cid::validate(id)){result.clear();return false;}
  entry.cid=id;
  entry.bytes=static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(),1));
  entry.created_at=static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(),2));
  entry.last_successful_download=static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(),3));
  entry.expires_at=static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(),4));
  entry.completed_downloads=static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(),5));
  entry.display_name=name?name:"";
  result.push_back(std::move(entry));
 }
 if(rc!=SQLITE_DONE){result.clear();return false;}
 return true;
}
bool Catalog::blocks(const std::string& id,std::vector<std::string>& chunks) {
 chunks.clear();
 if(!db_||!cid::validate(id))return false;
 auto s=prepare(db_,"SELECT block_cid FROM file_blocks WHERE file_cid=?1 ORDER BY sequence ASC");
 if(!s||!bind_text(s.get(),1,id))return false;
 int rc=SQLITE_OK;
 while((rc=sqlite3_step(s.get()))==SQLITE_ROW) {
  const auto* c=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),0));
  if(!c||!cid::validate(c)){chunks.clear();return false;}
  chunks.emplace_back(c);
 }
 if(rc!=SQLITE_DONE){chunks.clear();return false;}
 return true;
}
bool Catalog::mark_completed_download(const std::string& id,std::uint64_t now) {
 if(!db_||!cid::validate(id)||!legal_time(now))return false;
 auto s=prepare(db_,"UPDATE files SET last_success=?1,expires_at=?2,"
 "complete_count=complete_count+1 WHERE cid=?3 AND expires_at>?1 AND last_success<=?1");
 if(!s ||
    sqlite3_bind_int64(s.get(),1,static_cast<sqlite3_int64>(now))!=SQLITE_OK ||
    sqlite3_bind_int64(s.get(),2,static_cast<sqlite3_int64>(now+retention_seconds))!=SQLITE_OK ||
    !bind_text(s.get(),3,id))return false;
 if(sqlite3_step(s.get())!=SQLITE_DONE)return false;
 return sqlite3_changes(db_)==1;
}
bool Catalog::expired(std::uint64_t now,std::vector<std::string>& ids) {
 ids.clear();if(!db_||!legal_time(now))return false;
 auto s=prepare(db_,"SELECT cid FROM files WHERE expires_at<=?1 ORDER BY cid");
 if(!s||sqlite3_bind_int64(s.get(),1,static_cast<sqlite3_int64>(now))!=SQLITE_OK)return false;
 int rc=SQLITE_OK;
 while((rc=sqlite3_step(s.get()))==SQLITE_ROW) {
  const auto* p=reinterpret_cast<const char*>(sqlite3_column_text(s.get(),0));
  if(!p)return false;
  ids.emplace_back(p);
 }
 if(rc!=SQLITE_DONE){ids.clear();return false;}
 return true;
}
bool Catalog::purge_expired(std::uint64_t now,std::vector<std::string>& deleted) {
 deleted.clear();if(!db_||!legal_time(now)||!begin())return false;
 std::vector<std::string> ids;
 if(!expired(now,ids)){rollback();return false;}
 auto s=prepare(db_,"DELETE FROM files WHERE cid=?1 AND expires_at<=?2");
 if(!s){rollback();return false;}
 bool ok=true;
 for(const auto& id:ids) {
  sqlite3_reset(s.get());sqlite3_clear_bindings(s.get());
  ok=bind_text(s.get(),1,id) &&
    sqlite3_bind_int64(s.get(),2,static_cast<sqlite3_int64>(now))==SQLITE_OK &&
    sqlite3_step(s.get())==SQLITE_DONE && sqlite3_changes(db_)==1;
  if(!ok)break;
 }
 s.reset();
 if(ok && commit()){deleted.swap(ids);return true;}
 rollback();return false;
}
bool Catalog::block_is_referenced(const std::string& id,bool& referenced) {
 referenced=true;
 if(!db_||!cid::validate(id))return false;
 auto s=prepare(db_,"SELECT 1 FROM file_blocks WHERE block_cid=?1 LIMIT 1");
 if(!s||!bind_text(s.get(),1,id))return false;
 const int rc=sqlite3_step(s.get());
 if(rc!=SQLITE_ROW&&rc!=SQLITE_DONE)return false;
 referenced=rc==SQLITE_ROW;return true;
}
} // namespace meshcloud
