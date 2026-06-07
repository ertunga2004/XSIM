# final_stats.md

# XSIM Final Benchmark Statistics Report (Template)

> **Belge tipi:** Final deney raporu şablonu  
> **Amaç:** `final_run_matrix.csv` ile yürütülen nihai koşuların sonuçlarını, karşılaştırmalarını ve istatistiksel analizini standart bir yapıda toplamak.

---

## 1. Report Metadata

- **Report version:** `v1`
- **Date generated:** `YYYY-MM-DD`
- **Prepared by:** `NAME`
- **Project:** `XSIM`
- **Protocol version:** `experimental_protocol_v1.md`
- **Preset source:** `candidate_presets_v1.md`
- **Reference source:** `reference_table.csv`
- **Run matrix source:** `final_run_matrix.csv`
- **Results source:** `final_results.csv` / `aggregated_results.csv`

---

## 2. Reproducibility Block

### 2.1 Repository and build

- **Repository URL:** `...`
- **Commit hash:** `...`
- **Binary name:** `djssp_pso_hh.exe`
- **Binary checksum (optional):** `...`
- **Compiler:** `...`
- **Compiler version:** `...`
- **Build flags:** `...`
- **Build script:** `build.ps1` / `build_static.ps1`

### 2.2 Runtime environment

- **OS:** `...`
- **CPU:** `...`
- **Threads used:** `...`
- **RAM:** `...`
- **Timezone:** `...`

### 2.3 Experimental controls

- **Termination mode:** `wall_clock_time` / `iteration_budget` / `hybrid`
- **Seed list:** `...`
- **Reference type policy:** `optimum / BKS`
- **Gap formula:**

```text
gap(%) = 100 * (Cmax - reference) / reference
```

---

## 3. Benchmark Scope

### 3.1 Included instances

Aşağıdaki tablo final değerlendirmeye dahil edilen instance’ları listeler.

| Instance | Family | Jobs | Machines | Reference | Reference Type | Notes |
|---|---:|---:|---:|---:|---|---|
| `ft06` | `ft` | `...` | `...` | `...` | `optimum/BKS` | `...` |
| `la19` | `la` | `...` | `...` | `...` | `optimum/BKS` | `...` |
| `abz7` | `abz` | `...` | `...` | `...` | `optimum/BKS` | `...` |
| `orb01` | `orb` | `...` | `...` | `...` | `optimum/BKS` | `...` |
| `swv11` | `swv` | `...` | `...` | `...` | `optimum/BKS` | `...` |

### 3.2 Final candidate set

Aşağıdaki preset’ler final benchmarkta test edildi:

- `PRESET_...`
- `PRESET_...`
- `PRESET_...`

---

## 4. Data Quality and Run Integrity Checks

Bu bölümde final koşuların bütünlüğü kontrol edilir.

### 4.1 Expected vs completed runs

- **Expected run count:** `...`
- **Completed run count:** `...`
- **Missing runs:** `...`
- **Failed runs:** `...`
- **Feasible runs:** `...`
- **Infeasible runs:** `...`

### 4.2 Integrity checks

| Check | Result | Notes |
|---|---|---|
| `result.json` exists for all runs | `PASS/FAIL` | `...` |
| `metadata.json` exists for all runs | `PASS/FAIL` | `...` |
| `schedule_cmax == metrics.cmax` | `PASS/FAIL` | `...` |
| `feasibility.valid` all expected true | `PASS/FAIL` | `...` |
| `run_id` uniqueness | `PASS/FAIL` | `...` |
| No duplicate rows in aggregated results | `PASS/FAIL` | `...` |

### 4.3 Exclusions

Aşağıdaki koşular rapordan çıkarıldıysa burada listelenir:

| Run ID | Instance | Reason |
|---|---|---|
| `...` | `...` | `...` |

---

## 5. Per-Run Raw Summary

Bu bölüm, final koşuların ham özetinin bulunduğu dosyaya referans verir.

- **Raw run table file:** `final_results.csv`
- **Grouped summary file:** `aggregated_results.csv`

İstenirse küçük özet tablo eklenir:

| Instance | Preset ID | Seed | Cmax | Gap % | Runtime (s) | Status |
|---|---|---:|---:|---:|---:|---|
| `...` | `...` | `...` | `...` | `...` | `...` | `...` |

---

## 6. Aggregated Performance Summary

Bu bölüm ana değerlendirme bölümüdür.

### 6.1 Per instance / per preset summary

| Instance | Preset ID | Best Cmax | Mean Cmax | Median Cmax | Std Cmax | Best Gap % | Mean Gap % | Median Gap % | Runtime Mean (s) | Success Rate |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `...` | `...` | `...` | `...` | `...` | `...` | `...` | `...` | `...` | `...` | `...` |

### 6.2 Primary preset summary

Sadece `candidate_tier = primary` olan preset’ler için özet sini burada ver.

| Instance | Primary Preset | Best Cmax | Mean Gap % | Runtime Mean (s) | Notes |
|---|---|---:|---:|---:|---|
| `...` | `...` | `...` | `...` | `...` | `...` |

### 6.3 Challenger comparison summary

Primary ve challenger preset’lerin kısa kıyas özeti:

| Instance | Primary Preset | Challenger Preset | Primary Mean Gap % | Challenger Mean Gap % | Better Configuration |
|---|---|---|---:|---:|---|
| `...` | `...` | `...` | `...` | `...` | `...` |

---

## 7. Gap-to-Reference Analysis

### 7.1 Gap interpretation rules

- `reference_type = optimum` ise: **gap_to_optimum** olarak yorumlanır.
- `reference_type = best_known_solution` ise: **gap_to_BKS** olarak yorumlanır.

### 7.2 Final gap table

| Instance | Reference | Ref Type | Best Found | Best Gap % | Mean Gap % | Comment |
|---|---:|---|---:|---:|---:|---|
| `...` | `...` | `...` | `...` | `...` | `...` | `...` |

### 7.3 Overall gap summary

- **Macro-average mean gap (%):** `...`
- **Macro-average median gap (%):** `...`
- **Best overall gap-achievement instance:** `...`
- **Worst overall gap-achievement instance:** `...`

---

## 8. Runtime Analysis

### 8.1 Runtime summary table

| Instance | Preset ID | Runtime Min (s) | Runtime Mean (s) | Runtime Median (s) | Runtime Max (s) | Notes |
|---|---|---:|---:|---:|---:|---|
| `...` | `...` | `...` | `...` | `...` | `...` | `...` |

### 8.2 Runtime commentary

Bu bölümde şu sorulara cevap ver:

- Daha uzun runtime her zaman daha iyi kalite getirdi mi?
- Challenger preset’lerin runtime/quality trade-off’u nasıldı?
- Runtime bütçesi bazı instance’larda gereksiz yüksek miydi?

---

## 9. Robustness Analysis

Bu bölüm, seed değişkenliği karşısında preset’lerin ne kadar kararlı olduğunu analiz eder.

### 9.1 Distribution summary

| Instance | Preset ID | Mean Gap % | Std Gap % | IQR Gap % | Mean Cmax | Std Cmax | Interpretation |
|---|---|---:|---:|---:|---:|---:|---|
| `...` | `...` | `...` | `...` | `...` | `...` | `...` | `...` |

### 9.2 Robustness notes

- En kararlı preset: `...`
- En kararsız preset: `...`
- Seed değişkenliğine en duyarlı instance: `...`
- Seed değişkenliğine en dayanıklı instance: `...`

---

## 10. Baseline and Ablation Comparison

> Bu bölüm yalnızca baseline / ablation deneyleri gerçekten yapıldıysa doldurulmalıdır.

### 10.1 Included variants

- `Dispatching baseline`
- `PSO-HH only`
- `PSO-HH + local search`
- `PSO-HH + tabu`
- `PSO-HH + tabu + mixed move`

### 10.2 Comparison table

| Variant | Best Gap % | Mean Gap % | Runtime Mean (s) | Notes |
|---|---:|---:|---:|---|
| `...` | `...` | `...` | `...` | `...` |

### 10.3 Ablation conclusions

Bu bölümde şu sorular cevaplanır:

- Tabu search gerçekten katkı sağladı mı?
- Mixed move avantaj üretti mi?
- PSO katmanı baseline dispatching rules’a göre ne kadar kazanç sağladı?
- Bazı instance’larda local search / tabu etkisiz mi kaldı?

---

## 11. Statistical Analysis

### 11.1 Planned tests

- **Two-configuration comparisons:** Wilcoxon signed-rank
- **Three or more configuration comparisons:** Friedman + post-hoc

### 11.2 Test execution summary

| Comparison | Test | p-value | Effect / Rank Note | Decision |
|---|---|---:|---|---|
| `A vs B` | `Wilcoxon` | `...` | `...` | `...` |
| `A vs B vs C` | `Friedman` | `...` | `...` | `...` |

### 11.3 Statistical interpretation

Burada yalnızca p-value değil, şu sorular da yorumlanır:

- Fark istatistiksel olarak anlamlı mı?
- Fark pratik olarak anlamlı mı?
- Ortalama rank kimin lehine?
- Küçük sample nedeniyle sonuç ne kadar dikkatli yorumlanmalı?

---

## 12. Figures and Visualizations

Bu bölümde rapora eklenecek figürlerin listesi tutulur.

### 12.1 Required figures

- Per-instance boxplot of gap
- Per-preset runtime boxplot
- Convergence plots (selected runs)

### 12.2 Optional figures

- Critical difference diagram
- Scatter plot: runtime vs best gap
- Heatmap: instance × preset mean gap

### 12.3 Figure inventory

| Figure ID | Title | Source File | Included? |
|---|---|---|---|
| `Fig-1` | `Gap boxplot` | `...` | `Yes/No` |
| `Fig-2` | `Runtime boxplot` | `...` | `Yes/No` |

---

## 13. Main Findings

Bu bölüm, raporun en önemli sonuçlarını kısa ve net biçimde verir.

### 13.1 Key findings

1. `...`
2. `...`
3. `...`
4. `...`

### 13.2 Instance-level takeaways

- `ft06`: `...`
- `la19`: `...`
- `abz7`: `...`
- `orb01`: `...`
- `swv11`: `...`

### 13.3 Method-level takeaways

- `...`
- `...`
- `...`

---

## 14. Threats to Validity

Bu bölüm muhakkak doldurulmalıdır.

### 14.1 Internal validity

- Parser / logging errors riski
- Timeout / early termination etkisi
- Seed listesi sınırlılığı

### 14.2 External validity

- Küçük benchmark havuzu nedeniyle genelleme sorunu
- Sadece belirli instance ailelerine odaklanma
- Aynı solver sürümüne bağımlılık

### 14.3 Construct validity

- Sadece makespan odaklı değerlendirme
- Çok amaçlı kriterlerin dışarıda kalması
- Dynamic JSSP değerlendirmesinin bu protokolde yer almaması

---

## 15. Final Decision and Next Step

### 15.1 Which presets survive?

| Instance | Winning Preset | Reason |
|---|---|---|
| `...` | `...` | `...` |

### 15.2 Should another tuning round be run?

- `Yes/No`
- Gerekçe: `...`

### 15.3 Next planned artifact

- `...`

---

## 16. Appendix A — File Inventory

| File | Purpose |
|---|---|
| `experimental_protocol_v1.md` | Deney sözleşmesi |
| `candidate_presets_v1.md` | Pilot exploration’dan çıkan aday ayarlar |
| `reference_table.csv` | Optimum/BKS referans tablosu |
| `validation_run_matrix.csv` | Validation koşu matrisi |
| `final_run_matrix.csv` | Final koşu matrisi |
| `final_results.csv` | Ham final sonuçlar |
| `aggregated_results.csv` | Gruplanmış özet sonuçlar |
| `final_stats.md` | Bu rapor |

---

## 17. Appendix B — Recommended Output Files To Link

- `final_results.csv`
- `aggregated_results.csv`
- `stats_tests.csv`
- `plots/`
- selected `result.json` examples
- selected `convergence.csv` examples

---

## 18. Writing Notes

Bu şablon doldurulurken aşağıdaki ilkelere uyulmalıdır:

1. Pilot sonuçlarla final sonuçları karıştırma.
2. Optimum ile BKS ayrımını açık tut.
3. Tek bir best run’a aşırı vurgu yapma; dağılımı raporla.
4. İstatistiksel testleri kör uygulama; sample size ve effect size yorumunu da ekle.
5. Kaynak disiplinine dikkat et:
   - repo bilgileri → repo kaynakları
   - benchmark reference bilgileri → benchmark kaynakları
   - kendi sonuçların → kendi CSV / JSON / log çıktıları

