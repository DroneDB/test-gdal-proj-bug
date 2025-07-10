# Test GDAL PROJ Bug

Questo progetto testa un bug correlato all'integrazione tra GDAL e PROJ per la trasformazione di coordinate.

## Descrizione

Il progetto contiene un test che verifica il comportamento della trasformazione di coordinate utilizzando GDAL e PROJ. Il test verifica se le coordinate vengono trasformate correttamente e genera un hash del risultato per verificarne la consistenza.

## Dipendenze

Il progetto utilizza vcpkg per la gestione delle dipendenze:

- **GDAL**: Libreria per la manipolazione di dati geospaziali
- **PROJ**: Libreria per le trasformazioni di coordinate cartografiche
- **libgeotiff**: Supporto per file GeoTIFF
- **hash-library**: Libreria per il calcolo di hash

## Build

Il progetto utilizza CMake come sistema di build:

```bash
# Configura il progetto
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake

# Compila
cmake --build build --config Release

# Esegui il test
./build/Release/testcmd.exe
```

## Test

Il test verifica:
1. L'inizializzazione corretta di GDAL e PROJ
2. La lettura di un file GeoTIFF di test (`wro.tif`)
3. La trasformazione delle coordinate
4. La generazione di un hash del risultato per verificarne la consistenza

## Struttura del progetto

- `main.cpp`: File principale contenente il test
- `hash.cpp/hash.h`: Utilità per il calcolo dell'hash
- `CMakeLists.txt`: Configurazione del build system
- `vcpkg.json`: Definizione delle dipendenze
- `wro.tif`: File di test GeoTIFF

## CI/CD

Il progetto include una pipeline GitLab CI che:
- Compila il progetto automaticamente
- Esegue il test
- Restituisce successo (verde) se il test passa, fallimento (rosso) in caso contrario
