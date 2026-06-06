# XSIM

<<<<<<< HEAD
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
=======
XSIM, dinamik job shop scheduling problemleri icin gelistirilen config-driven bir C++ scheduling experimentation framework'udur. Proje, PSO tabanli hyper-heuristic yaklasimi, dispatching rule secimi, GT/event schedule generation, opsiyonel local search/tabu search iyilestirmeleri ve run-oriented cikti sozlesmesini tek bir arastirma akisi altinda toplar.

Bu README hem gelistiriciye hem de tez/danisman okuyucusuna hitap edecek sekilde yazilmistir: once sistemin ne yaptigini, sonra nasil derlenip calistirildigini, en sonda da cikti ve dogrulama sozlesmelerini aciklar.

## Current Status

Uygulanan ana yapi:

- Run-oriented output contract: her calisma `runs/<run_id>/` altina yazilir.
- Machine-readable `result.json`, canonical `schedule.csv`, `metadata.json` ve feasibility raporu vardir.
- `config.rules` aktif rule setini `RuleRegistry` uzerinden belirler.
- `config.features` aktif feature listesini `FeatureVectorBuilder` uzerinden belirler.
- `instance.source/name/path` ile OR-Library instance yukleme kontrollu hale getirilmistir.
- `--batch <suite.json>` ile coklu config calistirma ve `batch_summary.csv` uretimi vardir.
- CMake tabanli build yapisi, PowerShell wrapper scriptleri, smoke automation, CTest ve minimal Windows CI workflow'u vardir.

Kasten ertelenenler:

- Full benchmark registry veya BKS database.
- Tam `IStateFeature` plugin mimarisi.
- Objective abstraction.
- Algorithm tuning veya PSO temsil degisikligi.
- Packaging/release sistemi.

## Repository Structure

Onemli klasor ve dosyalar:

- `DJSSP_PSO_HH/`: Moduler C++ cekirdek. Solver, config loader, schedule generators, rules, features, feasibility, batch runner ve CLI burada.
- `configs/`: Tekil run ve batch suite JSON configleri.
- `configs/smoke/`: Kucuk dogrulama configleri.
- `data/jobshop1.txt`: OR-Library job shop benchmark verisi.
- `runs/`: Calismalardan uretilen run ve batch ciktilari.
- `scripts/`: Smoke test otomasyonu.
- `.github/workflows/windows-smoke.yml`: Minimal Windows build + CTest workflow.
- `CMakeLists.txt`: `xsim_core` ve `xsim_cli` hedeflerini tanimlayan CMake dosyasi.
- `build/`: CMake build klasoru. Uretilen bir klasordur.
- `Xsimv36.cpp`: Tarihsel/legacy tek dosya surum. Korunur, varsayilan build akisinin parcasi degildir.

Legacy veya tarihsel parcalar korunmus olabilir. Guncel calisma hedefi moduler kaynak agacindan uretilen `djssp_pso_hh.exe` dosyasidir.

## Build Quickstart

Windows PowerShell ile normal build:
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

<<<<<<< HEAD
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
=======
Script su akisi izler:

1. `cmake` komutunu arar.
2. CMake varsa:

   ```powershell
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```

3. Olusan executable dosyasini repo kokune kopyalar:

   ```text
   .\djssp_pso_hh.exe
   ```

4. CMake bulunamazsa mevcut Windows gelistirme akisinin bozulmamasi icin acik uyariyla legacy `g++` fallback build kullanir.

Static build wrapper:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_static.ps1
```

`build_static.ps1`, CMake varsa `-DXSIM_STATIC_RUNTIME=ON` ile static runtime denemesi yapar. CMake yoksa onceki MinGW static `g++` akisina duser. Tam static baglanti compiler ve platforma baglidir; script sessizce farkli davranmak yerine uyari verir.

## CMake Targets

CMake iki hedef uretir:

- `xsim_core`: Scheduling cekirdegi, config IO, instance loader, rules, features, schedule generators, feasibility, batch runner.
- `xsim_cli`: `main.cpp` uzerinden uretilen CLI executable.

Executable adi mevcut kullanimla uyumludur:
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)

```text
djssp_pso_hh.exe
```

<<<<<<< HEAD
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
=======
## Basic Usage

Tekil run, config ile:

```powershell
.\djssp_pso_hh.exe --config configs/smoke/ft06_smoke.json
```

Batch run:

```powershell
.\djssp_pso_hh.exe --batch configs/benchmark_suite.json
```

Legacy positional CLI hala desteklenir:

```powershell
.\djssp_pso_hh.exe ft06 --iters 1 --swarm 2 --evalk 1 --finalk 1 --seed 1 --no-report
```

Yardim:
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)

```powershell
.\djssp_pso_hh.exe --help
```

<<<<<<< HEAD
## Uretilen Cikti Dosyalari

Tek bir instance kosusundan sonra genellikle su dosya uretilir:

- `gantt_<instance>.csv`: Son elde edilen cizelgenin makine bazli zaman tablosu.
- `reports/<instance>/results_<instance>.csv`: Tekil instance icin PSO-HH ve kurallar arasi tekrar bazli Cmax karsilastirmasi.
- `reports/<instance>/summary_<instance>.csv`: Tekil instance karsilastirmasinin best, mean, worst ve std ozetleri.
- `reports/<instance>/convergence_<instance>.csv`: PSO iterasyonlarindaki iter_best, iter_avg, iter_worst ve BestCmax degerleri.
- `reports/<instance>/gantt_<instance>.csv`: Gantt CSV dosyasinin rapor klasorundeki kopyasi.
- `reports/<instance>/gantt_<instance>.html`: Tarayicida acilabilen Gantt chart gorunumu.

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
- `--no-report`: Tekil instance kosusunda otomatik `reports/<instance>/` raporlarini kapatir.
- `--report-runs <n>`: Tekil instance raporundaki yontem karsilastirmasi icin tekrar sayisini belirler. Varsayilan: `30`.
- `--report-dir <path>`: Tekil instance raporlarinin yazilacagi kok klasoru belirler. Varsayilan: `reports`.

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
=======
Tarihsel deney modu korunur:
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)

```powershell
.\djssp_pso_hh.exe experiments
```

<<<<<<< HEAD
Mevcut bir Gantt CSV dosyasindan HTML chart uretmek icin:

```powershell
python tools\gantt_chart.py reports\la01\gantt_la01.csv --output reports\la01\gantt_la01.html
```

## Notlar

- Repo icindeki `xsim.exe` ve `xsimv2.exe` onceki derlenmis binary dosyalaridir. Guncel kaynak akisi icin tavsiye edilen hedef `djssp_pso_hh.exe` dosyasidir.
- Program `data/jobshop1.txt` dosyasina baglidir. Bu dosya silinirse veya yeri degisirse instance yukleme basarisiz olur.
- Derleme scripti yalniz moduler kaynak agacini derler. Orijinal `Xsimv36.cpp` korunur ama varsayilan build akisinin parcasi degildir.
=======
## Configuration-Driven Runs

Tekil run icin config dosyasi ana alanlari:

- `run`: seed, output root ve report yazimi.
- `instance`: veri kaynagi, instance adi ve OR-Library path.
- `objective`: su an desteklenen hedef `cmax`.
- `solver`: SGS modu, PSO iterasyonlari, swarm, eval/final tekrar sayilari ve epsilon ayarlari.
- `rules`: aktif dispatching rule listesi. Verilmezse default rule set kullanilir.
- `features`: aktif state feature listesi. Verilmezse default feature set kullanilir.
- `improvement`: local search/tabu search ayarlari.
- `outputs`: cikti dosyasi secenekleri.

Minimal config ornegi:

```json
{
  "schema_version": "1.0",
  "run": {
    "name": "ft06_smoke",
    "seed": 1,
    "output_root": "runs",
    "write_reports": false
  },
  "instance": {
    "source": "orlib",
    "name": "ft06",
    "path": "data/jobshop1.txt"
  },
  "objective": "cmax",
  "solver": {
    "method": "pso_hh",
    "sgs": "gt",
    "iters": 1,
    "swarm": 2,
    "evalk": 1,
    "finalk": 1,
    "eps0": 0.25,
    "epsmin": 0.05,
    "fitavg": false,
    "traindet": false
  },
  "rules": ["SPT", "LPT", "MWKR", "MOR", "FIFO", "SIO", "PT+WINQ"],
  "features": ["utilization", "queue_length", "wip", "remaining_work_avg"],
  "improvement": {
    "enabled": false,
    "type": null,
    "slsiters": 0,
    "tsiters": 0,
    "tabu": 10,
    "tsmove": "mixed"
  },
  "outputs": {
    "write_result_json": true,
    "write_schedule_csv": true,
    "write_metadata_json": true,
    "write_convergence_csv": true,
    "write_gantt_html": false
  }
}
```

Notlar:

- Bilinmeyen rule adi sessizce ignore edilmez; program acik hata ile cikar.
- Bilinmeyen feature adi sessizce ignore edilmez; program acik hata ile cikar.
- Gecersiz instance path acik OR-Library hata mesaji ve exit code `1` uretir.

## Single Run Output Contract

Her tekil run icin su formatta bir klasor olusur:

```text
runs/<run_id>/
```

`run_id` ornegi:

```text
20260606_153012_ft06_seed1_cfgA1B2C3D4
```

Run klasoru icerigi:

- `result.json`: machine-readable sonuc, metrics, solver config, active rules/features ve feasibility.
- `schedule.csv`: canonical schedule cikti dosyasi.
- `metadata.json`: timestamp, build, machine, input ve command bilgisi.
- `convergence.csv`: PSO iterasyon ozeti.
- `gantt.csv`: legacy/canonical olmayan ama korunmus Gantt CSV.
- `gantt.html`: tarayicida incelenebilen Gantt gorunumu.
- `config.original.json`: kullanilan config dosyasinin orijinal kopyasi veya fallback snapshot.
- `config.resolved.json`: gercek calismada kullanilan resolve edilmis config.

Minimal `result.json` ornegi:

```json
{
  "schema_version": "1.0",
  "run_id": "20260606_153012_ft06_seed1_cfgA1B2C3D4",
  "instance": "ft06",
  "method": "PSO-HH",
  "status": "success",
  "objective": "cmax",
  "objective_value": 55,
  "metrics": {
    "cmax": 55
  },
  "seed": 1,
  "active_rules": ["SPT", "FIFO"],
  "active_features": ["utilization", "queue_length", "wip", "remaining_work_avg"],
  "feasibility": {
    "valid": true,
    "operation_count_ok": true,
    "duplicate_operation_ok": true,
    "precedence_ok": true,
    "machine_capacity_ok": true,
    "start_end_valid": true,
    "cmax_consistent": true,
    "expected_operation_count": 36,
    "actual_operation_count": 36,
    "schedule_cmax": 55,
    "violations": []
  }
}
```

`schedule.csv` kolonlari:

```text
run_id,instance,job_id,operation_id,machine_id,start,end,processing_time,sequence_index
```

Bu CSV, farkli araclarin log parse etmesine gerek kalmadan Cmax ve operasyon siralamalarini yeniden hesaplayabilmesi icin canonical schedule cikti dosyasidir.

## Batch Runner

Batch suite ornegi:

```json
{
  "schema_version": "1.0",
  "suite_name": "smoke_suite",
  "runs": [
    "configs/smoke/ft06_smoke.json",
    "configs/smoke/ft06_spt.json",
    "configs/smoke/ft06_fifo_mwkr.json"
  ]
}
```

Calistirma:

```powershell
.\djssp_pso_hh.exe --batch configs/benchmark_suite.json
```

Batch ciktisi:

```text
runs/batches/<batch_id>/
```

Ana dosya:

```text
runs/batches/<batch_id>/batch_summary.csv
```

Minimum kolonlar:

```text
batch_id,suite_name,run_id,config_path,instance,status,cmax,runtime_sec,feasibility_valid
```

Batch davranisi:

- Her run mevcut tekil `--config` akisini kullanir.
- Her run kendi `runs/<run_id>/` klasorunu uretir.
- Bir config basarisiz olursa batch crash etmeden mumkunse devam eder.
- Basarisiz run summary'de `status=failed` olarak gorunur.
- Gecersiz batch config dosyasi veya eksik batch dosyasi programi exit code `1` ile durdurur.

## Smoke Tests and CTest

Pozitif smoke test:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_smoke.ps1
```

Bu script sunlari kontrol eder:

- Tekil full config run.
- Tekil subset rule run.
- Tekil subset feature run.
- Batch run.
- `result.json`, `schedule.csv`, `metadata.json`.
- `feasibility.valid == true`.
- `metrics.cmax == feasibility.schedule_cmax`.
- `schedule.csv` satir sayisi beklenen operasyon sayisi ile tutarli.
- Batch icin `batch_summary.csv`.

Negatif smoke test:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_negative_smoke.ps1
```

Bu script sunlari kontrol eder:

- Bilinmeyen rule.
- Bilinmeyen feature.
- Gecersiz instance path.
- Eksik batch config dosyasi.

Her negatif senaryoda hata mesaji gorunur olmali ve process sessizce basarili donmemelidir.

CMake/CTest ile:

```powershell
ctest --test-dir build --output-on-failure -C Release
```

Eger `ctest` PATH'te degilse Windows'ta tam path ile calistirilabilir:

```powershell
& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure -C Release
```

CTest icindeki testler:

- `xsim_smoke_positive`
- `xsim_smoke_negative`

## CI

Minimal GitHub Actions workflow:

```text
.github/workflows/windows-smoke.yml
```

CI akisi:

1. Windows runner.
2. CMake configure.
3. CMake build.
4. CTest smoke.

Bu workflow packaging, release veya buyuk matrix kurmaz. Amac yalnizca build ve output contract smoke seviyesinde bozulma yakalamaktir.

## Legacy Behavior

Asagidaki davranislar bilincli olarak korunur:

- Pozisyonel instance CLI:

  ```powershell
  .\djssp_pso_hh.exe ft06 --iters 1 --swarm 2 --evalk 1 --finalk 1 --seed 1
  ```

- Legacy `gantt_<instance>.csv` cikti dosyasi.
- `reports/<instance>/` altindaki legacy rapor akisi.
- `experiments` modu.
- `Xsimv36.cpp` tarihsel tek dosya kaynak.

Bu parcalar, eski deney loglari ve onceki arastirma akislarinin izlenebilirligi icin korunur. Guncel output contract icin birincil kaynak `runs/<run_id>/` altindaki `result.json`, `schedule.csv` ve `metadata.json` dosyalaridir.

## Practical Examples

Build:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Tekil run:

```powershell
.\djssp_pso_hh.exe --config configs/smoke/ft06_smoke.json
```

Subset rule:

```powershell
.\djssp_pso_hh.exe --config configs/smoke/ft06_spt.json
```

Subset feature:

```powershell
.\djssp_pso_hh.exe --config configs/smoke/ft06_feature_subset.json
```

Event SGS ornegi:

```powershell
.\djssp_pso_hh.exe ft06 --sgs event --iters 1 --swarm 2 --evalk 1 --finalk 1 --seed 1 --no-report
```

Batch:

```powershell
.\djssp_pso_hh.exe --batch configs/benchmark_suite.json
```

Pozitif smoke:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_smoke.ps1
```

Negatif smoke:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_negative_smoke.ps1
```

CTest:

```powershell
ctest --test-dir build --output-on-failure -C Release
```

Gantt HTML helper:

```powershell
python tools\gantt_chart.py reports\ft06\gantt_ft06.csv --output reports\ft06\gantt_ft06.html
```

## Notes for Research Use

XSIM'in guncel yapisi, tek bir optimum deger arayisindan cok deneyin izlenebilirligini hedefler. Bu nedenle:

- Her run kendi klasorunde ayrilir.
- Config orijinal ve resolved olarak saklanir.
- Kullanilan aktif rule ve feature listeleri result/config ciktilarinda gorunur.
- Feasibility sonucu result dosyasina yazilir.
- Batch summary, coklu deneylerin Cmax ve validity durumunu tek CSV'de toplar.

Bu yapi tez calismalarinda tekrarlanabilirlik, danisman kontrolu ve deney sonuclari arasinda izlenebilir karsilastirma icin tasarlanmistir.
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
