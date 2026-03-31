# XSIM

XSIM, job shop scheduling problemleri uzerinde PSO tabanli hyper-heuristic yaklasimi, farkli cizelgeleme kurallari ve ek iyilestirme adimlarini bir araya getiren C++ projesidir. Depoda hem eski tek-dosya yapi korunur, hem de ayni mantigin moduler hale getirilmis surumu bulunur.

Bu repo icinde esas olarak iki kaynak yapisi vardir:

- `Xsimv36.cpp`: Orijinal, tek dosyali ana surum.
- `DJSSP_PSO_HH/`: Daha okunabilir ve parcalara ayrilmis moduler kaynak agaci.

Oncelikli derleme hedefi moduler surum olan `djssp_pso_hh.exe` dosyasidir.

## Proje Icerigi

- `DJSSP_PSO_HH/`: Moduler C++ kaynak kodu.
- `Xsimv36.cpp`: Eski ama korunmus tek-dosya surumu.
- `data/jobshop1.txt`: OR-Library job shop benchmark verisi.
- `build.ps1`: PowerShell ile hizli derleme scripti.
- `hunt_abz7/`, `hunt_la19/`, `hunt_orb01/`, `hunt_swv11/`: Daha once yapilan tarama kosulari, loglar ve bulunan iyi cozumler.
- `yedek/`: Tarihsel surumler ve ara kaynak kopyalari.
- `xsim.exe`, `xsimv2.exe`: Depoda tutulan onceki derlenmis binary dosyalari.

## Moduler Mimari

`DJSSP_PSO_HH/` altindaki ana dosyalar:

- `main.cpp`: Komut satiri girisi, egitim akisi, final kosu ve `experiments` modu.
- `Operation.h`, `Job.h`, `Machine.h`, `Event.h`: Temel veri modelleri.
- `StateFeatures.h`: Durum ozelliklerinin hesaplanmasi.
- `Particle.h`, `Pso.h`, `Pso.cpp`: PSO ve hyper-heuristic katmani.
- `InstanceGenerator.h`, `InstanceGenerator.cpp`: OR-Library instance yukleme ve veri hazirlama.
- `Simulation.h`, `Simulation.cpp`: Simulasyon, dispatching, schedule generation, local search ve tabu search.

## Gereksinimler

Projeyi PowerShell uzerinden derlemek icin sunlar gerekir:

- Windows ortaminda PowerShell 5+ veya PowerShell 7.
- `g++` derleyicisi. C++17 destekli bir MinGW-w64/MSYS2 kurulumu yeterlidir.
- `g++` komutunun `PATH` icinde erisilebilir olmasi.
- `data/jobshop1.txt` dosyasinin repo altinda mevcut olmasi.

Pratik kurulum icin rahat seceneklerden biri MSYS2 + MinGW-w64 kullanmaktir. Kurulumdan sonra `g++.exe` PATH'e eklenmelidir.

Derleyicinin gorunup gorunmedigini kontrol etmek icin:

```powershell
g++ --version
```

Bu komut calismiyorsa once derleyici kurulumu veya PATH duzeltmesi yapilmalidir.

## PowerShell ile Derleme

Repo klasorune girin:

```powershell
cd C:\Users\acer\Documents\.CODE\codex\XSIM
```

Normal derleme:

```powershell
.\build.ps1
```

Eger PowerShell execution policy script calistirmanizi engelliyorsa:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Bu script su komuta denktir:

```powershell
g++ -std=c++17 -O2 -I DJSSP_PSO_HH `
  DJSSP_PSO_HH/main.cpp `
  DJSSP_PSO_HH/InstanceGenerator.cpp `
  DJSSP_PSO_HH/Simulation.cpp `
  DJSSP_PSO_HH/Pso.cpp `
  -o djssp_pso_hh.exe
```

Derleme basarili olursa repo kokunde su dosya olusur:

```text
djssp_pso_hh.exe
```

## Derledikten Sonra Calistirma

Temel kullanim:

```powershell
.\djssp_pso_hh.exe ft06
```

Daha kontrollu bir ornek:

```powershell
.\djssp_pso_hh.exe ft06 --iters 100 --swarm 30 --evalk 5 --finalk 1000 --seed 1
```

Tabu search ile bir ornek:

```powershell
.\djssp_pso_hh.exe orb01 --iters 2000 --swarm 100 --evalk 5 --finalk 200000 --seed 66 --sgs gt --eps0 0 --epsmin 0 --tsiters 60000 --tabu 20 --tsmove mixed
```

Deney modu:

```powershell
.\djssp_pso_hh.exe experiments
```

Yardim ekrani:

```powershell
.\djssp_pso_hh.exe --help
```

## Uretilen Cikti Dosyalari

Tek bir instance kosusundan sonra genellikle su dosya uretilir:

- `gantt_<instance>.csv`: Son elde edilen cizelgenin makine bazli zaman tablosu.

`experiments` modu calistiginda su dosyalar yazilir:

- `raw_results.csv`: Tum tekrarlarin ham sonuclari.
- `summary_results.csv`: Ortalama, standart sapma, min ve max ozetleri.
- `comparison_avg.csv`: Yontemlerin ortalama sonuc karsilastirmasi.
- `comparison_best.csv`: Yontemlerin en iyi sonuc karsilastirmasi.

## Komut Satiri Kullanim Ozeti

```powershell
.\djssp_pso_hh.exe [instance] [flagler]
.\djssp_pso_hh.exe experiments
.\djssp_pso_hh.exe --help
```

`instance` verilmezse varsayilan olarak `ft06` kullanilir.

## Flag Aciklamalari

### Genel Kullanim

- `instance`: Pozisyonel argumandir. Cozulmek istenen OR-Library instance adini belirtir. Ornek: `ft06`, `la19`, `abz7`, `orb01`, `swv11`.
- `experiments`: Tek instance yerine toplu deney modu acilir. Kod icindeki sabit instance listesi icin birden fazla tekrar calistirir ve karsilastirma CSV dosyalari yazar.
- `--help` veya `-h`: Yardim metnini basar ve programdan cikar.

### PSO ve Egitim Flagleri

- `--eps0 <v>`: Egitimde kullanilan baslangic epsilon degeridir. Daha yuksek deger, kural secimlerinde daha fazla rastlantisallik yaratir. Varsayilan: `0.25`.
- `--epsmin <v>`: Iterasyonlar ilerledikce epsilon bu alt sinirin altina dusmez. Varsayilan: `0.05`.
- `--iters <n>`: PSO ana dongusunun kac iterasyon calisacagini belirler. Varsayilan: `30`.
- `--swarm <n>`: Surudeki particle sayisidir. Daha buyuk deger daha fazla arama cesitliligi saglar ama sureyi arttirir. Varsayilan: `15`.
- `--seed <n>`: Rastgelelik tohumudur. Ayni ayarlar ve ayni seed ile tekrar calistirildiginda ayni sonuca daha yakin davranis elde edilir. Varsayilan: `777`.
- `--evalk <n>`: Egitim sirasinda her particle'in kac tekrar uzerinden degerlendirilecegini belirler. Deger arttikca fitness olcumu daha guvenilir ama daha yavas olur. Varsayilan: `5`.
- `--finalk <n>`: Egitim bittikten sonra en iyi agirlik vektorunun final degerlendirmesinde kac tekrar denenecagini belirler. Varsayilan: `200`.
- `--fitavg`: Varsayilan davranis olan "tekrarlardan en iyi sonucu fitness kabul et" yerine, `evalk` tekrarlarinin ortalamasini fitness olarak kullanir.
- `--traindet`: Egitim sirasinda epsilon'u `0` kabul ederek daha deterministik degerlendirme yapar. Ozellikle rastgeleligin etkisini azaltmak istediginizde kullanislidir.

### Schedule Generation ve Simulasyon Flagleri

- `--sgs gt|event`: Cizelge uretim modunu secer. `gt`, Giffler-Thompson tabanli active schedule generator kullanir ve varsayilan secenektir. `event`, event-based simulasyon yolunu kullanir.

### Parametre-Uzayi Iyilestirme Flagleri

- `--lsiters <n>`: PSO'dan sonra bulunan en iyi agirliklar etrafinda ek parametre uzayi local search yapar. `0` verilirse bu adim kapali olur. Varsayilan: `0`.
- `--lsstep <v>`: `--lsiters` aktifse, agirliklara uygulanacak gurultu veya perturbation buyuklugunu kontrol eder. Varsayilan: `0.25`.

### Schedule-Seviyesi Iyilestirme Flagleri

- `--slsiters <n>`: Son olusturulan gantt veya schedule uzerinde local search iterasyon sayisini belirler. Varsayilan: `0`.
- `--tsiters <n>`: Son schedule uzerinde tabu search iterasyon sayisini belirler. Varsayilan: `0`.
- `--tabu <tenure>`: Tabu search kullanildiginda tabu tenure uzunlugunu belirler. Varsayilan: `10`.
- `--tsmove swap|insert|mixed`: Tabu search komsu cozum uretim turunu belirler. `swap` yalniz degistirme, `insert` yalniz araya sokma, `mixed` ise ikisini birlikte kullanir. Varsayilan: `mixed`.

Onemli not: Kod akisi geregi `--tsiters` sifirdan buyukse schedule-level iyilestirmede tabu search devreye girer. Bu durumda `--slsiters` verilmis olsa bile tabu search yolu tercih edilir.

## Ornek Komutlar

Hizli bir smoke test:

```powershell
.\djssp_pso_hh.exe ft06 --iters 1 --swarm 2 --evalk 1 --finalk 1 --seed 1
```

Daha deterministik bir egitim denemesi:

```powershell
.\djssp_pso_hh.exe la19 --iters 500 --swarm 50 --evalk 3 --finalk 5000 --seed 111 --eps0 0 --epsmin 0 --traindet
```

Schedule-level tabu search ile calisma:

```powershell
.\djssp_pso_hh.exe abz7 --iters 2000 --swarm 100 --evalk 5 --finalk 200000 --seed 11 --tsiters 60000 --tabu 18 --tsmove mixed
```

Toplu deney modu:

```powershell
.\djssp_pso_hh.exe experiments
```

## Notlar

- Repo icindeki `xsim.exe` ve `xsimv2.exe` onceki derlenmis binary dosyalaridir. Guncel kaynak akisi icin tavsiye edilen hedef `djssp_pso_hh.exe` dosyasidir.
- Program `data/jobshop1.txt` dosyasina baglidir. Bu dosya silinirse veya yeri degisirse instance yukleme basarisiz olur.
- Derleme scripti yalniz moduler kaynak agacini derler. Orijinal `Xsimv36.cpp` korunur ama varsayilan build akisinin parcasi degildir.
