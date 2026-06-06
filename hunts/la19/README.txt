Bu klasor tek bir problem icin hazirlanmis mini hunt paketidir.

Kullanim:
1. `calistir.bat` dosyasina cift tiklayin.
2. Script 3 hafif deneme yapar.
3. Loglar `logs/` klasorune yazilir.
4. Kisa ozet `results/summary.csv` ve `results/summary.json` dosyalarina yazilir.
5. Varsa modern run dosyalari `runs/<run_id>/` altinda kalir; secili kopyalar `results/` altina alinir.

Not:
- Klasor adi problem adi olarak kullanilir.
- Ornek: `hunts/ft06` klasoru otomatik olarak `ft06` instance'ini calistirir.
- Yeni XSIM ciktilari varsa script once `runs/<run_id>/result.json` dosyasindan Cmax, runtime ve feasibility okur.
- `result.json` yoksa eski stdout regex fallback'i ile `Done. Best Cmax=...` satiri okunur.
- `results/` altinda result, metadata, schedule ve gantt kopyalari bulunabilir.
- Binary static runtime mantigiyla derlenmistir; `libgcc_s_seh-1.dll` ve `libstdc++-6.dll` gerektirmez.
