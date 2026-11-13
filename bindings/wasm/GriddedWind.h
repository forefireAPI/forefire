#ifndef FOREFIRE_WASM_GRIDDED_WIND_H
#define FOREFIRE_WASM_GRIDDED_WIND_H
#include "FFTileDataLayer.h"
#include <algorithm>

namespace libforefire {
// A bounded rolling window of east/north reference winds. South-first rows,
// x fastest, then y, then time. Tiles retain their local response matrices.
struct GriddedWind {
    size_t nx=0, ny=0;
    double west=0, south=0, east=0, north=0;
    std::vector<double> times, u, v;
    void validate() const {
        auto finite=[](const std::vector<double>& a) {return std::all_of(a.begin(),a.end(),[](double x){return std::isfinite(x);});};
        auto finiteOrMissing=[](const std::vector<double>& a) {return std::all_of(a.begin(),a.end(),[](double x){return !std::isinf(x);});};
        if(!nx||!ny||nx>65||ny>65||times.size()<2||times.size()>48||
           !std::isfinite(west)||!std::isfinite(east)||!std::isfinite(south)||!std::isfinite(north)||
           east<=west||east-west>3||north<=south||south<=-90||north>=90||
           u.size()!=nx*ny*times.size()||v.size()!=u.size()||!finite(times)||!finiteOrMissing(u)||!finiteOrMissing(v))
            throw std::runtime_error("Invalid gridded wind dimensions, coverage or values");
        for(size_t i=1;i<times.size();++i) if(times[i]<=times[i-1])
            throw std::runtime_error("Wind timestamps must increase strictly");
    }
    std::array<double,2> at(double lat,double lon,double time) const {
        lon=west+std::remainder(lon-west,360.);
        if(!std::isfinite(time)||time<times.front()-1e-6||time>times.back()+1e-6||
           lon<west-1e-8||lon>east+1e-8||lat<south-1e-8||lat>north+1e-8)
            throw std::runtime_error("Wind forcing does not cover this position/time");
        double x=std::clamp((lon-west)/(east-west)*(nx-1),0.,double(nx-1));
        double y=std::clamp((lat-south)/(north-south)*(ny-1),0.,double(ny-1));
        size_t i=std::min(size_t(x),nx>1?nx-2:0),j=std::min(size_t(y),ny>1?ny-2:0);
        double fx=nx>1?x-i:0,fy=ny>1?y-j:0;
        auto hi=std::upper_bound(times.begin(),times.end(),time);
        size_t k=hi==times.begin()?0:std::min(size_t(hi-times.begin()-1),times.size()-2);
        double ft=std::clamp((time-times[k])/(times[k+1]-times[k]),0.,1.);
        auto sample=[&](const std::vector<double>& a,size_t t) {
            auto cell=[&](size_t di,size_t dj){return a[t*nx*ny+std::min(j+dj,ny-1)*nx+std::min(i+di,nx-1)];};
            double out=0;
            const double weights[]={ (1-fx)*(1-fy),fx*(1-fy),(1-fx)*fy,fx*fy };
            for(size_t n=0;n<4;++n)if(weights[n]>1e-12)out+=weights[n]*cell(n%2,n/2);
            return out;
        };
        auto temporal=[&](const std::vector<double>& a){
            if(ft<1e-12)return sample(a,k);
            if(1-ft<1e-12)return sample(a,k+1);
            return (1-ft)*sample(a,k)+ft*sample(a,k+1);
        };
        return {temporal(u),temporal(v)};
    }
};
class GriddedTileWindLayer : public FFTileDataLayer {
    std::shared_ptr<FFTileManager> manager;
    std::shared_ptr<GriddedWind> forcing;
    bool eastComponent;
public:
    GriddedTileWindLayer(const std::string& name,std::shared_ptr<FFTileManager> m,
                        std::shared_ptr<GriddedWind> f,FFPoint origin,FFPoint extent)
      : FFTileDataLayer(name,m,origin,extent),manager(std::move(m)),forcing(std::move(f)),eastComponent(name=="windU") {}
    double getValueAt(FFPoint p,const double& t) override {
        const double x=p.getX(),y=p.getY();auto q=manager->coordinates.geographic(x,y);
        auto ref=forcing->at(q[0],q[1],t);
        if(!std::isfinite(ref[0])||!std::isfinite(ref[1])) {
            if(manager->sample(FFTileField::Fuel,x,y)==0)return 0;
            throw std::runtime_error("Wind forcing is missing over burnable terrain");
        }
        double east=manager->sample(FFTileField::Muu,x,y)*ref[0]+manager->sample(FFTileField::Muv,x,y)*ref[1];
        double north=manager->sample(FFTileField::Mvu,x,y)*ref[0]+manager->sample(FFTileField::Mvv,x,y)*ref[1];
        // Invert the local Cartesian→east/north Jacobian. This includes the
        // latitude-dependent x scale of the standalone tile projection.
        auto a=manager->coordinates.geographic(x+.5,y),b=manager->coordinates.geographic(x-.5,y);
        auto c=manager->coordinates.geographic(x,y+.5),d=manager->coordinates.geographic(x,y-.5);
        const double meters=111195.,coslat=std::cos(q[0]*3.14159265358979323846/180.);
        double ex=std::remainder(a[1]-b[1],360.)*meters*coslat,ey=std::remainder(c[1]-d[1],360.)*meters*coslat;
        double nx=(a[0]-b[0])*meters,ny=(c[0]-d[0])*meters,det=ex*ny-ey*nx;
        if(std::abs(det)<1e-8)throw std::runtime_error("Singular wind coordinate Jacobian");
        return eastComponent?(ny*east-ey*north)/det:(ex*north-nx*east)/det;
    }
};
}
#endif
