#ifndef FOREFIRE_FFTILE_DATA_LAYER_H
#define FOREFIRE_FFTILE_DATA_LAYER_H
#include "DataLayer.h"
#include "fftile.h"
#include "PropagationModel.h"
#include "FluxModel.h"
#include <cmath>
#include <stdexcept>

namespace libforefire {
// Views share one paging cache. Fuel IDs are translated through the same
// model fuel tables as FuelDataLayer, not supplied as model properties.
class FFTileDataLayer : public DataLayer<double> {
    std::shared_ptr<FFTileManager> manager_;
    FFPoint origin_, extent_;
    double resolution_;
    template<class Model> size_t fuelProperties(double id, Model* m, size_t pos) {
        for (size_t i = 0; i < m->numFuelProperties; ++i)
            m->properties[pos+i] = (*m->fuelPropertiesTable)(size_t(id), i);
        return m->numFuelProperties;
    }
public:
    FFTileDataLayer(const std::string& name, std::shared_ptr<FFTileManager> manager,
                   FFPoint origin, FFPoint extent)
        : DataLayer<double>(name), manager_(std::move(manager)), origin_(origin), extent_(extent),
          resolution_(name == "fuel" ? 10 : name == "windU" || name == "windV" ? 100 : 50) {}
    double getValueAt(FFPoint p, const double&) override {
        auto s = [&](FFTileField f) { return manager_->sample(f,p.getX(),p.getY()); };
        const auto key = getKey();
        if (key == "fuel") return s(FFTileField::Fuel);
        if (key == "altitude") return s(FFTileField::Altitude);
        if (key == "slope") return std::hypot(s(FFTileField::SlopeX),s(FFTileField::SlopeY));
        if (key == "windU") return s(FFTileField::Muu)*manager_->windU + s(FFTileField::Muv)*manager_->windV;
        return s(FFTileField::Mvu)*manager_->windU + s(FFTileField::Mvv)*manager_->windV;
    }
    double getValueAt(FireNode* fn) override {
        auto p = fn->getLoc();
        if (getKey() != "slope") return getValueAt(p,fn->getTime());
        auto n = fn->getNormal();
        return manager_->sample(FFTileField::SlopeX,p.getX(),p.getY())*n.getVx() +
               manager_->sample(FFTileField::SlopeY,p.getX(),p.getY())*n.getVy();
    }
    size_t getValuesAt(FireNode* fn, PropagationModel* m, size_t p) override {
        if (getKey() == "fuel") return fuelProperties(getValueAt(fn),m,p);
        m->properties[p] = getValueAt(fn); return 1;
    }
    size_t getValuesAt(FFPoint loc, const double& t, FluxModel* m, size_t p) override {
        if (getKey() == "fuel") return fuelProperties(getValueAt(loc,t),m,p);
        m->properties[p] = getValueAt(loc,t); return 1;
    }
    double getDx() override { return resolution_; }
    double getDy() override { return resolution_; }
    double getDz() override { return 0; }
    double getOriginX() override { return origin_.getX(); }
    double getOriginY() override { return origin_.getY(); }
    double getOriginZ() override { return origin_.getZ(); }
    double getWidth() override { return extent_.getX(); }
    double getHeight() override { return extent_.getY(); }
    double getDepth() override { return extent_.getZ(); }
    void setValueAt(FFPoint, double, double) override {
        throw std::runtime_error("fftile: use a case-local fuel PNG or tile override for edits");
    }
    void setMatrix(std::string&, double*, const size_t&, size_t&, const double&) override {
        throw std::runtime_error("fftile: tile views cannot be replaced by a matrix");
    }
    void getMatrix(FFArray<double>** out, const double& t) override {
        size_t nx = std::max(size_t(1),size_t(std::ceil(getWidth()/resolution_)));
        size_t ny = std::max(size_t(1),size_t(std::ceil(getHeight()/resolution_)));
        if (nx > 16000000/ny) throw std::runtime_error("fftile: requested matrix is too large; sample a smaller region");
        *out = new FFArray<double>(getKey(),0.,nx,ny,1,1);
        for (size_t i = 0; i < nx; ++i) for (size_t j = 0; j < ny; ++j)
            (**out)(i,j) = getValueAt(FFPoint(getOriginX()+(i+.5)*getWidth()/nx,
                                            getOriginY()+(j+.5)*getHeight()/ny,0),t);
    }
    std::string print() override { return "fftile " + getKey(); }
    void dumpAsBinary(std::string, const double&, FFPoint&, FFPoint&, size_t&, size_t&) override {
        throw std::runtime_error("fftile: binary layer dumps are not supported; use plot");
    }
};
}
#endif
