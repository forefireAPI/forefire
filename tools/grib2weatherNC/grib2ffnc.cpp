#include <iostream>
#include <vector>
#include <string>
#include <eccodes.h>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <cmath>
#include <limits>
#include <map>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../src/stb_image_write.h"
// A grib dumper, example to make a 1080p t2m out of AROME : grib2ffnc 20241219.00Z.12H.SP1.grib2     -WSEN="-7,40.7,12.2,51.5"  -saveImage="debug{shortName}{level}.raw"  -imageSize=1920,1080 -filter{shortName=10efg}{level=10}  -index="debug{shortName}{level}.csv" -indexParams=forecastTime,stepRange,dataDate

static void usage(const char* prog) {
    std::cout << "Usage: " << prog << " <input.grib> "
              << "[-latlng=\"<latitude>,<longitude>\"] "
              << "[-WSEN=\"<lngW>,<latS>,<lngE>,<latN>\"] "
              << "[-saveImage=\"<pattern>\"] "
              << "[-imageSize=<width>,<height>] "
              << "[-filter{shortName=...}{level=...}] "
              << "[-index=\"<file.csv>\"] "
              << "[-indexParams=key1,key2,...]\n";
    exit(1);
}

static bool parse_latlng(const std::string& option, double& lat, double& lng) {
    size_t eq_pos = option.find('=');
    if (eq_pos == std::string::npos) return false;
    std::string value = option.substr(eq_pos + 1);
    if (!value.empty() && value.front() == '\"' && value.back() == '\"')
        value = value.substr(1, value.size() - 2);
    std::stringstream ss(value);
    std::string lat_str, lng_str;
    if (!std::getline(ss, lat_str, ',')) return false;
    if (!std::getline(ss, lng_str, ',')) return false;
    try { lat = std::stod(lat_str); lng = std::stod(lng_str); }
    catch (...) { return false; }
    return true;
}

static bool parse_WSEN(const std::string& option, double& lngW, double& latS, double& lngE, double& latN) {
    size_t eq_pos = option.find('=');
    if (eq_pos == std::string::npos) return false;
    std::string value = option.substr(eq_pos + 1);
    if (!value.empty() && value.front() == '\"' && value.back() == '\"')
        value = value.substr(1, value.size() - 2);
    std::stringstream ss(value);
    std::string lngW_str, latS_str, lngE_str, latN_str;
    if (!std::getline(ss, lngW_str, ',')) return false;
    if (!std::getline(ss, latS_str, ',')) return false;
    if (!std::getline(ss, lngE_str, ',')) return false;
    if (!std::getline(ss, latN_str, ',')) return false;
    try {
        lngW = std::stod(lngW_str); latS = std::stod(latS_str);
        lngE = std::stod(lngE_str); latN = std::stod(latN_str);
    } catch (...) { return false; }
    return true;
}

static bool parse_imageSize(const std::string& option, int& width, int& height) {
    size_t eq_pos = option.find('=');
    if (eq_pos == std::string::npos) return false;
    std::string value = option.substr(eq_pos + 1);
    if (!value.empty() && value.front() == '(' && value.back() == ')')
        value = value.substr(1, value.size() - 2);
    std::stringstream ss(value);
    std::string width_str, height_str;
    if (!std::getline(ss, width_str, ',')) return false;
    if (!std::getline(ss, height_str, ',')) return false;
    try {
        width = std::stoi(width_str);
        height = std::stoi(height_str);
    } catch (...) { return false; }
    return true;
}

static bool parse_filter(const std::string& arg, std::map<std::string,std::string>& filter_params) {
    if (arg.find("-filter") != 0) return false;
    size_t pos = 7;
    while (true) {
        size_t start_brace = arg.find('{', pos);
        if (start_brace == std::string::npos) break;
        size_t end_brace = arg.find('}', start_brace + 1);
        if (end_brace == std::string::npos) break;
        std::string content = arg.substr(start_brace + 1, end_brace - (start_brace + 1));
        size_t eq_pos = content.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = content.substr(0, eq_pos);
            std::string val = content.substr(eq_pos + 1);
            filter_params[key] = val;
        }
        pos = end_brace + 1;
    }
    return !filter_params.empty();
}

static std::string generate_filename(const std::string& pattern, const std::string& shortName, long level) {
    std::string filename = pattern;
    size_t pos;
    while ((pos = filename.find("{shortName}")) != std::string::npos)
        filename.replace(pos, 11, shortName);
    while ((pos = filename.find("{level}")) != std::string::npos)
        filename.replace(pos, 7, std::to_string(level));
    return filename;
}

static bool bilinear_interpolation(double tlat, double tlng, double lat1, double lat2, double dlat,
                                   double lon1, double lon2, double dlon, int Ni, int Nj,
                                   const std::vector<double>& values, double& interp_value) {
    double relative_lat = lat1 - tlat;
    if (relative_lat < 0 || relative_lat > (Nj - 1) * dlat) return false;
    int j = static_cast<int>(std::floor(relative_lat / dlat));
    double normalized_lng = tlng - lon1;
    while (normalized_lng < 0) normalized_lng += 360;
    while (normalized_lng >= 360) normalized_lng -= 360;
    double grid_lon_extent = (Ni - 1) * dlon;
    if (grid_lon_extent >= 360) grid_lon_extent = 360;
    if (normalized_lng < 0 || normalized_lng > grid_lon_extent) return false;
    int i = static_cast<int>(std::floor(normalized_lng / dlon));
    if (i >= Ni - 1) i = Ni - 2;
    if (j >= Nj - 1) j = Nj - 2;
    double x_frac = (normalized_lng - i * dlon) / dlon;
    double y_frac = (relative_lat - j * dlat) / dlat;
    int iQ11 = j * Ni + i;
    int iQ21 = j * Ni + (i + 1);
    int iQ12 = (j + 1) * Ni + i;
    int iQ22 = (j + 1) * Ni + (i + 1);
    iQ21 %= values.size();
    iQ22 %= values.size();
    double Q11 = values[iQ11], Q21 = values[iQ21];
    double Q12 = values[iQ12], Q22 = values[iQ22];
    if (std::isnan(Q11) || std::isnan(Q21) || std::isnan(Q12) || std::isnan(Q22)) {
        interp_value = std::numeric_limits<double>::quiet_NaN();
        return true;
    }
    interp_value = Q11*(1 - x_frac)*(1 - y_frac) + Q21*x_frac*(1 - y_frac)
                 + Q12*(1 - x_frac)*y_frac       + Q22*x_frac*y_frac;
    return true;
}

static std::string get_codes_value_as_string(codes_handle* h, const std::string& key) {
    char s[256]; size_t len = sizeof(s);
    if (codes_get_string(h, key.c_str(), s, &len) == 0) return s;
    long lval; double dval;
    if (codes_get_long(h, key.c_str(), &lval) == 0) return std::to_string(lval);
    if (codes_get_double(h, key.c_str(), &dval) == 0) return std::to_string(dval);
    return "N/A";
}

static bool file_exists(const std::string& fn) {
    std::ifstream f(fn.c_str());
    return f.good();
}

static bool generate_image(const std::string& filename, const std::vector<double>& values,
                           double min_val, double max_val, double lngW, double latS,
                           double lngE, double latN, int width, int height,
                           double lat_inc, double lon_inc,
                           double lat_first, double lon_first,
                           double dlat, double dlon, int Ni, int Nj,
                           const std::string& input_grib,
                           const std::string& index_filename,
                           const std::vector<std::string>& index_keys,
                           grib_handle* h) 
{
    bool is_raw = false;
    {
        size_t dot_pos = filename.rfind('.');
        if (dot_pos != std::string::npos) {
            std::string ext = filename.substr(dot_pos);
            if (ext == ".raw") is_raw = true;
        }
    }
    if (is_raw) {
        std::ios::openmode mode = std::ios::binary;
        bool already_exists = file_exists(filename);
        if (already_exists) mode |= std::ios::app;
        std::ofstream ofs(filename, mode);
        if (!ofs.is_open()) {
            std::cerr << "Cannot open raw file for writing: " << filename << "\n";
            return false;
        }
        std::streampos bytestart = ofs.tellp();
        double step_lon = (lngE - lngW) / (width - 1);
        double step_lat = (latN - latS) / (height - 1);
        for (int y = 0; y < height; ++y) {
            double clat = latS + y * step_lat;
            for (int x = 0; x < width; ++x) {
                double clng = lngW + x * step_lon;
                double val;
                bool ok = bilinear_interpolation(clat, clng, lat_first, lat_first+(Nj-1)*dlat,
                                                 dlat, lon_first, lon_first+(Ni-1)*dlon, dlon,
                                                 Ni, Nj, values, val);
                if (!ok || std::isnan(val) || val < min_val || val > max_val) {
                    float fval = std::numeric_limits<float>::quiet_NaN();
                    ofs.write(reinterpret_cast<const char*>(&fval), 4);
                } else {
                    float fval = static_cast<float>(val);
                    ofs.write(reinterpret_cast<const char*>(&fval), 4);
                }
            }
        }
        ofs.close();

        if (!index_filename.empty()) {
            bool idx_exists = file_exists(index_filename);
            std::ofstream iofs(index_filename, std::ios::app);
            if (!iofs.is_open()) {
                std::cerr << "Cannot open index file for writing: " << index_filename << "\n";
                return true; // raw is written anyway
            }
            if (!idx_exists) {
                iofs << "bytestart,filename,gribFile,Ni,Nj,lngW,latS,lngE,latN,format,minValue,maxValue";
                for (auto &k: index_keys) iofs << "," << k;
                iofs << "\n";
            }
            iofs << bytestart << "," <<filename<<","<< input_grib << "," << width << "," << height<<","<<
                 lngW<<","<<latS<<","<<lngE<<","<<latN<< ",float32," << min_val << "," << max_val;
            for (auto &k: index_keys) iofs << "," << get_codes_value_as_string(h, k); 
            // We'll need actual handle to retrieve keys; 
            // to keep it simple here, pass them as N/A or do nothing.
            // One can expand the interface to pass codes_handle*, or store them earlier.
            iofs << "\n";
        }
        return true;
    }
    else {
        std::vector<unsigned char> image(width*height*4, 0);
        double step_lon = (lngE - lngW) / (width - 1);
        double step_lat = (latN - latS) / (height - 1);
        for (int y = 0; y < height; ++y) {
            double clat = latS + y * step_lat;
            for (int x = 0; x < width; ++x) {
                double clng = lngW + x * step_lon;
                double val;
                bool ok = bilinear_interpolation(clat, clng, lat_first, lat_first+(Nj-1)*dlat,
                                                 dlat, lon_first, lon_first+(Ni-1)*dlon, dlon,
                                                 Ni, Nj, values, val);
                int idx = ((height-1 - y) * width + x) * 4;
                if (ok && !std::isnan(val) && val >= min_val && val <= max_val) {
                    unsigned char intensity = 0;
                    if (max_val != min_val) {
                        intensity = (unsigned char)(255.0*(val - min_val)/(max_val - min_val));
                    }
                    image[idx+0] = 255 - intensity;
                    image[idx+1] = 255 - intensity;
                    image[idx+2] = 255 - intensity;
                    image[idx+3] = 255;
                } else {
                    image[idx+0] = 0;
                    image[idx+1] = 0;
                    image[idx+2] = 0;
                    image[idx+3] = 0;
                }
            }
        }
        if (stbi_write_png(filename.c_str(), width, height, 4, image.data(), width*4)) {
            std::cout << "PNG image saved: " << filename << "\n";
            return true;
        } else {
            std::cerr << "Failed to write PNG: " << filename << "\n";
            return false;
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) usage(argv[0]);
    std::string input_grib = argv[1];
    bool do_interpolation = false;
    double interp_lat = 0, interp_lng = 0;
    bool generate_image_flag = false;
    double lngW = 0, latS = 0, lngE = 0, latN = 0;
    std::string saveImage_pattern;
    int image_width = 110, image_height = 120;
    std::map<std::string,std::string> filter_params;
    std::string index_filename;
    std::vector<std::string> index_keys;

    for (int i=2; i<argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("-latlng") == 0) {
            if (!parse_latlng(arg, interp_lat, interp_lng)) { std::cerr<<"Bad -latlng\n"; return 1; }
            do_interpolation = true;
        } else if (arg.find("-WSEN") == 0) {
            if (!parse_WSEN(arg, lngW, latS, lngE, latN)) { std::cerr<<"Bad -WSEN\n"; return 1; }
            generate_image_flag = true;
        } else if (arg.find("-saveImage") == 0) {
            size_t eq=arg.find('=');
            if (eq==std::string::npos) { std::cerr<<"Bad -saveImage\n"; return 1; }
            saveImage_pattern = arg.substr(eq+1);
            if (!saveImage_pattern.empty() && saveImage_pattern.front()=='\"' && saveImage_pattern.back()=='\"')
                saveImage_pattern = saveImage_pattern.substr(1, saveImage_pattern.size()-2);
        } else if (arg.find("-imageSize") == 0) {

            if (!parse_imageSize(arg, image_width, image_height)) { std::cerr<<"Bad -imageSize\n"; return 1; }
            if (image_width<=0 || image_height<=0) { std::cerr<<"imageSize must be positive\n"; return 1; }
        } else if (arg.find("-filter") == 0) {
            if (!parse_filter(arg, filter_params)) { std::cerr<<"Bad filter\n"; return 1; }
        } else if (arg.find("-index=") == 0) {

            index_filename = arg.substr(arg.find('=')+1);
            if (!index_filename.empty() && index_filename.front()=='\"' && index_filename.back()=='\"')
                index_filename=index_filename.substr(1,index_filename.size()-2);

        } else if (arg.find("-indexParams=") == 0) {
            std::string params = arg.substr(arg.find('=')+1);
            if (!params.empty() && params.front()=='\"' && params.back()=='\"')
                params=params.substr(1,params.size()-2);
            std::stringstream ss(params);
            std::string item;
            while(std::getline(ss,item,',')) {
                std::cout<<"Index key: "<<item<<"\n";
                index_keys.push_back(item);
            }
        } else {
            std::cerr << "Unknown opt: " << arg << "\n";
            usage(argv[0]);
        }
    }

    FILE* grib_file = fopen(input_grib.c_str(), "rb");
    if (!grib_file) { std::cerr<<"Cannot open "<<input_grib<<"\n"; return 1; }

    codes_handle* h=nullptr; int err=0;
    char short_param[100]; size_t short_len=sizeof(short_param);
    char long_param[200];  size_t long_len=sizeof(long_param);
    long level=0; int msg_count=0, processed=0;

    while ((h = codes_handle_new_from_file(nullptr, grib_file, PRODUCT_GRIB, &err)) != nullptr) {
        msg_count++;
        if (codes_get_string(h, "shortName", short_param, &short_len)!=0) { codes_handle_delete(h); continue; }
        if (codes_get_string(h, "name", long_param, &long_len)!=0)        { std::strcpy(long_param,"N/A"); }
        if (codes_get_long(h, "level", &level)!=0) { level=0; }
        bool skip=false;
        if (filter_params.count("shortName") && filter_params["shortName"]!=short_param) skip=true;
        if (!skip && filter_params.count("level")) {
            long lf=std::stol(filter_params["level"]);
            if (level!=lf) skip=true;
        }
        if (skip) {
            codes_handle_delete(h);
            short_len=sizeof(short_param); long_len=sizeof(long_param);
            continue;
        }
        size_t nvals=0;
        if (codes_get_size(h, "values", &nvals)!=0) { codes_handle_delete(h); continue; }
        std::vector<double> data(nvals);
        if (codes_get_double_array(h,"values",data.data(),&nvals)!=0) { codes_handle_delete(h); continue; }
        for (auto &v:data) {
            if (fabs(v-9999.0)<1e-8) v=std::numeric_limits<double>::quiet_NaN();
        }
        double locmax=-1e30, locmin=1e30;
        for (auto &v:data) {
            if (!std::isnan(v)) { locmax=std::max(locmax,v); locmin=std::min(locmin,v); }
        }
        double maxv=(locmax==-1e30?0:locmax), minv=(locmin==1e30?0:locmin);
        double lat1=0, lon1=0, lat2=0, lon2=0;
        double linc=0, binc=0; long Ni=0, Nj=0;
        codes_get_double(h, "latitudeOfFirstGridPointInDegrees", &lat1);
        codes_get_double(h, "longitudeOfFirstGridPointInDegrees",&lon1);
        codes_get_double(h, "latitudeOfLastGridPointInDegrees",  &lat2);
        codes_get_double(h, "longitudeOfLastGridPointInDegrees", &lon2);
        codes_get_double(h, "jDirectionIncrementInDegrees", &linc);
        codes_get_double(h, "iDirectionIncrementInDegrees", &binc);
        codes_get_long(h,"Ni",&Ni); codes_get_long(h,"Nj",&Nj);
        bool descending=(lat1>lat2); double ddlat=fabs(linc), ddlon=binc;
        double glat1=descending?lat1:lat1, glat2=descending?lat2:lat2;
        std::cout<<"Message "<<msg_count<<":\n"
                 <<"  Param: "<<long_param<<"("<<short_param<<") level:"<<level<<"\n"
                 <<"  min:"<<minv<<" max:"<<maxv<<"\n";
        if (do_interpolation) {
            double iv=0; bool ok=bilinear_interpolation(interp_lat,interp_lng,glat1,glat2,ddlat,lon1,lon1+(Ni-1)*ddlon,ddlon,Ni,Nj,data,iv);
            if (ok) std::cout<<"  Interp("<<interp_lat<<","<<interp_lng<<")="<<iv<<"\n";
            else    std::cout<<"  Interp failed\n";
        }
        if (generate_image_flag && !saveImage_pattern.empty()) {
            if (Ni>0 && Nj>0 && linc!=0 && binc!=0) {
                std::string fn = generate_filename(saveImage_pattern, short_param, level);
                std::string fni = generate_filename(index_filename, short_param, level);
                generate_image(fn, data, minv, maxv, lngW, latS, lngE, latN,
                               image_width, image_height, linc, binc,
                               lat1, lon1, ddlat, ddlon, Ni, Nj,
                               input_grib, fni, index_keys, h);
            } else {
                std::cerr<<"  Invalid grid\n";
            }
        }
        std::cout<<"---------------------------------------\n";
        processed++;
        codes_handle_delete(h);
        short_len=sizeof(short_param); long_len=sizeof(long_param);
    }
    if (err && err!=GRIB_END_OF_FILE) std::cerr<<"GRIB read error: "<<codes_get_error_message(err)<<"\n";
    fclose(grib_file);
    std::cout<<"Messages total:"<<msg_count<<" processed:"<<processed<<"\n";
    return 0;
}