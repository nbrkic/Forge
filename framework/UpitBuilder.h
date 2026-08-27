#pragma once
#include <string>
#include <vector>

class UpitBuilder{
    private:
        std::string sql;
        std::vector<std::string> uslovi;
        std::vector<std::string> parametri;
        int limit = -1;
        int offset = 0;

    public:
        UpitBuilder(const std::string& osnovniUpit) : sql(osnovniUpit) {}

        void dodajUslov(const std::string& uslov, const std::string& vrednost){
            uslovi.push_back(uslov);
            parametri.push_back(vrednost);
        }

        void postaviStranicenje(int brojPoStrani, int stranica){
            if(stranica < 1){
                stranica = 1;
            }
            limit = brojPoStrani;
            offset = (stranica - 1) * brojPoStrani;
        }

        std::string izgradiSql(){
            std::string rezultat = sql;
            if(!uslovi.empty()){
                rezultat += " WHERE ";
                for(size_t i = 0; i < uslovi.size(); i++){
                    if(i > 0){
                        rezultat += " AND ";
                    }
                    rezultat += uslovi[i];
                }
            }
            if(limit >= 0){
                rezultat += " LIMIT ? OFFSET ?";
            }
            return rezultat;
        }

        std::vector<std::string> getParametri(){
            std::vector<std::string> svi = parametri;
            if(limit >= 0){
                svi.push_back(std::to_string(limit));
                svi.push_back(std::to_string(offset));
            }
            return svi;
        }
};
