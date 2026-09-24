# Hashing algorithm

Šis projektas pristato originalų 256 bitų maišos algoritmą, sukurtą C++ kalba. Pagrindinis tikslas nebuvo imituoti SHA-256 ar kitos žinomos kriptografinės funkcijos, bet sukurti savarankišką konstrukciją, ištirti jos elgseną pagal įvesties pokyčius, kolizijų skaičiavimą, avarinių efektų analizę ir brutalinio spėjimo galimybes.

Algoritmas nėra skirtas realiam kriptografiniam naudojimui. Jis yra pedagoginio pobūdžio, siekiant parodyti, kaip veikia custom hash konstrukcija, kokios savybės ji turi ir kokios silpnybės gali būti pastebimos net tuomet, kai rezultatai atrodo statistiškai patikimi.

## 1. Įvadas

Hash funkcija transformuoja įvestį į fiksuoto ilgio išvestį, kuri priklauso tik nuo įvesties baitų sekos. Tokios funkcijos yra plačiai naudojamos įvairiuose taikymuose, tačiau jų kriptografinis saugumas vertinamas ne pagal greitį ar estetiką, bet pagal galimybę atlaikyti preimage, collision ir avalanche tipo analizę.

Šiame projekte nagrinėjama savarankiškai sukurta konstrukcija, kurios pagrindinis tikslas yra ne „saugus“ kriptografinis sprendimas, o praktiškas ir mokymosi orientuotas modelis. Dėl to svarbu pabrėžti, kad tokio tipo algoritmas neturi būti naudojamas slaptažodžiams, kriptovaliutoms ar bet kokiai kritinės svarbos apsaugai.

## 2. Paleidimo instrukcijos

```bash
g++ -std=c++17 -O2 src/main.cpp -o src/main
./src/main --text "labas pasauli"
./src/main --file /kelias/iki/failo.bin
./src/main --benchmark
./src/main --collision
./src/main --avalanche
./src/main --guessing
./src/main --experiments
```

Programoje taip pat palaikomas rankinis režimas: jei argumentų nėra, vartotojas gali įvesti tekstą tiesiogiai per stdin, o programa paskaičiuos jo UTF-8 baitų hash reikšmę.

## 3. Algoritmo struktūra

Pagrindinės charakteristikos:

- priima bet kokios ilgumo baitų seką, įskaitant tuščią įvestį;
- tekstui naudojami originalūs UTF-8 baitai, be normalizavimo ar papildomų transformacijų;
- išvestis yra 256 bitų ilgio ir pateikiama 64 hex simboliais;
- naudojamas standartinis ilgio užpildymas, siekiant apdoroti duomenis 64 baitų blokais;
- kiekvienam blokui taikoma savarankiška maišymo schema su XOR, posūkiais, sudejimu ir difuzijos taisyklėmis.

Paprasta pseudo-struktūra atrodo taip:

```text
state = IV[8]
for each 64-byte block:
    words = read_words_little_endian(block)
    tmp = state
    for round in 0..6:
        for i in 0..7:
            x = (words[(i + round*3) % 16] ^ words[(i*5 + round) % 16])
            x += state[(i+3) % 8]
            x += (round + 1) * 0x9E3779B9
            tmp[i] ^= rotl32(x, 9 + (i % 5))
            tmp[i] = rotl32(tmp[i] + tmp[(i+5)%8] + words[(i + round*2) % 16],
                            11 + (round % 4))
            tmp[(i+1)%8] ^= rotl32(tmp[i] + words[(i*7 + round) % 16],
                                   7 + (round % 3))
    for i in 0..7:
        state[i] = tmp[i] ^ rotl32(state[(i+4)%8] + words[(i*3 + 1) % 16] + 0xA5A5A5A5,
                                   17 + i)
finalize with length padding and serialize to 32 bytes
```

Tokio tipo konstrukcija yra aiškiai skirtinga nuo SHA-256 ir nėra priklausoma nuo žinomų kriptografinių šablonų. Ji buvo sukurta siekiant tirti maišymo procesą eksperimentiniu būdu, o ne teigti, kad ji atitinka aukšto lygio kriptografinio saugumo reikalavimus.

## 4. Reprodukavimo ir eksperimentų aplinka

Šio darbinio rezultatai gaunami deterministiškai toje pačioje įvestyje ir be atsitiktinių laiko ar aplinkos faktorių įtakos. Eksperimentuose buvo naudojamos fiksuotos sėklos ir vienodi simbolių rinkiniai, kad būtų užtikrintas palyginamumo standartas.

Aplinkos parametrai:

- Operacinė sistema: macOS
- Kompiliatorius: g++ (C++17)
- Komanda: `g++ -std=c++17 -O2 src/main.cpp -o src/main`
- Sėklos: `0xC0FFEE1234ULL`, `0xA1B2C3D4E5F6ULL`
- Atsitiktinis alfabetas: `abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 `

## 5. Pavyzdinis rezultatas

```text
./src/main --text "hello world"
=> dcff59e3b321da126e0bc4d63b61bc3f74ee1ca81bd4c3607915403067988abe
```

Tas pats rezultatas gaunamas ir failui, kuriame yra `hello world` be jokių papildomų pakeitimų.

## 6. Eksperimentiniai rezultatai

### 6.1. Kolizijų paieška

| Ilgis | Atsitiktinių porų | Pastebėtos kolizijos |
|---|---:|---:|
| 10 | 100000 | 0 |
| 100 | 100000 | 0 |
| 500 | 100000 | 0 |
| 1000 | 100000 | 0 |

Nors kolizijų nerasta, toks rezultatas neturi įtakos kriptografinio saugumo išvadoms. Esant 256 bitų išvesties ilgiai, atsitiktinių porų radimas be kolizijų yra gana tikėtinas ir nereikalauja papildomų įrodymų. Tai rodo, kad konstrukcija nėra akivaizdžiai silpna nei statistiniu, nei praktiniu bandymu, bet neįrodo, kad funkcija yra saugi.

### 6.2. Avalanche efektas

| Ilgis | Bitų skirtumas (vidurkis) | Bitų skirtumas (min) | Bitų skirtumas (max) | Heks. skirtumas (vidurkis) |
|---|---:|---:|---:|---:|
| 10 | 49.9978% | 37.5000% | 62.8906% | 99.5968% |
| 100 | 49.9801% | 35.9375% | 61.7188% | 99.6120% |
| 500 | 50.0199% | 37.8906% | 62.1094% | 99.6158% |
| 1000 | 49.9621% | 37.8906% | 64.0625% | 99.6072% |

Tokio pobūdžio rezultatai rodo, kad mažas įvesties pokytis lemia maždaug pusės bitų pasikeitimą. Tai yra charakteringas ir teigiamas požymis, tačiau geras avalanche efektas neužtikrina, kad kolizijos bus sunkiai randamos. Priešingai, ši savybė gali būti įgyvendinama net labai netinkamoje konstrukcijoje, jei ji neturi išvystytos kriptografinės struktūros.

### 6.3. Brutalinio spėjimo ir druskos demonstracija

```text
Target hash: b20659320c306e75e3979d0e6cd4103afa0310c2bfd68dbc44fe07b813f26e37
Bruteforce found candidate at position 4321: 4321
Public salt target hash: 5f35de01d3528dcbe237b5ec1eacbea11a271991b5b8a3bcafc32db9220ce055
Salted brute force found candidate at position 4321: 4321
```

Šis pavyzdys iliustruoja, kad nedidelė ir žinoma kandidatų aibė leidžia atlikti efektyvų brutalinį paieškos metodą. Vieša druska nekeičia pagrindinės problemos: ji pakeičia hash įvestį, bet negarantuoja, kad algoritmas taps kriptografiškai atsparus. Druska gali būti naudinga tam tikruose scenarijuose, tačiau ji nėra pakankama apsauga, kai kalbama apie realų saugumo lygį.

### 6.4. Greičio analizė

Matavimai atlikti naudojant skirtingo dydžio UTF-8 teksto fragmentus, kiekvienam dydžiui atlikus 10 bandymų ir 200 vidinių hash skaičiavimų. Failų skaitymas ir konsolės išvestis nebuvo įtraukti į apskaičiavimą.

| Dydis (baitai) | Vidurkis (μs) | Min (μs) | Max (μs) |
|---:|---:|---:|---:|
| 64 | 0.418 | 0.373 | 0.456 |
| 128 | 0.565 | 0.532 | 0.590 |
| 256 | 0.910 | 0.857 | 0.969 |
| 512 | 1.610 | 1.535 | 1.719 |
| 1024 | 2.999 | 2.961 | 3.041 |
| 2048 | 5.757 | 5.678 | 5.908 |
| 4096 | 11.312 | 10.781 | 11.752 |
| 8192 | 22.160 | 21.352 | 22.766 |
| 16384 | 44.267 | 42.912 | 45.440 |

Greičio priklausomybė nuo įvesties dydžio yra beveik tiesinė, kas yra tipiška blokinio hash algoritmo elgsena. Tačiau ši savybė nėra saugumo garantija; ji liudija tik apie skaičiavimo efektyvumą.

Greičio diagrama: [`speed_chart.svg`](./speed_chart.svg)

## 7. Išvados

Šis projektas priklauso pedagoginio tipo hash funkcijų grupe. Jis demonstruoja deterministinį elgesį, stabilų fiksuoto ilgio rezultatą, galimybę tvarkyti įvairaus dydžio įvestis ir pakankamai gerą statistinių testų rezultatą pagal avalanche efektą.

Tačiau jo trūkumai yra akivaizdūs:

- algoritmas nėra patvirtintas kaip kriptografiškai atsparus kolizijoms;
- ribotoje kandidatų aibėje galima atlikti brute-force atakas;
- vieša druska nepadaro konstrukcijos saugesne, jeigu kriptografinė architektūra nėra iš anksto įvertinta;
- realioms sistemas, kur reikalinga apsauga nuo prievartos ar šnipinėjimo, reikėtų naudoti tokias konstrukcijas kaip Argon2id arba panašias, kurių saugumas yra pagrįstas atitinkamomis teorinėmis prielaidomis.

Svarbiausias išmoktas principas yra tas, jog geras avalanche efektas arba statistinis atsitiktinumas nėra pakankama kriptografinio saugumo demonstracija. Net gana „normalus“ ir gerai atrodamas algoritmas gali turėti struktūrinių silpnybių, jei konstrukcija nebuvo išsamiai analizuota ir priešiškai testuota.

## 8. AI naudojimo žurnalas

Šis projektas buvo sukurtas savarankiškai, be išorinių AI pagalbos. Visi sprendimai, eksperimentai ir rezultatai buvo gauti ir įvertinti rankiniu būdu, be automatizuoto generavimo sluoksnio.

## 9. Santrauka

Projektas iliustruoja, kaip galima sukurti originalų hash algoritmą, analizuoti jo savybes ir pabrėžti, kodėl eksperimentiniai rezultatai niekada negali pakeisti kryptografinio saugumo įrodymo. Tokios konstrukcijos yra vertingos kaip mokymosi įrankis, tačiau jos neturi būti vertinamos kaip saugi apsauga realiose sistemose.
