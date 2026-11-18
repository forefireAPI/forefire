/** .fftile reader and provider boundary. No NetCDF, projection or HTTP dependency. */
#ifndef FOREFIRE_FFTILE_H
#define FOREFIRE_FFTILE_H
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace libforefire {
struct FFTileHeader {
    int latIndex = 0, lonIndex = 0;
    bool altitude16 = false;
    std::array<double, 4> core{}, image{};
    std::array<unsigned, 8> dimensions{};
    std::array<float, 14> ranges{};
    std::array<uint32_t, 8> payloads{};
    static FFTileHeader parse(const std::vector<uint8_t>&);
};
enum class FFTileField { Fuel, Altitude, SlopeX, SlopeY, Muu, Muv, Mvu, Mvv };
class FFTile {
    FFTileHeader header_;
    std::array<std::vector<uint8_t>, 4> images_;
    std::vector<uint16_t> altitude16_;
    std::vector<uint8_t> fuelOverride_;
public:
    explicit FFTile(const std::vector<uint8_t>& bytes);
    const FFTileHeader& header() const { return header_; }
    void overrideFuel(const std::vector<uint8_t>& png);
    const std::vector<uint8_t>& fuelOverride() const { return fuelOverride_; }
    double sample(FFTileField, double lat, double lon) const;
};
// A provider returns an empty buffer for unavailable coverage. Invalid content
// throws: corruption must not silently become nonburnable terrain.
class FFTileProvider {
public:
    virtual ~FFTileProvider() = default;
    virtual std::vector<uint8_t> load(int latIndex, int lonIndex) = 0;
    virtual std::vector<uint8_t> fuelOverride(int, int) { return {}; }
    virtual void refresh() {}
};
class FFTileFileProvider : public FFTileProvider {
    std::string local_, global_;
public:
    FFTileFileProvider(std::string localDirectory, std::string globalDirectory);
    static std::string filename(int latIndex, int lonIndex);
    std::vector<uint8_t> load(int, int) override;
    std::vector<uint8_t> fuelOverride(int, int) override;
};
// Native HTTP transport is optional; the format reader itself remains independent.
bool fftileHTTPAvailable();
bool fftileIsHTTP(const std::string&);
class FFTileHTTPProvider : public FFTileProvider {
    std::string base_, cacheDirectory_;
    FFTileFileProvider local_, disk_;
    std::set<int> absent_;
public:
    FFTileHTTPProvider(std::string baseURL, std::string localDirectory, std::string cacheDirectory);
    std::vector<uint8_t> load(int, int) override;
    std::vector<uint8_t> fuelOverride(int lat, int lon) override { return local_.fuelOverride(lat,lon); }
    void refresh() override { absent_.clear(); }
};
struct FFTileLoadOptions {
    std::string provider, date;
    double longitude = 0, latitude = 0, width = 2, height = 2;
    bool prefetch = true;
    static FFTileLoadOptions parse(const std::string& arguments);
};
// Meso-NH mass-point geography, with Cartesian axes in the parent-domain
// frame. Arrays use Fortran ordering (x varies fastest), including halos.
class FFTileGeographicGrid {
    friend class FFState;
    std::vector<double> xs_, ys_, lat_, lon_;
public:
    FFTileGeographicGrid(int nx, const double* xs, int ny, const double* ys,
                        const double* latitude, const double* longitude);
    std::array<double,2> geographic(double x, double y) const;
    std::array<double,2> cartesian(double latitude, double longitude) const;
};
struct FFTileCoordinates {
    double latitude = 0, longitude = 0, x = 0, y = 0;
    std::shared_ptr<const FFTileGeographicGrid> grid = nullptr;
    std::array<double, 2> geographic(double px, double py) const; // lat, lon
    std::array<double, 2> cartesian(double lat, double lon) const; // x, y
};
class FFTileManager {
    friend class FFState;
    struct Entry { std::shared_ptr<FFTile> tile; uint64_t touched; };
    std::shared_ptr<FFTileProvider> provider_;
    std::map<int, Entry> cache_;
    std::map<int,std::array<uint8_t,32>> sources_;
    std::map<int,std::vector<uint8_t>> fuelOverrides_;
    struct FuelPolygon { int fuel; std::vector<std::array<double,2>> points; };
    std::vector<FuelPolygon> modifiers_;
    size_t capacity_;
    uint64_t clock_ = 0;
public:
    FFTileCoordinates coordinates;
    double windU = 0, windV = 0;
    explicit FFTileManager(std::shared_ptr<FFTileProvider>, size_t capacity = 8);
    std::shared_ptr<FFTile> tileAt(double latitude, double longitude);
    double sample(FFTileField, double x, double y);
    void addFuelPolygon(int fuel, const std::string& longitudeLatitudePairs);
    std::string fuelPolygons() const;
    void restoreFuelPolygons(const std::string&);
    struct PrefetchResult { size_t available = 0, missing = 0; };
    PrefetchResult prefetch(double westX, double southY, double eastX, double northY);
    size_t cachedTiles() const { return cache_.size(); }
    void clearCache() { cache_.clear(); provider_->refresh(); }
};
}
#endif
