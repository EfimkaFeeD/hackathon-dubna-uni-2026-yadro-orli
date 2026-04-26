# Репозиторий команды "Орлы 🦅🦅🦅🦅🦅"
## Кейс от компании YADRO: Ускоритель аудио-подкастов

- Загружать аудиофайлы на сервер чанками (streaming upload)
- Получать обработанное аудио от сервера в потоковом режиме (streaming download)
- Воспроизводить аудио до завершения полной загрузки (progressive playback)
- Скачивать итоговый обработанный файл

## Технологический стек

|Технология| Назначение|
|-------|------|
| WebSocket API | Предоставляет двусторонний |потоковый обмен данными с сервером
|WebSocket API| Обеспечивает полнодуплексное соединение, необходимое для одновременной отправки и получения чанков в рамках одной сессии|
| MediaSource Extensions| Потоковое воспроизведение аудио без ожидания полной загрузки|
| Web Audio API | Валидация файла перед отправкой на сервер|
| File API / FileReader | Чтение файлов и нарезка на чанки|


## Структура проекта 

```
frontend/
├── index.html          
├── css/
│   └── style.css       
└── js/
    ├── streamer.js     
    └── script.js       
```


## Описание компонентов

Файл: `streamer.js`

Назначение: Управление жизненным циклом стриминга одного аудиофайла

#### Конструктор
```javascript
new AudioStreamer(wsUrl, sessionId, file)
```
| Параметр | Тип | Описание |
|----------|-----|----------|
| `wsUrl` | `string` | URL WebSocket сервера (например, `ws://localhost:8080/ws`) |
| `sessionId` | `string` | Идентификатор пользовательской сессии |
| `file` | `File` | Объект файла из `<input type="file">` |

#### Публичные свойства

| Свойство | Тип | Описание |
|----------|-----|----------|
| `id` | `string` | Уникальный идентификатор файла (генерируется случайно) |
| `file` | `File` | Ссылка на исходный файл |
| `status` | `string` | Текущий статус: `waiting`, `active`, `finished`, `error` |
| `onStatusChange` | `Function \| null` | Callback, вызываемый при изменении статуса |

#### Публичные методы

| Метод | Описание |
|-------|----------|
| `start()` | Инициализирует WebSocket соединение и начинает стриминг |
| `attachToPlayer(audioElement)` | Привязывает поток или готовый файл к HTMLAudioElement |

#### Приватные методы

| Метод | Назначение |
|-------|-----------|
| `_streamUpload(file)` | Нарезает файл на чанки по 32KB и отправляет через WebSocket |
| `_handleIncomingChunk(chunk)` | Обрабатывает полученный бинарный чанк от сервера |
| `_finalize()` | Собирает все чанки в Blob, создает Object URL |
| `_reset()` | Сбрасывает внутреннее состояние перед повторным запуском |
| `_updateStatus(status)` | Обновляет статус и вызывает `onStatusChange` |
| `_flushQueue()` / `_pushQueue()` | Управление очередью буферов MSE |


#### Логика воспроизведения

Класс реализует две стратегии воспроизведения:

1. MediaSource Extensions  — для форматов, поддерживаемых браузером в потоковом режиме:
   - mp3, ogg, m4a, aac, flac, opus, webm
   - Чанки добавляются в `SourceBuffer` по мере поступления
   - Воспроизведение начинается сразу после открытия MediaSource

2. Blob URL — для WAV и fallback:
   - WAV формат не поддерживается MSE в большинстве браузеров
   - Все чанки собираются в Blob, затем создается Object URL
   - Воспроизведение начинается только после полной загрузки

#### MIME-типы

```javascript
mimeMap = {
    'mp3':  'audio/mpeg',
    'wav':  'audio/wav',
    'ogg':  'audio/ogg',
    'm4a':  'audio/mp4',
    'aac':  'audio/aac',
    'flac': 'audio/flac',
    'opus': 'audio/ogg; codecs=opus',
    'webm': 'audio/webm; codecs=opus'
}
```

### script.js

**Файл**: `script.js`  
**Назначение**: Точка входа приложения 

#### Основные элементы 

| ID | Элемент | Назначение |
|----|---------|-----------|
| `audioInput` | `<input type="file">` | Выбор аудиофайлов |
| `audioPlayer` | `<audio controls>` | Плеер для воспроизведения |
| `fileList` | `<ul>` | Список загруженных/обрабатываемых файлов |
| `playbackRate` | `<select>` | Выбор скорости воспроизведения |


## Протокол WebSocket

### Client -> server

#### Инициализация
```json
{
  "type": "init",
  "sessionId": "sess_abc123...",
  "fileId": "xyz789...",
  "fileName": "track.mp3"
}
```

#### Чанк данных
Бинарные данные, 32KB
```
<ArrayBuffer>  
```

#### Завершение передачи
```json
{
  "type": "end"
}
```

### Сообщения server → client

#### Метаданные 
```json
{
  "extension": ".mp3"
}
```

#### Ошибка
```json
{
  "error": "Описание ошибки"
}
```

---



## 9. API и интерфейсы

### AudioStreamer

```typescript
class AudioStreamer {
    readonly id: string;
    readonly file: File;
    status: 'waiting' | 'active' | 'finished' | 'error';
    blobUrl: string | null;
    onStatusChange: (() => void) | null;

    constructor(wsUrl: string, sessionId: string, file: File);

    start(): void;
    attachToPlayer(audioElement: HTMLAudioElement): void;
}
```

### Глобальные константы (script.js)

```javascript
const WS_URL: string      // URL WebSocket сервера
const SESSION_ID: string  // Уникальный ID сессии пользователя
```
