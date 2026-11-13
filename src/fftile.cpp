#include "fftile.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 16384
#include "stb_image.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <cerrno>
#include <sys/stat.h>

namespace libforefire {
namespace {
constexpr double pi = 3.14159265358979323846, meters = 111195.;
uint64_t integer(const std::vector<uint8_t>& b, size_t at, size_t n) {
    uint64_t v = 0;
    for (size_t i = 0; i < n; ++i) v |= uint64_t(b.at(at + i)) << (8 * i);
    return v;
}
template<class T> T number(const std::vector<uint8_t>& b, size_t at) {
    auto bits = integer(b, at, sizeof(T));
    T value;
    if constexpr (sizeof(T) == 4) { uint32_t v = uint32_t(bits); std::memcpy(&value, &v, 4); }
    else { std::memcpy(&value, &bits, 8); }
    return value;
}
void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(std::string("fftile: ") + why);
}
uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = ~uint32_t(0);
    for (size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (int j = 0; j < 8; ++j) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1)));
    }
    return ~c;
}
std::vector<uint8_t> decode(const uint8_t* p, size_t n, unsigned w, unsigned h) {
    require(n <= size_t(std::numeric_limits<int>::max()), "image too large");
    int x = 0, y = 0, c = 0;
    require(stbi_info_from_memory(p, int(n), &x, &y, &c) != 0, "invalid image");
    require(x == int(w) && y == int(h) && c == 1, "image dimensions/channels disagree with header");
    require(!stbi_is_16_bit_from_memory(p,int(n)), "expected an 8-bit image");
    auto* image = stbi_load_from_memory(p, int(n), &x, &y, &c, 1);
    require(image != nullptr, "cannot decode image");
    std::vector<uint8_t> out(image, image + size_t(w) * h);
    stbi_image_free(image);
    return out;
}
std::vector<uint8_t> read(const std::string& name) {
    if (name.empty()) return {};
    struct stat info;
    if (stat(name.c_str(), &info) != 0) {
        if (errno == ENOENT || errno == ENOTDIR) return {};
        throw std::runtime_error("fftile: cannot inspect " + name);
    }
    std::ifstream in(name, std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("fftile: cannot read " + name);
    auto size = in.tellg();
    require(size > 0 && size <= 64 * 1024 * 1024, "invalid file size");
    std::vector<uint8_t> b(static_cast<size_t>(size));
    in.seekg(0);
    require(bool(in.read(reinterpret_cast<char*>(b.data()), size)), "truncated file");
    return b;
}
double wrap(double lon) { double v = std::remainder(lon,360.); return v == 180. ? -180. : v; }
}
FFTileHeader FFTileHeader::parse(const std::vector<uint8_t>& b) {
    require(b.size() >= 256 && std::memcmp(b.data(), "FFTILE01", 8) == 0, "invalid header or version");
    auto flags = integer(b, 12, 2);
    require((flags & 1) && !(flags & ~uint64_t(3)), "unsupported flags/orientation");
    auto crc = integer(b, 200, 4);
    require(!crc || crc == crc32(b.data(), 200), "header CRC mismatch");
    FFTileHeader h;
    h.altitude16 = (flags & 2) != 0;
    h.latIndex = int(integer(b, 8, 2)); h.lonIndex = int(integer(b, 10, 2));
    require(h.latIndex < 360 && h.lonIndex < 720, "invalid tile index");
    for (size_t i = 0; i < 4; ++i) {
        h.core[i] = number<double>(b, 16 + i * 8);
        h.image[i] = number<double>(b, 48 + i * 8);
        require(std::isfinite(h.core[i]) && std::isfinite(h.image[i]), "nonfinite bounds");
    }
    double south = h.latIndex * .5 - 90., west = h.lonIndex * .5 - 180.;
    require(std::abs(h.core[0] - west) < 1e-8 && std::abs(h.core[1] - south) < 1e-8 &&
            std::abs(h.core[2] - west - .5) < 1e-8 && std::abs(h.core[3] - south - .5) < 1e-8,
            "bounds disagree with tile index");
    require(h.image[0] < h.core[0] && h.image[1] <= h.core[1] &&
            h.image[2] > h.core[2] && h.image[3] >= h.core[3] &&
            h.image[1] >= -90 && h.image[3] <= 90, "invalid halo bounds");
    for (size_t i = 0; i < 8; ++i) {
        h.dimensions[i] = unsigned(integer(b, 88 + i * 2, 2));
        require(h.dimensions[i] > 0 && h.dimensions[i] <= 8192, "invalid dimensions");
        h.payloads[i] = uint32_t(integer(b, 160 + i * 4, 4));
    }
    size_t end = 256, pixels = 0;
    for (size_t i = 0; i < 4; ++i) {
        auto offset = h.payloads[2*i], size = h.payloads[2*i+1];
        require(offset == end && size > 0 && uint64_t(offset) + size <= b.size(), "invalid payload bounds");
        end = size_t(offset) + size;
        pixels += size_t(h.dimensions[2*i]) * h.dimensions[2*i+1] * (i == 2 ? 2 : i == 3 ? 4 : 1);
    }
    require(end == b.size() && pixels <= 128 * 1024 * 1024, "invalid total image size");
    for (size_t i = 0; i < 14; ++i) {
        h.ranges[i] = number<float>(b, 104 + 4*i);
        require(std::isfinite(h.ranges[i]), "nonfinite quantization range");
    }
    for (size_t i = 0; i < 14; i += 2) require(h.ranges[i+1] >= h.ranges[i], "reversed quantization range");
    return h;
}
FFTile::FFTile(const std::vector<uint8_t>& bytes) : header_(FFTileHeader::parse(bytes)) {
    for (size_t i = 0; i < 4; ++i) {
        unsigned w = header_.dimensions[2*i], h = header_.dimensions[2*i+1];
        if (i >= 2) w *= 2;
        if (i == 3) h *= 2;
        const auto* p = bytes.data() + header_.payloads[2*i];
        auto n = header_.payloads[2*i+1];
        require((i == 0 || (i == 1 && header_.altitude16)) ? (n >= 8 && p[0] == 137 && p[1] == 80 && p[2] == 78 && p[3] == 71)
                       : (n >= 2 && p[0] == 255 && p[1] == 216), "incorrect payload codec");
        if (i == 1 && header_.altitude16) {
            int x, y, c;
            require(stbi_info_from_memory(p,int(n),&x,&y,&c) && x == int(w) && y == int(h) && c == 1 &&
                    stbi_is_16_bit_from_memory(p,int(n)), "invalid 16-bit altitude image");
            auto* image = stbi_load_16_from_memory(p,int(n),&x,&y,&c,1);
            require(image != nullptr, "cannot decode 16-bit altitude image");
            altitude16_.assign(image,image+size_t(w)*h);
            stbi_image_free(image);
        } else images_[i] = decode(p, n, w, h);
    }
}
void FFTile::overrideFuel(const std::vector<uint8_t>& png) {
    require(png.size() >= 8 && png[0] == 137 && png[1] == 80, "fuel override must be grayscale PNG");
    images_[0] = decode(png.data(), png.size(), header_.dimensions[0], header_.dimensions[1]);
}
double FFTile::sample(FFTileField field, double lat, double lon) const {
    const auto& h = header_;
    if (!std::isfinite(lat) || !std::isfinite(lon)) return 0;
    // The last tile east of the antimeridian uses unwrapped halo coordinates.
    lon += 360.*std::round(((h.core[0]+h.core[2])*.5-lon)/360.);
    if (lon < h.image[0] || lon > h.image[2] || lat < h.image[1] || lat > h.image[3]) return 0;
    double u = (lon-h.image[0])/(h.image[2]-h.image[0]);
    double v = (lat-h.image[1])/(h.image[3]-h.image[1]);
    int f = int(field);
    if (!f) {
        unsigned x = std::min(unsigned(u*h.dimensions[0]), h.dimensions[0]-1);
        unsigned y = std::min(unsigned(v*h.dimensions[1]), h.dimensions[1]-1);
        return images_[0][size_t(y)*h.dimensions[0]+x];
    }
    unsigned payload = f == 1 ? 1 : f <= 3 ? 2 : 3;
    unsigned w = h.dimensions[2*payload], height = h.dimensions[2*payload+1];
    unsigned component = f == 1 ? 0 : f <= 3 ? f-2 : f-4;
    unsigned ox = payload == 1 ? 0 : (component%2)*w;
    unsigned oy = payload == 3 ? (component/2)*height : 0;
    unsigned stride = payload >= 2 ? 2*w : w;
    double x = std::clamp(u*w-.5, 0., double(w-1));
    double y = std::clamp(v*height-.5, 0., double(height-1));
    unsigned x0 = unsigned(x), y0 = unsigned(y), x1 = std::min(x0+1,w-1), y1 = std::min(y0+1,height-1);
    double a = x-x0, b = y-y0;
    auto at = [&](unsigned c, unsigned r) -> double {
        auto pos = size_t(r+oy)*stride+c+ox;
        return payload == 1 && h.altitude16 ? altitude16_[pos] : images_[payload][pos];
    };
    double q = (1-b)*((1-a)*at(x0,y0)+a*at(x1,y0)) + b*((1-a)*at(x0,y1)+a*at(x1,y1));
    return h.ranges[2*(f-1)] + q*(double(h.ranges[2*(f-1)+1])-h.ranges[2*(f-1)])/(payload == 1 && h.altitude16 ? 65535. : 255.);
}
FFTileFileProvider::FFTileFileProvider(std::string local, std::string global)
    : local_(std::move(local)), global_(std::move(global)) {}
std::string FFTileFileProvider::filename(int lat, int lon) {
    std::ostringstream s; s << lat*.5-90 << '_' << lon*.5-180 << ".fftile"; return s.str();
}
std::vector<uint8_t> FFTileFileProvider::load(int lat, int lon) {
    auto name = filename(lat,lon);
    auto b = local_.empty() ? std::vector<uint8_t>{} : read(local_+'/'+name);
    if (b.empty() && !global_.empty()) b = read(global_+'/'+name);
    return b;
}
std::vector<uint8_t> FFTileFileProvider::fuelOverride(int lat, int lon) {
    auto name = filename(lat,lon); name.resize(name.size()-7);
    return local_.empty() ? std::vector<uint8_t>{} : read(local_+'/'+name+"_fuel.png");
}
FFTileGeographicGrid::FFTileGeographicGrid(int nx, const double* xs, int ny,
        const double* ys, const double* latitude, const double* longitude) {
    require(nx>=2 && ny>=2 && size_t(nx)<=16000000/size_t(ny), "invalid Meso-NH geographic grid dimensions");
    require(xs && ys && latitude && longitude, "missing Meso-NH geographic grid arrays");
    xs_.assign(xs,xs+nx); ys_.assign(ys,ys+ny);
    lat_.assign(latitude,latitude+size_t(nx)*ny); lon_.resize(lat_.size());
    for (const auto* axis : {&xs_,&ys_}) for(size_t i=0;i<axis->size();++i)
        require(std::isfinite((*axis)[i]) && (!i || (*axis)[i]>(*axis)[i-1]), "Meso-NH mass-point axes must increase");
    for(size_t i=0;i<lat_.size();++i) {
        require(std::isfinite(lat_[i]) && std::abs(lat_[i])<90 && std::isfinite(longitude[i]), "invalid Meso-NH latitude/longitude");
        lon_[i]=longitude[0]+wrap(longitude[i]-longitude[0]);
    }
}
std::array<double,2> FFTileGeographicGrid::geographic(double x, double y) const {
    require(std::isfinite(x) && std::isfinite(y), "nonfinite Meso-NH position");
    auto cell=[](const std::vector<double>& axis, double p) {
        // One-cell extrapolation covers the half-cell between mass points
        // and the atmospheric domain edges, without clamping their position.
        require(p>=axis.front()-(axis[1]-axis[0]) &&
                p<=axis.back()+(axis.back()-axis[axis.size()-2]), "position outside Meso-NH geographic grid");
        auto end=std::upper_bound(axis.begin(),axis.end(),p);
        return std::min(axis.size()-2,size_t(std::max(ptrdiff_t(0),end-axis.begin()-1)));
    };
    size_t i=cell(xs_,x),j=cell(ys_,y),nx=xs_.size(),p=i+j*nx;
    double u=(x-xs_[i])/(xs_[i+1]-xs_[i]),v=(y-ys_[j])/(ys_[j+1]-ys_[j]);
    auto value=[&](const std::vector<double>& a) {
        return (1-v)*((1-u)*a[p]+u*a[p+1])+v*((1-u)*a[p+nx]+u*a[p+nx+1]);
    };
    return {value(lat_),wrap(value(lon_))};
}
std::array<double,2> FFTileGeographicGrid::cartesian(double lat, double lon) const {
    require(std::isfinite(lat) && std::isfinite(lon), "nonfinite geographic position");
    // Geographic ignition is infrequent: find a nearby vertex, then invert
    // the same bilinear map. Tile sampling itself only does axis searches.
    size_t nearest=0;double best=std::numeric_limits<double>::infinity();
    double c=std::cos(lat*pi/180.);
    for(size_t i=0;i<lat_.size();++i) {
        double a=lat_[i]-lat,b=wrap(lon_[i]-lon)*c,d=a*a+b*b;
        if(d<best){best=d;nearest=i;}
    }
    double x=xs_[nearest%xs_.size()],y=ys_[nearest/xs_.size()];
    const double hx=(xs_[1]-xs_[0])*.001,hy=(ys_[1]-ys_[0])*.001;
    for(int iteration=0;iteration<12;++iteration) {
        auto q=geographic(x,y);double a=q[0]-lat,b=wrap(q[1]-lon);
        if(std::abs(a)<1e-11 && std::abs(b)<1e-11)return {x,y};
        auto qx=geographic(x+hx,y),qy=geographic(x,y+hy);
        double ax=(qx[0]-q[0])/hx,ay=(qy[0]-q[0])/hy;
        double bx=wrap(qx[1]-q[1])/hx,by=wrap(qy[1]-q[1])/hy,det=ax*by-ay*bx;
        require(std::isfinite(det) && std::abs(det)>1e-20,"singular Meso-NH geographic grid");
        x-=(a*by-b*ay)/det;y-=(b*ax-a*bx)/det;
    }
    throw std::runtime_error("fftile: cannot invert Meso-NH geographic grid at this position");
}
std::array<double, 2> FFTileCoordinates::geographic(double px, double py) const {
    if(grid)return grid->geographic(px,py);
    double lat = latitude + (py-y)/meters;
    double c = std::cos(lat*pi/180.);
    return {lat, std::abs(c) < 1e-12 ? longitude : wrap(longitude+(px-x)/(meters*c))};
}
std::array<double, 2> FFTileCoordinates::cartesian(double lat, double lon) const {
    if(grid)return grid->cartesian(lat,lon);
    return {x+wrap(lon-longitude)*meters*std::cos(lat*pi/180.), y+(lat-latitude)*meters};
}
FFTileManager::FFTileManager(std::shared_ptr<FFTileProvider> p, size_t capacity)
    : provider_(std::move(p)), capacity_(std::max(size_t(1),capacity)) { require(bool(provider_), "missing provider"); }
std::shared_ptr<FFTile> FFTileManager::tileAt(double lat, double lon) {
    if (!std::isfinite(lat) || !std::isfinite(lon) || lat < -90 || lat >= 90) return {};
    lon = wrap(lon);
    int li = int(std::floor((lat+90)*2)), lj = int(std::floor((lon+180)*2)), key = li*720+lj;
    auto i = cache_.find(key);
    if (i != cache_.end()) { i->second.touched = ++clock_; return i->second.tile; }
    auto bytes = provider_->load(li,lj);
    std::shared_ptr<FFTile> tile;
    if (!bytes.empty()) {
        tile = std::make_shared<FFTile>(bytes);
        require(tile->header().latIndex == li && tile->header().lonIndex == lj, "provider returned wrong tile");
        auto fuel = provider_->fuelOverride(li,lj);
        if (!fuel.empty()) tile->overrideFuel(fuel);
    }
    if (cache_.size() >= capacity_) {
        auto oldest = std::min_element(cache_.begin(),cache_.end(),[](const auto& a,const auto& b){return a.second.touched<b.second.touched;});
        cache_.erase(oldest);
    }
    cache_.emplace(key,Entry{tile,++clock_});
    return tile;
}
double FFTileManager::sample(FFTileField field, double x, double y) {
    auto ll = coordinates.geographic(x,y);
    auto tile = tileAt(ll[0],ll[1]);
    return tile ? tile->sample(field,ll[0],ll[1]) : 0.;
}
}
