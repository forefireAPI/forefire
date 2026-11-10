#include "fftile.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <limits>
#ifdef FF_TILE_HTTP
#include <curl/curl.h>
#include <unistd.h>
#include <cstdio>
#endif

namespace libforefire {
namespace {
std::string trim(const std::string& s) {
    auto a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");
    return a==std::string::npos ? "" : s.substr(a,b-a+1);
}
std::array<double,2> pair(const std::string& text) {
    auto s=trim(text);
    if (s.size()<5 || s.front()!='(' || s.back()!=')')
        throw std::runtime_error("fftile: expected (longitude,latitude) or (width,height)");
    s=s.substr(1,s.size()-2);auto comma=s.find(',');
    if (comma==std::string::npos || s.find(',',comma+1)!=std::string::npos)
        throw std::runtime_error("fftile: expected exactly two numeric components");
    std::array<double,2> out;
    for (int i=0;i<2;++i) {
        auto item=trim(i==0?s.substr(0,comma):s.substr(comma+1));size_t used=0;
        out[i]=std::stod(item,&used);
        if (used!=item.size() || !std::isfinite(out[i])) throw std::runtime_error("fftile: invalid coordinates or size");
    }
    return out;
}
std::string cacheNamespace(const std::string& url) {
    // Stable service-specific namespace: never put credentials or URL text on disk.
    uint64_t hash=14695981039346656037ull;
    for (unsigned char c:url) {hash^=c;hash*=1099511628211ull;}
    std::ostringstream s;s<<std::hex<<std::setw(16)<<std::setfill('0')<<hash;return s.str();
}
void validate(const std::vector<uint8_t>& b,int lat,int lon) {
    FFTile tile(b);
    if (tile.header().latIndex!=lat || tile.header().lonIndex!=lon)
        throw std::runtime_error("fftile: HTTP service returned the wrong tile");
}
#ifdef FF_TILE_HTTP
size_t receive(char* p,size_t size,size_t count,void* context) {
    auto& b=*static_cast<std::vector<uint8_t>*>(context);
    constexpr size_t limit=64*1024*1024;
    if (size && count>(limit-b.size())/size) return 0;
    size_t n=size*count;
    try {b.insert(b.end(),reinterpret_cast<uint8_t*>(p),reinterpret_cast<uint8_t*>(p)+n);}
    catch (...) {return 0;}
    return n;
}
std::vector<uint8_t> download(const std::string& url,long& status) {
    // Function-local init avoids doing network-library setup in a DLL loader.
    static const CURLcode initialized=curl_global_init(CURL_GLOBAL_DEFAULT);
    if (initialized!=CURLE_OK) throw std::runtime_error("fftile: cannot initialize HTTP transport");
    std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> request(curl_easy_init(),curl_easy_cleanup);
    if (!request) throw std::runtime_error("fftile: cannot create HTTP request");
    std::vector<uint8_t> bytes;
    auto set=[&](CURLoption option,auto value) {
        if (curl_easy_setopt(request.get(),option,value)!=CURLE_OK)
            throw std::runtime_error("fftile: cannot configure HTTP request");
    };
    set(CURLOPT_URL,url.c_str());set(CURLOPT_NOSIGNAL,1L);
    set(CURLOPT_CONNECTTIMEOUT,10L);set(CURLOPT_TIMEOUT,30L);
    set(CURLOPT_FOLLOWLOCATION,1L);set(CURLOPT_MAXREDIRS,5L);
#if LIBCURL_VERSION_NUM >= 0x075500
    set(CURLOPT_PROTOCOLS_STR,"http,https");
    set(CURLOPT_REDIR_PROTOCOLS_STR,url.rfind("https://",0)==0 ? "https" : "http,https");
#else
    set(CURLOPT_PROTOCOLS,long(CURLPROTO_HTTP|CURLPROTO_HTTPS));
    set(CURLOPT_REDIR_PROTOCOLS,long(url.rfind("https://",0)==0 ? CURLPROTO_HTTPS : CURLPROTO_HTTP|CURLPROTO_HTTPS));
#endif
    set(CURLOPT_WRITEFUNCTION,&receive);set(CURLOPT_WRITEDATA,&bytes);
    set(CURLOPT_USERAGENT,"ForeFire-fftile/1");
    auto error=curl_easy_perform(request.get());
    if (error!=CURLE_OK) throw std::runtime_error(std::string("fftile: HTTP transfer failed: ")+curl_easy_strerror(error));
    if (curl_easy_getinfo(request.get(),CURLINFO_RESPONSE_CODE,&status)!=CURLE_OK)
        throw std::runtime_error("fftile: cannot read HTTP response status");
    if (status==404 || status==410) return {};
    if (status!=200) throw std::runtime_error("fftile: tile service returned HTTP "+std::to_string(status));
    return bytes;
}
void store(const std::filesystem::path& path,const std::vector<uint8_t>& bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::string pattern=path.string()+".part-XXXXXX";
    std::vector<char> temp(pattern.begin(),pattern.end());temp.push_back(0);
    int fd=mkstemp(temp.data());
    if (fd<0) throw std::runtime_error("fftile: cannot create HTTP cache temporary file");
    FILE* file=fdopen(fd,"wb");
    if (!file) {close(fd);std::remove(temp.data());throw std::runtime_error("fftile: cannot open HTTP cache temporary file");}
    bool ok=std::fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();
    ok=std::fclose(file)==0 && ok;
    try {
        if (!ok) throw std::runtime_error("fftile: cannot write HTTP tile cache");
        std::filesystem::rename(temp.data(),path);
    } catch (...) {std::remove(temp.data());throw;}

}
#endif
}
bool fftileIsHTTP(const std::string& url) {
    return url.rfind("http://",0)==0 || url.rfind("https://",0)==0;
}
bool fftileHTTPAvailable() {
#ifdef FF_TILE_HTTP
    return true;
#else
    return false;
#endif
}
FFTileHTTPProvider::FFTileHTTPProvider(std::string url,std::string local,std::string cache)
    : base_(std::move(url)),cacheDirectory_(std::move(cache)),local_(std::move(local),""),disk_("","") {
    while (!base_.empty() && base_.back()=='/') base_.pop_back();
    if (!fftileIsHTTP(base_) || base_.find_first_of("?#\r\n")!=std::string::npos)
        throw std::runtime_error("fftile: provider must be an HTTP(S) directory URL without query or fragment");
    if (!fftileHTTPAvailable()) throw std::runtime_error("fftile: HTTP tiles need a native build with libcurl and FOREFIRE_ENABLE_TILE_HTTP=ON");
    if (cacheDirectory_.empty()) throw std::runtime_error("fftile: HTTP tiles require a cache directory");
    cacheDirectory_=(std::filesystem::path(cacheDirectory_)/cacheNamespace(base_)).string();
    disk_=FFTileFileProvider("",cacheDirectory_);
}
std::vector<uint8_t> FFTileHTTPProvider::load(int lat,int lon) {
    auto local=local_.load(lat,lon);
    if (!local.empty()) return local;
    auto name=FFTileFileProvider::filename(lat,lon);
    auto path=std::filesystem::path(cacheDirectory_)/name;
    auto cached=disk_.load(lat,lon);
    if (!cached.empty()) {
        try {validate(cached,lat,lon);return cached;}
        catch (const std::exception&) {std::filesystem::remove(path);}
    }
    int key=lat*720+lon;
    if (absent_.count(key)) return {};
#ifdef FF_TILE_HTTP
    long status=0;
    auto b=download(base_+'/'+name,status);
    if (status==404 || status==410) {absent_.insert(key);return {};}
    validate(b,lat,lon);
    store(path,b);
    return b;
#else
    throw std::runtime_error("fftile: HTTP transport is disabled");
#endif
}
FFTileLoadOptions FFTileLoadOptions::parse(const std::string& arguments) {
    FFTileLoadOptions o;
    std::map<std::string,std::string> fields;
    std::istringstream in(arguments);std::string token;
    while (std::getline(in,token,';')) {
        token=trim(token);auto equals=token.find('=');
        if (fields.empty() && fftileIsHTTP(token)) {fields["provider"]=token;continue;}
        if (equals==std::string::npos) throw std::runtime_error("fftile: expected provider=...;center=(longitude,latitude)");
        auto key=trim(token.substr(0,equals)),value=trim(token.substr(equals+1));
        if (value.empty() || !fields.emplace(key,value).second) throw std::runtime_error("fftile: empty or duplicate loadData option");
    }
    for (const auto& field:fields)
        if (field.first!="provider" && field.first!="center" && field.first!="size" && field.first!="prefetch" && field.first!="date")
            throw std::runtime_error("fftile: unknown loadData option "+field.first);
    if (!fields.count("provider") || !fields.count("center")) throw std::runtime_error("fftile: provider and center are required");
    o.provider=fields.at("provider");auto center=pair(fields.at("center"));o.longitude=center[0];o.latitude=center[1];
    if (fields.count("size")) {auto size=pair(fields.at("size"));o.width=size[0];o.height=size[1];}
    if (o.longitude < -180 || o.longitude >= 180 || o.width<=0 || o.width>2 || o.height<=0 || o.height>2 ||
        o.latitude-o.height/2<=-90 || o.latitude+o.height/2>=90)
        throw std::runtime_error("fftile: invalid center or size (maximum 2 by 2 degrees; domain must not cross a pole)");
    if (fields.count("prefetch")) {
        if (fields.at("prefetch")!="0" && fields.at("prefetch")!="1") throw std::runtime_error("fftile: prefetch must be 0 or 1");
        o.prefetch=fields.at("prefetch")=="1";
    }
    if (fields.count("date")) o.date=fields.at("date");
    return o;
}
FFTileManager::PrefetchResult FFTileManager::prefetch(double x0,double y0,double x1,double y1) {
    if (!(x0<x1 && y0<y1)) throw std::runtime_error("fftile: invalid prefetch domain");
    double south=coordinates.geographic(x0,y0)[0],north=coordinates.geographic(x0,y1)[0];
    if (!std::isfinite(south) || !std::isfinite(north) || south<=-90 || north>=90) throw std::runtime_error("fftile: prefetch domain crosses a pole");
    double west=std::numeric_limits<double>::infinity(),east=-west;
    for (double x:{x0,x1}) for (double y:{y0,y1}) {
        auto ll=coordinates.geographic(x,y);
        double lon=coordinates.longitude+std::remainder(ll[1]-coordinates.longitude,360.);
        west=std::min(west,lon);east=std::max(east,lon);
    }
    if (!std::isfinite(west) || !std::isfinite(east)) throw std::runtime_error("fftile: nonfinite prefetch bounds");
    int firstLat=int(std::floor((south+90)*2)),lastLat=int(std::ceil((north+90)*2))-1;
    int firstLon=int(std::floor((west+180)*2)),lastLon=int(std::ceil((east+180)*2))-1;
    PrefetchResult result;
    for (int lat=firstLat;lat<=lastLat;++lat) for (int lon=firstLon;lon<=lastLon;++lon) {
        int wrapped=(lon%720+720)%720;
        if (provider_->load(lat,wrapped).empty()) ++result.missing; else ++result.available;
    }
    return result;
}
}
