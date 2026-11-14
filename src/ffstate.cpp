#include "ffstate.h"
#include "Command.h"
#include "FFTileDataLayer.h"
#include "include/SHA256.h"
#include "../bindings/wasm/GriddedWind.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <locale>
#include <set>
#include <unordered_map>
#ifndef FF_STATE_ENGINE_VERSION
#define FF_STATE_ENGINE_VERSION "unknown"
#endif

namespace libforefire {
namespace {
using Bytes=std::vector<uint8_t>;
constexpr uint64_t limit=512ull*1024*1024;
void require(bool ok,const std::string& why) {if(!ok)throw std::runtime_error("ffstate: "+why);}
uint32_t crc(const Bytes& b) {
    uint32_t c=~0u;
    for(auto v:b){c^=v;for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}
    return ~c;
}
void put(Bytes& b,uint64_t v,unsigned n=8){for(unsigned i=0;i<n;++i)b.push_back(uint8_t(v>>(8*i)));}
uint64_t get(const Bytes& b,size_t& at,unsigned n=8){
    require(at<=b.size()&&n<=b.size()-at,"truncated integer");uint64_t v=0;
    for(unsigned i=0;i<n;++i)v|=uint64_t(b[at++])<<(8*i);return v;
}
void putReal(Bytes& b,double x){uint64_t bits;static_assert(sizeof(x)==8,"float64 required");std::memcpy(&bits,&x,8);put(b,bits);}
double getReal(const Bytes& b,size_t& at){auto bits=get(b,at);double x;std::memcpy(&x,&bits,8);return x;}
Bytes bytes(const std::string& s){return Bytes(s.begin(),s.end());}
double real(std::istream& in,bool missing=false){
    std::string token;require(bool(in>>token)&&token.size()<80,"missing scalar");
    size_t used=0;double x=std::stod(token,&used);require(used==token.size()&&(missing||!std::isnan(x)),"invalid scalar");return x;
}
size_t count(std::istream& in,size_t maximum=1000000){uint64_t n=0;require(bool(in>>n)&&n<=maximum,"invalid record count");return size_t(n);}
std::string string(std::istream& in){std::string s;require(bool(in>>std::quoted(s))&&s.size()<16*1024*1024,"invalid string");return s;}
std::ostringstream output(){std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(17);return s;}
std::istringstream input(const Bytes& b){std::istringstream s(std::string(b.begin(),b.end()));s.imbue(std::locale::classic());return s;}
void end(std::istream& in){in>>std::ws;require(in.eof(),"unexpected trailing records");}
void point(std::ostream& out,FFPoint p){out<<p.getX()<<' '<<p.getY()<<' '<<p.getZ()<<' ';}
void vector(std::ostream& out,FFVector p){out<<p.getVx()<<' '<<p.getVy()<<' '<<p.getVz()<<' ';}
FFPoint point(std::istream& in){double x=real(in),y=real(in),z=real(in);require(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z),"nonfinite point");return FFPoint(x,y,z);}
FFVector vector(std::istream& in){auto p=point(in);return FFVector(p.getX(),p.getY(),p.getZ());}
void array(std::ostream& out,const std::vector<double>& v){out<<v.size()<<' ';for(double x:v)out<<x<<' ';out<<'\n';}
std::vector<double> array(std::istream& in,size_t maximum=4000000,bool missing=false){std::vector<double> v(count(in,maximum));for(auto& x:v)x=real(in,missing);return v;}
struct FrontRecord {long id;size_t parent;double time,update;bool expanding;};
struct NodeRecord {long id;size_t front,merge;double time,update,speed,depth,curvature;int state;FFPoint loc,next;FFVector velocity,normal;};
struct EventRecord {size_t kind,index;double time;bool input,output;};
struct HistoryRun {uint64_t first,length;double time;};
struct TileSource {std::array<uint8_t,32> hash;Bytes fuel;};
struct EmissionRecord {FFPoint center;double area,start,end,flux;};
struct FluxRecord {std::string name;double time;std::vector<double> cache;std::vector<EmissionRecord> emissions;};
class StateProvider : public FFTileProvider {
    std::shared_ptr<FFTileProvider> base;
    std::map<int,TileSource> sources;
public:
    StateProvider(std::shared_ptr<FFTileProvider> p,std::map<int,TileSource> records):base(std::move(p)),sources(std::move(records)){}
    Bytes load(int lat,int lon) override {
        auto i=sources.find(lat*720+lon);
        if(i!=sources.end()&&i->second.hash==sha256({}))return {};
        auto data=base->load(lat,lon);
        if(i!=sources.end())require(sha256(data)==i->second.hash,"tile content changed: "+FFTileFileProvider::filename(lat,lon));
        return data;
    }
    Bytes fuelOverride(int lat,int lon) override {
        auto i=sources.find(lat*720+lon);return i==sources.end()?base->fuelOverride(lat,lon):i->second.fuel;
    }
    void refresh() override {base->refresh();}
};
struct Snapshot {
    std::map<std::string,std::string> parameters;
    FFTileCoordinates frame;
    std::vector<double> xs,ys,lat,lon;
    FFPoint sw,ne;
    double clock=0,domainTime=0,domainUpdate=0,outputTime=0,outputUpdate=0,epoch=0,dx=0,dy=0,windU=0,windV=0;
    size_t nx=0,ny=0,subdivisions=0,nextID=0;
    std::array<double,4> geoBounds{};
    std::vector<FrontRecord> fronts;
    std::vector<NodeRecord> nodes;
    std::vector<EventRecord> events;
    std::vector<HistoryRun> history;
    std::shared_ptr<GriddedWind> wind;
    std::map<int,TileSource> tiles;
    std::vector<FluxRecord> fluxes;
    std::vector<double> runtime;
};
bool leap(int y){return y%4==0&&(y%100!=0||y%400==0);}
double epoch(int year,int day,double seconds){
    require(year>=1900&&year<=2500&&day>=1&&day<=365+int(leap(year))&&std::isfinite(seconds),"reference date is required (refYear/refDay/refTime)");
    int days=0;for(int y=1970;y<year;++y)days+=365+int(leap(y));for(int y=year;y<1970;++y)days-=365+int(leap(y));
    return (days+day-1)*86400.+seconds;
}
std::map<std::string,Bytes> read(const std::string& filename){
    std::ifstream f(filename,std::ios::binary|std::ios::ate);require(bool(f),"cannot open "+filename);
    auto length=f.tellg();require(length>=16&&uint64_t(length)<=limit,"invalid file size");
    Bytes b(size_t(length),0);f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),b.size());require(bool(f),"cannot read checkpoint");
    require(std::memcmp(b.data(),"FFSTATE1",8)==0,"invalid magic");size_t at=8;
    require(get(b,at,4)==1,"unsupported format version");auto n=get(b,at,4);require(n>0&&n<=32,"invalid chunk count");
    std::map<std::string,Bytes> chunks;
    for(size_t i=0;i<n;++i){
        auto namesize=get(b,at,4),size=get(b,at),checksum=get(b,at,4);
        require(namesize>0&&namesize<=128&&at<=b.size()&&namesize<=b.size()-at,"invalid chunk name");
        std::string name(b.begin()+at,b.begin()+at+namesize);at+=namesize;
        require(size<=limit&&size<=b.size()-at&&!chunks.count(name),"invalid or duplicate chunk");
        Bytes data(b.begin()+at,b.begin()+at+size);at+=size;require(crc(data)==checksum,"checksum mismatch: "+name);chunks.emplace(name,std::move(data));
    }
    require(at==b.size(),"trailing bytes");
    for(auto name:{"domain","parameters","objects.ff","events","arrival"})require(chunks.count(name),std::string("missing ")+name);
    return chunks;
}
Snapshot decode(const std::map<std::string,Bytes>& chunks){
    Snapshot s;
    auto p=input(chunks.at("parameters"));auto n=count(p,10000);
    for(size_t i=0;i<n;++i){auto k=string(p),v=string(p);require(!s.parameters.count(k),"duplicate parameter");s.parameters.emplace(k,v);}end(p);
    auto d=input(chunks.at("domain"));s.clock=real(d);s.domainTime=real(d);s.domainUpdate=real(d);s.epoch=real(d);
    s.outputTime=real(d);s.outputUpdate=real(d);s.nextID=count(d,(1ull<<40)-1);s.sw=point(d);s.ne=point(d);
    s.frame.latitude=real(d);s.frame.longitude=real(d);s.frame.x=real(d);s.frame.y=real(d);
    s.nx=count(d,32768);s.ny=count(d,32768);s.dx=real(d);s.dy=real(d);s.subdivisions=count(d,512);
    for(auto& b:s.geoBounds)b=real(d);s.windU=real(d);s.windV=real(d);
    s.xs=array(d);s.ys=array(d);s.lat=array(d);s.lon=array(d);end(d);
    require(std::isfinite(s.clock)&&std::isfinite(s.epoch)&&s.nx&&s.ny&&s.nx<=256*1024*1024/s.ny&&s.dx>0&&s.dy>0&&s.ne.getX()>s.sw.getX()&&s.ne.getY()>s.sw.getY(),"invalid domain");
    require(s.subdivisions==0||s.subdivisions==64||s.subdivisions==128||s.subdivisions==256||s.subdivisions==512,"unsupported arrival grid");
    if(!s.xs.empty()){
        require(s.xs.size()<=32768&&s.ys.size()<=32768&&s.lat.size()==s.xs.size()*s.ys.size()&&s.lon.size()==s.lat.size(),"invalid source geography");
        s.frame.grid=std::make_shared<FFTileGeographicGrid>(int(s.xs.size()),s.xs.data(),int(s.ys.size()),s.ys.data(),s.lat.data(),s.lon.data());
    }
    auto o=input(chunks.at("objects.ff"));require(string(o)=="FFOBJECTS1","unknown object schema");n=count(o);
    for(size_t i=0;i<n;++i){FrontRecord r;require(bool(o>>r.id),"missing front id");r.parent=count(o);r.time=real(o);r.update=real(o);r.expanding=count(o,1)!=0;require(i==0||r.parent<i,"invalid front hierarchy");s.fronts.push_back(r);}
    require(!s.fronts.empty(),"missing root front");n=count(o);
    std::set<long> ids;for(auto& r:s.fronts)require(ids.insert(r.id).second,"duplicate object id");
    for(size_t i=0;i<n;++i){NodeRecord r;require(bool(o>>r.id),"missing node id");r.front=count(o);r.merge=count(o);r.state=int(count(o,5));r.time=real(o);r.update=real(o);r.loc=point(o);r.next=point(o);r.velocity=vector(o);r.normal=vector(o);r.speed=real(o);r.depth=real(o);r.curvature=real(o);
        require(ids.insert(r.id).second,"duplicate node identity "+std::to_string(r.id));
        require(r.front>0&&r.front<s.fronts.size(),"invalid containing front for node "+std::to_string(r.id));
        require(r.merge<=n,"invalid merging reference for node "+std::to_string(r.id));
        require(std::isfinite(r.time)&&r.speed>=0&&std::isfinite(r.speed)&&std::isfinite(r.depth)&&std::isfinite(r.curvature),"invalid numerical fields for node "+std::to_string(r.id));s.nodes.push_back(r);}
    end(o);
    auto e=input(chunks.at("events"));n=count(e,2000000);
    for(size_t i=0;i<n;++i){EventRecord r;r.kind=count(e,3);r.index=count(e);r.time=real(e);r.input=count(e,1)!=0;r.output=count(e,1)!=0;
        require(std::isfinite(r.time)&&r.time>=s.clock-1e-5&&(r.kind!=1||r.index<s.fronts.size())&&(r.kind!=2||r.index<s.nodes.size()),"invalid pending event");s.events.push_back(r);}end(e);
    const auto& b=chunks.at("arrival");size_t at=0;auto runs=get(b,at);require(runs<=(b.size()-at)/24,"truncated arrival runs");uint64_t previous=0;
    for(size_t i=0;i<runs;++i){HistoryRun r{get(b,at),get(b,at),getReal(b,at)};
        require(r.length>0&&r.first>=previous&&r.first<uint64_t(s.nx)*s.ny&&r.length<=uint64_t(s.nx)*s.ny-r.first&&std::isfinite(r.time)&&r.time<=s.clock+1e-5,"invalid arrival history");previous=r.first+r.length;s.history.push_back(r);}
    require(at==b.size(),"trailing arrival data");
    if(chunks.count("wind")){
        auto w=input(chunks.at("wind"));s.wind=std::make_shared<GriddedWind>();auto& g=*s.wind;g.nx=count(w,65);g.ny=count(w,65);g.west=real(w);g.south=real(w);g.east=real(w);g.north=real(w);g.times=array(w,48);g.u=array(w,4000000,true);g.v=array(w,4000000,true);end(w);g.validate();
    }
    if(chunks.count("tiles")){
        const auto& b=chunks.at("tiles");size_t pos=0;auto size=get(b,pos);require(size<=259200,"too many tile references");
        for(size_t i=0;i<size;++i){auto key=get(b,pos,4);require(key<259200&&!s.tiles.count(int(key))&&b.size()-pos>=32,"invalid tile reference");TileSource t;std::copy_n(b.begin()+pos,32,t.hash.begin());pos+=32;auto nbytes=get(b,pos);require(nbytes<=64*1024*1024&&nbytes<=b.size()-pos,"invalid fuel override");t.fuel=Bytes(b.begin()+pos,b.begin()+pos+nbytes);pos+=nbytes;s.tiles.emplace(int(key),std::move(t));}
        require(pos==b.size(),"trailing tile data");
    }
    if(chunks.count("fluxes")){
        auto f=input(chunks.at("fluxes"));auto size=count(f,100);
        for(size_t i=0;i<size;++i){FluxRecord r;r.name=string(f);r.time=real(f);r.cache=array(f);auto n=count(f,100000);for(size_t j=0;j<n;++j){EmissionRecord e;e.center=point(f);e.area=real(f);e.start=real(f);e.end=real(f);e.flux=real(f);require(std::isfinite(e.area)&&e.area>=0&&std::isfinite(e.start)&&std::isfinite(e.end)&&e.end>e.start&&std::isfinite(e.flux),"invalid emission");r.emissions.push_back(e);}s.fluxes.push_back(std::move(r));}end(f);
    }
    if(chunks.count("runtime")){auto r=input(chunks.at("runtime"));s.runtime=array(r,6);require(s.runtime.size()==6,"invalid runtime counters");for(size_t i=0;i<6;++i)require(std::isfinite(s.runtime[i])&&(i<2||(s.runtime[i]>=-1&&s.runtime[i]<=2147483647.&&std::floor(s.runtime[i])==s.runtime[i])),"invalid runtime counter value");end(r);}
    return s;
}
FFPoint historyCenter(Snapshot& s,size_t i,size_t j){
    if(s.subdivisions){auto q=s.frame.cartesian(s.geoBounds[1]+(j+.5)*.1/s.subdivisions,s.geoBounds[0]+(i+.5)*.1/s.subdivisions);return FFPoint(q[0],q[1],0);}
    return FFPoint(s.sw.getX()+(i+.5)*s.dx,s.sw.getY()+(j+.5)*s.dy,0);
}
double historyAt(Snapshot& s,FFPoint p){
    int64_t i,j;
    if(s.subdivisions){auto q=s.frame.geographic(p.getX(),p.getY());double lon=s.frame.longitude+std::remainder(q[1]-s.frame.longitude,360.);i=int64_t(std::floor((lon-s.geoBounds[0])*s.subdivisions*10+1e-8));j=int64_t(std::floor((q[0]-s.geoBounds[1])*s.subdivisions*10+1e-8));}
    else{i=int64_t(std::floor((p.getX()-s.sw.getX())/s.dx));j=int64_t(std::floor((p.getY()-s.sw.getY())/s.dy));}
    if(i<0||j<0||i>=int64_t(s.nx)||j>=int64_t(s.ny))return std::numeric_limits<double>::infinity();
    uint64_t index=uint64_t(j)*s.nx+uint64_t(i);auto it=std::upper_bound(s.history.begin(),s.history.end(),index,[](uint64_t a,const HistoryRun& b){return a<b.first;});
    if(it==s.history.begin())return std::numeric_limits<double>::infinity();--it;return index-it->first<it->length?it->time:std::numeric_limits<double>::infinity();
}
}

void FFState::save(const std::string& filename,const std::string& scenario){
    auto& session=Command::currentSession;auto* d=Command::getDomain();require(d&&session.tt,"no fire domain");
    auto manager=d->dataBroker->getFFTileManager();require(bool(manager),"v1 checkpoints require an FFTile landscape");
    require(std::set<std::string>{"Rothermel","Balbi2015","Farsite","Iso","WindDriven"}.count(session.params->getParameter("propagationModel")),"propagation model has no audited checkpoint contract");
    auto* params=session.params;
    require(!Command::init||Command::currentFrontCompleted,"complete initialization with step[dt=0] before saving");
    // The parameter clock is used by standalone tile runs; Meso-NH uses seconds
    // from midnight in its atom clock and supplies a zero reference offset.
    double base=epoch(d->refYear,d->refDay,params->getDouble("refTime"));
    std::map<std::string,Bytes> chunks;
    chunks["manifest"]=bytes(std::string("ForeFire ")+FF_STATE_ENGINE_VERSION+"\nFFSTATE1 / FFOBJECTS1\n");
    auto dom=output();dom<<Command::getTime()<<' '<<d->getTime()<<' '<<d->getUpdateTime()<<' '<<base<<' ';
    auto* rep=session.fdp?session.outStrRepp:session.outStrRep;
    dom<<(rep?rep->getTime():0)<<' '<<(rep?rep->getUpdateTime():0)<<' '<<ForeFireAtom::instanceNRCount.load()<<' ';point(dom,d->SWCorner);point(dom,d->NECorner);
    auto& f=manager->coordinates;dom<<f.latitude<<' '<<f.longitude<<' '<<f.x<<' '<<f.y<<' '<<d->globalBMapSizeX<<' '<<d->globalBMapSizeY<<' '<<d->burningMatrixResX<<' '<<d->burningMatrixResY<<' ';
    dom<<(d->geographicArrival_?d->geographicArrival_->subdivisions():0)<<' ';
    auto bounds=d->geographicArrival_?d->geographicArrival_->bounds():std::array<double,4>{};for(auto x:bounds)dom<<x<<' ';
    dom<<manager->windU<<' '<<manager->windV<<'\n';
    if(f.grid){array(dom,f.grid->xs_);array(dom,f.grid->ys_);array(dom,f.grid->lat_);array(dom,f.grid->lon_);}
    else for(int i=0;i<4;++i)array(dom,{});
    chunks["domain"]=bytes(dom.str());
    auto runtime=output();array(runtime,{Command::refTime,Command::bmapOutputUpdate,double(Command::numBmapOutputs),double(Command::numAtmoIterations),double(FireDomain::atmoIterNumber),double(d->numIterationAtmoModel)});chunks["runtime"]=bytes(runtime.str());
    Bytes tiles;put(tiles,manager->sources_.size());
    for(auto& entry:manager->sources_){put(tiles,uint64_t(entry.first),4);tiles.insert(tiles.end(),entry.second.begin(),entry.second.end());auto it=manager->fuelOverrides_.find(entry.first);auto cached=manager->cache_.find(entry.first);const Bytes* fuel=it==manager->fuelOverrides_.end()?nullptr:&it->second;if(cached!=manager->cache_.end()&&cached->second.tile&&!cached->second.tile->fuelOverride().empty())fuel=&cached->second.tile->fuelOverride();size_t size=fuel?fuel->size():0;put(tiles,size);if(size)tiles.insert(tiles.end(),fuel->begin(),fuel->end());}
    chunks["tiles"]=std::move(tiles);
    auto parameters=params->parameters;
    auto setting=[&](const std::string& key,double value){auto out=output();out<<value;parameters[key]=out.str();};
    setting("perimeterResolution",d->perimeterResolution);setting("spatialIncrement",d->spatialIncrement);
    setting("propagationSpeedAdjustmentFactor",d->propagationSpeedAdjustmentFactor);
    setting("relax",FireNode::relax);setting("smoothing",FireNode::smoothing);setting("minSpeed",FireNode::minSpeed);
    setting("minimalPropagativeFrontDepth",FireNode::minFrontDepth);setting("frontDepthComputation",FireNode::fdepth);setting("curvatureComputation",FireNode::ccurvature);
    setting("ffstateMaxTimeStep",d->dtMax);
    setting("arrivalMapResolution",d->BMapsResolution);
    parameters["normalScheme"]=FireNode::nmlScheme==FireNode::spline?"splines":FireNode::nmlScheme==FireNode::weightedMedians?"weightedMedians":"medians";
    parameters["curvatureScheme"]=FireNode::curvScheme==FireNode::angle?"angle":"circumradius";
    // Save the live fuel table, including trigger[fuel] changes.
    auto& table=d->dataBroker->fuelPropertiesTable;std::set<std::string> keys;
    for(const auto& row:table)for(const auto& kv:row)keys.insert(kv.first);
    auto fuels=output();bool first=true;for(auto& key:keys){if(!first)fuels<<';';fuels<<key;first=false;}fuels<<'\n';
    for(auto& row:table){first=true;for(auto& key:keys){if(!first)fuels<<';';auto it=row.find(key);fuels<<(it==row.end()?0:it->second);first=false;}fuels<<'\n';}
    parameters["fuelsTable"]=fuels.str();
    parameters["fftileFuelPolygons"]=d->dataBroker->getFFTileManager()->fuelPolygons();
    if(parameters.count("ffstateTileProvider")&&!parameters.at("ffstateTileProvider").empty())parameters["fftilesPath"]=parameters.at("ffstateTileProvider");
    auto pars=output();pars<<parameters.size()<<'\n';for(auto& kv:parameters)pars<<std::quoted(kv.first)<<' '<<std::quoted(kv.second)<<'\n';chunks["parameters"]=bytes(pars.str());
    auto fluxes=output();fluxes<<d->dataBroker->fluxLayersMap.size()<<'\n';
    for(auto& entry:d->dataBroker->fluxLayersMap){auto* f=entry.second;require(f->mapSize==1,"spatial flux-model maps are not supported by FFState v1");fluxes<<std::quoted(entry.first)<<' '<<f->latestCallGetMatrix<<' ';std::vector<double> cache(f->flux->getData(),f->flux->getData()+f->flux->getSize());array(fluxes,cache);fluxes<<f->emissions.size()<<'\n';for(auto& e:f->emissions){point(fluxes,e.center);fluxes<<e.area<<' '<<e.startTime<<' '<<e.endTime<<' '<<e.flux<<'\n';}}
    chunks["fluxes"]=bytes(fluxes.str());
    std::vector<FireFront*> fronts;std::vector<FireNode*> nodes;
    std::function<void(FireFront*)> visit=[&](FireFront* front){fronts.push_back(front);if(front->headNode){auto* n=front->headNode;do{require(nodes.size()<1000000,"too many nodes or broken ring");nodes.push_back(n);n=n->nextInFront;require(n,"broken front ring");}while(n!=front->headNode);}for(auto* child:front->innerFronts)visit(child);};
    visit(d->domainFront);
    std::unordered_map<ForeFireAtom*,std::pair<size_t,size_t>> atoms;atoms[d]={0,0};
    std::unordered_map<FireFront*,size_t> fi;std::unordered_map<FireNode*,size_t> ni;
    for(size_t i=0;i<fronts.size();++i){fi[fronts[i]]=i;atoms[fronts[i]]={1,i};}
    for(size_t i=0;i<nodes.size();++i){ni[nodes[i]]=i;atoms[nodes[i]]={2,i};}
    if(rep)atoms[rep]={3,0};
    auto objects=output();objects<<std::quoted("FFOBJECTS1")<<'\n'<<fronts.size()<<'\n';
    for(auto* front:fronts)objects<<front->getID()<<' '<<(front->containingFront?fi.at(front->containingFront):0)<<' '<<front->getTime()<<' '<<front->getUpdateTime()<<' '<<front->expanding<<'\n';
    objects<<nodes.size()<<'\n';
    for(auto* node:nodes){require(node->currentState!=FireNode::merging||(node->mergingNode&&ni.count(node->mergingNode)),"pending merging node is outside serialized graph");objects<<node->getID()<<' '<<fi.at(node->front)<<' '<<(node->currentState==FireNode::merging?ni.at(node->mergingNode)+1:0)<<' '<<int(node->currentState)<<' '<<node->getTime()<<' '<<node->getUpdateTime()<<' ';point(objects,node->location);point(objects,node->nextloc);vector(objects,node->velocity);vector(objects,node->normal);objects<<node->speed<<' '<<node->frontDepth<<' '<<node->curvature<<'\n';}
    chunks["objects.ff"]=bytes(objects.str());
    auto events=output();std::vector<EventRecord> evs;auto* head=session.tt->getHead();auto* ev=head;
    if(ev)do{auto it=atoms.find(ev->getAtom());
        if(session.fdp&&ev->getAtom()==session.outStrRep){ev=ev->getNext();continue;}
        // Recycled nodes can retain an inert event after detachment.
        // They are not live state and must not become scheduled active nodes.
        auto* retired=dynamic_cast<FireNode*>(ev->getAtom());
        if(it==atoms.end()&&retired&&retired->currentState==FireNode::final&&!retired->front&&std::isinf(retired->getTime())&&std::isinf(retired->getUpdateTime())&&std::find(FireDomain::trashNodes.begin(),FireDomain::trashNodes.end(),retired)!=FireDomain::trashNodes.end()){ev=ev->getNext();continue;}
        require(it!=atoms.end(),"pending command or unsupported scheduled object; save at a boundary without pending scripts: "+(it==atoms.end()?ev->getAtom()->toString():std::string()));evs.push_back({it->second.first,it->second.second,ev->getTime(),ev->input,ev->output});ev=ev->getNext();require(evs.size()<2000000,"broken schedule");}while(ev!=head);
    events<<evs.size()<<'\n';for(auto& e:evs)events<<e.kind<<' '<<e.index<<' '<<e.time<<' '<<e.input<<' '<<e.output<<'\n';chunks["events"]=bytes(events.str());
    std::map<uint64_t,double> history;
    if(d->geographicArrival_){auto& g=*d->geographicArrival_;for(size_t b=0;b<g.blocks_.size();++b)if(g.blocks_[b]){
        int64_t bx=g.blockX_+int64_t(b/g.blocksY_),by=g.blockY_+int64_t(b%g.blocksY_);
        for(size_t x=0;x<g.subdivisions_;++x)for(size_t y=0;y<g.subdivisions_;++y){auto i=bx*g.subdivisions_+int64_t(x)-g.x0_,j=by*g.subdivisions_+int64_t(y)-g.y0_;double t=(*g.blocks_[b])[x*g.subdivisions_+y];if(i>=0&&j>=0&&i<int64_t(g.nx_)&&j<int64_t(g.ny_)&&std::isfinite(t))history[uint64_t(j)*g.nx_+uint64_t(i)]=t;}
    }}else for(size_t x=0;x<d->atmoNX;++x)for(size_t y=0;y<d->atmoNY;++y){auto* b=d->cells[x][y].getBurningMap();if(!b)continue;
        for(size_t i=0;i<b->getSizeX();++i)for(size_t j=0;j<b->getSizeY();++j){double t=(*b)(i,j);if(std::isfinite(t))history[(y*d->localBMapSizeY+j)*d->globalBMapSizeX+x*d->localBMapSizeX+i]=t;}
    }
    std::vector<HistoryRun> runs;for(auto& h:history){require(h.second<=Command::getTime()+1e-5,"arrival time is after checkpoint");if(!runs.empty()&&runs.back().first+runs.back().length==h.first&&std::memcmp(&runs.back().time,&h.second,sizeof(double))==0)++runs.back().length;else runs.push_back({h.first,1,h.second});}
    Bytes arrival;put(arrival,runs.size());for(auto& r:runs){put(arrival,r.first);put(arrival,r.length);putReal(arrival,r.time);}chunks["arrival"]=std::move(arrival);
    auto* wind=dynamic_cast<GriddedTileWindLayer*>(d->getDataLayer("windU"));
    if(wind){auto* v=dynamic_cast<GriddedTileWindLayer*>(d->getDataLayer("windV"));require(v&&v->forcing==wind->forcing,"wind components do not share one forcing window");auto w=output();auto& g=*wind->forcing;g.validate();w<<g.nx<<' '<<g.ny<<' '<<g.west<<' '<<g.south<<' '<<g.east<<' '<<g.north<<'\n';array(w,g.times);array(w,g.u);array(w,g.v);chunks["wind"]=bytes(w.str());}
    else if(!d->atmosphericCoupling)require(dynamic_cast<FFTileDataLayer*>(d->getDataLayer("windU"))&&dynamic_cast<FFTileDataLayer*>(d->getDataLayer("windV")),"unsupported wind layer");
    require(dynamic_cast<FFTileDataLayer*>(d->getDataLayer("fuel")),"unsupported modified fuel layer");
    require(dynamic_cast<FFTileDataLayer*>(d->getDataLayer("altitude"))&&dynamic_cast<FFTileDataLayer*>(d->getDataLayer("slope")),"unsupported modified terrain layer");
    if(!scenario.empty()){std::ifstream script(scenario,std::ios::binary);require(bool(script),"cannot read scenario");std::string text((std::istreambuf_iterator<char>(script)),{});require(text.size()<4*1024*1024,"scenario too large");chunks["scenario.ff"]=bytes(text);}
    Bytes file;file.insert(file.end(),{'F','F','S','T','A','T','E','1'});put(file,1,4);put(file,chunks.size(),4);
    for(auto& chunk:chunks){put(file,chunk.first.size(),4);put(file,chunk.second.size());put(file,crc(chunk.second),4);file.insert(file.end(),chunk.first.begin(),chunk.first.end());file.insert(file.end(),chunk.second.begin(),chunk.second.end());require(file.size()<=limit,"checkpoint exceeds size limit");}
    // Verify semantic invariants as well as checksums before replacing a file.
    decode(chunks);
    auto temporary=filename+".tmp";std::ofstream out(temporary,std::ios::binary|std::ios::trunc);require(bool(out),"cannot create "+temporary);
    out.write(reinterpret_cast<const char*>(file.data()),file.size());out.flush();require(bool(out),"checkpoint write failed");out.close();
    std::error_code ec;std::filesystem::rename(temporary,filename,ec);require(!ec,"cannot publish checkpoint: "+ec.message());
}

void FFState::load(const std::string& filename,bool attach,const std::string& provider){
    require(!attach||provider.empty(),"configure the tile provider on the host before attach");
    auto snapshot=decode(read(filename));auto& s=snapshot;auto& session=Command::currentSession;
    auto* params=session.params;auto* target=Command::getDomain();require(attach?target!=nullptr:target==nullptr,attach?"attach requires a host domain":"domain exists; use an empty session or mode=attach");
    if(attach)require(target->domainFront->innerFronts.empty(),"attach requires an empty fire domain; replacing a running fire is not implicit");
    auto previous=params->parameters;
    std::unique_ptr<FireDomain> owned;
    std::unique_ptr<TimeTable> ownedSchedule;
    std::unique_ptr<StringRepresentation> ownedRepresentation;
    std::unique_ptr<Simulator> ownedSimulator;
    bool published=false;
    try {
        if(!attach){
            params->parameters=s.parameters;params->parameters["runmode"]="standalone";params->parameters["parallel"]="0";params->parameters.erase("BMapFiles");
            // Runtime destinations belong to this machine/session.
            for(auto key:{"caseDirectory","fireOutputDirectory","ForeFireDataDirectory","outputFiles","outputsUpdate","fftileCacheDirectory"})if(previous.count(key))params->parameters[key]=previous.at(key);
            params->parameters["arrivalMapSubdivisions"]=std::to_string(s.subdivisions);
            if(!provider.empty())params->parameters["fftilesPath"]=provider;
            if(s.frame.grid)require(false,"coupled-source standalone restore needs an explicitly attached registered geographic grid");
            owned.reset(new FireDomain(s.clock,s.sw,s.ne));target=owned.get();
            if(s.parameters.count("ffstateMaxTimeStep")){auto time=input(bytes(s.parameters.at("ffstateMaxTimeStep")));target->dtMax=real(time);require(target->dtMax>0,"invalid maximum node timestep");}
        }
        auto manager=target->dataBroker->getFFTileManager();require(bool(manager),"destination must use FFTiles");
        bool sameFrame=attach&&s.frame.grid&&manager->coordinates.grid&&s.xs==manager->coordinates.grid->xs_&&s.ys==manager->coordinates.grid->ys_&&s.lat==manager->coordinates.grid->lat_&&s.lon==manager->coordinates.grid->lon_;
        bool preservePending=sameFrame&&s.nx==target->globalBMapSizeX&&s.ny==target->globalBMapSizeY&&s.dx==target->burningMatrixResX&&s.dy==target->burningMatrixResY&&s.sw.getX()==target->SWCorner.getX()&&s.sw.getY()==target->SWCorner.getY()&&target->perimeterResolution==std::stod(s.parameters.at("perimeterResolution"))&&target->spatialIncrement==std::stod(s.parameters.at("spatialIncrement"));
        manager->restoreFuelPolygons(s.parameters.count("fftileFuelPolygons")?s.parameters.at("fftileFuelPolygons"):"");
        auto checkedProvider=std::make_shared<StateProvider>(manager->provider_,s.tiles);
        auto probe=std::make_shared<FFTileManager>(checkedProvider,manager->capacity_);probe->coordinates=manager->coordinates;
        double offset=0;
        if(attach){
            double hostEpoch=epoch(target->refYear,target->refDay,params->getDouble("refTime"));offset=s.epoch-hostEpoch;
            require(std::abs(s.clock+offset-Command::getTime())<1e-4,"checkpoint UTC does not match host coupling time");
        }
        auto mapPoint=[&](FFPoint p){if(!attach||sameFrame)return p;auto q=s.frame.geographic(p.getX(),p.getY());auto xy=manager->coordinates.cartesian(q[0],q[1]);require(std::isfinite(xy[0])&&std::isfinite(xy[1]),"coordinate mapping failed");return FFPoint(xy[0],xy[1],p.getZ());};
        auto mapVector=[&](FFPoint p,FFVector v){if(!attach||sameFrame)return v;double h=.5;auto a=mapPoint(p+FFPoint(h,0,0)),b=mapPoint(p-FFPoint(h,0,0)),c=mapPoint(p+FFPoint(0,h,0)),e=mapPoint(p-FFPoint(0,h,0));return FFVector((a.getX()-b.getX())*v.getVx()+(c.getX()-e.getX())*v.getVy(),(a.getY()-b.getY())*v.getVx()+(c.getY()-e.getY())*v.getVy(),v.getVz());};
        for(auto& n:s.nodes){auto p=mapPoint(n.loc),next=mapPoint(n.next);require(target->striclyWithinDomain(p)&&target->striclyWithinDomain(next),"current or pending node lies outside destination domain");probe->sample(FFTileField::Fuel,p.getX(),p.getY());probe->sample(FFTileField::Altitude,p.getX(),p.getY());}
        require(attach||(target->globalBMapSizeX==s.nx&&target->globalBMapSizeY==s.ny),"arrival dimensions differ from checkpoint");
        // Build transfer before touching the host. Resampling is explicit in
        // attach mode; same-grid standalone restore remains lossless.
        std::map<uint64_t,double> arrivals;
        if(!attach||preservePending){for(auto& r:s.history)for(uint64_t i=0;i<r.length;++i)arrivals[r.first+i]=r.time+offset;}
        else {
            require(!target->geographicArrival_,"geographic attach target is not supported yet");
            // Visit destination cells only within the mapped burned footprint.
            double xmin=target->NECorner.getX(),ymin=target->NECorner.getY(),xmax=target->SWCorner.getX(),ymax=target->SWCorner.getY();
            for(auto& r:s.history)for(uint64_t k=0;k<r.length;++k){uint64_t index=r.first+k;auto p=mapPoint(historyCenter(s,index%s.nx,index/s.nx));require(target->striclyWithinDomain(p),"burned history lies outside destination domain");xmin=std::min(xmin,p.getX()-s.dx*2);xmax=std::max(xmax,p.getX()+s.dx*2);ymin=std::min(ymin,p.getY()-s.dy*2);ymax=std::max(ymax,p.getY()+s.dy*2);}
            auto low=[](double x,size_t n){return size_t(std::clamp(std::floor(x),0.,double(n)));};
            auto i0=low((xmin-target->SWCorner.getX())/target->burningMatrixResX,target->globalBMapSizeX),i1=low((xmax-target->SWCorner.getX())/target->burningMatrixResX+1,target->globalBMapSizeX);
            auto j0=low((ymin-target->SWCorner.getY())/target->burningMatrixResY,target->globalBMapSizeY),j1=low((ymax-target->SWCorner.getY())/target->burningMatrixResY+1,target->globalBMapSizeY);
            // Refuse coarsening that can erase narrow burn support. A future
            // area-aware flux adapter can relax this limit explicitly.
            require(target->burningMatrixResX<=s.dx*1.01&&target->burningMatrixResY<=s.dy*1.01,"attach would coarsen arrival history; use a host arrival grid at least as fine as the source");
            for(size_t i=i0;i<i1;++i)for(size_t j=j0;j<j1;++j){FFPoint p(target->SWCorner.getX()+(i+.5)*target->burningMatrixResX,target->SWCorner.getY()+(j+.5)*target->burningMatrixResY,0);auto geo=manager->coordinates.geographic(p.getX(),p.getY());auto xy=s.frame.cartesian(geo[0],geo[1]);double t=historyAt(s,FFPoint(xy[0],xy[1],0));if(std::isfinite(t))arrivals[uint64_t(j)*target->globalBMapSizeX+i]=t+offset;}
        }
        // In attach mode, the host chooses atmospheric bindings and numerical
        // settings. Require matching fire physics until model reconfiguration
        // can be transactional; do not silently override a constructed model.
        if(attach)for(auto key:{"propagationModel","propagationSpeedAdjustmentFactor","relax","windReductionFactor","Farsite.adjustementWind","temperature","moisture"})if(s.parameters.count(key))require(params->getParameter(key)==s.parameters.at(key),std::string("host fire parameter differs; configure ffstateFile before construction: ")+key);
        manager->provider_=checkedProvider;manager->cache_=std::move(probe->cache_);
        for(auto& entry:s.tiles){manager->sources_[entry.first]=entry.second.hash;if(!entry.second.fuel.empty())manager->fuelOverrides_[entry.first]=entry.second.fuel;}
        if(!attach){ownedSchedule.reset(new TimeTable(new FFEvent(target)));ownedRepresentation.reset(new StringRepresentation(target));ownedSimulator.reset(new Simulator(ownedSchedule.get(),false));}
        auto* schedule=attach?session.tt:ownedSchedule.get();
        target->setTimeTable(schedule);
        if(!attach)schedule->clear();
        // Factories register ownership and spatial-cell membership; no parser
        // interpolation or initial burning scan is performed.
        std::vector<FireFront*> fronts{target->domainFront};
        for(size_t i=1;i<s.fronts.size();++i)fronts.push_back(target->addFireFront(attach?Command::getTime():s.fronts[i].time+offset,fronts[s.fronts[i].parent]));
        long largest=0;
        for(size_t i=0;i<fronts.size();++i){auto* f=fronts[i];auto& r=s.fronts[i];f->setTime(r.time+offset);f->setUpdateTime(r.update+offset);f->expanding=r.expanding;if(!attach||preservePending)f->setID(r.id);largest=std::max(largest,f->getShortID());}
        std::vector<FireNode*> nodes;std::vector<FireNode*> tails(fronts.size(),nullptr);
        for(auto& r:s.nodes){auto loc=mapPoint(r.loc);auto vel=mapVector(r.loc,r.velocity);auto* n=target->addFireNode(loc,vel,r.time+offset,r.depth,r.curvature,fronts[r.front],tails[r.front],0,0,FireNode::final);tails[r.front]=n;nodes.push_back(n);
            n->location=loc;n->nextloc=mapPoint(r.next);n->velocity=vel;n->normal=(!attach||sameFrame)?r.normal:mapVector(r.loc,r.normal).normed();n->speed=(!attach||sameFrame)?r.speed:vel.norm();n->frontDepth=r.depth;n->curvature=r.curvature;n->setTime(r.time+offset);n->setUpdateTime(r.update+offset);n->currentState=FireNode::State(r.state);if(!attach||preservePending)n->setID(r.id);largest=std::max(largest,n->getShortID());
        }
        for(size_t i=0;i<nodes.size();++i)nodes[i]->mergingNode=s.nodes[i].merge?nodes[s.nodes[i].merge-1]:nullptr;
        for(auto& r:arrivals)target->setArrivalTime(size_t(r.first%target->globalBMapSizeX),size_t(r.first/target->globalBMapSizeX),r.second);
        if(!attach){manager->windU=s.windU;manager->windV=s.windV;if(s.wind){auto extent=target->NECorner-target->SWCorner;target->dataBroker->registerLayer("windU",new GriddedTileWindLayer("windU",manager,s.wind,target->SWCorner,extent));target->dataBroker->registerLayer("windV",new GriddedTileWindLayer("windV",manager,s.wind,target->SWCorner,extent));}}
        for(auto& r:s.fluxes){auto* f=target->getFluxLayer(r.name);if(!f){require(target->addFluxLayer(r.name),"cannot restore flux layer "+r.name);f=target->getFluxLayer(r.name);}require(f,"missing flux layer");if(!attach){require(f->flux->getSize()==r.cache.size(),"flux cache dimensions differ");std::copy(r.cache.begin(),r.cache.end(),f->flux->getData());f->latestCallGetMatrix=r.time;}f->emissions.clear();for(auto& e:r.emissions)f->addEmission(mapPoint(e.center),e.area,e.start+offset,e.end+offset,e.flux);}
        auto* rep=attach?(session.fdp?session.outStrRepp:session.outStrRep):ownedRepresentation.get();
        if(!attach){
            target->setTime(s.domainTime);target->setUpdateTime(s.domainUpdate);if(rep){rep->setTime(s.outputTime);rep->setUpdateTime(s.outputUpdate);}
            // Preserve the linked timetable's order, including epsilon ties.
            schedule->clear();std::vector<FFEvent*> evs;
            for(auto& r:s.events){ForeFireAtom* a=r.kind==0?static_cast<ForeFireAtom*>(target):r.kind==1?static_cast<ForeFireAtom*>(fronts[r.index]):r.kind==2?static_cast<ForeFireAtom*>(nodes[r.index]):static_cast<ForeFireAtom*>(rep);require(a,"missing output object");auto* e=new FFEvent(a,r.time,"none");e->input=r.input;e->output=r.output;evs.push_back(e);}
            schedule->head=evs.empty()?nullptr:evs.front();schedule->incr=evs.size();schedule->decr=0;
            for(size_t i=0;i<evs.size();++i){evs[i]->setNext(evs[(i+1)%evs.size()]);evs[i]->setPrev(evs[(i+evs.size()-1)%evs.size()]);}
        }else{
            // A forcing change starts new pending motion at the attachment
            // instant, retaining current velocity for the normal relaxation.
            if(preservePending){
                std::set<ForeFireAtom*> restored(fronts.begin(),fronts.end());
                std::vector<FFEvent*> discard;auto* e=schedule->getHead();auto* head=e;
                if(e)do{if(restored.count(e->getAtom()))discard.push_back(e);e=e->getNext();}while(e!=head);
                for(auto* ev:discard)schedule->dropEvent(ev);
                for(auto& r:s.events)if(r.kind==1||r.kind==2){ForeFireAtom* atom=r.kind==1?static_cast<ForeFireAtom*>(fronts[r.index]):static_cast<ForeFireAtom*>(nodes[r.index]);auto* ev=new FFEvent(atom,r.time+offset,"none");ev->input=r.input;ev->output=r.output;schedule->insert(ev);}
            }else for(auto* n:nodes)if(n->currentState!=FireNode::final&&n->currentState!=FireNode::link){n->setTime(Command::getTime());n->setUpdateTime(Command::getTime());n->nextloc=n->location;if(n->currentState==FireNode::moving)n->currentState=FireNode::init;target->addNewAtomToSimulation(n);}
            auto stamp=output();stamp<<Command::getTime();params->parameters["ffstateAttachedTime"]=stamp.str();params->parameters["ffstateAttachedFile"]=filename;
            for(auto& layer:target->dataBroker->fluxLayersMap)layer.second->setFirstCall(Command::getTime());
        }
        largest=std::max(largest,long(s.nextID)-1);
        auto old=ForeFireAtom::instanceNRCount.load();while(old<=largest&&!ForeFireAtom::instanceNRCount.compare_exchange_weak(old,largest+1)){}
        if(!attach){session.fd=owned.release();session.tt=ownedSchedule.release();session.sim=ownedSimulator.release();session.outStrRep=ownedRepresentation.release();published=true;}
        session.ff=target->domainFront;Command::currentFrontCompleted=true;Command::init=false;Command::currentLevel=1;Command::previousNode=nullptr;Command::leftLinkNode=nullptr;Command::rightLinkNode=nullptr;delete Command::lastReadLoc;Command::lastReadLoc=nullptr;
        Command::setStartTime(s.clock+offset);Command::endTime=s.clock+offset;
        if(!attach){Command::setReferenceTime(s.runtime.empty()?0:s.runtime[0]);if(!s.runtime.empty()){Command::bmapOutputUpdate=s.runtime[1];Command::numBmapOutputs=int(s.runtime[2]);Command::numAtmoIterations=int(s.runtime[3]);FireDomain::atmoIterNumber=size_t(s.runtime[4]);target->numIterationAtmoModel=int(s.runtime[5]);}}
    }catch(...){if(!attach&&!published)params->parameters=previous;throw;}
}

void FFState::prepareCoupling(const std::string& filename) {
    auto s=decode(read(filename));auto* p=SimulationParameters::GetInstance();
    // These must be set before Meso-NH constructs the fire's models. Host
    // geometry, clock, MPI ownership and wind bindings are deliberately absent.
    for(auto key:{"propagationModel","fuelsTable","propagationSpeedAdjustmentFactor",
                  "perimeterResolution","spatialIncrement","relax","smoothing","minSpeed",
                  "minimalPropagativeFrontDepth","initialFrontDepth","initialBurningDuration",
                  "normalScheme","curvatureScheme","curvatureComputation","frontDepthComputation",
                  "windReductionFactor","Farsite.adjustementWind","temperature","moisture",
                  "moistures.ones","moistures.tens","moistures.hundreds","moistures.liveh","moistures.livew",
                  "nominalHeatFlux","nominalVaporFlux","burningDuration","burningTresholdFlux"})
        if(s.parameters.count(key))p->parameters[key]=s.parameters.at(key);
    double resolution=s.frame.grid?std::min(s.dx,s.dy):std::min(20.,std::min(s.dx,s.dy)/2);
    if(p->isValued("arrivalMapResolution"))resolution=std::min(resolution,p->getDouble("arrivalMapResolution"));
    auto out=output();out<<resolution;p->parameters["arrivalMapResolution"]=out.str();
}

void FFState::attachEnvironment(const std::string& filename,FireDomain& domain) {
    auto s=decode(read(filename));auto* db=domain.dataBroker;
    auto manager=db->getFFTileManager();require(bool(manager),"coupled environment requires FFTiles");
    manager->restoreFuelPolygons(s.parameters.count("fftileFuelPolygons")?s.parameters.at("fftileFuelPolygons"):"");
    manager->provider_=std::make_shared<StateProvider>(manager->provider_,s.tiles);manager->cache_.clear();
    for(auto& entry:s.tiles){manager->sources_[entry.first]=entry.second.hash;if(!entry.second.fuel.empty())manager->fuelOverrides_[entry.first]=entry.second.fuel;}
    auto csv=s.parameters.at("fuelsTable");db->fuelPropertiesTable.clear();db->readTableFromString(csv,db->fuelPropertiesTable);
    for(auto* model:FireDomain::propModelsTable)if(model&&model->dataBroker==db&&model->numFuelProperties)db->extractFuelProperties(db->fuelPropertiesTable,model);
    for(auto* model:FireDomain::fluxModelsTable)if(model&&model->dataBroker==db&&model->numFuelProperties)db->extractFuelProperties(db->fuelPropertiesTable,model);
    double offset=s.epoch-epoch(domain.refYear,domain.refDay,SimulationParameters::GetInstance()->getDouble("refTime"));
    for(auto& r:s.fluxes){auto* f=domain.getFluxLayer(r.name);if(!f){require(domain.addFluxLayer(r.name),"cannot attach flux layer "+r.name);f=domain.getFluxLayer(r.name);}f->emissions.clear();for(auto& e:r.emissions){auto geo=s.frame.geographic(e.center.getX(),e.center.getY());auto xy=manager->coordinates.cartesian(geo[0],geo[1]);f->addEmission(FFPoint(xy[0],xy[1],e.center.getZ()),e.area,e.start+offset,e.end+offset,e.flux);}}
}
}
