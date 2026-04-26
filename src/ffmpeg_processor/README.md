# FFmpeg Audio Processor – Silence Detection & Classification

**Стек:** C / C++20 + FFmpeg (≥ 5.1) + WebRTC VAD (libfvad)

Модуль предназначен для потоковой обработки аудио с целью классификации кадров (фреймов) на голос, тишину и начала слов/предложений/абзацев. Может использоваться как динамическая библиотека (C API) или как консольное приложение.

## Возможности

- Приём несжатого PCM-аудио (любая частота дискретизации, моно/стерео, целочисленные и float форматы).
- Ресэмплинг в моно 48 кГц 16‑бит знаковое целое (внутренний формат) с помощью libswresample.
- Оценка уровня шума на основе скользящего окна заданной длительности и заданного процентиля.
- Простой детектор голоса по превышению RMS‑порога над шумом.
- Высокоточный детектор голосовой активности (WebRTC VAD, режим 0–3).
- Классификация кадров:
  - `kVoice` – голос,
  - `kSilence` – тишина,
  - `kWordStart` – начало слова (после ≥ 150 мс тишины),
  - `kSentenceStart` – начало предложения (≥ 300 мс),
  - `kParagraphStart` – начало абзаца (≥ 500 мс).
- Два интерфейса:
  - чистый C API для встраивания в плагины / другие языки,
  - C++ классы для прямого использования.
- Поддержка чтения аудиофайлов через FFmpeg (`AudioFileToFFmpegFormat`).

## Зависимости

### Системные

- CMake ≥ 3.31
- Компилятор с поддержкой C++20 (GCC ≥ 11, Clang ≥ 14, MSVC 2022)
- FFmpeg ≥ 5.1 (библиотеки: `libavformat`, `libavcodec`, `libavfilter`, `libswresample`, `libavutil`)
- WebRTC VAD: [libfvad](https://github.com/dpirch/libfvad) (только VAD, без остального WebRTC)

Для Ubuntu / Debian можно установить:

```bash
sudo apt install cmake g++ libavformat-dev libavcodec-dev libavfilter-dev libswresample-dev libavutil-dev libfvad-dev
```

### Сборка libfvad из исходников

```bash
git clone https://github.com/dpirch/libfvad.git
cd libfvad
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

## Сборка проекта

### Статическое приложение (по умолчанию)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

Будет собран исполняемый файл `ffmpeg_processor`, который принимает на вход имя аудиофайла.

### Динамическая библиотека

```bash
cmake .. -DBUILD_SHARED_LIB=ON
make
```

Создаст `libffmpeg_processor.so` (или `.dll` на Windows). Вместе с библиотекой автоматически собирается тестовый исполняемый файл `silence_test`, линкующийся с ней.

### Опции CMake

| Параметр                | По умолчанию | Описание |
|-------------------------|--------------|----------|
| `BUILD_SHARED_LIB`      | `OFF`        | Собирать как разделяемую библиотеку |
| `BUILD_TEST_EXECUTABLE` | `ON`         | Собрать тестовый `silence_test` (только при `BUILD_SHARED_LIB=ON`) |

## Использование

### Консольное тестовое приложение

```bash
./ffmpeg_processor path/to/audio.mp3
```

Оно обрабатывает весь файл, выводит прогресс (каждые 1000 фреймов) и в конце показывает итоговое количество фреймов каждого класса.

### C++ API

Основной класс – `FfmpegProcessor`. Пример создания и обработки:

```cpp
#include "ffmpeg_processor.hpp"

AudioSpec spec;
spec.sample_rate     = 48000;      // исходная частота
spec.is_mono         = true;
spec.bits_per_sample = 16;
spec.is_signed       = true;
spec.is_float        = false;

FfmpegProcessor proc(spec, 3.0f, 1, 10000);   // margin 3 dB, VAD mode 1, окно шума 10 с

uint8_t buffer[960];                           // 480 сэмплов * 2 байта = 960 байт (моно 16‑бит)
// заполнение buffer ...

FfmpegResult res = proc.process(packet_num, buffer, sizeof(buffer));
// res.chunk_type содержит AudioClassifier::Tag (kVoice, kSilence, ...)
```

### C API

Заголовочный файл: `audio_processor_c.h`.

```c
#include "audio_processor_c.h"

CAudioSpec spec = { .sample_rate = 48000, .is_mono = 1, .bits_per_sample = 16, .is_signed = 1, .is_float = 0 };
ProcessorHandle handle = processor_create(&spec, 3.0, 1, 10000);

uint8_t buffer[960];
// заполнение ...

PluginResult res = processor_process(handle, packet_num, buffer, sizeof(buffer));
// res.chunk_type – класс фрейма

processor_destroy(handle);
```

## Архитектура и поток обработки

1. **`FfmpegProcessor`** получает пакет сырых PCM-данных (interleaved).
2. Создаётся временный `AVFrame` и через `AudioResampler` преобразуется в моно 48 кГц S16.
3. Полученный `int16_t`‑буфер (480 отсчётов = 10 мс) передаётся в:
   - **`AudioNoiseLevel`** – вычисляет текущий RMS (в dB), добавляет в скользящее окно и возвращает оценку шумового пола как `kPercentileOfNoise`-й процентиль окна (по умолчанию 10 %).
   - **`AudioSimpleVoiceDetect`** – сравнивает RMS кадра с `noiseFloor + marginDb`. Если выше – предварительно считается голосом.
4. Если простой детектор сказал «голос», результат проверяется **`AudioVoiceActivityDetect`** (WebRTC VAD) на основе тех же 480 отсчётов.
5. Итоговое решение (голос / тишина) подаётся в **`AudioClassifier`**.
6. `AudioClassifier` ведёт очередь тегов длиной до `kSegmenterBufferFrames` (по умолчанию 30 с). При поступлении текущего кадра он анализирует длину предшествующей непрерывной тишины и, если она превышает пороги, выставляет соответствующий стартовый тег. Иначе возвращает `kVoice` или `kSilence`.

## Конфигурационные константы (`consts.h`)

| Константа | Значение | Описание |
|-----------|----------|----------|
| `kOutputSampleRate` | 48000 | Внутренняя частота дискретизации (Гц) |
| `kOutputFrameMs` | 10 | Длительность одного фрейма (мс) |
| `kOutputSamples` | 480 | Количество сэмплов в фрейме |
| `kNoiseWindowTargetMs` | 10000 | Длина окна для оценки шума (мс) |
| `kPercentileOfNoise` | 0.10 | Процентиль для шумового пола (10 %) |
| `kDefaultMarginDb` | 3.0 | Запас над шумом для простого детектора (dB) |
| `kDefaultVADMode` | 1 | Режим WebRTC VAD (0–3, чем выше, тем агрессивнее) |
| `kSegmenterBufferMs` | 30000 | Максимальная глубина истории классификатора (мс) |
| `kWordStartMs` | 150 | Порог тишины для начала слова (мс) |
| `kSentenceStartMs` | 300 | Порог тишины для начала предложения (мс) |
| `kParagraphStartMs` | 500 | Порог тишины для начала абзаца (мс) |

Значения `kGapWordStartFrames`, `kGapSentenceStartFrames`, `kGapParagraphStartFrames` вычисляются автоматически как `k*StartMs / kOutputFrameMs`.

## Структура проекта

```
src/
  audio_classifier.hpp/.cpp      – классификация тегов
  audio_noise_level.hpp/.cpp     – оценка шумового пола
  audio_simple_voice_detect.hpp/.cpp – простой пороговый детектор
  audio_voice_activity_detect.hpp/.cpp – обёртка над libfvad
  audio_resampler.hpp/.cpp       – ресэмплинг и конвертация формата
  ffmpeg_processor.hpp/.cpp      – главный процессор, объединяющий компоненты
  audio_processor_c.h/.cpp       – C-обёртка для плагинов
  audio_file_to_ffmpeg_format.hpp/.cpp – чтение аудиофайлов через FFmpeg
  consts.h                       – все конфигурационные константы
  main.cpp                       – тестовое консольное приложение
CMakeLists.txt
README.md
```

## Интеграция

- Динамическая библиотека устанавливается через `make install` (пути: `lib/` и `include/ffmpeg_processor/`). Подключается в сторонние проекты стандартным `find_package` или ручным указанием путей.
- C API можно использовать из Rust.
- Класс `AudioFileToFFmpegFormat` можно применять для чтения и конвертации аудиофайлов без полного процессинга – он предоставляет сырые `AVFrame*` с нативной частотой и форматом.
