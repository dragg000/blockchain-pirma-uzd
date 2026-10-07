# Mokomasis maišos generatorius

Šis projektas yra Vilniaus universiteto blokų grandinių technologijų pirmosios
užduoties realizacija. Tai savarankiškai sukurtas mokomasis algoritmas, o ne
SHA-256, MD5 ar kitos standartinės maišos kopija. Jo negalima naudoti
slaptažodžiams, pinigams ar realioms saugumo sistemoms.

## Paleidimas

Reikalingas C++20 kompiliatorius.

```bash
mkdir -p build
c++ -std=c++20 -O2 -Wall -Wextra -pedantic src/main.cpp -o build/hash_generator

build/hash_generator --text "Lietuva"
build/hash_generator --file kelias/iki/failo
printf 'tekstas' | build/hash_generator --stdin
```

`--text` maišo tiksliai komandų eilutės argumento baitus; papildoma naujos
eilutės žymė nepridedama. UTF-8 tekstas perduodamas jo UTF-8 baitais.
`--file` failą skaito dvejetainiu režimu, todėl išsaugomi visi baitai, tarpai ir
eilučių pabaigos. `--stdin` skirtas dvejetainiam srautui. Neegzistuojantis arba
neperskaitomas failas pateikia klaidos pranešimą ir nenagrinėjamas kaip tuščias
įvesties failas.

Išvestis visada yra 256 bitai, arba 32 baitai, užrašyti 64 mažosiomis
šešioliktainėmis raidėmis. Pradiniai nuliai neišmetami. Įvesties dydį praktiškai
riboja turima atmintis, nes vienas papildomas 64 baitų blokas sukuriamas
lygiavimui.

## Algoritmo idėja

1. Įvesties baitai papildomi `0x80`, nuliais ir 64 bitų įvesties ilgiu.
2. Kiekvienas 64 baitų blokas paverčiamas į 16 didžiųjų baitų tvarkos
   32 bitų žodžių.
3. Žodžių seka išplečiama iki 64 žodžių naudojant `sigmaZero` ir `sigmaOne`.
4. Aštuoni 32 bitų būsenos žodžiai atnaujinami 64 suspaudimo etapais.
   Naudojamos rotacijos, XOR, XNOR, pasirinkimo ir daugumos bitinės operacijos.
5. Aštuoni būsenos žodžiai sujungiami į 32 baitų maišą ir išvedami hex formatu.

Pseudokodas:

```text
hash(baitai):
    baitai = papildyti(baitai)
    būsena = pradinė_būsena
    kiekvienam 64 baitų blokui:
        W[0..15] = blokas
        W[16..63] = sigmaOne(W[t-3]) + W[t-4]
                     + sigmaZero(W[t-10]) + W[t-16]
        a..h = būsena
        64 kartus:
            T1 = h + BigSigmaOne(e) + Choose(a..g) + K[t] + W[t]
            T2 = BigSigmaZero(a) + Majority(a..e)
            paslinkti a..h ir įrašyti T1 + T2
        pridėti a..h prie būsenos
    grąžinti būseną kaip 32 baitus
```

## Testai ir eksperimentai

Pagrindiniai testai tikrina tuščią įvestį, vieno baito įvestis, UTF-8,
determinizmą, failo ir teksto režimų sutapimą bei klaidas:

```bash
python3 -m unittest discover -s tests -v
```

Visi atkuriami eksperimentai paleidžiami taip:

```bash
python3 experiments/run_experiments.py
```

Rezultatai išsaugomi `results/` kataloge:

* `correctness.json` – įvesties, ilgio ir determinizmo patikros;
* `benchmark.csv` ir `benchmark.svg` – bent penkių matavimų spartos lentelė ir
  grafikas; matuojamas tik maišos skaičiavimas, o įvestis paruošiama iš anksto;
* `collisions.csv` – 100 000 skirtingų porų kiekvienam ilgiui 10, 100, 500 ir
  1 000 bei struktūruoti atvejai;
* `avalanche.csv` ir `avalanche_histogram.csv` – iš viso 100 000 vieno ASCII
  simbolio pakeitimo porų, bitų ir hex skirtumų statistika;
* `preimage.csv` – `0000`–`9999` kandidatų perrinkimas be druskos ir su vieša
  druska `VU-2026`.

Eksperimentų generatoriaus `seed` yra `20261007`, ASCII abėcėlė yra
`abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?-_`.
Kompiuterio, operacinės sistemos, kompiliatoriaus ir parinkčių informacija
fiksuojama vykdymo metu kartu su pradiniais CSV duomenimis. Paketinis `--batch`
režimas naudojamas tik eksperimentams, kad 100 000 porų būtų apdorojamos vienu
procesu ir matavimų neiškraipytų programos paleidimo kaina.

Nerastos kolizijos ir maždaug 50 % vidutinis bitų skirtumas nėra saugumo
įrodymas. Eksperimentai neįrodo atsparumo didelėse įvestyse, kriptografinio
atsparumo, slaptumo ar atsparumo specialiai parinktoms atakoms. Mažą kandidatų
rinkinį galima perrinkti net tada, kai maišos išvestis atrodo atsitiktinė.
Vieša druska nepadidina vieno taikinio kandidatų skaičiaus, tačiau neleidžia
aklai pakartotinai naudoti iš anksto apskaičiuotos lentelės skirtingoms
druskoms. Slaptas atsitiktinis `r` pakeistų paieškos erdvę, kol `r` neatskleistas,
bet šis projektas didelės `r` erdvės neperrenka.

## Versijos ir DI naudojimas

`v0.1` buvo pradėta kaip savarankiška algoritmo versija. Šiame etape naudojau
DI asistento pagalbą kodo peržiūrai, testų ir atkuriamų eksperimentų
infrastruktūrai. Priimti pasiūlymai buvo CLI režimai, paketinis apdorojimas,
testai, rezultatų CSV ir SVG išsaugojimas. Kiekvienas pasiūlymas patikrintas
kompiliavimu, vienetiniais testais ir atkuriamais eksperimentais. Standartinė
maišos funkcija nenaudojama pačiam algoritmui.
