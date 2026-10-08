# Mokomasis maišos generatorius

Šis failas aprašo dvi atskiras projekto versijas. Jų kodas, kūrimo būdas ir
rezultatai nėra maišomi:

- **`v0.1`** – mano savarankiškai sukurta versija, parengta be DI pagalbos.
- **`main`** – atskira versija, kuriai buvo naudojama DI asistento pagalba.

Toliau pateiktas pirmasis skyrius aprašo mano `v0.1` versiją. DI asistuota
`main` versija aprašyta atskirame skyriuje dokumento pabaigoje.

## A. `v0.1` – mano savarankiškas darbas

Ši versija sukurta savarankiškai, be DI pagalbos. Algoritmo kūrimas pradėtas
nuo SHA-256 struktūros nukopijavimo, o vėliau jo komponentai buvo keičiami ir
pritaikomi savam algoritmui.

### A.1. Įvadas

Maišos funkcija kintamo ilgio įvesties baitų seką paverčia fiksuoto ilgio
reikšme. Ta pati baitų seka turi duoti tą patį rezultatą, o nedidelis įvesties
pakeitimas turėtų pakeisti didelę išvesties dalį.

Šio darbo tikslas yra praktiškai išbandyti šias savybes ir parodyti jų
ribotumą. Nerastos kolizijos arba geras lavinos efektas savaime neįrodo
kriptografinio saugumo.

### A.2. Kas pakeista lyginant su SHA-256

Kūrimas pradėtas nuo SHA-256 struktūros, tačiau `v0.1` nėra tiesioginis
standartinio algoritmo iškvietimas. Pagrindiniai pakeitimai yra šie:

- **`sigmaZero`**: SHA-256 naudoja tris operacijas su vienu žodžiu
  (`ROTR(7) XOR ROTR(18) XOR SHR(3)`). Šiame algoritme naudojama
  `ROTL(7)`, `ROTL(13)`, poslinkis į kairę per 5 bitus ir dvigubas XNOR:
  `XNOR(XNOR(ROTL(x, 7), ROTL(x, 13)), x << 5)`.
- **`sigmaOne`**: vietoje SHA-256 `ROTR(17) XOR ROTR(19) XOR SHR(10)`
  naudojama `ROTR(8) XOR ROTR(19) XOR SHR(5)`.
- **Žodžių išplėtimas**: SHA-256 formulė naudoja
  `W[t-2]`, `W[t-7]`, `W[t-15]` ir `W[t-16]`. Čia naudojama pakeista
  formulė `sigmaOne(W[t-3]) + W[t-4] + sigmaZero(W[t-10]) + W[t-16]`.
- **`choose`**: SHA-256 `Ch` funkcija turi tris įvesties žodžius
  (`x`, `y`, `z`). Čia funkcija gauna **šešis** žodžius: `a`, `b`, `c`, `e`,
  `f`, `g`. Lyginiams bitų numeriams naudojami `a,b,c`, nelyginiams –
  `e,f,g`.
- **`majority`**: SHA-256 `Maj` funkcija naudoja tris žodžius. Čia dauguma
  skaičiuojama iš **penkių** žodžių: `a`, `b`, `c`, `e`, `f`. Bitas nustatomas
  į 1, kai bent trys iš penkių atitinkamų bitų yra 1.
- **Didžiosios sigma funkcijos** taip pat pakeistos: naudojamos kairinės
  rotacijos `ROTL(3)`, `ROTL(14)`, `ROTL(25)` ir `ROTL(5)`, `ROTL(11)`,
  `ROTL(27)`, o ne SHA-256 rotacijų rinkiniai.

### Pseudokodas

```text
maiša(įvesties_baitai):
    papildyti įvestį 0x80, nuliais ir pradiniu ilgiu
    būsena H = 8 pradiniai 32 bitų žodžiai

    kiekvienam 64 baitų blokui:
        W[0..15] = bloko žodžiai
        kiekvienam t nuo 16 iki 63:
            W[t] = sigmaOne(W[t-3]) + W[t-4]
                   + sigmaZero(W[t-10]) + W[t-16]

        a,b,c,d,e,f,g,h = H
        kiekvienam t nuo 0 iki 63:
            T1 = h + bigSigmaOne(e)
                 + choose(a,b,c,e,f,g) + K[t] + W[t]
            T2 = bigSigmaZero(a) + majority(a,b,c,e,f)

            h = g; g = f; f = e; e = d + T1
            d = c; c = b; b = a; a = T1 + T2

        H = H + (a,b,c,d,e,f,g,h)

    grąžinti H kaip 32 baitus hex formatu
```

Taigi SHA-256 buvo pradinis atskaitos taškas, tačiau pakeistos būtent
maišymo, žodžių išplėtimo, pasirinkimo ir daugumos operacijos. Dėl to
rezultatas nėra SHA-256 rezultatas ir nėra suderinamas su SHA-256
realizacijomis.

SHA-256 veikimo principą ir pagrindines algoritmo dalis mokiausi iš šio
vaizdo įrašo: [SHA-256 paaiškinimas](https://www.youtube.com/watch?v=orIgy2MjqrA).

### A.3. Paleidimo instrukcijos (`v0.11`)

Reikalingas C++20 kompiliatorius:

```bash
mkdir -p build
c++ -std=c++20 -O2 -Wall -Wextra -pedantic src/main.cpp -o build/hash_generator

build/hash_generator --text "labas pasauli"
build/hash_generator --file /kelias/iki/failo.bin
printf 'tekstas' | build/hash_generator --stdin
```

Programos režimai:

- `--text` maišo komandų eilutės argumento baitus. Papildomas `Enter` simbolis
  nepridedamas.
- `--file` failą skaito dvejetainiu režimu, todėl maišomi tikslūs jo baitai,
  o ne failo pavadinimas.
- `--stdin` skaito tikslų dvejetainį standartinės įvesties srautą.
- eksperimentų scenarijus įvestis perduoda paketais per atskirą palyginimo
  adapterį.

UTF-8 tekstas apdorojamas kaip jo UTF-8 baitai. Tarpai, raidžių registras,
eilučių pabaigos ir kiti baitai nėra normalizuojami. Neegzistuojantis arba
neperskaitomas failas pateikia klaidos pranešimą ir nelaikomas tuščia įvestimi.

Išvestis yra 256 bitų, arba 32 baitų, maiša, užrašyta 64 mažosiomis
šešioliktainėmis raidėmis. Pradiniai nuliai išsaugomi. Įvesties dydį praktiškai
riboja turima operatyvioji atmintis.

### A.4. Testai

Automatiniai testai tikrina:

- tuščią, vieno baito ir UTF-8 įvestį;
- fiksuotą 64 simbolių hex išvesties ilgį;
- determinizmą ir skirtingų įvesčių atskyrimą;
- failo ir teksto režimų sutapimą, kai baitai vienodi;
- dvejetainę `stdin` įvestį;
- klaidos pranešimą neperskaitomo failo atveju.

Paleidimas:

```bash
python3 -m unittest discover -s tests -v
```

### A.5. Reprodukavimo aplinka

Versijų palyginimo scenarijus yra `experiments/compare_versions.py`. Jis
kompiliuoja abi versijas su vienodomis parinktimis ir išsaugo duomenis
`results/` kataloge:

```bash
python3 experiments/compare_versions.py
```

## C. `v0.11` – patobulinta mano versija

`v0.11` yra DI asistuota `v0.1` algoritmo patobulinta versija. `v0.1` išlieka
nepakeistas kaip mano savarankiškas atskaitos taškas, o ši šaka aiškiai
atskiria AI pasiūlytus maišos funkcijos pakeitimus.

Palyginti su `v0.1`, `v0.11`:

- pakeičia pradinių būsenos žodžių rinkinius į naują pirminių skaičių rinkinį;
- pakeičia `bigSigmaZero` ir `bigSigmaOne` rotacijų kombinacijas;
- sustiprina `sigmaZero` ir `sigmaOne`, papildydama jas papildomomis
  rotacijomis;
- pakeičia žodžių išplėtimą į
  `sigmaOne(W[t-2]) + W[t-7] + sigmaZero(W[t-15]) + W[t-16]`;
- pataiso penkių įvesčių `majority` pavadinimus ir tiksliai dokumentuoja,
  kad naudojami `a,b,c,d,e`;
- nebehashina visada tik `"hello"`, o priima `--text`, `--file` ir `--stdin`;
- failus skaito dvejetainiu režimu, todėl nekeičiami jų baitai;
- tikrina neteisingą režimą ir neperskaitomą failą, grąžindama aiškią klaidą;
- vienodai pateikia 256 bitų rezultatą su pradiniais nuliais;
- turi vienodą mašininę sąsają, kurią galima naudoti atkuriamiems bandymams.

`v0.1` ir `v0.11` buvo kompiliuoti tais pačiais parametrais ir išbandyti su
tais pačiais 1000 deterministiškai sugeneruotų įvesčių. Lavinos efektui vienas
baitas kiekvienoje įvestyje pakeistas, o pakeistų išvesties bitų procentas
skaičiuotas nuo 256 bitų.

| Versija | Įvesčių skaičius | Vid. laikas (µs) | Lavinos efektas bitais (vid.) | Hex skirtumas (vid.) |
|---|---:|---:|---:|---:|
| `v0.1` | 1000 | 34.941 | 49.900% | 93.778% |
| `v0.11` | 1000 | 34.235 | 49.977% | 93.906% |

Pilni skaičiavimai pateikti faile
[`results/v01_v011_comparison.csv`](results/v01_v011_comparison.csv), o juos
atkartoja `python3 experiments/compare_versions.py`.

### `v0.1` ir `v0.11` grafikai

**Sparta:**

![v0.1 ir v0.11 spartos palyginimas](results/v01_v011_speed.svg)

**Lavinos efektas:**

![v0.1 ir v0.11 lavinos efekto palyginimas](results/v01_v011_avalanche.svg)

## B. `main` – DI asistuota versija

Šis skyrius aprašo ne mano savarankišką `v0.1` darbą, o atskirą `main`
šakos versiją, kuri buvo kuriama naudojant DI asistento pagalbą. `main`
šakos `README.md` ir `src/main.cpp` yra šios versijos šaltiniai. Šių rezultatų
negalima priskirti mano savarankiškai sukurtai `v0.1` versijai.

### B.1. DI asistuotos versijos struktūra

`main` versijoje įvestis papildoma `0x80`, nuliais ir 64 bitų ilgiu, tada
apdorojama 64 baitų blokais. Ji naudoja kitokią nei `v0.1` būseną, pradines
konstantas ir maišymo konstrukciją:

- kiekvienas blokas paverčiamas į 16 žodžių mažosios baitų tvarkos formatu;
- `mixBlock` atlieka 7 raundus po 8 vidinius atnaujinimus;
- naudojamos rotacijos, XOR, sudėtis ir žodžių indeksavimo schemos;
- galutinė būsena išvedama kaip 32 baitų, 256 bitų rezultatas.

Tai yra DI asistuotos `main` versijos aprašas, o ne `v0.1` algoritmo
paaiškinimas.

### B.2. DI asistuotos versijos eksperimentai

`main` versijai dokumentuoti buvo pateikti kolizijų, lavinos efekto, spartos
ir kandidatų spėjimo eksperimentai. Ši versija taip pat buvo palyginta su MD5,
SHA-1 ir SHA-256. Palyginimui naudotas tikslus
`origin/main:src/main.cpp`, o ne `v0.1` šaltinis:

| Algoritmas | Vid. laikas vienai maišai (µs) | Bitų skirtumas (vid.) | Hex skirtumas (vid.) |
|---|---:|---:|---:|
| `main` (DI) | 8.525 | 49.923% | 93.758% |
| MD5 | 0.736 | 50.126% | 93.603% |
| SHA-1 | 0.395 | 49.831% | 93.503% |
| SHA-256 | 0.400 | 49.824% | 93.519% |

Ši lentelė ir failai `results/standard_speed.csv` bei
`results/standard_avalanche.csv` yra pažymėti kaip **DI asistuotos `main`
versijos rezultatai**. Jie nėra mano savarankiškos `v0.1` versijos
eksperimentų rezultatai.

### B.3. Versijų palyginimas

`v0.1` ir `main` nėra tas pats algoritmas:

| Sritis | `v0.1` – mano darbas | `main` – DI asistuota versija |
|---|---|---|
| Kūrimas | Savarankiškai, be DI | Naudojant DI asistento pagalbą |
| Pagrindinė konstrukcija | Pakeista SHA-256 struktūra | Atskirta `mixBlock` maišymo konstrukcija |
| Išvestis | 256 bitai | 256 bitai |
| Palyginimo rezultatai | Aprašyti A skyriuje | Aprašyti B.2 skyriuje |

Šis atskyrimas leidžia aiškiai nurodyti, kurios idėjos, kodo dalys ir
eksperimentai priklauso mano `v0.1`, o kurios buvo sukurtos DI asistuotoje
`main` versijoje.

Naudotos sąlygos:

- sistema: macOS Darwin 25.6.0 arm64;
- kompiliatorius: Apple clang 17.0.0;
- Python: 3.13.7;
- kompiliavimas: `-std=c++20 -O2 -Wall -Wextra -pedantic`;
- atsitiktinių duomenų `seed`: `20261007`;
- ASCII abėcėlė: `abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?-_`;
- spartos matavimai: 3 apšilimo kvietimai ir 5 matavimai;
- laikas matuojamas `steady_clock` mikrosekundėmis, neįtraukiant failo įvesties
  ir konsolės išvesties.

Visa ši informacija išsaugota faile
[`results/metadata.txt`](results/metadata.txt).

### A.6. Eksperimentų rezultatai

#### A.6.1. Įvesties ir išvesties patikra

`results/correctness.json` patvirtina, kad išbandytos tuščia, vieno baito,
ASCII, UTF-8, eilučių pabaigos ir 256 baitų įvestys. Visais atvejais išvesties
ilgis buvo 64 hex simboliai, o pakartotinis skaičiavimas davė tą pačią reikšmę.

Pavyzdžiai:

| Įvesties baitų skaičius | Maišos ilgis | Maišos reikšmė |
|---:|---:|---|
| 0 | 64 | `56009b816c5b998746edf3cb5a49cc81c9c1181511a79186dedce2305cdb39f2` |
| 1 (`a`) | 64 | `b753271708c8b3c4c2823aeaabfd3728f5a5c240f726fc0f80ceeb321f8e8d46` |
| 7 (`ąžuolas`) | 64 | `5ccd4c32cf1bdb641ac839cc9f70fa3d4bc8de7ce0f5f1386e5e0c8165b9a081` |

#### A.6.2. Kolizijų paieška

Kiekvienam ilgiui sugeneruota ir patikrinta 100 000 skirtingų ASCII porų.
Papildomai tikrintas nedidelis struktūruotų įvesčių rinkinys.

| Įvesties ilgis | Porų skaičius | Porų kolizijos | Viso rinkinio kolizijų grupės |
|---:|---:|---:|---:|
| 10 | 100 000 | 0 | 0 |
| 100 | 100 000 | 0 | 0 |
| 500 | 100 000 | 0 | 0 |
| 1 000 | 100 000 | 0 | 0 |
| Struktūruotos įvestys | 5 | 0 | 0 |

Rezultatai saugomi [`results/collisions.csv`](results/collisions.csv). Kadangi
maišos ilgis yra 256 bitai, tokio dydžio atsitiktiniame bandyme kolizijos
neradimas yra tikėtinas ir neįrodo atsparumo kryptingai atakai.

#### A.6.3. Lavinos efektas

Iš viso patikrinta 100 000 porų, po 25 000 kiekvienam ilgiui. Kiekvienoje
poroje pakeistas vienas ASCII simbolis kitu tos pačios abėcėlės simboliu.

| Ilgis | Bitų skirtumas min. | Bitų skirtumas maks. | Bitų skirtumas vid. | Hex skirtumas min. | Hex skirtumas maks. | Hex skirtumas vid. |
|---:|---:|---:|---:|---:|---:|---:|
| 10 | 37.109% | 62.109% | 49.965% | 76.563% | 100.000% | 93.734% |
| 100 | 36.719% | 62.109% | 50.029% | 78.125% | 100.000% | 93.769% |
| 500 | 37.891% | 62.891% | 50.011% | 79.688% | 100.000% | 93.777% |
| 1 000 | 37.500% | 61.719% | 50.012% | 79.688% | 100.000% | 93.747% |

Vidutinis bitų skirtumas yra apie 50 %, o hex simbolių skirtumas – apie
93,75 %. Tai atitinka orientacines nepriklausomų atsitiktinių išvesčių
reikšmes. Bitų skirtumo histograma pateikta
[`results/avalanche_histogram.csv`](results/avalanche_histogram.csv), o visos
statistikos – [`results/avalanche.csv`](results/avalanche.csv).

Geras lavinos efektas gali egzistuoti ir konstrukcijoje, kuri turi kitų
struktūrinių silpnybių, todėl šis eksperimentas nėra saugumo įrodymas.

#### A.6.4. Spartos analizė

Naudotas fiksuotas sugeneruotas UTF-8 tekstas. Buvo matuojamos 1, 2, 4, 8, ir
taip toliau eilučių ištraukos bei visas failas. Įvestis buvo paruošta prieš
matavimą, atlikti 3 apšilimo ir 5 matavimo kvietimai.

| Baitai | Eilutės | Vidurkis (µs) | Min. (µs) | Maks. (µs) |
|---:|---:|---:|---:|---:|
| 85 | 1 | 6.175 | 6.125 | 6.292 |
| 680 | 8 | 25.892 | 25.250 | 27.958 |
| 5 440 | 64 | 151.367 | 151.166 | 151.709 |
| 43 520 | 512 | 879.250 | 855.125 | 922.209 |
| 87 040 | 1 024 | 1 557.200 | 1 495.420 | 1 604.920 |
| 174 080 | 2 048 | 2 771.390 | 2 594.620 | 2 914.620 |

Pilna lentelė pateikta [`results/benchmark.csv`](results/benchmark.csv), o
grafikas – [`results/benchmark.svg`](results/benchmark.svg). Didėjant įvesties
dydžiui laikas auga beveik tiesiškai, nes didėja apdorojamų 64 baitų blokų
skaičius. Mažų įvesčių matavimus labiau veikia pastovios funkcijos sąnaudos.

#### A.6.5. Spėjimas, vieša druska ir slaptas atsitiktinumas

Tikslinė įvestis buvo `0420`, o kandidatų rinkinį sudarė visos eilutės nuo
`0000` iki `9999`. Be druskos ir su vieša druska `VU-2026` rasta po vieną
sutampantį kandidatą:

| Režimas | Kandidatų | Sutapę kandidatai | Laikas (s) |
|---|---:|---|---:|
| Be druskos | 10 000 | `0420` | 0.026895 |
| Vieša druska `VU-2026` | 10 000 | `0420` | 0.027444 |

Rezultatai saugomi [`results/preimage.csv`](results/preimage.csv). Mažas
kandidatų rinkinys gali būti perrenkamas net ir tada, kai maišos išvestis atrodo
atsitiktinė. Vieša druska nekeičia vieno taikinio paieškos dydžio, tačiau
neleidžia aklai pakartotinai naudoti iš anksto apskaičiuotų rezultatų kitai
druskai. Slaptas atsitiktinis `r` padidintų paieškos erdvę tol, kol `r` nebūtų
atskleistas; šiame darbe didelės `r` erdvės perrinkimas neatliekamas.

#### A.6.6. Išvados

Eksperimentai patvirtino, kad programa yra deterministinė, grąžina fiksuoto
ilgio rezultatą, apdoroja skirtingo dydžio baitų sekas ir rodo gerą statistinį
lavinos efektą. Spartos priklausomybė nuo įvesties dydžio yra beveik tiesinė.

Tačiau testai neįrodo atsparumo kryptingoms kolizijų ar pirmavaizdžio atakoms,
neįrodo, kad konstrukcija nenutekina informacijos, ir neįvertina visų galimų
įvesčių. Nenustatyta kolizija tik reiškia, kad jos nerasta naudotame ribotame
rinkinyje. Realioms sistemoms turi būti naudojamos patikrintos kriptografinės
konstrukcijos, o slaptažodžiams – specializuotos schemos, pavyzdžiui,
`Argon2id`.

#### A.6.7. DI versijos palyginimas su standartinėmis maišomis

Papildomai `main` šakos DI versija palyginta su standartinėmis MD5, SHA-1 ir
SHA-256 realizacijomis. Naudotas tikslus `origin/main:src/main.cpp`, todėl
lyginama ne su `v0.1`, o su AI asistuota `main` realizacija. Visos keturios funkcijos gavo tas pačias 1 000 atsitiktinių
ASCII įvesčių: po 250 įvesčių, kurių ilgis buvo 10, 100, 500 ir 1 000 baitų.
Sparta matuota 5 kartus, o kiekvieno matavimo metu apdorotos visos įvestys.
Lavinos efektui kiekvienoje poroje pakeistas vienas simbolis, o bitų skirtumas
normalizuotas pagal konkrečios funkcijos išvesties ilgį.

Rezultatai atkuriami:

```bash
python3 experiments/standard_compare.py
```

| Algoritmas | Išvestis | Vid. laikas vienai maišai (µs) | Bitų skirtumas (vid.) | Hex skirtumas (vid.) |
|---|---:|---:|---:|---:|
| main (DI) | 256 bitai | 8.525 | 49.923% | 93.758% |
| MD5 | 128 bitų | 0.736 | 50.126% | 93.603% |
| SHA-1 | 160 bitų | 0.395 | 49.831% | 93.503% |
| SHA-256 | 256 bitai | 0.400 | 49.824% | 93.519% |

Standartinės funkcijos skaičiuojamos Python `hashlib` bibliotekoje, o `main (DI)`
skaičiuojama iš `origin/main:src/main.cpp` C++ realizacijos. Dėl skirtingų vykdymo aplinkų
spartos skaičiai yra orientaciniai, todėl svarbiausia išlaikyta vienoda įvesčių
imtis ir vienodas matavimų skaičius. Palyginimas neįrodo `v0.1` saugumo.

### A.7. Rezultatų atkūrimas

Visi pradiniai matavimai, statistikos ir grafikas laikomi repozitorijoje:

- [`results/correctness.json`](results/correctness.json)
- [`results/benchmark.csv`](results/benchmark.csv)
- [`results/benchmark.svg`](results/benchmark.svg)
- [`results/collisions.csv`](results/collisions.csv)
- [`results/avalanche.csv`](results/avalanche.csv)
- [`results/avalanche_histogram.csv`](results/avalanche_histogram.csv)
- [`results/preimage.csv`](results/preimage.csv)
- [`results/standard_speed.csv`](results/standard_speed.csv)
- [`results/standard_avalanche.csv`](results/standard_avalanche.csv)
- [`results/metadata.txt`](results/metadata.txt)

Eksperimentai atkuriami viena komanda:

```bash
python3 experiments/run_experiments.py
```
