# Duck Pocket — разведка и проект редизайна

Дата: 4 октября 2026, Москва. Статус: **фаза 0 завершена, реализация ожидает подтверждения**.

Это проект решений, а не отчёт о выполненном редизайне. Source/, Tests/, сборка и workflow не изменены. Удалений нет. Изучены все шесть файлов Source/, все Tests/, CMakeLists.txt, активный .github/workflows/build-macos.yml, README, PERFORMANCE-VALIDATION.md, все *-NOTES.md, FILES-TO-DELETE.txt и вспомогательные инструкции/скрипты сборки. Оба изображения открыты: image(7).png — текущий интерфейс, image(8).png — AI-макет.

## 1. База и границы проверки

Репозиторий: https://github.com/RainlineMusic/duck-pocket/tree/Release-1.0 . GitHub-плагин подтвердил существование ветки. Git blob-хеши всех шести Source-файлов и активного workflow совпали с ZIP; CMakeLists.txt также совпал с полученным содержимым. Проверка сделана по файлам, а не по закреплённому SHA коммита. Перед реализацией закрепить SHA и работать в отдельной ветке.

Идентификаторы сохраняются: ru.rainlinemusic.duckpocket, RnLn, DkPk. Исходные девять параметров: amount, scLow, scHigh, bypass, msBalance, duration, processLow, processHigh, outputGain. Их порядок/ID и семантика старых сессий сохраняются. Исходный Duration 5…2000 ms, верхний endpoint — AUTO/infinity. **Дополнение пользователя после начала разведки разрешает отдельное изменение DSP Duration: новый интерфейс в процентах, 100% по умолчанию = прежний AUTO, 50% = половина длительности key-события.** Остальное звучание не меняется ради внешнего вида; новая Duration-функция оформляется отдельным функциональным коммитом, а не называется багфиксом.

В этой Linux-среде нет JUCE, Pro Tools, macOS/Windows и pluginval. Численные CPU/GPU и frame-time замеры UI, AAX-проверка и фактическое состояние GitHub CI **не установлены**. Наличие проверок в workflow не означает успешное выполнение последнего запуска.

Существующие актуальные тесты собраны напрямую, C++17, -O2 -Wall -Wextra:

| Тест | Результат |
| --- | --- |
| Tests/v10_test.cpp | PASS; предупреждений компилятора нет |
| Tests/short_duration_test.cpp | PASS; по одному событию на хвостах 1/2/5/10/20 ms; значения 1/2 ms внутри движка ограничиваются минимумом 5 ms |

Тесты проверяют Engine, но не JUCE processor, XML, шины, FIFO или UI. Это не полный аудит фазы 1. Исторические тесты не являются актуальной регрессией: часть обращается к удалённым API или ожидает прежнюю латентность/поведение bypass.

## 2. Карта UI и отрисовки

Номера строк относятся к исходному ZIP, до добавления этого документа.

| Класс / функция | Ответственность | Место |
| --- | --- | --- |
| PocketLook | Палитры, кнопки, линейные диапазоны и M/S | PluginEditor.h:7–25; PluginEditor.cpp:43–86 |
| ResettableRangeSlider | Логарифмическая шкала, reset ближайшего thumb через double/Alt-click | PluginEditor.h:27–59 |
| ModernDial | Influence, Duration, компактный Output; тела, дуги, указатель, текст | PluginEditor.h:61–72; PluginEditor.cpp:87–97 |
| DuckPocketAudioProcessorEditor | Композиция, attachments, настройки, история, snapshots, VBlank | PluginEditor.h:74–136; PluginEditor.cpp:98–425 |
| uiFont / text / stroke / glowStroke | Системный шрифт, текст, векторные линии и псевдосвечение | PluginEditor.cpp:3–41 |
| resized / scaled | Координаты и фиксированное соотношение сторон | PluginEditor.cpp:164–165 |
| paintChrome | Фон, заголовок, стекло/сетка двух графиков, оси, панели | PluginEditor.cpp:289–340 |
| graph | Динамика Gain History и KEY/OUT осциллографа | PluginEditor.cpp:174–288 |
| paintDynamicLabels | Значения нижних диапазонов и M/S | PluginEditor.cpp:342–356 |
| paint / paintOverChildren | Композиция chrome+графики; размытие BYPASSED поверх controls | PluginEditor.cpp:358–386 |
| showSettingsMenu | Реальное меню Graph window и четырёх тем | PluginEditor.cpp:156 |
| setPanelExpanded / saveSize | Раскрытие нижней панели и сохранение width/expanded | PluginEditor.cpp:137,157 |
| setFrozen | Одна снежинка замораживает **оба** графика | PluginEditor.cpp:141–155 |

В текущем коде нет трёхточечных меню у графиков и нет утёнка. Эти элементы существуют только на AI-макете. Freeze — совместный для двух графиков; сохранить это фактическое поведение и tooltip.

Текущий design-space: 960×636 в свёрнутом виде, 960×890 в раскрытом. Gain/Scope: x=24, width=674, height=230, y=94/344. Правая колонка x=714, width=222. Нижняя панель содержит два диапазона друг под другом и M/S ниже. Масштабирование идёт от ширины; width ограничен 800…1500.

## 3. Кэш, данные и частота кадров сейчас

Chrome — ARGB Image на **физическом** масштабе Graphics context. Ключ включает pixel width/height и chromeScale; пересоздание при resized, теме, Graph window, раскрытии и смене DPI. Диапазоны рисуют подписи отдельно и не инвалидируют chrome при drag. Следовательно, пункт «не во время drag, как сейчас» уже выполнен для range-drag. При resize chrome всё ещё может создаваться на каждом изменении размера. Статичные подписи нижних controls пока находятся в динамическом слое.

Тела ручек не кэшируются: каждый paint снова строит эллипсы, градиенты, дорожки, дуги и текст. Свечение — широкие полупрозрачные strokes/ellipses, не Gaussian blur. Blur используется отдельно для snapshot в bypass: 1/6 размера, kernel 9, sigma 2.2; это разовое действие на UI-потоке, не GPU glow.

Аудиопоток → extrema capture → AbstractFifo SPSC на 8192 записей → editor history на 16384 записи → rollup на 4096 записей по 2 ms → time-locked buckets → пути. Packet содержит keyLo/keyHi/outLo/outHi/gain/time. Producer публикует запись после заполнения, consumer освобождает после копирования. По структуре это корректный SPSC-паттерн, но доказательство отсутствия гонок требует проверки JUCE 8.0.4 и TSan/стресс-теста. GPU никогда не должен становиться вторым consumer аудио-FIFO.

Захват выполняется только при editorOpen. Частота — SR / floor(SR/2400), то есть 2450 Hz при 44.1 kHz и 2400 Hz при 48/96/192 kHz. 16k даёт 6.69…6.83 s; FIFO около 3.34…3.41 s, после заполнения новые trace-пакеты теряются без ожидания аудио. Rollup покрывает около 8.19 s при непрерывном потоке.

VBlankAttachment вызывает frameTick, а elapsed gate 9 ms/28 ms ограничивает вызовы. На 60 Hz это обычно 60/30 fps, но на 120 Hz короткое окно даёт 60, на 144 Hz может дать 72: жёсткого 60 fps cap сейчас нет. Нет juce::Timer. Деструктор первым отключает VBlank, завершает активные gestures, отвязывает LookAndFeel. Асинхронные меню и theme-snapshot используют SafePointer.

frameTick читает FIFO до опустошения, обновляет rollup и display-clock, сравнивает bypass и перерисовывает gainArea/scopeArea при fresh или движении времени. Это **прямоугольники целых панелей**, не только plot interior. Frozen/bypassed панели не перерисовываются, но FIFO продолжает читаться. Когда audio callback идёт с нулями, fresh=true и панели продолжают обновляться. Когда данных нет, repaint обычно прекращается после стабилизации clock, но VBlank-callback остаётся активным. Абсолютный «0 CPU» обещать нельзя; критерий — отсутствие бесконечного рендера и минимальные пробуждения.

### Расчётная стоимость, не профилирование

Кривые ограничены 1200 физическими колонками. 5 s используют примерно 2500 summary-записей вместо 12000 raw. Gain делает один scan, scope — два. Для 1 s это примерно 7200 посещений записей на frame, для 5 s — примерно 7500; к этому добавляются buckets, сглаживание, path construction и растеризация. Векторы reserve заранее, но локальные juce::Path могут выделять память: утверждение «полностью без аллокаций в paint» пока не доказано.

Chrome при раскрытом 960×890: около 3.26 MiB на 1× и 13.04 MiB на 2×; при width=1500 и 2× — около 31.83 MiB. Это только ARGB pixels, без GPU/CPU копий, путей, knob/glow кэшей и snapshots. Суммировать память для 32 экземпляров обязательно. Пересоздание такого изображения во время resize потенциально заметнее обычного frame.

## 4. Наблюдения для аудита фазы 1

Это подтверждённые свойства исходников и кандидаты на проверку, **не список уже исправленных DSP-багов**.

| Место | Статус / значимость | Проверка и предполагаемое действие |
| --- | --- | --- |
| PluginEditor.cpp:128–134 | Подтверждено: 60/30 cap зависит от display Hz; средняя | Frame-count тест 60/90/120/144/165/240 Hz; elapsed-deadline scheduler с пределами 60/30 |
| PluginEditor.cpp:389–424; PluginProcessor.cpp:106–118 | Подтверждено: тишина порождает repaint; средняя | Repaint counter: отрисовать уход последнего сигнала, затем прекратить графический рендер до изменения данных/состояния |
| .github/workflows/build-macos.yml:40–43,148–154 | Подтверждено: short-duration suite не запущен CI; средняя | Запускать оба CMake/CTest targets на обеих OS |
| CMakeLists.txt:2; workflow env/artifacts | Подтверждено: 1.0.0 в CMake против 0.10 в артефактах; низкая | Согласовать маркировку сборки, не менять IDs |
| V010-NOTES.md | Подтверждено: описывает latched Duration/obsolete slots, исходник live Duration и удалённые IDs; средняя для документации | Пометить противоречащие разделы историческими, опираться на Source/README |
| PluginEditor.cpp:312 | Подтверждено: WAVEFORM ENVELOPE над графиком gain misleading; низкая | Сменить на точный смысл gain; не выдавать gain за waveform |
| PluginEditor.cpp:90–96,305–312 | Подтверждено: idle-glow, AUTO как полная дуга, мелкие подписи Output; средняя для UX | Material cache, приглушённый AUTO, минимум 11 logical px |
| PluginProcessor.cpp:99; PocketDSP.h M/S path | Кандидат: mono дублируется в L/R, при msBalance=+1 Mid исключается; поведение слышимо | Processor-тест mono при -1/0/+1 и sidechain disabled/mono/stereo. Не менять DSP до фиксации ожидаемой семантики; UI может обозначать M/S как stereo-only |
| PluginProcessor.cpp:47–60,104 | Кандидат: traceTime/FIFO не сброшены при prepare/reset; старые SR-данные могут дожить в UI | Повторный prepare, смена SR, reopen, длинный UI stall. Нельзя reset FIFO параллельно consumer без протокола |
| PocketDSP.h reset/process | Кандидаты: reset vector allocation на host callback, крайние finite input, overflow промежуточных значений | Проверить договор host reset и allocation tracing; FLT_MAX/cancellation и normal input; отдельный фикс только с воспроизведением |
| PluginProcessor.cpp:137–151 | Нужна проверка XML/версий/неполных данных | Corrupt/truncated XML, wrong root, пропущенные параметры, старые width/expanded, неверные числа; не доверять APVTS без теста |

Предварительно per-sample Engine не делает I/O/UI/lock и не расширяет vectors. prepare/reset делают assign; releaseResources пуст. Latency ceil(SR×0.005): 221/240/480/960 samples для 44.1/48/96/192 kHz, processor обновляет setLatencySamples при prepare. Bypass в Engine смешивает два выровненных пути. Полный аудит должен проверить host callbacks, а не только эту формулу.

## 5. Визуальное направление и разбор AI-макета

Направление: компактный измерительный прибор с утопленными экранами и сатинированными controls. Сложность возникает из шкал, реального GR-кольца, читаемых значений и согласованного материала. Сохранить туннельную сетку как узнаваемую черту Duck Pocket. Не превращать waveform display в спектральный анализатор.

| Что выдаёт искусственный макет | Как убрать |
| --- | --- |
| Цветные точки возле заголовков ничего не измеряют | Удалить |
| Жёлтая Influence, голубой Output, розовая Duration, розовый range, жёлтый M/S | Два data-акцента по ролям; Duration/M/S нейтральные |
| Все дуги максимально яркие даже в AUTO | AUTO — текстовый badge и приглушённая дорожка без заполненной дуги |
| Декоративные троеточия и дополнительный разделитель возле Settings | Не создавать действия без функции; оставить Settings, power и freeze |
| WAVEFORM ENVELOPE над Gain History | Заменить точной подписью; шкала retained gain 0–100%, GR-кольцо показывать отдельно |
| Иллюстративные гладкие синусоиды без связи с входом | Все демонстрационные captures — из реальных тестовых аудиоданных, сценарий указан |
| Одинаково тяжёлые рамки в несколько слоёв на каждом блоке | Одна тонкая кромка + локальная глубина; избегать повторного panel-in-panel |
| Блик/свечение по всему периметру, одинаковая пластмассовая фактура | Один свет сверху-слева, фрезерованный кант только там, где он читается |
| Output выглядит случайно вставленным между двумя большими ручками | Общая ось/сетка правого блока, единый Dial renderer и достаточный размер текста |

Нет объективного детектора «нейросетевого дизайна»: это оценка функции, иерархии и физической согласованности именно приложенного макета.

## 6. Зафиксированные токены (проект v1)

Цвета ниже — решение для реализации после подтверждения, не текущие цвета плагина. Второй data-акцент называется OUT/GAIN: он объединяет OUT, историю gain, Influence, Output и Processing range. KEY — sidechain waveform/filter/thumb. Duration/M/S нейтральны. Утёнок отдельного brand-цвета, небольшая площадь, без свечения; он не считается третьим data-акцентом.

| Роль | Neon | Solid Dark | Solid White | Amber |
| --- | --- | --- | --- | --- |
| Chassis | #10161D | #191D23 | #EBECEF | #211C17 |
| Raised controls | #1C2530 | #252B33 | #F5F5F7 | #302820 |
| Recessed glass | #0B1118 | #10151B | #E2E5E9 | #171410 |
| Primary text | #E5EAF0 | #E4E7EB | #202833 | #EEE4D8 |
| Secondary text | #A8B2BF | #A7AEB6 | #58616D | #BDB0A1 |
| Border | #36404D | #3B444F | #BCC2CB | #514538 |
| Major grid | #394957 | #36424F | #AEB8C4 | #514739 |
| Minor grid | #24313E | #26313C | #CDD3DA | #342D25 |
| KEY | #63CCD6 | #6CBBC5 | #176773 | #CFA96D |
| OUT/GAIN | #BCA1F3 | #AA9BCD | #6C4CA3 | #E78555 |
| Neutral active | #B7C0CD | #B8C0CA | #58616D | #BEB2A4 |
| Brand duck | #E5BB68 | #D7B46A | #89611C | #C6AD78 |

Secondary text контраст проверен формулой WCAG relative luminance на трёх основных подложках: минимум Neon 7.21:1, Dark 6.37:1, White 4.97:1, Amber 6.82:1. В реализации не понижать его opacity. Повторить проверку на конечных градиентах/hover/bypass. Сетки могут быть слабее, поскольку не заменяют осевые подписи. KEY/OUT дополнительно различаются легендой и характером линии, не только цветом.

Геометрия: spacing 4/8/12/16/24/32 px. Внешние поля 24, между крупными блоками 16, внутренние поля 16. Радиусы: buttons 6, controls 10, glass 10, chassis 14; круг только для dial. Кромка 1 physical px с DPI snapping, curve-core 1.5 logical px. Hit target маленькой иконки минимум 28×28 logical px.

Типографика: IBM Plex Sans Regular/Medium + IBM Plex Mono Regular/Medium, встроенные файлы и OFL; не зависеть от системного SF/Segoe. Section 11/14 medium с ограниченной разрядкой, оси 11/14, body 13/18, названия controls 15/20, values 26/32 или 30/36, product 24/30. Минимум 11 logical px на минимальной ширине: текст не должен просто уменьшаться пропорционально геометрии. Числа табличные, знак минус и infinity из встроенного шрифта. Пакет и конкретные license-файлы проверить при добавлении assets. После дополнения пользователя главное значение Duration — 100%, а AUTO лишь вторичный badge у 100%.

Материал: один свет сверху-слева. Верхняя кромка белая 8% dark/20% light, нижняя — чёрная 16% dark/10% light. Raised shadow offset (0,2), blur radius 8, opacity 20% dark/12% light. Glass inset offset (0,2), blur 6, opacity 22%; слабое отражение слева-сверху 3%, vignette максимум 8%. Noise tile детерминированный, процедурный, 2% opacity по умолчанию и максимум 4%, кэшируется. Не анимируется, не накладывается на glyphs и plot-data. Избегать зерна на светлой теме, если оно ухудшает читаемость.

Свечение: core остаётся резким на полном разрешении; glow отдельный 1/2-resolution слой, 1/4 только после QA мерцания. Начальные радиусы 3…8 logical px, alpha 0…0.16 от реального peak/GR; одна узкая и одна широкая компонента максимум. При отсутствии сигнала **ноль** luminous-элементов. Не светятся заголовки, панели, логотип или полная ручка. Одна краткая NOW-вспышка по реальному фронту reduction, не по произвольному таймеру. Frozen capture сохраняет линии, но активный glow и trigger затухают.

Возрастная дымка: яркость data-core 0.6 слева → 1.0 у NOW; мягкость применяется к следу, не к единственной читаемой кривой. Никакого сдвига extrema по времени. Decay-след scope 120…180 ms, обновляется только от новых данных и затухает до полного покоя. Gain fill увеличивается с 1−gain; подпись верхней шкалы соответствует retained gain. Для 0 gain GR-dB отображается как −∞ или ограниченный явно обозначенный meter floor. % остаются основной шкалой текущего gain-графика; осциллограф сохраняет ±1, не получает фиктивную dB-ось.

Ручки: cached body/well, 48…64 негромких насечек, major/minor scale, sampled conical highlight в кэше с тем же источником света. Outline активного значения и маркер динамические. GR-кольцо Influence питается 1−trace.gain и подписывается как фактическая control reduction; при band/M/S это не измерение ослабления всех частот. Output тем же renderer, меньшего диаметра, но с читаемыми value/unit. Duration 100%/AUTO — нейтральная дорожка, числовое значение 100% и вторичный badge AUTO, без яркой full arc. Конец активной дуги светится только при сигнале/взаимодействии, не постоянно.

Hover 120 ms, drag/release 160 ms, trigger 150 ms, renderer-fallback без декоративной анимации. После завершения перехода repaint прекращается. Сигнальные кривые продолжают обновляться по данным.

## 6a. Дополнение: Duration в процентах

Получено 4 октября 2026 после начала фазы 0. Это изменение требований имеет приоритет над исходным запретом менять Duration-DSP.

| Значение | Требуемый смысл |
| --- | --- |
| 100% (default) | Целое входящее key-событие, поведение прежнего AUTO |
| 50% | Сократить длительность управляющего события вдвое |
| 25% | Четверть длительности события |

Проценты относятся к длительности, не к глубине ducking, громкости или скорости воспроизведения аудио. Длина должна измеряться по входящему событию до Duration-gate; нельзя измерять уже сокращённый KEY и получать самоукорачивающуюся петлю. Точный выбор filtered/unfiltered detector закрепить в фазе 1; предварительно использовать существующий filtered key detector. Ширина нового диапазона пока не задана пользователем: предложение 1…100%, step 1%, reset 100%; не вводить >100% без необходимости. Доля действует на окно управления с плавным окончанием, не делает abrupt cut и не time-stretch-ит main/key audio.

**Причинное ограничение:** на первом произвольном ударе нельзя узнать его полный будущий конец с текущими 5 ms lookahead. Точный 50%-timeout и неизменная латентность одновременно невозможны для неизвестного live key. Это не устраняется OpenGL или новым UI. Нельзя незаметно трактовать проценты как 50% амплитуды или процент от фиксированных 2000 ms.

Рекомендуемый кандидат без дополнительной латентности — оценка длины из завершённых предыдущих событий с ограниченной адаптацией; на повторяющейся бочке 50% приближается к половине измеренного удара. Вариант требует согласования и тестов: первый удар, внезапная смена длины, перекрывающиеся хвосты, отсутствие тишины, изменение tempo и automation. На первом ударе нет достоверной истории: кандидат безопасного поведения — полный AUTO до получения первой длины, затем процентный gate. UI/tooltip должен честно обозначать оценку, а не обещать sample-accurate долю первого удара. Если нужна точная доля любого единичного звука, потребуется предварительное обучение/анализ либо увеличение латентности — это отдельное решение, пока не реализуется.

100% обходят новый конечный gate и используют старый AUTO path: проверить bit-identical render с прежним duration=2000. Detector/end/retrigger правила остаются исходными, пока тесты не докажут необходимость изменения. Остальные % — сознательно новое звучание, null-test для них невозможен по определению.

**Совместимость:** не менять range/normalization старого duration ID с миллисекунд на проценты — иначе старая host automation переназначит значения. Предлагается добавить новый durationPercent ID в конец списка параметров, оставить старый duration и его диапазон для legacy playback, в UI новых экземпляров показывать процентный control. Версия состояния выбирает legacy/new mode. Старая сессия сохраняет прежнее звучание и обозначение ms/legacy до явного перехода на проценты; простое открытие окна не должно включать новый алгоритм. Для новых экземпляров durationPercent=100 по умолчанию. Точный протокол переключения/host automation проверить на AAX и VST3 перед реализацией. Автоматического точного перевода 200 ms в проценты без длительности входящего события не существует.

Отдельный commit после baseline-аудита: новая модель длительности, параметр/state migration и тесты; отдельный следующий commit подключает percent UI. Не смешивать с багфиксами и материалами. До подтверждения фазы 0 код не меняется.

## 7. Layout после подтверждения

Проект базового окна 960×760, одна постоянная пропорция; диапазон width 800…1500 сохранить. Старый uiWidth читать и clamp, высоту пересчитать по новой пропорции. uiExpanded/duckPocket.ui.expanded принять без ошибки и игнорировать; не писать обратно. UI schema version добавить отдельно от DSP-параметров, точный формат определить после state-тестов.

| Блок | Предлагаемая геометрия в design-space |
| --- | --- |
| Header | x24 y16 w912 h56; слева duck+название, справа Settings/Bypass |
| Gain History | x24 y88 w640 h224 |
| Oscilloscope | x24 y328 w640 h224; Freeze рядом с NOW |
| Controls | x680 y88 w256 h464; Influence сверху, Duration снизу; Output в средней строке на общей сетке |
| Нижний блок | x24 y568 w912 h168; три равные колонки по 304 |
| Колонки | Sidechain filter / Processing range / Mid/Side: одна baseline заголовков и tracks |

Под «три графика на одном уровне» принят явно описанный в фазе 2 вариант: **три нижние секции в одном горизонтальном ряду**; Gain и Scope остаются двумя одинаковыми stacked panels. Дополнительный частотный график без данных/функции не добавляется.

Arrow и setPanelExpanded исчезают из UI. Нижняя панель всегда видна. На минимальной ширине колонки проверяются с 20 Hz / 20.0 kHz и Mid/Side 100%; fonts не ниже 11. Все interactions сохраняют parameter gestures/reset. Нельзя задействовать wheel/drag в чисто декоративных шкалах. Подписи и tooltip для каждой иконки, semantic KEY/OUT legend; graph-menu отсутствует, поскольку его функций сейчас нет. Settings сохраняет темы и Graph window, позднее получает renderer preference.

## 8. Рендеринг: сначала кэш, затем GPU

Разделить layout и rendering: единый UI snapshot на message thread; отдельные GlassGraph/InstrumentDial/RangeStrip и общий ThemeTokens. Не переносить доступ к Slider/ValueTree в GL callback. SPSC audio→UI остаётся с одним consumer; UI→GPU получает отдельный coherent double/triple-buffer snapshot и безопасное владение до конца frame.

Кэши: chrome/materials ключ (size, actualScale, theme, fonts); dial body ключ (diameter, scale, theme). Grid и static labels в chrome, live labels отдельно. Пересоздание при theme/resize/DPI/graph-window; latter меняет подписи и grid, не material-body. При range/dial drag chrome не меняется. Во время resize допускается временно масштабировать предыдущий chrome и собрать точный кэш после коалесцирования событий; не задерживать hit-test/layout. Ограничить память при max-size/2×, не создавать безлимитный cache на каждый промежуточный размер.

Glow pass: emissive mask → horizontal blur → vertical blur → additive composite, при нулевом сигнале pass пропущен. Software fallback воспроизводит читаемые core/data/materials; haze/glow можно упростить, сохранив семантику. Полноэкранный blur на каждом paint запрещён. Не путать attachTo с готовым shader-пайплайном: JUCE component painting может ускоряться отдельно, но offscreen glow потребует реального renderer/FBO/shader implementation.

DUCK_ENABLE_OPENGL — CMake option, link juce_opengl только при включении. Один OpenGLContext на editor; continuous repainting=false. Frame submission по VBlank, жёсткий 60/30 cap, отсутствие dirty-data означает отсутствие GPU frame. Runtime Settings: renderer automatic/native/software/OpenGL по фактической доступности; GL стартует opt-in до замеров. Host UI-state не меняет automation/DSP-state.

Attach после появления native peer. Context/shader/FBO failure даёт software/native fallback через message-thread SafePointer; конкретный контекст может успешно attach, но не создать shader — проверить оба случая. GL resources освобождаются на корректном GL callback. Destructor: снять VBlank и прекратить публикацию frames → setContinuousRepainting(false) → detach context → уничтожать renderer/children. Проверить DPI/monitor moves и context recreation; не держать UI mutex на аудиопотоке.

OpenGL на macOS deprecated, поэтому обещания выигрыша нет. JUCE 8 на Windows уже использует Direct2D по умолчанию: сравнение должно включать native D2D, software и GL, а не приписывать существующее ускорение новому OpenGL. Документация современного JUCE служит ориентиром; API проверять именно в 8.0.4. Не использовать setBackupEnabled(false) из более позднего JUCE.

## 9. Последовательность коммитов и критерии

1. **Фаза 1 / аудит:** закрепить base SHA, baseline renders; processor harness для variable/zero/huge blocks, buses, prepare/reset, SR 44.1/48/88.2/96/176.4/192; latency/bypass; NaN/Inf/finite extremes/denormals; XML; FIFO stall/drop/reopen; duration automation и amount 100…150. Отдельно audio allocation instrumentation, ASan/UBSan и TSan UI↔audio где среда позволяет. Каждый воспроизведённый баг — отдельный commit+regression+описание звукового эффекта. Предположение не становится багфиксом автоматически. После baseline согласовать кандидат процентной Duration из раздела 6a и реализовать отдельной функциональной дорожкой с сохранением legacy playback.
2. **Фаза 2 / layout:** постоянная нижняя панель, три колонки, migrated UI state, Output/grid/tooltip/freeze. Этот commit не меняет DSP. Resize/open/load checks.
3. **Фазы 3–4 / software-кэш и материалы:** токены уже зафиксированы здесь; embedded font/OFL, chrome/knob caches, grid/labels, material-lighting, сигнал-зависимые core/glow. Сначала 4 темы и три реальных состояния в software/native. Screenshot QA 1×/2×, minimum/max width, контраст и прищур.
4. **Фаза 3 / optional GPU:** option+runtime fallback отдельным commit; затем shader glow и profiling. Отдельно lifecycle/100 reopen/32 instances. Default GL off до подтверждённого выигрыша и стабильности в Pro Tools.
5. **Фаза 5 / антислоп:** idle/quiet/strong ducking для всех тем; удалить неинформативный decor, проверить роли/свет/TRUE values, tooltip и AUTO.
6. **Фаза 6 / итог:** Mac universal/Win x64, оба DSP suites, pluginval strictness 5 VST3; AAX/Pro Tools вручную, Reaper/Live VST3. Исходный baseline vs visual-only branch null-test с одинаковой архитектурой/toolchain; багфиксы отдельной дорожкой. Скриншоты 4×3 состояний ×1×/2×. Отчёт с реально измеренными цифрами и незавершёнными host-checks.

Фаза 1 и layout не ждут положительного результата GPU-бенчмарка. Редизайн должен оставаться полноценным при выключенном GL. Уборка архивных файлов — отдельное решение: ничего из FILES-TO-DELETE.txt не удалять до согласия. Предложение — архивировать устаревшие notes/tests и удалить дубликат workflow после подтверждения, сохранив историю в Git.

### Матрица измерений

| Сценарий | Метрики | Условия |
| --- | --- | --- |
| Editor closed / open idle / silent playback | Host CPU delta, GUI CPU, repaint count, GPU activity | Один host/session/SR/buffer; после 5 s settling |
| Quiet / strong ducking, 100 ms/1/2/5 s windows | Frame p50/p95/p99, FPS, stalls, GUI/audio CPU, GPU, memory | Все четыре темы, 1×/2×; 30 s captures |
| 1/8/32 instances | Scaling CPU/GPU/memory и audio underruns | Различать количество loaded plugins и одновременно open editors |
| 100 open/close, resize/DPI/monitor switch | Crash/assert/leak, retained resources, reopen timing | AAX Pro Tools и VST3 Reaper/Live; GL on/off |

Предлагаемый gate: frame p95 укладывается в 16.7 ms/33.3 ms для 60/30, ноль новых underruns/crashes/leaks; idle не выполняет бесконечные графические frames. Выигрыш GL оценивать вместе с энергией/GPU и scaling, не по снижению одного CPU-счётчика. Если p95/CPU не улучшаются либо Pro Tools нестабилен, GL остаётся выключенным. Точные абсолютные CPU thresholds устанавливаются после исходного host-бенчмарка. PERFORMANCE-VALIDATION.md остаётся основой ручного теста, расширяется матрицей выше.

## 10. Найденные материалы

Материалы подтверждают технические приёмы; эстетические токены и решения — авторский проект для Duck Pocket, не цитата из этих источников.

- JUCE OpenGLContext: attach/detach, threaded renderer, continuous repaint и triggerRepaint: https://docs.juce.com/master/classjuce_1_1OpenGLContext.html . Detach до уничтожения target/renderer обязателен.
- NVIDIA GPU Gems, глава 21 Real-Time Glow: отдельный emissive layer, низкое разрешение, separable convolution и additive blending; отдельно обсуждается aliasing/мерцание при downsample: https://developer.nvidia.com/gpugems/gpugems/part-iv-image-processing/chapter-21-real-time-glow . Использовать принцип, не копировать старый Direct3D код.
- JUCE 8 Direct2D: default native renderer на Windows, renderer switching после появления peer и цена CPU↔GPU transfers: https://juce.com/blog/juce-8-feature-overview-direct-2d/ . Статья обновлялась после 8.0.4, поэтому поздние API не переносить автоматически.
- Apple NSOpenGLContext: статус deprecated: https://developer.apple.com/documentation/appkit/nsopenglcontext . Это аргумент в пользу optional backend, не утверждение о конкретном crash в AAX.
- IBM Plex: Sans/Mono и Open Font License: https://github.com/IBM/plex ; руководство по гарнитуре: https://www.ibm.com/design/language/typography/typeface/ . При поставке включить конкретный OFL из скачанного пакета.

**Точка остановки:** исходный код не изменён. Следующий шаг после подтверждения — фаза 1, аудит с воспроизводимыми регрессиями, затем layout и реализация визуальных токенов.
