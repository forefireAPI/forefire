/**
 * @file main.cpp
 * @brief GRIB2FFNC - A GRIB Dumper Tool
 *
 * GRIB2FFNC is a command-line tool designed to process GRIB2 files, perform data extraction and filtering,
 * and generate visual representations or raw data outputs for processing by forefire.  
 *
 *  A grib dumper, example to make a 1080p t2m out of AROME : grib2ffnc 20241219.00Z.12H.SP1.grib2     -WSEN="-7,40.7,12.2,51.5"  -saveImage="debug{shortName}{level}.raw"  -imageSize=1920,1080 -filter{shortName=10efg}{level=10}  -index="debug{shortName}{level}.csv" -indexParams=forecastTime,stepRange,dataDate
 *  grib2ffnc 20241219.00Z.13H.SP1.grib2  -WSEN="-7,40.7,12.2,51.5"  -saveImage="debug{shortName}{level}.raw"  -imageSize=1920,1080 -filter{shortName=10efg}{level=10}  -index="debug{shortName}{level}.csv" -indexParams=forecastTime,stepRange,dataDate,name 
 *  grib2ffnc 07/01/2022/08/PRECIP_SOL_0.grib -filter{dataDate="20220818"} -WSEN="7,40.7,11.2,43.5" -saveImage="debug{dataDate}{level}.png"
 * @author Jean-Baptiste Filippi
 * @date 01/01/2025
 */

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
#include <iomanip>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../src/stb_image_write.h"
#include "../../src/colormap.h"

/**
 * @brief Prints usage information and exits the program.
 * @param prog The name of the executable.
 */

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
    return "NOTFOUND";
}

static std::string generate_filename(const std::string& pattern, codes_handle* h) {
    std::string filename = pattern;
    size_t pos;
    while ((pos = filename.find('{')) != std::string::npos) {
        size_t end_pos = filename.find('}', pos);
        if (end_pos == std::string::npos) break; // No matching closing brace

        std::string key = filename.substr(pos + 1, end_pos - pos - 1);
        std::string replacement = get_codes_value_as_string(h, key);
        
        // Replace the key enclosed in {} with the actual value from the GRIB message
        filename.replace(pos, end_pos - pos + 1, replacement);
    }
    return filename;
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
                           const std::string& colormapName,
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
                if (!ok || std::isnan(val)) {
                    float fval = std::numeric_limits<float>::quiet_NaN();
                    ofs.write(reinterpret_cast<const char*>(&fval), sizeof(float));
                    } else {
                        // Clamp `val` to ensure it's within [min_val, max_val]
                        if (val < min_val) val = min_val;
                        if (val > max_val) val = max_val;

                        float fval = static_cast<float>(val);
                        ofs.write(reinterpret_cast<const char*>(&fval), sizeof(float));
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
            
            iofs    << bytestart << "," << filename << "," << input_grib << "," << width << "," << height << ","
                    << lngW << "," << latS << "," << lngE << "," << latN << ",float32," 
                    << std::setprecision(std::numeric_limits<double>::max_digits10) 
                    << min_val << ","
                    << std::setprecision(std::numeric_limits<double>::max_digits10) 
                    << max_val;
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
        auto it = colormapMap.find(colormapName);
        const ColorEntry* colorMap = it != colormapMap.end() ? it->second : greyColormap;
        int mapSize = colormapSize; // Assuming all colormaps have the same size

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
                if (val < min_val) val = min_val;
                if (val > max_val) val = max_val;

                int idx = ((height-1 - y) * width + x) * 4;
                if (ok && !std::isnan(val) && val >= min_val && val <= max_val) {
                    
                    int colorIndex = static_cast<int>((val - min_val) / (max_val - min_val) * (mapSize - 1));
                    colorIndex = std::max(0, std::min(colorIndex, mapSize - 1));
                    const auto& color = colorMap[colorIndex];
                    image[idx] = color[0];
                    image[idx + 1] = color[1];
                    image[idx + 2] = color[2];
                    image[idx + 3] = 255;//color[3]; //  last byte is alpha

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
// Normalize longitude to [-180, 180]
double normalizeLon(double lon) {
    while (lon < -180) lon += 360;
    while (lon > 180) lon -= 360;
    return lon;
}

// Function to find the min and max of two longitudes considering the wrapping around the globe
void findMinMaxLon(double lon1, double lon2, double &minLon, double &maxLon) {
    lon1 = normalizeLon(lon1);
    lon2 = normalizeLon(lon2);
    
    // If the difference is more than 180, one is on the opposite side of the globe
    if (std::abs(lon1 - lon2) > 180) {
        if (lon1 < lon2) {
            minLon = lon2;
            maxLon = lon1 + 360;
        } else {
            minLon = lon1;
            maxLon = lon2 + 360;
        }
        
        // Ensure maxLon does not exceed 360
        maxLon = fmod(maxLon, 360.0);
    } else {
        minLon = std::min(lon1, lon2);
        maxLon = std::max(lon1, lon2);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) usage(argv[0]);
    std::string input_grib = argv[1];
    bool do_interpolation = false;
    double interp_lat = 0, interp_lng = 0;
    bool subset_image_flag = false;
    double lngW = 0, latS = 0, lngE = 0, latN = 0;
    std::string saveImage_pattern;
    int image_width = 0, image_height = 0;
    std::map<std::string,std::string> filter_params;
    std::string index_filename;
    std::string cmap = "grey";
    std::vector<std::string> index_keys;

    for (int i=2; i<argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("-latlng") == 0) {
            if (!parse_latlng(arg, interp_lat, interp_lng)) { std::cerr<<"Bad -latlng\n"; return 1; }
            do_interpolation = true;
        } else if (arg.find("-WSEN") == 0) {
            if (!parse_WSEN(arg, lngW, latS, lngE, latN)) { std::cerr<<"Bad -WSEN\n"; return 1; }
            subset_image_flag = true;
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
        } else if (arg.find("-cmap") == 0) {
            cmap = arg.substr(arg.find('=')+1);
        }else if (arg.find("-index=") == 0) {

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

    int msg_count=0, processed=0;

    while ((h = codes_handle_new_from_file(nullptr, grib_file, PRODUCT_GRIB, &err)) != nullptr) {
        msg_count++;
      
        bool skip = false;
        for (std::map<std::string, std::string>::const_iterator it = filter_params.begin(); it != filter_params.end(); ++it) {
            std::string key = it->first;
            std::string value = it->second;
            std::string param_value = get_codes_value_as_string(h, key);
            if (param_value != value) {
                skip = true;
                break; // No need to check further if one condition fails
            }
        }
        
        if (skip) {
            codes_handle_delete(h);

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

  
        
        bool descending=(lat1>lat2); 
        double ddlat=fabs(linc), ddlon=binc;
        double glat1=descending?lat1:lat1, glat2=descending?lat2:lat2;

        if (subset_image_flag == false) {
            double minLon, maxLon;
            findMinMaxLon(lon1, lon2, minLon, maxLon);
            
            lngW = minLon;
            latS = std::min(lat1, lat2); 
            lngE = maxLon;
            latN = std::max(lat1, lat2);

            if (image_width <= 0) {
                image_width = Nj;
            }
            if (image_height <= 0) {
                image_height = Ni;
            }
        } else {
            if (image_width <= 0) {
                image_width = int((lngE - lngW) / binc);
            }
            if (image_height <= 0) {
                image_height = int((latN - latS) / linc);
            }
        }

       std::cout<<"-WSEN\""<<lngW<<","<<latS<<","<<lngE<<","<<latN<<"\" image Size w:"<<image_width<<" h:"<<image_height<<" min:"<<locmin<<" max:"<<locmax<<std::endl;
       
        if (do_interpolation) {
            double iv=0; bool ok=bilinear_interpolation(interp_lat,interp_lng,glat1,glat2,ddlat,lon1,lon1+(Ni-1)*ddlon,ddlon,Ni,Nj,data,iv);
            if (!ok) std::cerr<<input_grib<<"  Interp failed \n";
        }
        if (!saveImage_pattern.empty()) {
            if (Ni>0 && Nj>0 && linc!=0 && binc!=0) {
                std::string fn = generate_filename(saveImage_pattern, h);
                std::string fni = generate_filename(index_filename, h);
                generate_image(fn, data, minv, maxv, lngW, latS, lngE, latN,
                               image_width, image_height, linc, binc,
                               lat1, lon1, ddlat, ddlon, Ni, Nj,
                               input_grib, fni, cmap,index_keys, h);
            } else {
                std::cerr<<"  Invalid grid\n";
            }
        }
        processed++;
        codes_handle_delete(h);
    }
    if (err && err!=GRIB_END_OF_FILE) std::cerr<<"GRIB read error: "<<codes_get_error_message(err)<<"\n";
    fclose(grib_file);
    std::cout<<input_grib<<" total:"<<msg_count<<" processed:"<<processed<<
    "\n";
    if (processed>0)   return 0;
    return 1;
}