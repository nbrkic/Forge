#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Multipart.h"
#include <vector>
#include <fstream>
#include "StaticFiles.h"
#include "Auth.h"
#include "Logger.h"
#include "Config.h"

void parsirajMultipart(Request& req){
    if(!req.headers.count("Content-Type") || req.headers.at("Content-Type").find("multipart/form-data") == std::string::npos){
        return;
    }

    std::string ct = req.headers.at("Content-Type");
    size_t boundaryPos = ct.find("boundary=");
    if(boundaryPos == std::string::npos){
        return;
    }
    std::string boundary = ct.substr(boundaryPos + 9);
    std::string delimiter = "--" + boundary;

    std::vector<std::string> delovi;
    size_t pos1 = req.body.find(delimiter);
    if(pos1 == std::string::npos){
        return;
    }
    pos1 += delimiter.length();
    while(true){
        size_t sledeci = req.body.find(delimiter, pos1);
        if(sledeci == std::string::npos) break;
        std::string deo = req.body.substr(pos1, sledeci - pos1);
        delovi.push_back(deo);
        pos1 = sledeci + delimiter.length();
    }

    for(auto& deo : delovi){
        size_t deoPos = 2;
        std::map<std::string, std::string> partHeaders;
        while(true){
            size_t krajLinije = deo.find("\r\n", deoPos);
            std::string linija = deo.substr(deoPos, krajLinije - deoPos);
            if(linija.empty()){
                deoPos = krajLinije + 2;
                break;
            }
            size_t dvotackaPos = linija.find(": ");
            std::string kljuc = linija.substr(0, dvotackaPos);
            std::string vrednost = linija.substr(dvotackaPos + 2);
            partHeaders[kljuc] = vrednost;
            deoPos = krajLinije + 2;
        }

        std::string sadrzaj = deo.substr(deoPos, deo.length() - deoPos - 2);

        std::string disposition = partHeaders["Content-Disposition"];
        size_t namePos = disposition.find("name=\"");
        if(namePos == std::string::npos){
            continue;
        }
        std::string ime = disposition.substr(namePos + 6, disposition.find("\"", namePos + 6) - (namePos + 6));

        size_t filenamePos = disposition.find("filename=\"");
        if(filenamePos != std::string::npos){
            std::string filename = disposition.substr(filenamePos + 10, disposition.find("\"", filenamePos + 10) - (filenamePos + 10));
            UploadFajl fajl;
            fajl.imeFajla = filename;
            fajl.contentType = partHeaders.count("Content-Type") ? partHeaders["Content-Type"] : "application/octet-stream";
            fajl.sadrzaj = sadrzaj;
            req.multipartFajlovi[ime].push_back(fajl);
        }
        else{
            req.multipartPolja[ime] = sadrzaj;
        }
    }
}

static void pisiUBafer(void* context, void* podaci, int velicina){
    std::vector<unsigned char>* bafer = reinterpret_cast<std::vector<unsigned char>*>(context);
    unsigned char* bajtovi = reinterpret_cast<unsigned char*>(podaci);
    bafer->insert(bafer->end(), bajtovi, bajtovi + velicina);
}

std::string sacuvajSliku(const UploadFajl& fajl, const std::string& direktorijum){
    std::string pravaVrsta = proveriStvarniTipSlike(fajl.sadrzaj);
    if(pravaVrsta.empty()){
        return "";
    }

    int sirina, visina, kanali;
    unsigned char* podaci = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(fajl.sadrzaj.data()),
        (int)fajl.sadrzaj.size(), &sirina, &visina, &kanali, 3);

    if(podaci == nullptr){
        log(NivoLoga::WARNING, "Ne mogu da dekodiram sliku za kompresiju.");
        return "";
    }

    std::vector<unsigned char> komprimovano;
    stbi_write_jpg_to_func(pisiUBafer, &komprimovano, sirina, visina, 3, podaci, config.kvalitetSlike);
    stbi_image_free(podaci);

    if(komprimovano.empty()){
        return "";
    }

    std::string imeFajla = generisiSalt() + ".jpg";
    std::ofstream out(direktorijum + imeFajla, std::ios::binary);
    if(!out.is_open()){
        log(NivoLoga::LOG_ERROR, "Ne mogu da sacuvam fajl: " + direktorijum + imeFajla);
        return "";
    }
    out.write(reinterpret_cast<const char*>(komprimovano.data()), komprimovano.size());
    out.close();

    return imeFajla;
}

std::vector<std::string> sacuvajSlike(const std::vector<UploadFajl>& fajlovi, const std::string& direktorijum){
    std::vector<std::string> imenaFajlova;
    for(auto& fajl : fajlovi){
        std::string imeFajla = sacuvajSliku(fajl, direktorijum);
        if(!imeFajla.empty()){
            imenaFajlova.push_back(imeFajla);
        }
    }
    return imenaFajlova;
}
