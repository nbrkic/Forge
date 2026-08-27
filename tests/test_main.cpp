#include <iostream>
#include <string>
#include "Auth.h"
#include "StaticFiles.h"
#include <thread>
#include <vector>
#include "Database.h"
#include "Template.h"

using namespace std;

extern string urlDecode(const string& encoded);
extern string dekodirajChunked(const string& sirovoTelo);

int brojTestova = 0;
int brojProslih = 0;

void assertTrue(bool uslov, const string& opis){
    brojTestova++;
    if(uslov){
        brojProslih++;
        cout << "[OK] " << opis << endl;
    }
    else{
        cout << "[FAIL] " << opis << endl;
    }
}

void assertEqual(const string& ocekivano, const string& dobijeno, const string& opis){
    assertTrue(ocekivano == dobijeno, opis + " (ocekivano: \"" + ocekivano + "\", dobijeno: \"" + dobijeno + "\")");
}

void testUrlDecode(){
    assertEqual("Zdravo svete", urlDecode("Zdravo%20svete"), "urlDecode dekodira %20 kao razmak");
    assertEqual("Zdravo svete", urlDecode("Zdravo+svete"), "urlDecode dekodira + kao razmak");
    assertEqual("a?b", urlDecode("a%3Fb"), "urlDecode dekodira %3F kao ?");
    assertEqual("obican tekst", urlDecode("obican tekst"), "urlDecode ne menja tekst bez posebnih znakova");
}

void testDekodirajChunked(){
    string ulaz = "5\r\nZdrav\r\n2\r\no \r\n6\r\nsvete!\r\n0\r\n\r\n";
    assertEqual("Zdravo svete!", dekodirajChunked(ulaz), "dekodirajChunked spaja chunk-ove u celo telo");
    assertEqual("", dekodirajChunked("0\r\n\r\n"), "dekodirajChunked vraca prazan string za telo bez sadrzaja");
}

void testBezbednoUporedi(){
    assertTrue(bezbednoUporedi("abc123", "abc123"), "bezbednoUporedi vraca true za identicne stringove");
    assertTrue(!bezbednoUporedi("abc123", "abc124"), "bezbednoUporedi vraca false za razlicit sadrzaj");
    assertTrue(!bezbednoUporedi("abc", "abcd"), "bezbednoUporedi vraca false za razlicitu duzinu");
}

void testHesiranjeLozinke(){
    string salt = generisiSalt();
    assertTrue(salt.length() == 32, "generisiSalt vraca 32 hex karaktera (16 bajtova)");

    string drugiSalt = generisiSalt();
    assertTrue(salt != drugiSalt, "generisiSalt generise razlicit salt svaki put");

    string hash = heshirajLozinku("mojaLozinka123", salt);
    assertTrue(proveriLozinku("mojaLozinka123", salt, hash), "proveriLozinku prihvata ispravnu lozinku");
    assertTrue(!proveriLozinku("pogresnaLozinka", salt, hash), "proveriLozinku odbija pogresnu lozinku");
}

void testMimeType(){
   assertEqual("text/html", mimeType("index.html"), "mimeType prepoznaje .html");
    assertEqual("image/png", mimeType("slika.png"), "mimeType prepoznaje .png");
    assertEqual("application/octet-stream", mimeType("fajl.nepoznato"), "mimeType vraca default za nepoznatu ekstenziju");
}

void testEscapeHtml(){
    assertEqual("&lt;script&gt;alert(1)&lt;/script&gt;", escapeHtml("<script>alert(1)</script>"), "escapeHtml enkodira <script> tag");
    assertEqual("Tom &amp; Jerry", escapeHtml("Tom & Jerry"), "escapeHtml enkodira &");
    assertEqual("obican tekst", escapeHtml("obican tekst"), "escapeHtml ne menja tekst bez posebnih znakova");
    assertEqual("&quot;citat&quot; i &#39;apostrof&#39;", escapeHtml("\"citat\" i 'apostrof'"), "escapeHtml enkodira navodnike");
}

void testKonkurentniUpisi(){
    initDatabase();
    izvrsiUpit("DELETE FROM oglasi");

    const int brojNiti = 10;
    std::vector<std::thread> niti;
    for(int i = 0; i < brojNiti; i++){
        niti.push_back(std::thread([i](){
            izvrsiUpit("INSERT INTO oglasi (marka, model, godiste, cena, opis) VALUES (?, ?, ?, ?, ?)",
                {"MarkaTest", "ModelTest", "2020", "1000", "Oglas broj " + std::to_string(i)});
        }));
    }
    for(auto& t : niti){
        t.join();
    }

    auto rezultati = izvrsiSelect("SELECT COUNT(*) as broj FROM oglasi");
    assertEqual(std::to_string(brojNiti), rezultati[0]["broj"], "Svih " + to_string(brojNiti) + " konkurentnih upisa je uspesno zavrseno");
}

int main(){
    testUrlDecode();
    testDekodirajChunked();
    testBezbednoUporedi();
    testHesiranjeLozinke();
    testMimeType();
    testKonkurentniUpisi();
    testEscapeHtml();

    cout << endl << brojProslih << "/" << brojTestova << " testova proslo." << endl;

    return (brojProslih == brojTestova) ? 0 : 1;
}
