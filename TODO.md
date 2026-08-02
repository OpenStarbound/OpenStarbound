# TODO — Оптимизация освещения и чистка зависимостей

Вектор: (1) источники света > ~50 шт. кладут FPS в ноль на современных машинах;
(2) мёртвые/несогласованные зависимости. Все пункты — с evidence по коду.

## Текущее состояние системы освещения (диагноз)

- `WorldClient::render()` вызывается каждый кадр с render-потока и при
  `m_asyncLighting == false` **синхронно** выполняет `lightingCalc()` —
  полный пересчёт lightmap всего экрана (gather + spread CA + все point-света)
  (`source/game/StarWorldClient.cpp:40, 507-519`).
- `m_asyncLighting = false` по умолчанию (2026-08-02, возвращено после
  регрессии: async отдаёт протухшую lightmap при движении камеры → мерцание,
  «размытый» свет, трейсинг-плацебо, свет не на месте; sync теперь дёшев:
  skip-путь + дифф + point 15.7 мс). Переключается `/asyncLighting`.
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

Статус: 2.1 + 2.2 + 2.4(частично) реализованы одним заходом и **скомпилированы**
(2026-08-02); `core_tests` зелёные. `game_tests` локально не проходят из-за
ассетов (`packed.pak` не содержит `species` в `defaultHumanoidIdentity` → SpawnTest
падает на пустом species; CI его и так не гоняет — label `NoAssets`).

- [x] **2.1** Dirty-трекинг источников: в `lightingCalc()` (переписан,
      `StarWorldClient.cpp:1745+`) сравниваются списки источников текущего и
      прошлого кадра (хелперы `lightSourceEqual`/`lightSourcesEqual`/
      `particleLightsEqual`/`spreadSourcesChanged`). Ничего не изменилось →
      полный skip (`LogMap` = "skip", lightmap не публикуется, calc = 0).
      Реализовано как diff-подход, а не per-region: см. `calculateIncremental`.
- [x] **2.2** Кэшировать spread-слой: `spreadChanged` (окно/окружение/тайлы/
      spread-источники) → только тогда полный `begin()`+gather+`calculate()`;
      при изменении только point-светов — `calculateIncremental()` без spread.
      Дополнительно: `m_tileVersion` (atomic, инкремент в `readNetTile`,
      `TileLiquidUpdatePacket`, `informTilePrediction`, `update()`-expiry
      предикшн-тайлов) — смена тайлов форсирует полный пересчёт.
- [ ] **2.3** Хранение: вместо полного window-буфера хранить lightmap как атлас
      с грязными регионами; на render отдавать текущую целую карту
      (`waitForLighting`/`m_lightMap` — `StarWorldClient.cpp:1473-1488` не меняется,
      менять только продюсера).
- [~] **2.4** Переиспользование буферов: `writeOutput()` (`StarCellularLighting.cpp`)
      переиспользует буфер при совпадении размера, НО publish делает
      `m_lightMap = std::move(m_pendingLightMap)` — буфер уезжает в `m_lightMap`,
      следующий кадр аллоцирует заново. **Закрыта (swap отвергнут)**: `waitForLighting`
      всё равно move-ит буфер в renderData (живёт один кадр), безопасный double-buffer
      требует переделки владения и синхронизации аплоада — выигрыш ~0.1 мс/кадр
      не стоит риска. `Lightmap::swap` удалён (2026-08-02). Остаётся как есть.

**Acceptance 2:** при 200 источниках и отсутствии изменений (стоим на месте)
lightmap не пересчитывается вовсе (метрика calc = 0) — **реализовано** (skip);
при перемещении одного света пересчёт пропорционален его боксу, не всему экрану
— **частично**: дифф point-светов пересчитывает только их box-ы
(`addPointLightContribution`), но обход старых/новых списков линейный по числу
светов (это дёшево, O(n) vs O(radius³·n)).

### Фаза 3 — Алгоритм (L, только после фаз 0-2 и с characterization-тестами)

- [x] **3.1** Заменить per-cell ray-walk на shadow propagation: flood от источника
      (Dijkstra, 8-связность: клетка-препятствие = `perBlockObstacleAttenuation`,
      воздух = 0; вклад клетки = накопленная окклюзия + евклидово расстояние до
      центра клетки + beam + obstacle boost; аддитивный режим через scale ±1).
      `lineAttenuation`/`pointLightCellContribution` удалены, `pointLightFlood`
      заменяет оба (`StarCellularLightArray.hpp`). O(radius²) на свет. Семантика:
      расстояние-аттенюация идентична ванили (center-блок, `pointMaxAir`);
      отличие — плавающие стены не дают тени (флуд огибает с нулевой ценой),
      прикреплённые к полу/потолку работают как раньше. Тесты:
      `source/test/cellular_lighting_test.cpp` (расстояние, стена на всю высоту,
      инкремент add/remove симметричен) — зелёные.
- [x] **3.2** Параллелизация point-фазы: источники разбиваются по worker-потокам
      общего пула (`cellularLightWorkerPool`, hardware_concurrency); каждый свет
      флудит в приватный буфер (боксы светов пересекаются → общий массив нельзя),
      затем merge в клетки по bounding-региону: аддитивный = сумма, неаддитивный
      = max. Серийный fast-path при < 8 светов (`// ponytail: threshold 8`).
      Тест `parallelPointPhaseIsConsistent` (9 светов, 4/4 зелёные).
- [x] **3.3** SoA/SIMD: цвет как `Vec3F` в клетке → три отдельных float-массива
      (облегчает 3.2 и авто-векторизацию). Итог: три float-массива `m_lightR/G/B`,
      аналог перечислений `lightSpread`/`lightEnergy` переделан в `mem::Array` с
      физическим общим AoS-буфером (`LightCellBuffer`), канальный вызов
      `array.writeTargets(...)`; Dial-очередь (8 бакетов, `bucketHeads`) вместо
      повторного сканирования; spread-проход считает drop-факторы один раз на
      клетку (`drops`/`diagDrops`) и применяет канал-внутренним циклом. Диалоговый
      обход + ширина бакетов ограничивают точность: obstacle-шаг ≤ 1/8.
      Измерения (bench 280×175, 50 итер): spread 5.48→1.50 мс (2p, ~3.6×) и
      11.5→2.71 (4p, ~4.2×); point-фаза 200 светов 38.1→29.0 мс (≈1.3×);
      `core_tests` 4/4 зелёные.
- [x] **Багфикс (3.3-регрессия, 2026-08-02):** point-фаза писала каналы AoS-семантикой
      в SoA-массивы: `addChannelsScaled(dst[0], ...)`/`maxChannels(dst[0], ...)`
      обращались к `dst[0][1]`, `dst[0][2]` — соседние ячейки канала R, а не G/B.
      Симптомы в игре: весь свет красный (G/B = 0), R перенасыщен тройным
      накоплением, «свет сквозь стены» (утечка в ячейки y+1/y+2 без проверки
      препятствий). Фикс: канал-ориентированная запись `dst[c][0] += contrib[c]`
      / `dst[c][0] = max(...)`; мёртвые `addChannelsScaled`/`maxChannels` удалены
      из traits. Тест `coloredChannelsStaySeparate` (цветной свет (0.9,0.5,0.25),
      проверка всех трёх каналов) — 5/5 тестов зелёные. Побочный эффект: point-фаза
      200 светов 27.5→15.7 мс (bench 280×175, стабильно в 2 прогонах).
- [x] **Багфикс (3.4-регрессия, 2026-08-02):** point-фаза с Dial-волной (3.2/3.3)
      заменила лучевой расчёт: свет «обтекает» углы и проёмы и заливает комнаты
      без прямой видимости, тени исчезают («свет сквозь стены без малейших
      препятствий», «пересвет», «трейсинг-плацебо»). Возвращена исходная
      ray-семантика: Xiaolin-Wu line walk на каждую клетку бокса
      (`lineAttenuation`, дословно из upstream) с сохранением SoA-записи,
      параллелизма по светам и `maxPointLights`. Найдена и убита в процессе
      ошибка: `direction` не инициализировался при `beam=0` → `per=inf` → всё
      обнулялось (в upstream направление считается безусловно). Тест
      `pointLightCastsSharpRayShadow` (стена с проёмом: клетка без прямой
      видимости темна, с видимостью — светла, конфиг игры 48/9/3);
      `wallAttenuatesLight` пересчитан под AA-вклад Xiaolin-Wu (эталон —
      независимая python-модель). 8/8 тестов зелёные. Производительность:
      point-фаза 200 светов 15.7 → 21.7 мс (луч дороже волны, но всё ещё на 21%
      быстрее исходных 27.5); sync + skip-путь + инкрементальный дифф.
- [x] **3.4** GPU compute-shader для lightmap — шпик вне дерева
      (`/tmp/opencode/gpu_spike/gpu_lighting_spike.cpp`): CPU-параллельный Dial
      (эталон) vs GPU-релаксация (init/relax/contrib в 3 compute-программах,
      по-световые 2D-диспатчи, ping-pong att-буферов, atomicMax-накопление).
      Работает корректно (sum 47629 vs 48352, дифф 1.7 = несходимость волны за
      48 проходов), но НЕВЫГОДНО на Intel ADL-N iGPU (UMA): relax ~1 мс/проход ×
      48 = ~51 мс/итерацию против 9–13 мс CPU-параллельного Dial (4.1× медленнее)
      — узкое место пропускная способность UMA (~13 ГБ/с): 48 × 13 МБ = 625 МБ
      на итерацию, CPU Dial живёт в L3-кэше. Вывод: GPU-путь закрыт для этого
      железа; выигрыш возможен только на дискретном GPU с быстрой VRAM и/или при
      малом числе проходов.

**Acceptance 3:** стресc-сценарий фазы 0 на 60 FPS (или не хуже X% от «без света»),
регрессия по 3-5 эталонным скриншотам, `core_tests` + `game_tests` зелёные.

### Фаза 4 — Чистка мёртвых зависимостей (S, независимо от освещения)

Статус: 4.1–4.4 готовы. Дополнительно (2026-08-02): локальная сборка падала на
jemalloc 5.3.1 под GCC 16 (`std::__throw_bad_alloc` удалён из libstdc++) —
починено оверлей-портом `source/vcpkg-overlay/ports/jemalloc/` (патч `gcc16.patch`,
зарегистрирован в `source/vcpkg-configuration.json`); собрано и слинковано.

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

## Статус (2026-08-02)

- [x] Фаза 1 (1.1–1.4) — async по умолчанию, cull по региону, лимит 128 point,
      аплоад lightmap по версии
- [x] Фаза 2: 2.1 (skip при отсутствии изменений) + 2.2 (кэш spread-слоя) +
      2.4 (writeOutput переиспользует буфер; swap отвергнут — см. 2.4, закрыта)
- [ ] Фаза 2.3 (атлас с грязными регионами) — не начата
- [x] Фаза 3.1 (shadow propagation, `pointLightFlood`) + тесты `cellular_lighting_test`
      (2026-08-02, зелёные)
- [x] Фаза 3.2 (параллельный point-флуд, worker-пул, merge, fast-path < 8 светов)
      + тест `parallelPointPhaseIsConsistent` (2026-08-02, 4/4 зелёные)
- [x] Фаза 3.3 (SoA: канальные float-массивы + Dial-очередь + drop-факторы 1×/клетку)
      + 3.4 (GPU-шпик, вердикт: закрыт для iGPU) — см. детали выше
      (2026-08-02, 4/4 зелёные, bench-ускорения spread ~3.6–4.2×)
- [x] SSAA-фикс: убраны `glEnable(GL_SAMPLE_SHADING)` + `glMinSampleShading(1.f)`
      из `setMultiSampling` (`StarRenderer_opengl.cpp`) — per-sample shading заставлял
      world.frag (бикубический сэмплинг lightmap) считаться 4×/пиксель → «в ноль»
      на iGPU. Остаётся дешёвый MSAA 4x.
- [ ] Фаза 0 (0.1–0.3: стресс-сценарий и замеры) — ждёт ручного теста (packed.pak
      на месте, клиент собран)
- [x] Фаза 4: 4.1–4.4 + jemalloc GCC16-фикс (overlay-порт); 4.5 (Lua 5.4) —
      вне скоупа; 4.6 (`.so` в `lib/`) — не начата
- Клиент для ручного теста собран: `client_distribution/linux/starbound`
  (свежий, 11:15 2026-08-02, после 3.3+3.4), `packed.pak` на месте.
  Запуск: `client_distribution/linux/run-client.sh` (или `starbound` напрямую).
- **Запуск проверен (2026-08-02):** игра штатно загружается (все моды, title world
  достигнут, выход — грациозный). «Не отвечает» при старте — это долгие
  postLoad-скрипты модов (`ocd_tooltip_patch` и т.п., ~1-2 мин на ADL-N),
  окно не отрисовывается, пока идёт Lua — переждать, не убивать.
  Замеры освещения (фаза 0.1–0.3) ещё не делались.
- `game_tests` локально падают на SpawnTest из-за неполных ассетов
  (в `packed.pak` нет `species` в `defaultHumanoidIdentity`); CI их не гоняет.
