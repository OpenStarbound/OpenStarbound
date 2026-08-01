# TODO — Оптимизация освещения и чистка зависимостей

Вектор: (1) источники света > ~50 шт. кладут FPS в ноль на современных машинах;
(2) мёртвые/несогласованные зависимости. Все пункты — с evidence по коду.

## Текущее состояние системы освещения (диагноз)

- `WorldClient::render()` вызывается каждый кадр с render-потока и при
  `m_asyncLighting == false` **синхронно** выполняет `lightingCalc()` —
  полный пересчёт lightmap всего экрана (gather + spread CA + все point-света)
  (`source/game/StarWorldClient.cpp:40, 507-519`).
- `m_asyncLighting = true` по умолчанию с 2026-08 (фаза 1.1); раньше —
  `false`, переключался только консольной командой `/asyncLighting`.
- Каждый кадр в `renderWorld` без dirty-трекинга выставляется
  `m_pendingLightReady = true` и пересобираются ВСЕ источники света всех сущностей
  (`StarWorldClient.cpp:493-514`) — никакой инкрементальности.
- Point-свет: для каждого света цикл по bounding-box `maxRange = maxIntensity * m_pointMaxAir`
  (`source/base/StarCellularLightArray.cpp:23-26, 94-99`), для каждой клетки —
  Xiaolin-Wu ray-walk `lineAttenuation` (до O(radius) шагов, 2 чтения `cell()` на шаг,
  рандомный доступ → кэш-миссы) (`StarCellularLightArray.cpp:28-77, 404-538`).
  Стоимость на свет ≈ O(radius³) кэш-миссных операций. Гибридные света
  (`PointAsSpread`) дополнительно считаются как spread с радиусом `m_spreadMaxAir`
  (`StarWorldClient.cpp:1757-1763`).
- Lightmap-текстура заново аплоадится в GPU каждый кадр через
  `setEffectTexture("lightMap", ...)` (`source/rendering/StarWorldPainter.cpp:86-88`),
  даже если содержимое не менялось.
- При включённом async есть гонка: lighting-поток читает тайлы/секторы, которые
  main-поток модифицирует и освобождает без общего лока
  (`StarWorldClient.cpp:1319, 1711`; `source/core/StarSectorArray2D.hpp:438-460`) —
  включение async по умолчанию требует закрыть и её.

### Фаза 0 — Замер (обязательно, ~1 день)

- [ ] **0.1** Собрать сценарий-стресс: база со ~100-200 источниками (факелы,
      лампы, лава, спавн объектов с lights), прогнать при zoom-out и zoom-in.
- [ ] **0.2** Замерить существующие метрики (дебаг-оверлей, `LogMap`):
      `client_render_world_async_light_calc`, `..._light_gather`,
      `client_render_world_client`, `client_render_world_painter` —
      с `asyncLighting 0` и `asyncLighting 1`. Определить долю gather / spread /
      point в общем времени кадра.
- [ ] **0.3** Добавить тайминги внутрь `CellularLightArray::calculate`:
      отдельно spread и point-фазы, число источников и суммарный radius²
      (лог в `StarCellularLightArray.cpp` или метрику). Проверить реальные
      `pointMaxAir/spreadMaxAir` из `packed.pak:/lighting.config` —
      от них зависит оценка radius².
- [x] **0.4** `git blame` по `StarWorldClient.cpp:40` — выяснить, почему
      async выключен по умолчанию (историческая причина: гонка? мерцание?),
      зафиксировать в комментарии. Если причина — гонка, закрыть её (см. 1.1).
      **Вывод:** гонка lighting-потока с `unloadSector` — закрыта в 1.1.

**Acceptance фазы 0:** таблица замеров «N источников → мс на фазу» для async 0/1,
сценарий воспроизводим.

### Фаза 1 — Быстрые победы (S, без изменения алгоритма)

- [x] **1.1** Включить `m_asyncLighting = true` по умолчанию
      (`StarWorldClient.cpp:40`). При этом:
  - гонка закрыта частично: `unloadSector` при прореживании секторов выполняется
    под `m_lightMapPrepMutex` (`StarWorldClient.cpp` `update()`); `modifyTile` не
    локается — остаток гонки известен, см. Фазу 2/3;
  - мерцание при телепортах проверить при ручном тесте (нужен `packed.pak`).
  Проверка: играбельность в стрессе, `client_render_world_client` метрика не растёт,
  визуальных артефактов нет.
- [x] **1.2** Отсечение источников света при сборе: в `render()`
      фильтруются `renderLightSources` и particle-света по
      `cullRegion = window.padded(1 + ceil(max(pointMaxAir, spreadMaxAir)))`
      (из `/lighting.config:lighting`) до добавления в список.
- [x] **1.3** Лимит point-светов на кадр: `maxPointLights` из конфига
      (дефолт 128), сортировка по `color.max()` (сначала яркие),
      spread/гибридные света лимитом не режутся.
- [x] **1.4** Аплоад lightmap в GPU только при изменении: `lightMapVersion`
      (uint64, инкремент в `lightingCalc()`), `StarWorldPainter` пропускает
      `setEffectTexture("lightMap", ...)`, если версия не менялась.

### Фаза 2 — Инкрементальный расчёт (M, меняет `StarCellularLightArray` / `WorldClient`)

- [ ] **2.1** Dirty-трекинг источников: в `renderWorld` сравнивать
      `m_pendingLights` с предыдущим кадром (по id сущности + позиции + цвету),
      помечать «добавлен/сместился/удалён». Пересчитывать только удалённые и
      переместившиеся регионы (box источника × 2).
- [ ] **2.2** Кэшировать spread-слой: он меняется только при изменении тайлов
      (radiant/obstacle) и статичных источников; при изменении только point-светов
      переиспользовать готовый spread (сейчас `calculate()` всегда считает
      `calculateLightSpread` заново — `StarCellularLightArray.hpp:295-298`).
- [ ] **2.3** Хранение: вместо полного window-буфера хранить lightmap как атлас
      с грязными регионами; на render отдавать текущую целую карту
      (`waitForLighting`/`m_lightMap` — `StarWorldClient.cpp:1473-1488` не меняется,
      менять только продюсера).
- [ ] **2.4** Переиспользование буферов: `m_pendingLightMap`/`m_lightMap` —
      double-buffer вместо move+аллокации при каждом кадре.

**Acceptance 2:** при 200 источниках и отсутствии изменений (стоим на месте)
lightmap не пересчитывается вовсе (метрика calc = 0); при перемещении одного
света пересчёт пропорционален его боксу, не всему экрану.

### Фаза 3 — Алгоритм (L, только после фаз 0-2 и с characterization-тестами)

- [ ] **3.1** Заменить per-cell ray-walk на shadow propagation: 2-проходный
      flood/octant-скан от источника (как в vanilla-распространении spread),
      с окклюзией по obstacle. Цель — O(radius²) на свет без
      `lineAttenuation` per cell. Требование: результат визуально эквивалентен
      (или конфигурируемо близок) — снимки до/после по эталонным сценам.
- [ ] **3.2** Параллелизация point-фазы: разбить источники по worker-потокам
      (каждый свет пишет в свою полосу/регион с последующим merge), либо
      перевернуть цикл: per-cell × все света (лучше для кэша, векторуется).
- [ ] **3.3** SoA/SIMD: цвет как `Vec3F` в клетке → три отдельных float-массива
      (облегчает 3.2 и авто-векторизацию).
- [ ] **3.4** (Альтернатива 3.1-3.3, дороже) GPU compute-shader для lightmap —
      перенос `calculate` в GLSL compute. Отдельная задача-шпик: прототип вне
      дерева, оценка FPS-выигрыша, потом решение.

**Acceptance 3:** стресc-сценарий фазы 0 на 60 FPS (или не хуже X% от «без света»),
регрессия по 3-5 эталонным скриншотам, `core_tests` + `game_tests` зелёные.

### Фаза 4 — Чистка мёртвых зависимостей (S, независимо от освещения)

- [x] **4.1** Удалить `source/extern/tinyformat.h` (1164 строки, 0 включений
      вне extern; форматирование — через vendored fmt + `StarFormat.hpp`)
      и запись о нём в `source/extern/CMakeLists.txt:26`.
      Проверка: `grep -rn "tinyformat\|tfm::" source/` пусто, сборка зелёная.
- [x] **4.2** Удалить `lib/linux/libcrypto.a` (4 МБ, 0 ссылок в CMake/скриптах;
      TLS у cpr идёт из vcpkg-овского openssl).
      Проверка: сборка Linux зелёная, `rg -l "libcrypto"` пусто (кроме удаляемого файла).
- [x] **4.3** Аллокаторы — оставить один: **сделано иначе, чем в плане**: удалён
      `mimalloc` из `source/vcpkg.json` (не использовался, только платили сборку);
      оставлен `jemalloc` (его linux-пресет и так собирал и подключал через
      `STAR_USE_JEMALLOC`); вендоренный `source/extern/rpmalloc*` оставлен
      (используется Windows-пресетом, `STAR_USE_RPMALLOC`).
      Проверка: дефолтная сборка не включает mimalloc (`vcpkg list`), сборка зелёная.
- [x] **4.4** Зафиксировать версии вендоренного: `source/vcpkg-configuration.json`
      УЖЕ пинил `default-registry.baseline: d015e31e...` — добавленный в
      `vcpkg.json` `builtin-baseline` игнорируется (варнинг vcpkg) и удалён.
      Создан `source/extern/README.md` с версиями:
      fmt 10.2.1 (`extern/fmt/core.h:21` FMT_VERSION), lua 5.3.6 (`extern/lua.h:18-21`),
      xxhash, fast_float, gtest (`source/test/gtest/`). Проверка: `vcpkg list` даёт
      воспроизводимый набор версий.
- [ ] **4.5** Lua 5.3.6 (EOL с 2018) — отдельный трек, НЕ в этой фазе: миграция на
      5.4 = L/High-risk (меняется API, семантика `//`, `lua_resume`). Минимум —
      зафиксировать версию и патчи (если есть) в README extern (пункт 4.4).
- [ ] **4.6** Добить: `lib/linux/libdiscord_game_sdk.so` / `libsteam_api.so` /
      `lib/osx` / `lib/windows` — проверить, какие из них реально линкуются
      (grep по CMake) и какие используются (Discord SDK, Steam API при запуске
      через Steam), неиспользуемые убрать из репозитория.

## Порядок и зависимости

1. Фаза 0 обязательна перед фазами 1-3 (без замеров оптимизация вслепую).
2. 1.1 и CORR-1 (гонка lighting-потока) — связаны: async по умолчанию без
   закрытия гонки = деградация. Делать вместе.
3. Фаза 4 полностью независима, можно параллелить с любой другой.
4. Перед фазой 3 (алгоритм): прогнать существующие тесты как baseline
   (`cmake --preset linux-release && ctest --preset linux-release`) и добавить
   characterization-тесты на `CellularLightingCalculator` (сейчас их нет —
   `source/test/` не содержит ничего про освещение).

## Команды проверки (baseline)

```bash
cmake --preset linux-release && ctest --preset linux-release   # тесты (core_tests)
# стресс-сценарий: запуск клиента, база со 100+ источниками света
# метрики: дебаг-оверлей (LogMap): client_render_world_async_light_calc/_gather
```
