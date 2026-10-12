// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone CoinGecko HTTPS client (no PC/server, no API key).
// All requests run in a 1 MiB worker thread, never in the render loop.
#include "market_client.hpp"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "ca_bundle.inc"
#include <atomic>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include <string>
#include <string_view>
#include <vector>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace tracker {
namespace {

std::mutex g_lock;
std::atomic<bool> g_busy{false};
Snapshot g_snapshot;
struct Job { bool history = false; std::string id; int days = 1; };
Job g_job;

void whitespace(std::string_view s, std::size_t& p) {
    while (p < s.size() && (s[p]==' ' || s[p]=='\n' || s[p]=='\r' || s[p]=='\t')) ++p;
}
std::size_t value_end(std::string_view s, std::size_t p) {
    whitespace(s,p);
    if (p >= s.size()) return p;
    if (s[p]=='"') {
        ++p;
        while (p < s.size()) { if (s[p]=='\\') { p += 2; continue; } if (s[p++]=='"') break; }
        return std::min(p,s.size());
    }
    if (s[p]=='[' || s[p]=='{') {
        int depth=0; bool quoted=false; bool escape=false;
        for (;p<s.size();++p) {
            char c=s[p];
            if (quoted) { if (escape) escape=false; else if(c=='\\') escape=true; else if(c=='"') quoted=false; continue; }
            if (c=='"') {quoted=true;continue;}
            if (c=='[' || c=='{') ++depth;
            if (c==']' || c=='}') {if(--depth==0) return p+1;}
        }
        return p;
    }
    while (p < s.size() && s[p]!=',' && s[p]!='}' && s[p]!=']' && !std::isspace((unsigned char)s[p])) ++p;
    return p;
}
std::string_view field(std::string_view obj, std::string_view wanted) {
    std::size_t p=0; whitespace(obj,p);
    if (p>=obj.size() || obj[p++]!='{') return {};
    for (;;) {
        whitespace(obj,p);
        if (p>=obj.size() || obj[p]=='}') return {};
        if (obj[p]!='"') return {};
        std::size_t end=value_end(obj,p);
        std::string_view name=obj.substr(p+1,end-p-2);
        p=end; whitespace(obj,p);
        if (p>=obj.size() || obj[p++]!=':') return {};
        whitespace(obj,p);
        std::size_t e=value_end(obj,p);
        if(name==wanted) return obj.substr(p,e-p);
        p=e; whitespace(obj,p);
        if(p>=obj.size() || obj[p]=='}') return {};
        if(obj[p++]!=',') return {};
    }
}
std::string ascii(std::string_view v) {
    std::string out;
    if(v.size()<2 || v.front()!='"')return out;
    for(std::size_t i=1;i+1<v.size() && out.size()<48;++i) {
        unsigned char c=(unsigned char)v[i];
        if(c=='\\' && i+1<v.size()-1) {
            char e=v[++i];
            if(e=='n' || e=='r' || e=='t') out+=' ';
            else if(e=='"')out+='"';
            else if(e=='\\')out+='\\';
            continue;
        }
        if(c>=32 && c<127) out+=(char)c;
    }
    return out;
}
double number(std::string_view v) {
    if(v.empty() || v=="null")return 0;
    std::string copy(v);
    char* end=nullptr; double n=std::strtod(copy.c_str(), &end);
    return end!=copy.c_str() && std::isfinite(n) ? n : 0;
}
std::vector<double> numbers(std::string_view arr, std::size_t maxCount) {
    std::vector<double> out;
    std::size_t p=0; whitespace(arr,p);
    if(p>=arr.size() || arr[p++]!='[')return out;
    while(p<arr.size() && out.size()<maxCount) {
        whitespace(arr,p); if(p>=arr.size() || arr[p]==']')break;
        const std::size_t e=value_end(arr,p); if(e<=p)break;
        out.push_back(number(arr.substr(p,e-p))); p=e;
        whitespace(arr,p);if(p<arr.size() && arr[p]==',')++p;
        else break;
    }
    return out;
}
bool parse_coins(std::string_view json,std::vector<Coin>& out) {
    std::size_t p=0; whitespace(json,p);
    if(p>=json.size() || json[p++]!='[')return false;
    while(p<json.size() && out.size()<100) {
        whitespace(json,p); if(p>=json.size() || json[p]==']')break;
        const std::size_t e=value_end(json,p);
        if(e<=p)break;
        const auto object=json.substr(p,e-p); p=e;
        Coin coin;
        coin.id=ascii(field(object,"id"));
        coin.name=ascii(field(object,"name"));
        coin.symbol=ascii(field(object,"symbol"));
        std::transform(coin.symbol.begin(),coin.symbol.end(),coin.symbol.begin(),
                       [](unsigned char c){return (char)std::toupper(c);});
        coin.rank=(int)number(field(object,"market_cap_rank"));
        coin.price=number(field(object,"current_price"));
        coin.change24h=number(field(object,"price_change_percentage_24h"));
        coin.marketCap=number(field(object,"market_cap"));
        coin.volume24h=number(field(object,"total_volume"));
        coin.circulating=number(field(object,"circulating_supply"));
        coin.totalSupply=number(field(object,"total_supply"));
        coin.ath=number(field(object,"ath"));
        coin.atl=number(field(object,"atl"));
        const auto spark=field(object,"sparkline_in_7d");
        coin.spark=numbers(field(spark,"price"),220);
        if(!coin.id.empty() && coin.price>0)out.push_back(std::move(coin));
        whitespace(json,p); if(p<json.size() && json[p]==',')++p;
        else break;
    }
    return !out.empty();
}
bool parse_chart(std::string_view json,std::vector<double>& chart) {
    const auto prices=field(json,"prices");
    std::size_t p=0; whitespace(prices,p);
    if(p>=prices.size() || prices[p++]!='[')return false;
    while(p<prices.size() && chart.size()<2000) {
        whitespace(prices,p);
        if(p>=prices.size() || prices[p]==']')break;
        const auto end=value_end(prices,p);if(end<=p)break;
        const auto row=prices.substr(p,end-p);
        if(row.size()>2 && row[0]=='[') {
            std::size_t comma=row.find(',');
            if(comma!=std::string_view::npos) {
                std::size_t idx=comma+1;whitespace(row,idx);
                double val=number(row.substr(idx));
                if(val>0)chart.push_back(val);
            }
        }
        p=end;whitespace(prices,p);
        if(p<prices.size() && prices[p]==',')++p;else break;
    }
    return chart.size()>=2;
}
bool decode_chunks(std::string_view in,std::string& out) {
    std::size_t p=0;
    while(p<in.size()) {
        const auto line=in.find("\r\n",p);
        if(line==std::string_view::npos || line-p>40)return false;
        std::string hex(in.substr(p,line-p));
        const auto semicolon=hex.find(';');
        if(semicolon!=std::string::npos)hex.resize(semicolon);
        char* end=nullptr;
        unsigned long n=std::strtoul(hex.c_str(),&end,16);
        if(end==hex.c_str() || *end!='\0' || n>4*1024*1024)return false;
        p=line+2;
        if(n==0)return true;
        if(n>in.size()-p || out.size()+n>4*1024*1024)return false;
        out.append(in.data()+p,n);p+=n;
        if(p+2>in.size() || in.substr(p,2)!="\r\n")return false;
        p+=2;
    }
    return false;
}

bool https_get(const std::string& path,std::string& body,std::string& error) {
    const char* host="api.coingecko.com";
    SSL_CTX* ctx=SSL_CTX_new(TLS_client_method());
    if(!ctx){error="TLS initialization failed";return false;}
    SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
    SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);
    // Trust anchors are built into eboot.bin, independent of PS5 mounts and
    // /app0 access rules (which vary by loader and sandbox configuration).
    BIO* bundle=BIO_new_mem_buf(kCaBundle, -1);
    if(!bundle){error="TLS trust bundle initialization failed";SSL_CTX_free(ctx);return false;}
    X509_STORE* store=SSL_CTX_get_cert_store(ctx);
    int trusted=0;
    while(X509* cert=PEM_read_bio_X509(bundle,nullptr,nullptr,nullptr)){
        if(X509_STORE_add_cert(store,cert)==1)++trusted;
        X509_free(cert);
    }
    BIO_free(bundle);
    ERR_clear_error(); // PEM reaches EOF after the final certificate
    if(trusted==0){error="Embedded TLS certificate bundle invalid";SSL_CTX_free(ctx);return false;}
    addrinfo hints{};
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    addrinfo* results=nullptr;
    if(getaddrinfo(host,"443",&hints,&results)!=0) {
        error="DNS unavailable";SSL_CTX_free(ctx);return false;
    }
    int fd=-1;
    for(addrinfo* addr=results;addr;addr=addr->ai_next) {
        fd=socket(addr->ai_family,addr->ai_socktype,addr->ai_protocol);
        if(fd<0)continue;
        timeval timeout{10,0};
        setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
        if(connect(fd,addr->ai_addr,addr->ai_addrlen)==0)break;
        close(fd);fd=-1;
    }
    freeaddrinfo(results);
    if(fd<0){error="Network connection failed";SSL_CTX_free(ctx);return false;}
    SSL* ssl=SSL_new(ctx);
    if(!ssl) {error="TLS allocation failed";close(fd);SSL_CTX_free(ctx);return false;}
    SSL_set_fd(ssl,fd);
    SSL_set_tlsext_host_name(ssl,host);
    SSL_set1_host(ssl,host);
    if(SSL_connect(ssl)!=1 || SSL_get_verify_result(ssl)!=X509_V_OK) {
        error="TLS handshake/certificate failed";
        SSL_free(ssl);close(fd);SSL_CTX_free(ctx);return false;
    }
    const std::string request="GET "+path+" HTTP/1.1\r\nHost: "+host+
      "\r\nUser-Agent: PS5CryptoTracker/0.1\r\nAccept: application/json\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
    int sent=0;
    while(sent<(int)request.size()) {
        const int n=SSL_write(ssl,request.data()+sent,(int)request.size()-sent);
        if(n<=0){error="HTTPS write failed";SSL_free(ssl);close(fd);SSL_CTX_free(ctx);return false;}
        sent+=n;
    }
    std::string response;response.reserve(256*1024);
    char buf[8192];
    for(;;) {
        int n=SSL_read(ssl,buf,sizeof(buf));
        if(n<=0)break;
        if(response.size()+(std::size_t)n>4*1024*1024){error="API response too large";break;}
        response.append(buf,n);
    }
    SSL_shutdown(ssl); SSL_free(ssl); close(fd); SSL_CTX_free(ctx);
    if(!error.empty())return false;
    auto split=response.find("\r\n\r\n");
    if(split==std::string::npos){error="Invalid HTTP response";return false;}
    auto header=response.substr(0,split);
    int status=0;
    if(std::sscanf(header.c_str(),"HTTP/%*s %d",&status)!=1){error="Invalid HTTP status";return false;}
    if(status!=200){char detail[64];std::snprintf(detail,sizeof(detail),"CoinGecko HTTP %d",status);error=detail;return false;}
    std::string lower=header;
    std::transform(lower.begin(),lower.end(),lower.begin(),
                   [](unsigned char c){return (char)std::tolower(c);});
    const std::string_view raw(response.data()+split+4,response.size()-split-4);
    if(lower.find("transfer-encoding: chunked")!=std::string::npos) {
        if(!decode_chunks(raw,body)){error="Invalid chunked JSON";return false;}
    } else body.assign(raw);
    return !body.empty();
}
void* worker(void*) {
    Job job;
    {
        std::lock_guard<std::mutex> guard(g_lock);
        job=g_job;
    }
    std::string json,error;
    std::vector<Coin> coins;
    std::vector<double> chart;
    bool ok=false;
    if(job.history) {
        const std::string path="/api/v3/coins/"+job.id+"/market_chart?vs_currency=usd&days="+
            std::to_string(job.days);
        ok=https_get(path,json,error) && parse_chart(json,chart);
        if(!ok && error.empty())error="Invalid chart API response";
    } else {
        ok=https_get("/api/v3/coins/markets?vs_currency=usd&order=market_cap_desc&per_page=100&page=1&sparkline=true",
                     json,error) && parse_coins(json,coins);
        if(!ok && error.empty())error="Invalid market API response";
    }
    {
        std::lock_guard<std::mutex> guard(g_lock);
        if(ok) {
            if(job.history) {
                g_snapshot.chart=std::move(chart);
                g_snapshot.chartCoin=job.id;
                g_snapshot.chartDays=job.days;
                g_snapshot.chartLoaded=true;
                g_snapshot.status="Chart updated";
            } else {
                g_snapshot.coins=std::move(coins);
                g_snapshot.marketLoaded=true;
                g_snapshot.refreshedAtUs=(std::uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                g_snapshot.status="Live prices - CoinGecko";
            }
        } else {
            g_snapshot.status=error.empty()?"Network unavailable":error;
        }
        g_snapshot.busy=false;
    }
    g_busy.store(false,std::memory_order_release);
    return nullptr;
}
bool launch(Job job) {
    bool expected=false;
    if(!g_busy.compare_exchange_strong(expected,true))return false;
    {
        std::lock_guard<std::mutex> guard(g_lock);
        g_job=std::move(job);
        g_snapshot.busy=true;
        g_snapshot.status=g_job.history?"Updating chart...":"Updating live markets...";
        if(g_job.history)g_snapshot.chartLoaded=false;
    }
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setstacksize(&attrs,1024*1024);
    pthread_t thread{};
    int rc=pthread_create(&thread,&attrs,worker,nullptr);
    pthread_attr_destroy(&attrs);
    if(rc!=0) {
        std::lock_guard<std::mutex> guard(g_lock);
        g_snapshot.busy=false;
        g_snapshot.status="Unable to start network worker";
        g_busy.store(false);
        return false;
    }
    pthread_detach(thread);
    return true;
}

} // namespace

bool requestMarket(){return launch(Job{});}
bool requestChart(const std::string& id,int days) {
    if(id.empty() || id.size()>80)return false;
    for(unsigned char c:id)if(!std::isalnum(c) && c!='-')return false;
    if(days!=1 && days!=7 && days!=30 && days!=90 && days!=365)return false;
    return launch(Job{true,id,days});
}
Snapshot snapshot() {
    std::lock_guard<std::mutex> guard(g_lock);
    return g_snapshot;
}
} // namespace tracker
