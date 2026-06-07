# candidate_presets_v1.md

# XSIM İçin Aday Preset Listesi v1

Bu belge, şimdiye kadar yapılan **pilot exploration / gece taramaları / önceki güçlü koşular** içinden öne çıkan konfigürasyonları toplar.

> Önemli not: Bu preset’ler **final akademik sonuç değildir**.
> Bu preset’ler yalnızca:
> - validation koşuları,
> - daraltılmış tuning uzayı,
> - automatic configuration (ör. irace) öncesi başlangıç noktaları
> olarak kullanılacaktır.

---

## Preset Alanları

Her preset için şu alanlar tutulur:

- `preset_id`
- `instance_scope`
- `preset_type`
- `rationale`
- `params`
- `best_observed_result`
- `observed_runtime`
- `status`
- `notes`

---

# 1) ft06

## PRESET_FT06_FAST_GT

- **preset_id:** `PRESET_FT06_FAST_GT`
- **instance_scope:** `ft06`
- **preset_type:** `fast-good`
- **rationale:** Küçük problem için optimum değeri yakalayan, düşük maliyetli ve hızlı aday bölge.
- **params:**
  - `sgs = gt`
  - `iters = 50`
  - `swarm = 20`
  - `evalk = 1`
  - `finalk = 50`
  - `eps0 = 0`
  - `epsmin = 0`
  - `fitavg = true`
  - `traindet = true`
  - `tsiters = 0`
  - `seed = 55`
- **best_observed_result:** `Cmax = 55`
- **observed_runtime:** `ft06 result.json ile ayrıca teyit edilecek`
- **status:** `candidate`
- **notes:** Gece taramasında optimum eşleşmesine ulaşan hızlı aday.

## PRESET_FT06_TABU_HISTORICAL

- **preset_id:** `PRESET_FT06_TABU_HISTORICAL`
- **instance_scope:** `ft06`
- **preset_type:** `historical-strong`
- **rationale:** Önceki raporda optimumu veren tarihsel güçlü varyant.
- **params:**
  - `seed = 55`
  - `tabu_tenure = 5`
  - `ts_limit = 30`
  - `move_mode = mixed`
- **best_observed_result:** `Cmax = 55`
- **observed_runtime:** `historical report only`
- **status:** `candidate`
- **notes:** Küçük problem olduğu için final benchmarkta baseline / sanity check amacıyla kullanılabilir.

---

# 2) la19

## PRESET_LA19_TABU_STRONG_HISTORICAL

- **preset_id:** `PRESET_LA19_TABU_STRONG_HISTORICAL`
- **instance_scope:** `la19`
- **preset_type:** `historical-strong`
- **rationale:** Şimdiye kadar görülen en güçlü la19 sonucu bu varyanttan geldi.
- **params:**
  - `seed = 111`
  - `tabu_tenure = 16`
  - `ts_limit = 120000`
  - `move_mode = mixed`
- **best_observed_result:** `Cmax = 842`
- **observed_runtime:** `historical report only`
- **status:** `priority-candidate`
- **notes:** la19 için en önemli geri dönüş noktası. İkinci faz validation’da mutlaka tekrar denenmeli.

## PRESET_LA19_FAST_REPRODUCIBLE

- **preset_id:** `PRESET_LA19_FAST_REPRODUCIBLE`
- **instance_scope:** `la19`
- **preset_type:** `fast-reproducible`
- **rationale:** Yeni yapıda result.json ile temiz biçimde doğrulanmış ve tekrar üretilebilir güçlü aday.
- **params:**
  - `sgs = gt`
  - `iters = 200`
  - `swarm = 30`
  - `evalk = 3`
  - `finalk = 2000`
  - `eps0 = 0`
  - `epsmin = 0`
  - `fitavg = true`
  - `traindet = true`
  - `tsiters = 0`
  - `seed = 222`
- **best_observed_result:** `Cmax = 928`
- **observed_runtime:** `~0.87s`
- **status:** `candidate`
- **notes:** Hızlı ve yeniden üretilebilir ama tarihsel en iyi sonucun oldukça gerisinde. Validation’da tabu açık varyantlarla birlikte ele alınmalı.

---

# 3) abz7

## PRESET_ABZ7_BALANCED_NIGHT

- **preset_id:** `PRESET_ABZ7_BALANCED_NIGHT`
- **instance_scope:** `abz7`
- **preset_type:** `balanced-night-best`
- **rationale:** Gece taramasında kalite/zaman dengesi açısından en iyi görünen aday.
- **params:**
  - `sgs = gt`
  - `iters = 1000`
  - `swarm = 100`
  - `evalk = 3`
  - `finalk = 50000`
  - `eps0 = 0`
  - `epsmin = 0`
  - `fitavg = true`
  - `traindet = true`
  - `tsiters = 30000`
  - `tabu = 18`
  - `tsmove = mixed`
  - `seed = 11`
- **best_observed_result:** `Cmax = 681`
- **observed_runtime:** `101.276768s`
- **status:** `priority-candidate`
- **notes:** swarm=100 faydalı görünüyor; iters=2000 kalite kazancı getirmedi.

## PRESET_ABZ7_LONG_TABU_HISTORICAL

- **preset_id:** `PRESET_ABZ7_LONG_TABU_HISTORICAL`
- **instance_scope:** `abz7`
- **preset_type:** `historical-strong`
- **rationale:** Önceki raporda daha iyi kalite veren tarihsel ayar.
- **params:**
  - `seed = 11`
  - `tabu_tenure = 18`
  - `ts_limit = 60000`
  - `move_mode = mixed`
- **best_observed_result:** `Cmax = 667`
- **observed_runtime:** `historical report only`
- **status:** `priority-candidate`
- **notes:** Gece koşusundan daha iyi kalite vermiştir; hybrid tuning’de yeniden denenmelidir.

---

# 4) orb01

## PRESET_ORB01_BALANCED_NIGHT

- **preset_id:** `PRESET_ORB01_BALANCED_NIGHT`
- **instance_scope:** `orb01`
- **preset_type:** `balanced-night-best`
- **rationale:** Gece koşusunda en iyi kalite/zaman dengesi gösteren aday.
- **params:**
  - `sgs = gt`
  - `iters = 1000`
  - `swarm = 100`
  - `evalk = 3`
  - `finalk = 50000`
  - `eps0 = 0`
  - `epsmin = 0`
  - `fitavg = true`
  - `traindet = true`
  - `tsiters = 30000`
  - `tabu = 20`
  - `tsmove = mixed`
  - `seed = 66`
- **best_observed_result:** `Cmax = 1084`
- **observed_runtime:** `19.840979s`
- **status:** `priority-candidate`
- **notes:** swarm=100 etkili; iters=2000 kalite kazandırmadı.

## PRESET_ORB01_LONG_TABU_HISTORICAL

- **preset_id:** `PRESET_ORB01_LONG_TABU_HISTORICAL`
- **instance_scope:** `orb01`
- **preset_type:** `historical-strong`
- **rationale:** Önceki raporda daha iyi sonuca ulaşan tarihsel aday.
- **params:**
  - `seed = 66`
  - `tabu_tenure = 20`
  - `ts_limit = 60000`
  - `move_mode = mixed`
- **best_observed_result:** `Cmax = 1064`
- **observed_runtime:** `historical report only`
- **status:** `priority-candidate`
- **notes:** Gece koşusundan daha iyi kalite verdiği için validation setinde mutlaka tekrar değerlendirilmeli.

---

# 5) swv11

## PRESET_SWV11_NIGHT_BEST

- **preset_id:** `PRESET_SWV11_NIGHT_BEST`
- **instance_scope:** `swv11`
- **preset_type:** `night-best`
- **rationale:** Gece taramasında en iyi kaliteyi veren aday ve mevcut en umut verici preset.
- **params:**
  - `sgs = gt`
  - `iters = 500`
  - `swarm = 50`
  - `evalk = 3`
  - `finalk = 10000`
  - `eps0 = 0`
  - `epsmin = 0`
  - `fitavg = true`
  - `traindet = true`
  - `tsiters = 10000`
  - `tabu = 18`
  - `tsmove = mixed`
  - `seed = 77`
- **best_observed_result:** `Cmax = 3351`
- **observed_runtime:** `53.877171s`
- **status:** `priority-candidate`
- **notes:** Daha maliyetli varyantlar kaliteyi artırmadı. Bu instance için seed ve swarm etkileşimi daha kritik görünüyor.

## PRESET_SWV11_SEED88_ALT

- **preset_id:** `PRESET_SWV11_SEED88_ALT`
- **instance_scope:** `swv11`
- **preset_type:** `alternative-robustness`
- **rationale:** Farklı seed bölgesinde daha iyi secondary cluster veren alternatif aday.
- **params:**
  - `sgs = gt`
  - `iters = 500`
  - `swarm = 80`
  - `evalk = 3`
  - `finalk = 10000`
  - `eps0 = 0`
  - `epsmin = 0`
  - `fitavg = true`
  - `traindet = true`
  - `tsiters = 10000`
  - `tabu = 18`
  - `tsmove = mixed`
  - `seed = 88`
- **best_observed_result:** `Cmax = 3375`
- **observed_runtime:** `65.736804s`
- **status:** `candidate`
- **notes:** Mevcut best’ten kötü ama seed-duyarlılığı ve robustness çalışmaları için tutulmalı.

## PRESET_SWV11_HISTORICAL_LONG_TABU

- **preset_id:** `PRESET_SWV11_HISTORICAL_LONG_TABU`
- **instance_scope:** `swv11`
- **preset_type:** `historical-reference`
- **rationale:** Önceki rapordaki güçlü tarihsel referans ayar.
- **params:**
  - `seed = 22`
  - `tabu_tenure = 15`
  - `ts_limit = 60000`
  - `move_mode = mixed`
- **best_observed_result:** `Cmax = 3407`
- **observed_runtime:** `historical report only`
- **status:** `reference-candidate`
- **notes:** Yeni best’in gerisinde kalsa da tarihsel kıyas ve ablation için saklanmalı.

---

# 6) Preset Kullanım Stratejisi

## 6.1 Validation aşaması için öneri

Validation setinde aşağıdaki yaklaşım önerilir:

- `ft06`: 1 hızlı preset + 1 tarihsel preset
- `la19`: 1 hızlı yeniden üretilebilir preset + 1 güçlü tarihsel tabu preset
- `abz7`: 1 gece dengeli preset + 1 tarihsel uzun tabu preset
- `orb01`: 1 gece dengeli preset + 1 tarihsel uzun tabu preset
- `swv11`: 1 mevcut best preset + 1 alternatif seed preset + 1 tarihsel referans preset

## 6.2 Bu dosya ne için kullanılacak?

Bu dosya:
- `validation_run_matrix.csv`
- `final_run_matrix.csv`
- daraltılmış parametre uzayı
- automatic configuration başlangıç noktaları

oluşturulurken referans alınacaktır.

---

# 7) Son Not

Bu listedeki preset’ler **nihai doğrular** değildir.
Ama artık sistematik tuning öncesinde elinizde bulunan en değerli özet bilgi bunlardır.

Bir sonraki adım:

1. `reference_table.csv`
2. `validation_run_matrix.csv`
3. gerekiyorsa `irace` veya yarı-otomatik tuning uzayı

şeklinde ilerlemektir.
