# experimental_protocol_v1.md

# XSIM İçin Deney Protokolü v1

**Belgenin amacı:** Bu doküman, XSIM solver’ı ile yürütülecek pilot tarama, doğrulama ve final benchmark deneylerini **tekrarlanabilir**, **istatistiksel olarak savunulabilir** ve **literatürle uyumlu** hale getirmek için kullanılacak deney sözleşmesini tanımlar.

---

## 1. Study Objective

Bu çalışmanın amacı, XSIM içindeki **PSO tabanlı hyper-heuristic + ek iyileştirme (özellikle local search / tabu search)** akışının klasik Job Shop Scheduling Problem (JSSP) benchmark instance’larında ne kadar etkili olduğunu değerlendirmektir.

Çalışma üç seviyede hedef tanımlar:

1. **Kalite hedefi:** Makespan (`Cmax`) değerini, ilgili instance için bilinen optimum veya best-known solution (BKS) değerine mümkün olduğunca yaklaştırmak.
2. **Kararlılık hedefi:** Tekil “şanslı koşu” yerine, bir konfigürasyonun farklı seed’lerde benzer kalite üretip üretmediğini ölçmek.
3. **Maliyet hedefi:** Aynı kaliteyi sağlayan konfigürasyonlar arasında runtime bütçesi daha düşük olanı tercih etmek.

### 1.1 Birincil performans ölçütü

Birincil ölçüt, instance bazında **normalized gap** olacaktır:

```text
gap(%) = 100 * (Cmax - reference) / reference
```

Burada `reference`, instance için:
- optimum biliniyorsa **optimum**,
- optimum bilinmiyor ama güncel en iyi üst sınır / BKS biliniyorsa **BKS**
olarak alınacaktır.

### 1.2 İkincil performans ölçütleri

- Best `Cmax`
- Mean gap
- Median gap
- Std / IQR
- Runtime (s)
- Success rate (hedef / reference eşleşmesi veya belirli eşik altı gap)
- Average rank (çoklu instance kıyaslarında)

---

## 2. Problem Class and Benchmark Instances

### 2.1 Problem sınıfı

Bu protokol, **klasik statik JSSP benchmark instance’ları** için geçerlidir.

> Not: Repo içinde `DJSSP_PSO_HH` adı bulunmakla birlikte, bu protokol yalnızca klasik benchmark dosyaları üzerinde çalıştırılan deneyleri kapsar. Dinamik iş gelişi, machine breakdown, online rescheduling veya stochastic processing time gibi unsurlar bu sürümde deney sınıfına dahil edilmemiştir.

### 2.2 Kullanılan benchmark aileleri

Başlangıçta aşağıdaki ailelerden instance’lar kullanılacaktır:

- `ft`
- `la`
- `abz`
- `orb`
- `swv`

### 2.3 Başlangıç pilot havuzu

Şu anda elimizde pilot exploration için kullanılan ana instance seti:

- `ft06`
- `la19`
- `abz7`
- `orb01`
- `swv11`

Bu set **pilot keşif seti** olarak etiketlenecek, final performans iddiası yalnızca bu set üzerinden kurulmayacaktır.

### 2.4 Genişletilmiş benchmark hedefi

Final akademik değerlendirme için benchmark havuzu genişletilecektir. Hedef yapı:

- **Small / easy:** `ft06`, `ft10`
- **Medium:** seçilmiş `la` instance’ları
- **Hard:** seçilmiş `abz`, `orb`, `swv` instance’ları
- **Opsiyonel genişleme:** `ta` ailesinden instance’lar

### 2.5 Reference table zorunluluğu

Her instance için ayrı bir `reference_table.csv` tutulacaktır. Minimum alanlar:

```text
instance,family,jobs,machines,reference_value,reference_type,source
```

Burada `reference_type` alanı şu değerlerden biri olmalıdır:

- `optimum`
- `best_known_solution`

---

## 3. Solver and Execution Contract

### 3.1 Solver kimliği

Her final deney seti şu bilgilerle birlikte dondurulacaktır:

- Repo commit hash
- Binary adı
- Binary SHA256 checksum (opsiyonel ama önerilir)
- Compiler adı ve sürümü
- Build flags
- İşletim sistemi
- CPU bilgisi
- Thread sayısı

### 3.2 Tekrarlanabilir çalıştırma sözleşmesi

Her koşu için aşağıdaki çıktıların üretilmesi beklenir:

- `result.json`
- `metadata.json`
- `schedule.csv`
- varsa `convergence.csv`
- varsa `config.original.json`
- varsa `config.resolved.json`
- varsa `gantt.csv` / `gantt.html`

### 3.3 Çalıştırma biçimi

Solver, komut satırından **black-box** biçimde çağrılacaktır. Tüm tuning ve benchmark araçları solver’ı dışarıdan şu mantıkla kullanacaktır:

```text
solver_binary instance_name [parametreler...]
```

### 3.4 Artifact kayıt zorunluluğu

Her run için en az şu alanlar tutulmalıdır:

```text
run_id
instance
seed
solver params
runtime_sec
cmax
status
feasibility.valid
output paths
```

---

## 4. Pilot Exploration Phase

### 4.1 Tanım

Pilot exploration fazı, geniş sweep / gece taraması / ad-hoc screening gibi daha önce yapılan tüm parametre taramalarını kapsar.

Bu fazın amacı:

- etkili parametreleri tanımak,
- etkisiz parametre bölgelerini elemek,
- runtime patlaması yaratan ayarları görmek,
- problem-özel iyi aday bölgeleri bulmaktır.

### 4.2 Pilot fazın sınırı

Pilot fazdan elde edilen sonuçlar:

- **final akademik performans sonucu olarak raporlanmayacaktır,**
- yalnızca **candidate preset selection** ve **parameter space reduction** için kullanılacaktır.

### 4.3 Pilot veri standardizasyonu

Pilot koşuların tamamı `pilot_results_report.csv` dosyasında birleştirilecektir. Minimum alanlar:

```text
phase
instance
seed
sgs
iters
swarm
evalk
finalk
tsiters
tabu
tsmove
fitavg
traindet
cmax
runtime_sec
reference
reference_type
gap_percent
status
run_id
run_dir
source_file
```

---

## 5. Candidate Configuration Selection

### 5.1 Amaç

Pilot taramalardan sonra her problem ailesi veya kritik instance için **candidate preset** listesi çıkarılacaktır.

### 5.2 Candidate preset nedir?

Candidate preset, final performans iddiası olmayan; fakat ileride validation veya automatic tuning başlangıcı olarak kullanılabilecek, umut verici konfigürasyondur.

### 5.3 Belgeleme formatı

`candidate_presets_v1.md` içinde her preset için şu alanlar bulunmalıdır:

```text
preset_id
instance_scope (tek instance / family)
rationale
params
best_observed_result
observed_runtime
notes
```

### 5.4 Seçim kuralları

Bir parametre kombinasyonu candidate preset olmak için en az şu ölçütlerden ikisini sağlamalıdır:

1. Pilot fazdaki düşük gap bölgelerinden birinde yer almak
2. Benzer kaliteyi daha düşük runtime ile verebilmek
3. Aynı problemde birden fazla trial/seed altında benzer kalite göstermek
4. Başarısızlık / feasibility problemi üretmemek

---

## 6. Tuning / Validation / Test Separation

### 6.1 Temel ilke

Aynı instance’lar hem tuning hem final performans raporu için kullanılmayacaktır.

### 6.2 Faz ayrımı

#### Faz A — Pilot
- geniş sweep
- exploratory screening
- parameter space reduction

#### Faz B — Tuning / Validation
- candidate preset seçimi
- automatic configuration veya yarı-sistematik tuning
- validation bazlı eleme

#### Faz C — Final Test
- tuning’de kullanılmamış instance’lar üzerinde
- dondurulmuş konfigürasyonlarla
- çoklu seed tekrarlarıyla final raporlama

### 6.3 Küçük benchmark havuzunda geçici çözüm

Eğer instance sayısı kısa vadede azsa, aşağıdaki kontrollü kullanım tercih edilecektir:

- Pilot set ayrı tutulur
- Validation set küçük ama sabit tutulur
- Final test seti tuning sürecine **doğrudan** karıştırılmaz

Bu sürümde benchmark havuzu sınırlıysa, raporlama dili şu ifadeyle sınırlandırılabilir:

> “Bu protokol, sınırlı benchmark havuzunda kontrollü validation yaklaşımı uygular; geniş ölçekli genelleme iddiası daha büyük final test seti oluşturulana kadar sınırlıdır.”

---

## 7. Seed Policy

### 7.1 Temel ilke

`seed`, final akademik değerlendirmede **tune edilecek ana parametre** olarak ele alınmayacaktır.

### 7.2 Seed’in rolü

Seed şu amaçlarla kullanılacaktır:

- stochastic tekrarlar üretmek
- konfigürasyonun kararlılığını ölçmek
- rastgelelik etkisini istatistiksel olarak görmek

### 7.3 Uygulama

- Pilot exploration aşamasında seed taranabilir.
- Validation ve final test aşamasında seed, önceden belirlenmiş bir `seed_list.txt` veya benzeri sabit liste üzerinden kullanılacaktır.

### 7.4 Raporlama

Bir konfigürasyon için tek seed sonucu yerine şu özet raporlanacaktır:

- best
- mean
- median
- std / IQR
- success rate

---

## 8. Runtime Budget

### 8.1 Adil karşılaştırma ilkesi

Aynı karşılaştırma grubundaki tüm algoritma / konfigürasyon koşuları aynı bütçe altında değerlendirilmelidir.

### 8.2 Bütçe türleri

Aşağıdaki bütçe türlerinden biri seçilecek ve açıkça sabitlenecektir:

1. **Wall-clock time budget** (önerilen)
2. Iteration budget
3. Hybrid budget (ör. hem max iter hem max time)

### 8.3 Bu protokol için öneri

Öncelik wall-clock bütçesindedir. Çünkü farklı parametre kombinasyonları iteration sayısı aynı olsa da farklı sürelerde çalışabilir.

### 8.4 Kayıt zorunluluğu

Her koşu için `runtime_sec` alanı zorunlu olarak raporlanacaktır.

---

## 9. Baselines and Ablation Variants

### 9.1 Neden gerekli?

Yalnızca “iyi sonuç bulmak” yeterli değildir; hangi bileşenin katkı sağladığını göstermek gerekir.

### 9.2 Minimum baseline seti

Bu protokol kapsamında mümkün olduğunda aşağıdaki baseline / karşılaştırma setleri oluşturulacaktır:

1. **Dispatching baseline**
   - SPT
   - LPT
   - MWKR
   - MOR
   - FIFO
   - SIO
   - PT+WINQ

2. **Metaheuristic varyantları**
   - PSO-HH only
   - PSO-HH + local search
   - PSO-HH + tabu search
   - PSO-HH + tabu + mixed move

### 9.3 Ablation soruları

Aşağıdaki sorular açıkça test edilmelidir:

- PSO katkısı var mı?
- Local search katkısı var mı?
- Tabu search katkısı var mı?
- `tsmove=mixed` gerçekten anlamlı mı?
- Büyük swarm sürekli daha iyi mi?
- Daha büyük iteration veya finalk gerçekten kalite kazandırıyor mu?

---

## 10. Performance Metrics

### 10.1 Zorunlu metrikler

Her benchmark tablosunda en az şu metrikler yer alacaktır:

- Best `Cmax`
- Mean `Cmax`
- Median `Cmax`
- Gap to optimum / BKS (%)
- Runtime mean / median
- Success rate

### 10.2 Opsiyonel ama güçlü metrikler

- Time-to-target
- Anytime curve / convergence analysis
- Average rank
- Wins / ties / losses tablosu

### 10.3 Gap raporlama kuralı

- Optimum biliniyorsa `gap_to_optimum`
- Sadece BKS biliniyorsa `gap_to_BKS`

aynı tabloda karıştırılmayacak, `reference_type` alanıyla birlikte raporlanacaktır.

---

## 11. Statistical Analysis Plan

### 11.1 Temel ilke

Final akademik iddia yalnızca tekil best run’lara değil, çoklu instance ve çoklu seed dağılımlarına dayandırılacaktır.

### 11.2 İki yöntem / iki konfigürasyon karşılaştırması

İki yöntem veya iki final aday konfigürasyon karşılaştırılırken, instance bazlı aggregate performans değerleri üzerinde **Wilcoxon signed-rank test** tercih edilir.

### 11.3 Üç veya daha fazla yöntem karşılaştırması

Üç veya daha fazla algoritma / konfigürasyon / ablation varyantı karşılaştırıldığında **Friedman test** ve uygun post-hoc testler kullanılacaktır.

### 11.4 Görsel raporlama

Mümkünse şu görseller eklenir:

- boxplot / violin plot (gap dağılımı)
- critical difference diagram
- convergence plot

### 11.5 Etki büyüklüğü

P-value raporlamak yeterli değildir; uygun olduğunda effect size veya rank farkı da raporlanacaktır.

---

## 12. Reproducibility Requirements

### 12.1 Zorunlu tekrar üretilebilirlik öğeleri

Aşağıdaki unsurlar final rapora eklenmelidir:

- repo URL
- commit hash
- build script / compiler bilgisi
- benchmark instance listesi
- `reference_table.csv`
- seed listesi
- final run matrix
- kullanılan parametre uzayı
- kullanılan candidate preset listesi

### 12.2 Dosya organizasyonu önerisi

```text
artifacts/
  pilot/
    pilot_results_report.csv
  presets/
    candidate_presets_v1.md
  protocol/
    experimental_protocol_v1.md
  validation/
    validation_run_matrix.csv
  final/
    final_run_matrix.csv
    final_results.csv
    final_stats.md
```

### 12.3 Freeze kuralı

`final_run_matrix.csv` oluşturulduktan sonra:

- solver kodu değiştirilmeyecek,
- benchmark seti değiştirilmeyecek,
- seed listesi değiştirilmeyecek,
- metrik tanımı değiştirilmeyecek,
- parse mantığı değiştirilmeyecek

ve yeni denemeler yalnızca **aynı sözleşme altında** yapılacaktır.

---

## 13. Immediate Next Steps

Bu protokolün kabulünden sonra yürütülecek ilk dosya üretim sırası:

1. `pilot_results_report.csv`
2. `candidate_presets_v1.md`
3. `reference_table.csv`
4. `validation_run_matrix.csv`
5. `final_run_matrix.csv`

---

## 14. Versioning Notes

- Bu belge ilk taslaktır.
- Parametre uzayı ve benchmark havuzu genişledikçe `v2`, `v3` sürümleri üretilebilir.
- Bir sürüm final deney için kullanıldıysa, o sürüm geriye dönük oynatılmayacaktır.
