#pragma once
#include "fftile.h"
#include "FFPoint.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace libforefire {
// A fixed global lattice; allocation is lazy in 0.1-degree blocks.
class GeographicArrivalMap {
    FFTileCoordinates frame_;
    int64_t x0_,y0_,blockX_,blockY_;
    size_t nx_,ny_,blocksX_,blocksY_,allocated_=0;
    unsigned subdivisions_;
    double scale_;
    std::vector<double> cosines_;
    std::vector<std::unique_ptr<std::vector<double>>> blocks_;
    static constexpr double meters=111195.,radians=0.017453292519943295;
    static int64_t divide(int64_t a,int64_t b){return a>=0?a/b:-((-a+b-1)/b);}
    struct Address {size_t block,pixel;};
    Address address(size_t i,size_t j)const{
        auto x=x0_+int64_t(i),y=y0_+int64_t(j),bx=divide(x,subdivisions_),by=divide(y,subdivisions_);
        return {size_t(bx-blockX_)*blocksY_+size_t(by-blockY_),size_t(x-bx*subdivisions_)*subdivisions_+size_t(y-by*subdivisions_)};
    }
    std::array<double,4> geographicBounds(FFPoint sw,FFPoint ne)const{
        std::array<double,4>b={1e9,1e9,-1e9,-1e9};
        for(double x:{sw.getX(),ne.getX()})for(double y:{sw.getY(),ne.getY()}){
            auto q=frame_.geographic(x,y);double lon=frame_.longitude+std::remainder(q[1]-frame_.longitude,360.);
            b[0]=std::min(b[0],lon);b[1]=std::min(b[1],q[0]);b[2]=std::max(b[2],lon);b[3]=std::max(b[3],q[0]);
        }return b;
    }
public:
    GeographicArrivalMap(FFTileCoordinates frame,FFPoint sw,FFPoint ne,unsigned subdivisions):frame_(std::move(frame)),subdivisions_(subdivisions),scale_(10.*subdivisions){
        if(frame_.grid||!(subdivisions==64||subdivisions==128||subdivisions==256||subdivisions==512))throw std::runtime_error("Geographic arrival map: choose 64,128,256 or 512 subdivisions in a standalone tile domain");
        auto b=geographicBounds(sw,ne);x0_=int64_t(std::floor((b[0]+180)*scale_));y0_=int64_t(std::floor((b[1]+90)*scale_));
        nx_=size_t(std::ceil((b[2]+180)*scale_)-x0_)+1;ny_=size_t(std::ceil((b[3]+90)*scale_)-y0_)+1;
        if(nx_>32768||ny_>32768||nx_*ny_>256*1024*1024)throw std::runtime_error("Geographic arrival map is too large");
        blockX_=divide(x0_,subdivisions_);blockY_=divide(y0_,subdivisions_);
        blocksX_=size_t(divide(x0_+int64_t(nx_-1),subdivisions_)-blockX_+1);blocksY_=size_t(divide(y0_+int64_t(ny_-1),subdivisions_)-blockY_+1);
        blocks_.resize(blocksX_*blocksY_);cosines_.resize(ny_+1);
        for(size_t j=0;j<=ny_;++j)cosines_[j]=std::cos(((y0_+double(j))/scale_-90)*radians);
    }
    unsigned subdivisions()const{return subdivisions_;}
    size_t width()const{return nx_;} size_t height()const{return ny_;}
    size_t allocatedBlocks()const{return allocated_;}
    double stepDegrees()const{return 1/scale_;}
    double spacingY()const{return meters/scale_;}
    double spacingX()const{return spacingY()*std::cos(frame_.latitude*radians);}
    std::array<double,4> bounds()const{return {x0_/scale_-180,y0_/scale_-90,(x0_+double(nx_))/scale_-180,(y0_+double(ny_))/scale_-90};}
    bool index(FFPoint p,size_t& i,size_t& j)const{
        auto q=frame_.geographic(p.getX(),p.getY());double lon=frame_.longitude+std::remainder(q[1]-frame_.longitude,360.);
        auto x=int64_t(std::floor((lon+180)*scale_))-x0_,y=int64_t(std::floor((q[0]+90)*scale_))-y0_;
        if(x<0||y<0||x>=int64_t(nx_)||y>=int64_t(ny_))return false;i=size_t(x);j=size_t(y);return true;
    }
    FFPoint vertex(size_t i,size_t j)const{
        double lon=(x0_+double(i))/scale_-180,lat=(y0_+double(j))/scale_-90;
        return FFPoint(frame_.x+(lon-frame_.longitude)*meters*cosines_[j],frame_.y+(lat-frame_.latitude)*meters,0);
    }
    double get(size_t i,size_t j)const{
        if(i>=nx_||j>=ny_)return std::numeric_limits<double>::infinity();auto a=address(i,j);auto& b=blocks_[a.block];return b?(*b)[a.pixel]:std::numeric_limits<double>::infinity();
    }
    void set(size_t i,size_t j,double t){
        if(i>=nx_||j>=ny_)return;auto a=address(i,j);auto& b=blocks_[a.block];
        if(!b){b=std::make_unique<std::vector<double>>(size_t(subdivisions_)*subdivisions_,std::numeric_limits<double>::infinity());++allocated_;}
        (*b)[a.pixel]=std::min((*b)[a.pixel],t);
    }
    double speed(size_t i,size_t j)const{
        if(!std::isfinite(get(i,j)))return std::numeric_limits<double>::infinity();
        double dx=0,dy=0;if(i>0&&i+1<nx_&&std::isfinite(get(i-1,j))&&std::isfinite(get(i+1,j)))dx=(get(i+1,j)-get(i-1,j))/(2*spacingY()*cosines_[j]);
        if(j>0&&j+1<ny_&&std::isfinite(get(i,j-1))&&std::isfinite(get(i,j+1)))dy=(get(i,j+1)-get(i,j-1))/(2*spacingY());
        // Fixed longitude lines shear slightly in the tile Cartesian frame.
        double lon=(x0_+double(i))/scale_-180,lat=(y0_+double(j))/scale_-90;
        double shear=-(lon-frame_.longitude)*std::sin(lat*radians)*radians;
        double magnitude=std::hypot(dx,dy-shear*dx);return magnitude>0?1/magnitude:std::numeric_limits<double>::infinity();
    }
    template<class Inside,class Time>void scan(FFPoint sw,FFPoint ne,Inside inside,Time time){
        auto b=geographicBounds(sw,ne);
        auto low=[](double v,size_t n){return size_t(std::clamp(std::floor(v),1.,double(n-1)));};
        size_t i0=low((b[0]+180)*scale_-x0_,nx_),i1=low((b[2]+180)*scale_-x0_+1,nx_);
        size_t j0=low((b[1]+90)*scale_-y0_,ny_),j1=low((b[3]+90)*scale_-y0_+1,ny_);
        for(size_t i=i0;i<=i1;++i)for(size_t j=j0;j<=j1;++j){
            auto p=vertex(i,j);double t=time(p);
            if((get(i-1,j-1)>t||get(i-1,j)>t||get(i,j-1)>t||get(i,j)>t)&&inside(p)){
                set(i-1,j-1,t);set(i-1,j,t);set(i,j-1,t);set(i,j,t);
            }
        }
    }
};
}
