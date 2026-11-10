#include "doctest/doctest.h"
#include "fftile.h"
#include "FFTileDataLayer.h"
#include "model_sandbox.h"
#include "FireDomain.h"
#include "Command.h"
#include "TimeTable.h"
#include "stb_image_write.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cstring>
#include <limits>

using namespace libforefire;
namespace {
using Bytes = std::vector<uint8_t>;
void uintAt(Bytes& out, size_t at, uint64_t v, size_t n) {
    for (size_t i=0;i<n;++i) out[at+i] = uint8_t(v>>(8*i));
}
template<class T> void realAt(Bytes& out, size_t at, T value) {
    uint64_t bits=0;
    if constexpr(sizeof(T)==4) {uint32_t u;std::memcpy(&u,&value,4);bits=u;}
    else std::memcpy(&bits,&value,8);
    uintAt(out,at,bits,sizeof(T));
}
Bytes image(int w,int h,const Bytes& data,bool png) {
    // Pillow-generated L-mode fixtures: stb_image_write emits RGB JPEG
    // even for a one-channel input, which is outside the tile contract.
    if (!png) {
        if (w==8 && h==8) return Bytes{255,216,255,224,0,16,74,70,73,70,0,1,1,0,0,1,0,1,0,0,255,219,0,67,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,255,192,0,11,8,0,8,0,8,1,1,17,0,255,196,0,31,0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,255,196,0,181,16,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125,1,2,3,0,4,17,5,18,33,49,65,6,19,81,97,7,34,113,20,50,129,145,161,8,35,66,177,193,21,82,209,240,36,51,98,114,130,9,10,22,23,24,25,26,37,38,39,40,41,42,52,53,54,55,56,57,58,67,68,69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,103,104,105,106,115,116,117,118,119,120,121,122,131,132,133,134,135,136,137,138,146,147,148,149,150,151,152,153,154,162,163,164,165,166,167,168,169,170,178,179,180,181,182,183,184,185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,214,215,216,217,218,225,226,227,228,229,230,231,232,233,234,241,242,243,244,245,246,247,248,249,250,255,218,0,8,1,1,0,0,63,0,43,255,217};
        if (w==16 && h==8) return Bytes{255,216,255,224,0,16,74,70,73,70,0,1,1,0,0,1,0,1,0,0,255,219,0,67,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,255,192,0,11,8,0,8,0,16,1,1,17,0,255,196,0,31,0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,255,196,0,181,16,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125,1,2,3,0,4,17,5,18,33,49,65,6,19,81,97,7,34,113,20,50,129,145,161,8,35,66,177,193,21,82,209,240,36,51,98,114,130,9,10,22,23,24,25,26,37,38,39,40,41,42,52,53,54,55,56,57,58,67,68,69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,103,104,105,106,115,116,117,118,119,120,121,122,131,132,133,134,135,136,137,138,146,147,148,149,150,151,152,153,154,162,163,164,165,166,167,168,169,170,178,179,180,181,182,183,184,185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,214,215,216,217,218,225,226,227,228,229,230,231,232,233,234,241,242,243,244,245,246,247,248,249,250,255,218,0,8,1,1,0,0,63,0,254,127,235,253,0,43,255,217};
        if (w==16 && h==16) return Bytes{255,216,255,224,0,16,74,70,73,70,0,1,1,0,0,1,0,1,0,0,255,219,0,67,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,255,192,0,11,8,0,16,0,16,1,1,17,0,255,196,0,31,0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,255,196,0,181,16,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125,1,2,3,0,4,17,5,18,33,49,65,6,19,81,97,7,34,113,20,50,129,145,161,8,35,66,177,193,21,82,209,240,36,51,98,114,130,9,10,22,23,24,25,26,37,38,39,40,41,42,52,53,54,55,56,57,58,67,68,69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,103,104,105,106,115,116,117,118,119,120,121,122,131,132,133,134,135,136,137,138,146,147,148,149,150,151,152,153,154,162,163,164,165,166,167,168,169,170,178,179,180,181,182,183,184,185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,214,215,216,217,218,225,226,227,228,229,230,231,232,233,234,241,242,243,244,245,246,247,248,249,250,255,218,0,8,1,1,0,0,63,0,254,127,235,244,2,191,64,43,250,0,175,255,217};
        throw std::runtime_error("unknown grayscale JPEG fixture");
    }
    Bytes result;
    auto write=[](void* c,void* data,int n){
        auto& b=*static_cast<Bytes*>(c);auto* p=static_cast<uint8_t*>(data);
        b.insert(b.end(),p,p+n);
    };
    int ok=png?stbi_write_png_to_func(write,&result,w,h,1,data.data(),w):
               stbi_write_jpg_to_func(write,&result,w,h,1,data.data(),100);
    if (!ok) throw std::runtime_error("test fixture encoding failed");
    return result;
}
Bytes tileBytes(int li=264,int lj=376,int fuel=1,bool altitude16=false) {
    Bytes out(256,0);
    std::memcpy(out.data(),"FFTILE01",8);
    uintAt(out,8,li,2);uintAt(out,10,lj,2);uintAt(out,12,altitude16?3:1,2);
    double w=lj*.5-180,s=li*.5-90;
    double bounds[]={w,s,w+.5,s+.5,w-.01,std::max(-90.,s-.01),w+.51,std::min(90.,s+.51)};
    for (size_t i=0;i<8;++i) realAt(out,16+8*i,bounds[i]);
    for (size_t i=0;i<8;++i) uintAt(out,88+2*i,8,2);
    float ranges[]={-100,155,-1,1,-1,1,0,2,0,2,0,2,0,2};
    for(size_t i=0;i<14;++i) realAt(out,104+4*i,ranges[i]);
    Bytes fuels(64,uint8_t(fuel));
    if (fuel<0) {std::fill(fuels.begin(),fuels.begin()+32,2);std::fill(fuels.begin()+32,fuels.end(),3);}
    Bytes slopes(16*8),wind(16*16);
    for(int y=0;y<8;++y) for(int x=0;x<16;++x) slopes[y*16+x]=x<8?64:192;
    for(int y=0;y<16;++y) for(int x=0;x<16;++x) wind[y*16+x]=y<8?(x<8?64:96):(x<8?128:192);
    std::array<Bytes,4> images={image(8,8,fuels,true),(altitude16?Bytes{137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,8,0,0,0,8,16,0,0,0,0,177,244,61,20,0,0,0,40,73,68,65,84,120,156,99,96,64,3,140,42,147,208,4,60,85,80,5,88,208,85,176,168,162,107,217,118,11,77,224,118,14,154,192,255,255,168,2,0,130,200,7,115,166,75,247,43,0,0,0,0,73,69,78,68,174,66,96,130}:image(8,8,Bytes(64,128),false)),
                                image(16,8,slopes,false),image(16,16,wind,false)};
    for(size_t i=0;i<4;++i) {
        uintAt(out,160+i*8,out.size(),4);uintAt(out,164+i*8,images[i].size(),4);
        out.insert(out.end(),images[i].begin(),images[i].end());
    }
    return out;
}
struct Provider : FFTileProvider {
    int calls=0; bool override=false;
    std::vector<std::pair<int,int>> requested;
    Bytes load(int li,int lj) override {
        ++calls; requested.emplace_back(li,lj);
        if (lj==380) return {};
        return tileBytes(li,lj);
    }
    Bytes fuelOverride(int,int) override {return override?image(8,8,Bytes(64,0),true):Bytes{};}
};
struct Directory {
    std::filesystem::path path = std::filesystem::temp_directory_path()/
        ("forefire-fftile-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory(){std::filesystem::create_directories(path);}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
    void write(const std::string& name,const Bytes& b) {
        auto p=path/name;std::filesystem::create_directories(p.parent_path());
        std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));
    }
};
}
TEST_SUITE("fftile") {
TEST_CASE("south origin, signed fields and atlas components survive decoding") {
    FFTile tile(tileBytes(264,376,-1));
    CHECK(tile.sample(FFTileField::Fuel,42.1,8.25)==2);
    CHECK(tile.sample(FFTileField::Fuel,42.4,8.25)==3);
    CHECK(tile.sample(FFTileField::Altitude,42.2,8.2)==doctest::Approx(28).epsilon(.01));
    CHECK(tile.sample(FFTileField::SlopeX,42.2,8.2)==doctest::Approx(-1+64.*2/255).epsilon(.01));
    CHECK(tile.sample(FFTileField::SlopeY,42.2,8.2)==doctest::Approx(-1+192.*2/255).epsilon(.01));
    for(int i=0;i<4;++i) {
        int q[]={64,96,128,192};
        CHECK(tile.sample(FFTileField(4+i),42.2,8.2)==doctest::Approx(q[i]*2./255).epsilon(.01));
    }
}
TEST_CASE("16-bit altitude preserves its south-to-north order and precision") {
    FFTile t(tileBytes(264,376,1,true));
    CHECK(t.sample(FFTileField::Altitude,42.0225,8.25)==doctest::Approx(-100));
    CHECK(t.sample(FFTileField::Altitude,42.4775,8.25)==doctest::Approx(155));
    CHECK(t.header().altitude16);
}
TEST_CASE("malformed headers and payloads are rejected before sampling") {
    auto b=tileBytes();
    SUBCASE("magic"){b[0]='X';}
    SUBCASE("unsupported flags"){b[12]=8;}
    SUBCASE("CRC"){b[200]=1;}
    SUBCASE("overlapping payload"){uintAt(b,160,255,4);}
    SUBCASE("truncated payload"){b.resize(b.size()-1);}
    SUBCASE("oversized decoded image"){b[88]=255;b[89]=255;}
    SUBCASE("bounds mismatch"){realAt(b,16,0.);}
    SUBCASE("image dimensions"){b[88]=9;}
    SUBCASE("nonfinite range"){realAt(b,104,std::numeric_limits<float>::infinity());}
    CHECK_THROWS_AS(FFTile{b},std::runtime_error);
}
TEST_CASE("provider paging caches unavailable coverage and bounds memory") {
    auto p=std::make_shared<Provider>();
    FFTileManager m(p,2);m.coordinates={42,8,100,200};
    auto xy=m.coordinates.cartesian(42.2,8.2);
    CHECK(m.cachedTiles()==0);
    CHECK(m.sample(FFTileField::Fuel,xy[0],xy[1])==1);
    CHECK(p->calls==1);
    CHECK(m.sample(FFTileField::Fuel,xy[0],xy[1])==1);
    CHECK(p->calls==1);
    CHECK_FALSE(m.tileAt(42.2,10.2));
    CHECK_FALSE(m.tileAt(42.2,10.2));
    CHECK(p->calls==2);
    CHECK(m.tileAt(42.2,8.7));
    CHECK(m.cachedTiles()==2);
    CHECK(m.tileAt(42.2,8.2));
    CHECK(p->calls==4);
}
TEST_CASE("a local override cannot leak across a core boundary through a halo") {
    auto p=std::make_shared<Provider>();p->override=true;
    FFTileManager m(p);
    REQUIRE(m.tileAt(42.2,8.499));
    REQUIRE(m.tileAt(42.2,8.501));
    REQUIRE(p->requested.size()==2);
    CHECK(p->requested[0].second==376);
    CHECK(p->requested[1].second==377);
    CHECK(m.tileAt(42.2,8.2)->sample(FFTileField::Fuel,42.2,8.2)==0);
}
TEST_CASE("geographic transforms round trip offsets, hemispheres and date line") {
    for (auto c : {FFTileCoordinates{42,8,1000,2000},FFTileCoordinates{-42,-8,-1000,500},
                   FFTileCoordinates{0,179.9,0,0}}) {
        for (auto xy : {std::array<double,2>{10000,20000},std::array<double,2>{-20000,-10000}}) {
            auto ll=c.geographic(xy[0],xy[1]);auto back=c.cartesian(ll[0],ll[1]);
            CHECK(back[0]==doctest::Approx(xy[0]));CHECK(back[1]==doctest::Approx(xy[1]));
        }
    }
    FFTile tile(tileBytes(180,0));
    CHECK(tile.sample(FFTileField::Fuel,.1,179.995)==1);
    CHECK(FFTileFileProvider::filename(89,355)=="-45.5_-2.5.fftile");
}
TEST_CASE("local files precede global tiles and fuel PNG overrides precede both") {
    Directory global,local;
    global.write("42_8.fftile",tileBytes(264,376,2));
    auto p=std::make_shared<FFTileFileProvider>(local.path.string(),global.path.string());
    FFTileManager m(p);
    CHECK(m.tileAt(42.1,8.1)->sample(FFTileField::Fuel,42.1,8.1)==2);
    local.write("42_8.fftile",tileBytes(264,376,3));m.clearCache();
    CHECK(m.tileAt(42.1,8.1)->sample(FFTileField::Fuel,42.1,8.1)==3);
    local.write("42_8_fuel.png",image(8,8,Bytes(64,0),true));m.clearCache();
    CHECK(m.tileAt(42.1,8.1)->sample(FFTileField::Fuel,42.1,8.1)==0);
}
TEST_CASE("broker integrates tile fuel properties, wind and zero spread on no data") {
    Directory data;
    data.write("42_8.fftile",tileBytes());
    ff_test::ModelSandbox sandbox;
    auto* d=sandbox.fireDomain();
    sandbox.parameters()->setParameter("caseDirectory",data.path.string());
    sandbox.parameters()->setParameter("ForeFireDataDirectory","");
    d->getDataBroker()->loadFFTiles(data.path.string());
    auto manager=d->getDataBroker()->getFFTileManager();
    manager->coordinates={42,8,0,0};manager->windU=10;manager->windV=5;
    FFPoint p(500,500,0);
    CHECK(d->getDataLayer("fuel")->getValueAt(p,0)==1);
    CHECK(d->getDataLayer("windU")->getValueAt(p,0)==doctest::Approx((64*10+96*5)*2./255).epsilon(.01));
    auto* model=sandbox.propagation("Rothermel");REQUIRE(model!=nullptr);
    // Missing tile must return zero speed even for a propagation model that
    // does not request fuel properties at all.
    TimeTable schedule;d->setTimeTable(&schedule);
    auto* front=d->addFireFront(0.);
    FFVector velocity(1,0);
    auto* fn=d->addFireNode(p,velocity,0.,20.,0.,front,nullptr);
    REQUIRE(model->numFuelProperties > 0);
    CHECK(d->getDataLayer("fuel")->getValuesAt(fn,model,0)==model->numFuelProperties);
    for (size_t i=0;i<model->numFuelProperties;++i)
        CHECK(model->properties[i]==(*model->fuelPropertiesTable)(1,i));
    manager->coordinates={0,0,0,0};
    CHECK(d->getDataLayer("fuel")->getValueAt(fn)==0);
    CHECK(d->getPropagationSpeed(fn)==0);
    CHECK(d->getLatFromY(d->getYFromLat(.2))==doctest::Approx(.2));
    double y=d->getYFromLat(.2);
    CHECK(d->getLonFromX(d->getXFromLon(.3,y),y)==doctest::Approx(.3));
}
TEST_CASE("a short fire pages four tiles and continues around missing coverage") {
    Directory data;
    bool missing=false,edited=false;
    SUBCASE("four available tiles") {}
    SUBCASE("missing northeast tile") {missing=true;}
    SUBCASE("case-local northeast firebreak") {edited=true;}
    for (int li : {263,264}) for (int lj : {376,377}) {
        if (missing && li==264 && lj==377) continue;
        data.write(FFTileFileProvider::filename(li,lj),tileBytes(li,lj));
    }
    if (edited) data.write("tiles/42_8.5_fuel.png",image(8,8,Bytes(64,0),true));
    ff_test::ModelSandbox snapshot;
    auto run=[](std::string command){Command::ExecuteCommand(command);};
    struct ClearSession { ~ClearSession() {std::string command="clear[]";Command::ExecuteCommand(command);} } clear;
    run("clear[]");
    auto* p=snapshot.parameters();
    p->setParameter("caseDirectory",data.path.string());
    p->setParameter("ForeFireDataDirectory","");
    p->setParameter("fftilesPath",data.path.string());
    p->setParameter("SWLngLat","8.4995,41.9995");
    p->setParameter("propagationModel","Iso");p->setDouble("Iso.speed",1);
    p->setDouble("spatialIncrement",1);p->setDouble("perimeterResolution",2);
    p->setDouble("minimalPropagativeFrontDepth",0);p->setInt("bmapLayer",1);
    run("FireDomain[sw=(0,0,0);ne=(200,200,0);t=0]");
    auto* domain=Command::getDomain();REQUIRE(domain!=nullptr);
    auto manager=domain->getDataBroker()->getFFTileManager();REQUIRE(manager);
    CHECK(manager->cachedTiles()==0);
    run("trigger[wind;vel=(10,5,0)]");
    CHECK(manager->windU==10);CHECK(manager->windV==5);
    run("startFire[loc=(20,20,0);t=0]");
    run("goTo[t=80]");
    CHECK(domain->getNumFN()>50);
    CHECK(manager->cachedTiles()==4);
    CHECK(manager->sample(FFTileField::Fuel,100,100)==(missing||edited?0:1));
    CHECK(manager->sample(FFTileField::Fuel,100,20)==1);
    CHECK(manager->sample(FFTileField::Fuel,20,100)==1);
}

TEST_CASE("service options require a center and a finite bounded footprint") {
    auto o=FFTileLoadOptions::parse("provider=https://tiles.example.org/base;center=(8.5,42)");
    CHECK(o.longitude==8.5);CHECK(o.latitude==42);CHECK(o.width==2);CHECK(o.height==2);CHECK(o.prefetch);
    o=FFTileLoadOptions::parse("http://localhost:1234/tiles;center=(-179.9,-42);size=(.5,1);prefetch=0");
    CHECK(o.width==.5);CHECK(o.height==1);CHECK_FALSE(o.prefetch);
    for (const char* invalid : {"provider=x", "provider=x;center=(8,42);size=(0,2)",
         "provider=x;center=(8,42);size=(3,2)", "provider=x;center=(0,89.5)",
         "provider=x;center=(nan,42)", "provider=x;center=(1,2,3)",
         "provider=x;center=(8,42);prefetch=maybe", "provider=x;provider=y;center=(8,42)",
         "provider=x;center=(8,42);unknown=1"})
        CHECK_THROWS(FFTileLoadOptions::parse(invalid));
}
TEST_CASE("loadData creates a fixed centered tile domain and prefetches compressed coverage") {
    Directory data;
    for (int li:{263,264}) for (int lj:{376,377})
        data.write(FFTileFileProvider::filename(li,lj),tileBytes(li,lj));
    ff_test::ModelSandbox snapshot;
    struct ClearSession { ~ClearSession(){std::string s="clear[]";Command::ExecuteCommand(s);} } clear;
    auto run=[](std::string s){Command::ExecuteCommand(s);};run("clear[]");
    snapshot.parameters()->setParameter("caseDirectory",data.path.string());
    snapshot.parameters()->setParameter("ForeFireDataDirectory","");
    run("loadData[provider="+data.path.string()+";center=(8.5,42);size=(.002,.002)]");
    auto* domain=Command::getDomain();REQUIRE(domain);
    auto manager=domain->getDataBroker()->getFFTileManager();REQUIRE(manager);
    CHECK(manager->cachedTiles()==0);
    double cx=(domain->getSWCorner().getX()+domain->getNECorner().getX())/2;
    double cy=(domain->getSWCorner().getY()+domain->getNECorner().getY())/2;
    CHECK(domain->getLonFromX(cx,cy)==doctest::Approx(8.5));
    CHECK(domain->getLatFromY(cy)==doctest::Approx(42));
    CHECK((domain->getNECorner().getY()-domain->getSWCorner().getY())==doctest::Approx(.002*111195));
    CHECK(manager->sample(FFTileField::Fuel,cx,cy)==1);
    run("loadData[provider="+data.path.string()+";center=(8,41);size=(.002,.002)]");
    CHECK(Command::getDomain()==domain);
    run("clear[]");
    run("loadData[provider="+data.path.string()+";center=(179.9,0);prefetch=0]");
    domain=Command::getDomain();REQUIRE(domain);
    CHECK((domain->getNECorner().getY()-domain->getSWCorner().getY())==doctest::Approx(2*111195));
    cx=(domain->getNECorner().getX()-domain->getSWCorner().getX())/2;cy=(domain->getNECorner().getY()-domain->getSWCorner().getY())/2;
    CHECK(domain->getLonFromX(cx,cy)==doctest::Approx(179.9));
    CHECK(domain->getLatFromY(cy)==doctest::Approx(0));
}
TEST_CASE("prefetch wraps longitude at the date line without decoding into the LRU") {
    auto p=std::make_shared<Provider>();FFTileManager m(p);
    m.coordinates={0,179.99,5000,5000};
    auto result=m.prefetch(0,0,10000,10000);
    CHECK(result.available==4);CHECK(result.missing==0);CHECK(m.cachedTiles()==0);
    bool east=false,west=false;
    for (auto index:p->requested) {east|=index.second==719;west|=index.second==0;}
    CHECK(east);CHECK(west);
}

}

#ifdef FF_TILE_HTTP
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
namespace {
struct TileReply {int status=200;Bytes body;std::string location;};
class TileServer {
    int fd_=-1,port_=0;
    std::thread worker_;
    std::atomic<bool> running_{true};
    std::map<std::string,TileReply> replies_;
public:
    std::atomic<int> requests{0};
    explicit TileServer(std::map<std::string,TileReply> replies):replies_(std::move(replies)) {
        fd_=socket(AF_INET,SOCK_STREAM,0);
        if (fd_<0) throw std::runtime_error("test HTTP socket failed");
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        if (::bind(fd_,reinterpret_cast<sockaddr*>(&address),sizeof(address))<0 || listen(fd_,8)<0) {
            close(fd_);throw std::runtime_error("test HTTP bind failed");
        }
        socklen_t n=sizeof(address);getsockname(fd_,reinterpret_cast<sockaddr*>(&address),&n);port_=ntohs(address.sin_port);
        worker_=std::thread([this]{
            while (running_) {
                int peer=accept(fd_,nullptr,nullptr);
                if (peer<0) break;
                std::string request;char buffer[1024];
                while (request.find("\r\n\r\n")==std::string::npos && request.size()<4096) {
                    auto n=recv(peer,buffer,sizeof(buffer),0);if(n<=0)break;request.append(buffer,size_t(n));
                }
                ++requests;
                auto end=request.find(' ',4);auto path=request.substr(4,end==std::string::npos?0:end-4);
                auto i=replies_.find(path);
                TileReply reply=i==replies_.end()?TileReply{404,{},{}}:i->second;
                std::string response="HTTP/1.1 "+std::to_string(reply.status)+" response\r\nConnection: close\r\nContent-Length: "+std::to_string(reply.body.size())+"\r\n";
                if(!reply.location.empty())response+="Location: "+reply.location+"\r\n";
                response+="\r\n";response.append(reply.body.begin(),reply.body.end());
                size_t sent=0;
                while(sent<response.size()) {auto n=send(peer,response.data()+sent,response.size()-sent,0);if(n<=0)break;sent+=size_t(n);}
                close(peer);
            }
        });
    }
    std::string url(const std::string& path="/tiles")const {return "http://127.0.0.1:"+std::to_string(port_)+path;}
    void stop(){if(fd_>=0){running_=false;shutdown(fd_,SHUT_RDWR);close(fd_);if(worker_.joinable())worker_.join();fd_=-1;}}
    ~TileServer(){stop();}
};
}
TEST_SUITE("fftile") {
TEST_CASE("HTTP tiles are cached for offline reuse and local overrides take priority") {
    Directory local,cache;
    TileServer server({{"/tiles/42_8.fftile",{200,tileBytes(),{}}}});
    auto p=std::make_shared<FFTileHTTPProvider>(server.url(),local.path.string(),cache.path.string());
    FFTileManager manager(p);
    REQUIRE(manager.tileAt(42.2,8.2));CHECK(server.requests==1);
    manager.clearCache();REQUIRE(manager.tileAt(42.2,8.2));CHECK(server.requests==1);
    auto url=server.url();server.stop();
    FFTileHTTPProvider offline(url,local.path.string(),cache.path.string());
    CHECK_FALSE(offline.load(264,376).empty());
    local.write("42_8.fftile",tileBytes(264,376,3));manager.clearCache();
    CHECK(manager.tileAt(42.2,8.2)->sample(FFTileField::Fuel,42.2,8.2)==3);
    local.write("42_8_fuel.png",image(8,8,Bytes(64,0),true));manager.clearCache();
    CHECK(manager.tileAt(42.2,8.2)->sample(FFTileField::Fuel,42.2,8.2)==0);
}
TEST_CASE("HTTP absent coverage is zero while server failures and corrupt tiles are errors") {
    Directory local,cache;
    TileServer server({{"/tiles/42_8.fftile",{404,{},{}}},
                       {"/tiles/42_8.5.fftile",{500,{},{}}},
                       {"/tiles/42_9.fftile",{200,Bytes{'b','a','d'},{}}},
                       {"/tiles/42_9.5.fftile",{200,tileBytes(),{}}},
                       {"/tiles/42_10.fftile",{410,{},{}}}});
    auto p=std::make_shared<FFTileHTTPProvider>(server.url(),local.path.string(),cache.path.string());
    FFTileManager m(p,1);
    CHECK_FALSE(m.tileAt(42.1,8.1));CHECK_FALSE(m.tileAt(42.1,8.1));CHECK(server.requests==1);
    CHECK_FALSE(m.tileAt(42.1,10.1));CHECK_FALSE(m.tileAt(42.1,8.1));CHECK(server.requests==2);
    m.clearCache();CHECK_FALSE(m.tileAt(42.1,8.1));CHECK(server.requests==3);
    CHECK_THROWS(m.tileAt(42.1,8.6));CHECK_THROWS(m.tileAt(42.1,9.1));CHECK_THROWS(m.tileAt(42.1,9.6));
    CHECK_THROWS(m.tileAt(42.1,9.1));CHECK(server.requests==7);
}
TEST_CASE("loadData registers HTTP service and prefetches without filling the decoded cache") {
    Directory data;
    std::map<std::string,TileReply> replies;
    for(int lat:{263,264})for(int lon:{376,377})
        replies["/tiles/"+FFTileFileProvider::filename(lat,lon)]={200,tileBytes(lat,lon),{}};
    TileServer server(replies);
    ff_test::ModelSandbox snapshot;
    struct ClearSession {~ClearSession(){std::string s="clear[]";Command::ExecuteCommand(s);}} clear;
    auto run=[](std::string s){Command::ExecuteCommand(s);};run("clear[]");
    snapshot.parameters()->setParameter("caseDirectory",data.path.string());
    snapshot.parameters()->setParameter("ForeFireDataDirectory","");
    run("loadData[provider="+server.url()+";center=(8.5,42);size=(.002,.002)]");
    auto* domain=Command::getDomain();REQUIRE(domain);
    auto manager=domain->getDataBroker()->getFFTileManager();REQUIRE(manager);
    CHECK(server.requests==4);CHECK(manager->cachedTiles()==0);
    CHECK(manager->sample(FFTileField::Fuel,100,100)==1);CHECK(server.requests==4);
}
TEST_CASE("HTTP redirects work and disk caches are isolated per service") {
    Directory local,cache;
    TileServer first({{"/redirect/42_8.fftile",{302,{},"/tiles/42_8.fftile"}},
                      {"/tiles/42_8.fftile",{200,tileBytes(264,376,1),{}}}});
    TileServer second({{"/tiles/42_8.fftile",{200,tileBytes(264,376,2),{}}}});
    FFTileHTTPProvider a(first.url("/redirect"),local.path.string(),cache.path.string());
    FFTileHTTPProvider b(second.url(),local.path.string(),cache.path.string());
    CHECK(FFTile(a.load(264,376)).sample(FFTileField::Fuel,42.1,8.1)==1);
    CHECK(FFTile(b.load(264,376)).sample(FFTileField::Fuel,42.1,8.1)==2);
    CHECK(first.requests==2);CHECK(second.requests==1);
}
}
#endif
