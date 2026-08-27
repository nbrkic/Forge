#include "Forge.h"

using namespace std;

Response pocetnaStrana(const Request& req){
    return Response(200, "OK", "Zdravo iz Forge frameworka!");
}

int main(){
    routes[{"GET", "/"}] = RutaInfo(pocetnaStrana);

    middlewares.push_back(logRequest);

    pokreniServer(8081);
    return 0;
}
