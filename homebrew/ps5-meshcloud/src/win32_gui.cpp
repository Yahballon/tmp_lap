// SPDX-License-Identifier: GPL-3.0-or-later
// MeshCloud by YAHBALLON v0.6.0 - native, dependency-light Windows desktop UI.
// Shares the same C++ storage engine with the CLI; NO P2P listener or bypass.
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <cwchar>
#include <exception>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "core.hpp"
#include "vault.hpp"
#include "stream.hpp"
#include "cid.hpp"

namespace {
namespace fs=std::filesystem;
constexpr COLORREF background=RGB(9,16,30);
constexpr COLORREF panel=RGB(16,28,47);
constexpr COLORREF textbox=RGB(19,33,53);
constexpr COLORREF ink=RGB(228,240,255);
constexpr COLORREF muted=RGB(138,162,194);
constexpr COLORREF accent=RGB(51,148,255);
constexpr COLORREF danger=RGB(238,119,112);
constexpr int id_vault=101,id_cid=102,id_list=103,id_status=104,id_progress=105,id_summary=106;
constexpr int id_browse=201,id_import=202,id_export=203,id_hash=204,id_refresh=205,id_prune=206,id_copy=207,id_cancel=208;
constexpr UINT wm_progress=WM_APP+8,wm_finished=WM_APP+9;
enum class Job { Refresh, Prune, Import, Export, Hash };
struct Work {
 Job action=Job::Refresh;
 fs::path root,source,key_file,destination;
 std::string cid;
};
struct Outcome {
 bool success=false,has_listing=false;
 std::wstring message;
 std::string cid;
 std::vector<meshcloud::CatalogEntry> items;
 std::uint64_t removed_files=0,removed_blocks=0;
};
struct App {
 HWND window=nullptr,vault=nullptr,cid=nullptr,list=nullptr,status=nullptr,progress=nullptr;
 HWND summary=nullptr,browse=nullptr,import_file=nullptr,export_file=nullptr,hash=nullptr;
 HWND refresh=nullptr,prune=nullptr,copy=nullptr,cancel=nullptr;
 HFONT font=nullptr;
 HBRUSH bg_brush=nullptr,edit_brush=nullptr;
 std::thread worker;
 std::atomic<bool> cancel_requested{false};
 bool busy=false,close_when_finished=false;
 std::vector<meshcloud::CatalogEntry> items;
};
std::wstring wstr(const std::string& s) {
 if(s.empty())return {};
 const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),
                                   static_cast<int>(s.size()),nullptr,0);
 if(n<=0)return L"[invalid UTF-8]";
 std::wstring out(static_cast<std::size_t>(n),L'\0');
 MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),
                      static_cast<int>(s.size()),out.data(),n);
 return out;
}
std::wstring read_text(HWND control) {
 const int n=GetWindowTextLengthW(control);
 if(n<=0||n>32767)return {};
 std::wstring value(static_cast<std::size_t>(n)+1,L'\0');
 const int copied=GetWindowTextW(control,value.data(),n+1);
 value.resize(static_cast<std::size_t>(std::max(0,copied)));
 return value;
}
std::uint64_t now() {
 const auto t=std::chrono::system_clock::now().time_since_epoch();
 return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(t).count());
}
std::wstring date(std::uint64_t epoch) {
 const auto time=static_cast<time_t>(epoch);
 tm local{};
 if(localtime_s(&local,&time)!=0)return L"Unknown";
 wchar_t text[40]{};
 swprintf_s(text,L"%04d-%02d-%02d %02d:%02d",
     local.tm_year+1900,local.tm_mon+1,local.tm_mday,local.tm_hour,local.tm_min);
 return text;
}
std::wstring size_label(std::uint64_t bytes) {
 const wchar_t* units[]={L"B",L"KiB",L"MiB",L"GiB",L"TiB",L"PiB",L"EiB"};
 double number=static_cast<double>(bytes);
 int u=0;while(number>=1024&&u<6){number/=1024;u++;}
 std::wostringstream s;
 if(u==0)s<<bytes;else s<<std::fixed<<std::setprecision(2)<<number;
 s<<L" "<<units[u];return s.str();
}
fs::path default_storage() {
 wchar_t local[MAX_PATH]{};
 if(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,
   SHGFP_TYPE_CURRENT,local)==S_OK)
  return fs::path(local)/L"MeshCloud"/L"vault";
 return fs::current_path()/L"meshcloud-vault";
}
void message(App& a,const std::wstring& text) {
 SetWindowTextW(a.status,text.c_str());
}
std::wstring browse_file(HWND parent,bool save,const wchar_t* filter,
                         const wchar_t* default_ext=nullptr) {
 std::vector<wchar_t> name(32768,0);
 OPENFILENAMEW dialog{};
 dialog.lStructSize=sizeof(dialog);
 dialog.hwndOwner=parent;
 dialog.lpstrFilter=filter;
 dialog.lpstrFile=name.data();
 dialog.nMaxFile=static_cast<DWORD>(name.size());
 dialog.lpstrDefExt=default_ext;
 dialog.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST;
 if(!save)dialog.Flags|=OFN_FILEMUSTEXIST;
 if(save?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog))
  return name.data();
 return {};
}
std::wstring choose_folder(HWND parent) {
 BROWSEINFOW bi{};
 bi.hwndOwner=parent;
 bi.lpszTitle=L"Choose MeshCloud local encrypted vault directory";
 bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
 PIDLIST_ABSOLUTE id=SHBrowseForFolderW(&bi);
 if(!id)return {};
 wchar_t buffer[MAX_PATH]{};
 const bool okay=SHGetPathFromIDListW(id,buffer)!=FALSE;
 CoTaskMemFree(id);
 return okay?std::wstring(buffer):std::wstring();
}
bool copy_cid(HWND win,const std::wstring& content) {
 if(content.empty()||!OpenClipboard(win))return false;
 EmptyClipboard();
 const auto n=(content.size()+1)*sizeof(wchar_t);
 HGLOBAL object=GlobalAlloc(GMEM_MOVEABLE,n);
 bool okay=false;
 if(object) {
  void* memory=GlobalLock(object);
  if(memory) {
   std::memcpy(memory,content.c_str(),n);
   GlobalUnlock(object);
   okay=SetClipboardData(CF_UNICODETEXT,object)!=nullptr;
  }
  if(!okay)GlobalFree(object);
 }
 CloseClipboard();return okay;
}
void buttons_enabled(App& a,bool enabled) {
 for(auto h:{a.browse,a.import_file,a.export_file,a.hash,a.refresh,a.prune,a.copy})
  EnableWindow(h,enabled);
 EnableWindow(a.vault,enabled);
 EnableWindow(a.cid,enabled);
 EnableWindow(a.list,enabled);
 EnableWindow(a.cancel,!enabled);
}
HWND make_label(HWND parent,const wchar_t* text,int id=0) {
 return CreateWindowExW(0,L"STATIC",text,WS_CHILD|WS_VISIBLE|SS_LEFT,
                        0,0,10,10,parent,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                        GetModuleHandleW(nullptr),nullptr);
}
HWND make_edit(HWND parent,int id,const wchar_t* content) {
 return CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",content,
   WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
   0,0,10,10,parent,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
   GetModuleHandleW(nullptr),nullptr);
}
HWND make_button(HWND parent,int id,const wchar_t* text) {
 return CreateWindowExW(0,L"BUTTON",text,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
     0,0,10,10,parent,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
     GetModuleHandleW(nullptr),nullptr);
}
void create_children(App& a) {
 a.font=CreateFontW(-19,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                     DEFAULT_PITCH,L"Segoe UI");
 a.bg_brush=CreateSolidBrush(background);
 a.edit_brush=CreateSolidBrush(textbox);
 a.vault=make_edit(a.window,id_vault,default_storage().wstring().c_str());
 a.browse=make_button(a.window,id_browse,L"Browse folder");
 a.summary=make_label(a.window,L"Network: LOCKED  |  Member verification pending  |  Minimum 10 GiB",id_summary);
 a.import_file=make_button(a.window,id_import,L"Encrypt + import");
 a.export_file=make_button(a.window,id_export,L"Restore selected");
 a.hash=make_button(a.window,id_hash,L"File CID");
 a.refresh=make_button(a.window,id_refresh,L"Refresh list");
 a.prune=make_button(a.window,id_prune,L"Cleanup 90 days");
 a.list=CreateWindowExW(WS_EX_CLIENTEDGE,WC_LISTVIEWW,L"",
   WS_CHILD|WS_VISIBLE|WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,
   0,0,10,10,a.window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id_list)),GetModuleHandleW(nullptr),nullptr);
 ListView_SetExtendedListViewStyle(a.list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
 ListView_SetBkColor(a.list,textbox);
 ListView_SetTextBkColor(a.list,textbox);
 ListView_SetTextColor(a.list,ink);
 const wchar_t* headings[]={L"File",L"Size",L"Expires",L"Manifest CID"};
 const int widths[]={340,130,190,500};
 for(int i=0;i<4;i++) {
  LVCOLUMNW column{};column.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_FMT;
  column.pszText=const_cast<wchar_t*>(headings[i]);
  column.cx=widths[i];column.fmt=LVCFMT_LEFT;
  ListView_InsertColumn(a.list,i,&column);
 }
 a.cid=make_edit(a.window,id_cid,L"");
 a.copy=make_button(a.window,id_copy,L"Copy CID");
 a.cancel=make_button(a.window,id_cancel,L"Cancel operation");
 a.status=make_label(a.window,L"Local vault only. No public P2P access is enabled.",id_status);
 a.progress=CreateWindowExW(0,PROGRESS_CLASSW,L"",WS_CHILD|WS_VISIBLE|PBS_SMOOTH,
   0,0,10,10,a.window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id_progress)),
   GetModuleHandleW(nullptr),nullptr);
 SendMessageW(a.progress,PBM_SETRANGE32,0,100);
 SendMessageW(a.progress,PBM_SETBARCOLOR,0,static_cast<LPARAM>(accent));
 EnableWindow(a.cancel,FALSE);
 for(auto h:{a.vault,a.summary,a.browse,a.import_file,a.export_file,a.hash,
              a.refresh,a.prune,a.list,a.cid,a.copy,a.cancel,a.status})
  SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(a.font),TRUE);
}
void move(HWND h,int x,int y,int w,int height) {
 if(h)MoveWindow(h,x,y,std::max(1,w),std::max(1,height),TRUE);
}
void layout(App& a,int width,int height) {
 // Dynamic layout, scalable down to 1000x680. No fixed 1080p requirement.
 const int side=34;
 const int inner=width-side*2;
 move(a.vault,side+115,152,inner-285,39);
 move(a.browse,width-side-157,152,157,39);
 move(a.summary,side+6,211,inner-12,37);
 const int y=274,gap=10,bw=(inner-5*gap)/6;
 move(a.import_file,side,y,bw,45);
 move(a.export_file,side+(bw+gap),y,bw,45);
 move(a.hash,side+2*(bw+gap),y,bw,45);
 move(a.refresh,side+3*(bw+gap),y,bw,45);
 move(a.prune,side+4*(bw+gap),y,bw,45);
 move(a.cancel,side+5*(bw+gap),y,bw,45);
 move(a.list,side,336,inner,std::max(160,height-518));
 const int cy=height-159;
 move(a.cid,side,cy,inner-165,39);
 move(a.copy,width-side-155,cy,155,39);
 move(a.status,side,height-105,inner,32);
 move(a.progress,side,height-54,inner,15);
 ListView_SetColumnWidth(a.list,0,static_cast<int>(inner*.27));
 ListView_SetColumnWidth(a.list,1,static_cast<int>(inner*.12));
 ListView_SetColumnWidth(a.list,2,static_cast<int>(inner*.17));
 ListView_SetColumnWidth(a.list,3,static_cast<int>(inner*.43));
}
void populate(App& a,const std::vector<meshcloud::CatalogEntry>& entries) {
 a.items=entries;
 ListView_DeleteAllItems(a.list);
 for(std::size_t i=0;i<a.items.size();i++) {
  const auto& entry=a.items[i];
  const auto name=wstr(entry.display_name.empty()?"Unnamed file":entry.display_name);
  auto size=size_label(entry.bytes),expire=date(entry.expires_at);
  auto cid=wstr(entry.cid);
  LVITEMW item{};
  item.mask=LVIF_TEXT;item.iItem=static_cast<int>(i);
  item.pszText=const_cast<wchar_t*>(name.c_str());
  const int pos=ListView_InsertItem(a.list,&item);
  if(pos>=0){
   ListView_SetItemText(a.list,pos,1,const_cast<wchar_t*>(size.c_str()));
   ListView_SetItemText(a.list,pos,2,const_cast<wchar_t*>(expire.c_str()));
   ListView_SetItemText(a.list,pos,3,const_cast<wchar_t*>(cid.c_str()));
  }
 }
 const std::wstring line=std::to_wstring(a.items.size())+
  L" local encrypted file(s)  |  90-day inactivity expiry  |  NETWORK LOCKED - verified 10 GiB membership unavailable";
 SetWindowTextW(a.summary,line.c_str());
}
struct Runner {
 static std::unique_ptr<Outcome> execute(App& a,const Work& work) {
  auto result=std::make_unique<Outcome>();
  try {
   const meshcloud::ProgressCallback callback=[&a,last_percent=-1](std::uint64_t completed,std::uint64_t total) mutable {
    if(a.cancel_requested.load())return false;
    const unsigned percent=total?static_cast<unsigned>(
      std::min(100.0,100.0*static_cast<double>(completed)/static_cast<double>(total))):100;
    if(static_cast<int>(percent)!=last_percent) {
     last_percent=static_cast<int>(percent);
     PostMessageW(a.window,wm_progress,percent,0);
    }
    return !a.cancel_requested.load();
   };
   if(work.action==Job::Hash) {
    std::string cid;std::uint64_t bytes=0;
    result->success=meshcloud::cid::from_file(work.source,cid,bytes,callback);
    if(result->success){
     result->cid=cid;result->message=L"File SHA-256 CID calculated: "+size_label(bytes);
    }else result->message=a.cancel_requested.load()?L"CID hashing cancelled":L"Unable to calculate CID from selected file";
    return result;
   }
   meshcloud::LocalVault vault(work.root);
   if(!vault.open()) {result->message=L"Vault: "+wstr(vault.error());return result;}
   std::uint64_t removed=0,blocks=0;
   if(!vault.prune(now(),removed,blocks)) {
    result->message=L"Cleanup: "+wstr(vault.error());return result;
   }
   result->removed_files=removed;result->removed_blocks=blocks;

   switch(work.action) {
    case Job::Import: {
     meshcloud::FileKey key{};
     if(!meshcloud::make_key(key)||!meshcloud::save_key(work.key_file,key)) {
      result->message=L"Cannot create private key file (existing keys are never overwritten)";
      break;
     }
     std::string cid;
     if(vault.import_file(work.source,now(),key,cid,callback)){
      result->success=true;result->cid=cid;
      result->message=L"Encrypted local import completed. Keep your private key safe.";
     }else result->message=a.cancel_requested.load()?L"Import cancelled":L"Import failed: "+wstr(vault.error());
     break;
    }
    case Job::Export: {
     meshcloud::FileKey key{};
     if(!meshcloud::load_key(work.key_file,key)) {
      result->message=L"Could not read valid private key";break;
     }
     if(vault.export_file(work.cid,work.destination,key,now(),callback)){
      result->success=true;result->cid=work.cid;
      result->message=L"Verified local restore completed; 90-day timer renewed.";
     }else result->message=a.cancel_requested.load()?L"Restore cancelled":L"Restore failed: "+wstr(vault.error());
     break;
    }
    case Job::Refresh: {
     result->success=true;
     const auto pf=meshcloud::preflight(work.root,meshcloud::minimum_bytes);
     result->message=pf.eligible
      ?L"Disk preflight: 10 GiB + 2 GiB safety margin available. NETWORK REMAINS LOCKED."
      :L"Disk preflight: "+wstr(pf.message)+L". NETWORK REMAINS LOCKED.";
     break;
    }
    case Job::Prune:
     result->success=true;
     result->message=L"Cleanup: "+std::to_wstring(removed)+L" expired file(s), "+
       std::to_wstring(blocks)+L" unreferenced blocks removed.";
     break;
    case Job::Hash:break;
   }
   // The vault lock is still held: read the complete list consistently.
   if(!vault.catalog().list_live(now(),result->items)) {
    result->success=false;
    result->message=L"Vault listing failed (no data was erased).";
   }else result->has_listing=true;
  } catch(const std::exception& e) {
   result->success=false;result->message=L"Operation error: "+wstr(e.what());
  } catch(...) {
   result->success=false;result->message=L"Unexpected operation error";
  }
  return result;
 }
};
void launch(App& a,Work work) {
 if(a.busy)return;
 if(a.worker.joinable())a.worker.join();
 a.busy=true;a.cancel_requested=false;
 buttons_enabled(a,false);
 SendMessageW(a.progress,PBM_SETPOS,0,0);
 message(a,L"Working locally... streaming operation in progress");
 const auto hwnd=a.window;
 a.worker=std::thread([&a,work=std::move(work),hwnd]() {
  auto result=Runner::execute(a,work);
  if(!PostMessageW(hwnd,wm_finished,0,reinterpret_cast<LPARAM>(result.get())))
   return; // unique_ptr frees result if the window no longer exists
  result.release();
 });
}
bool confirm_file_absent(HWND hwnd,const fs::path& p) {
 std::error_code ec;
 if(fs::exists(p,ec) || ec) {
  MessageBoxW(hwnd,L"That file already exists. MeshCloud will not overwrite it.",
              L"Choose another filename",MB_OK|MB_ICONWARNING);
  return false;
 }
 return true;
}
void execute_command(App& a,int action) {
 if(a.busy)return;
 Work work;work.root=read_text(a.vault);
 if(work.root.empty()){message(a,L"Choose a local vault directory first.");return;}
 switch(action) {
 case id_refresh:work.action=Job::Refresh;break;
 case id_prune:work.action=Job::Prune;break;
 case id_import: {
  auto input=browse_file(a.window,false,L"All files\0*.*\0\0");
  if(input.empty())return;
  auto key=browse_file(a.window,true,L"MeshCloud key\0*.key\0All files\0*.*\0\0",L"key");
  if(key.empty()||!confirm_file_absent(a.window,key))return;
  work.action=Job::Import;work.source=input;work.key_file=key;break;
 }
 case id_export: {
  work.cid=std::string();
  const auto current=read_text(a.cid);
  if(current.size()!=59){message(a,L"Paste or select a valid CIDv1 first.");return;}
  for(const wchar_t c:current) {
   if(c<0x20 || c>0x7e) {
    message(a,L"CID contains invalid non-ASCII characters.");return;
   }
   work.cid.push_back(static_cast<char>(c));
  }
  if(!meshcloud::cid::validate(work.cid)){
   message(a,L"CIDv1 is malformed.");return;
  }
  auto key=browse_file(a.window,false,L"MeshCloud key\0*.key\0All files\0*.*\0\0");
  if(key.empty())return;
  auto output=browse_file(a.window,true,L"All files\0*.*\0\0");
  if(output.empty()||!confirm_file_absent(a.window,output))return;
  work.action=Job::Export;work.key_file=key;work.destination=output;break;
 }
 case id_hash: {
  auto file=browse_file(a.window,false,L"All files\0*.*\0\0");
  if(file.empty())return;
  work.action=Job::Hash;work.source=file;break;
 }
 default:return;
 }
 launch(a,std::move(work));
}
void paint_button(const DRAWITEMSTRUCT& di) {
 const bool disabled=(di.itemState&ODS_DISABLED)!=0;
 const bool pressed=(di.itemState&ODS_SELECTED)!=0;
 const bool primary=di.CtlID==id_import;
 const COLORREF color=disabled?RGB(42,54,71):
       pressed?RGB(25,88,156):primary?RGB(24,112,217):RGB(30,57,90);
 HBRUSH fill=CreateSolidBrush(color);
 HPEN border=CreatePen(PS_SOLID,1,disabled?RGB(70,81,96):RGB(66,112,162));
 auto old_brush=SelectObject(di.hDC,fill);
 auto old_pen=SelectObject(di.hDC,border);
 const RECT& r=di.rcItem;
 RoundRect(di.hDC,r.left,r.top,r.right,r.bottom,12,12);
 SelectObject(di.hDC,old_brush);SelectObject(di.hDC,old_pen);
 DeleteObject(fill);DeleteObject(border);
 wchar_t text[100]{};GetWindowTextW(di.hwndItem,text,100);
 SetBkMode(di.hDC,TRANSPARENT);
 SetTextColor(di.hDC,disabled?muted:ink);
 RECT label=r;DrawTextW(di.hDC,text,-1,&label,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 if((di.itemState&ODS_FOCUS)!=0)DrawFocusRect(di.hDC,&r);
}
LRESULT CALLBACK window_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam) {
 App* a=reinterpret_cast<App*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
 if(msg==WM_NCCREATE){
  const auto* creation=reinterpret_cast<CREATESTRUCTW*>(lparam);
  auto* app=reinterpret_cast<App*>(creation->lpCreateParams);
  if(!app)return FALSE;
  app->window=hwnd; // WM_CREATE runs before CreateWindowExW returns.
  SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app));
  return TRUE;
 }
 if(!a)return DefWindowProcW(hwnd,msg,wparam,lparam);
 switch(msg) {
 case WM_CREATE: {
  create_children(*a);
  RECT r{};GetClientRect(hwnd,&r);layout(*a,r.right,r.bottom);
  execute_command(*a,id_refresh);return 0;
 }
 case WM_SIZE: {
  layout(*a,LOWORD(lparam),HIWORD(lparam));InvalidateRect(hwnd,nullptr,FALSE);
  return 0;
 }
 case WM_GETMINMAXINFO: {
  auto* info=reinterpret_cast<MINMAXINFO*>(lparam);
  info->ptMinTrackSize={1010,740};return 0;
 }
 case WM_COMMAND: {
  const int id=LOWORD(wparam);
  if(HIWORD(wparam)!=BN_CLICKED)return 0;
  if(id==id_browse) {
   const auto selected=choose_folder(hwnd);
   if(!selected.empty()){SetWindowTextW(a->vault,selected.c_str());execute_command(*a,id_refresh);}
   return 0;
  }
  if(id==id_copy) {
   message(*a,copy_cid(hwnd,read_text(a->cid))?L"CID copied to clipboard":L"No CID to copy");
   return 0;
  }
  if(id==id_cancel) {a->cancel_requested=true;message(*a,L"Cancellation requested...");return 0;}
  execute_command(*a,id);return 0;
 }
 case WM_NOTIFY: {
  const auto* header=reinterpret_cast<NMHDR*>(lparam);
  if(header->idFrom==id_list&&header->code==LVN_ITEMCHANGED&&!a->busy) {
   const auto* change=reinterpret_cast<NMLISTVIEW*>(lparam);
   if((change->uNewState&LVIS_SELECTED)&&change->iItem>=0&&
      static_cast<std::size_t>(change->iItem)<a->items.size())
    SetWindowTextW(a->cid,wstr(a->items[static_cast<std::size_t>(change->iItem)].cid).c_str());
  }
  return 0;
 }
 case WM_CTLCOLORSTATIC: {
  const HDC dc=reinterpret_cast<HDC>(wparam);
  SetBkMode(dc,TRANSPARENT);
  SetTextColor(dc,reinterpret_cast<HWND>(lparam)==a->summary?danger:ink);
  return reinterpret_cast<LRESULT>(a->bg_brush);
 }
 case WM_CTLCOLOREDIT: {
  const HDC dc=reinterpret_cast<HDC>(wparam);
  SetTextColor(dc,ink);SetBkColor(dc,textbox);
  return reinterpret_cast<LRESULT>(a->edit_brush);
 }
 case WM_DRAWITEM: {
  if(wparam>=id_browse&&wparam<=id_cancel){
   paint_button(*reinterpret_cast<DRAWITEMSTRUCT*>(lparam));return TRUE;
  }
  break;
 }
 case wm_progress:
  SendMessageW(a->progress,PBM_SETPOS,wparam,0);return 0;
 case wm_finished: {
  std::unique_ptr<Outcome> outcome(reinterpret_cast<Outcome*>(lparam));
  if(a->worker.joinable())a->worker.join();
  a->busy=false;
  if(!a->close_when_finished) {
   buttons_enabled(*a,true);
   if(outcome) {
    if(outcome->has_listing)populate(*a,outcome->items);
    if(!outcome->cid.empty())SetWindowTextW(a->cid,wstr(outcome->cid).c_str());
    message(*a,outcome->message);
   }
   SendMessageW(a->progress,PBM_SETPOS,0,0);
  } else DestroyWindow(hwnd);
  return 0;
 }
 case WM_PAINT: {
  PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);
  RECT client{};GetClientRect(hwnd,&client);
  FillRect(dc,&client,a->bg_brush);
  RECT top{0,0,client.right,132};HBRUSH header=CreateSolidBrush(panel);
  FillRect(dc,&top,header);DeleteObject(header);
  SetBkMode(dc,TRANSPARENT);
  HFONT large=CreateFontW(-36,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
  auto old=SelectObject(dc,large);
  SetTextColor(dc,ink);RECT title{35,19,client.right-30,67};
  DrawTextW(dc,L"MESHCLOUD", -1,&title,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  SelectObject(dc,old);DeleteObject(large);
  SetTextColor(dc,muted);
  RECT sub{37,75,client.right-30,112};
  DrawTextW(dc,L"BY YAHBALLON    /    v0.6.0 PRE-ALPHA    /    ENCRYPTED LOCAL VAULT", -1,
      &sub,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  RECT vault_title{36,150,146,192};
  DrawTextW(dc,L"Vault folder", -1,&vault_title,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  RECT files_title{35,320,client.right-30,346};
  DrawTextW(dc,L"YOUR FILES", -1,&files_title,DT_LEFT|DT_TOP|DT_SINGLELINE);
  RECT cid_title{35,client.bottom-185,client.right-30,client.bottom-163};
  DrawTextW(dc,L"MANIFEST CID  /  RESTORE BY CID", -1,&cid_title,DT_LEFT|DT_TOP|DT_SINGLELINE);
  EndPaint(hwnd,&ps);return 0;
 }
 case WM_CLOSE:
  if(a->busy) {
   a->close_when_finished=true;
   a->cancel_requested=true;
   message(*a,L"Finishing safe cancellation before closing...");
  } else DestroyWindow(hwnd);
  return 0;
 case WM_DESTROY:
  PostQuitMessage(0);return 0;
 default:break;
 }
 return DefWindowProcW(hwnd,msg,wparam,lparam);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show) {
 SetProcessDPIAware();
 INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_LISTVIEW_CLASSES|ICC_PROGRESS_CLASS};
 InitCommonControlsEx(&controls);
 CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 App app{};
 WNDCLASSEXW cls{};
 cls.cbSize=sizeof(cls);cls.hInstance=instance;
 cls.lpfnWndProc=window_proc;cls.lpszClassName=L"MeshCloudWin32Main";
 cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
 cls.hIcon=LoadIconW(nullptr,IDI_APPLICATION);
 cls.style=CS_HREDRAW|CS_VREDRAW;
 if(!RegisterClassExW(&cls))return 1;
 app.window=CreateWindowExW(0,cls.lpszClassName,
    L"MeshCloud by YAHBALLON - v0.6.0 pre-alpha",
    WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
    CW_USEDEFAULT,CW_USEDEFAULT,1280,830,nullptr,nullptr,instance,&app);
 if(!app.window)return 1;
 ShowWindow(app.window,show);UpdateWindow(app.window);
 MSG msg{};
 while(GetMessageW(&msg,nullptr,0,0)>0) {
  TranslateMessage(&msg);DispatchMessageW(&msg);
 }
 if(app.worker.joinable())app.worker.join();
 if(app.font)DeleteObject(app.font);
 if(app.bg_brush)DeleteObject(app.bg_brush);
 if(app.edit_brush)DeleteObject(app.edit_brush);
 CoUninitialize();return static_cast<int>(msg.wParam);
}
