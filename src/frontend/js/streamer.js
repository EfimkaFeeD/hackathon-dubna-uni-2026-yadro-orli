class AudioStreamer {
  constructor(wsUrl) {
    this.wsUrl = wsUrl;
    this.socket = null;
    this.collectedChunks = [];
    this.mediaSource = null;
    this.sourceBuffer = null;
    this.queue = [];
    this.sessionId = "sess_" + Math.random().toString(36).substr(2, 9);

    this.mimeMap = {
      mp3: "audio/mpeg",
      wav: "audio/wav",
      ogg: "audio/ogg",
      m4a: "audio/mp4",
      aac: "audio/aac",
      flac: "audio/flac",
      opus: "audio/ogg; codecs=opus",
      webm: "audio/webm; codecs=opus",
    };

    this.currentExt = "wav";
    this.onStatus = null;
    this.onFinish = null;
    this.useMSE = true; // Флаг переключения режима
    this.audioElement = null;
  }

  async start(file, audioElement) {
    this._reset();
    this.audioElement = audioElement;

    this.socket = new WebSocket(this.wsUrl);
    this.socket.binaryType = "arraybuffer";

    this.socket.onopen = () => {
      this.onStatus?.("active", "Соединение установлено. Отправка...");
      this.socket.send(
        JSON.stringify({
          type: "init",
          sessionId: this.sessionId,
          fileName: file.name,
        }),
      );
      this._streamUpload(file);
    };

    this.socket.onmessage = (event) => {
      if (typeof event.data === "string") {
        const msg = JSON.parse(event.data);
        if (msg.extension) {
          this.currentExt = msg.extension.replace(".", "").toLowerCase();
          this._initMediaSource(audioElement);
        }
      } else {
        this._handleIncomingChunk(event.data);
      }
    };

    this.socket.onclose = () => {
      this._finalize();
    };

    this.socket.onerror = () => {
      this.onStatus?.("error", "Ошибка связи с сервером");
    };
  }

  _reset() {
    this.collectedChunks = [];
    this.queue = [];
    if (this.mediaSource && this.mediaSource.readyState === "open") {
      try {
        this.mediaSource.endOfStream();
      } catch (e) {}
    }
  }

  _initMediaSource(audioElement) {
    const mimeType =
      this.mimeMap[this.currentExt] || `audio/${this.currentExt}`;

    // Проверяем поддержку MSE для данного формата
    if (
      this.useMSE &&
      "MediaSource" in window &&
      MediaSource.isTypeSupported(mimeType)
    ) {
      this.mediaSource = new MediaSource();
      audioElement.src = URL.createObjectURL(this.mediaSource);

      this.mediaSource.addEventListener("sourceopen", () => {
        this.sourceBuffer = this.mediaSource.addSourceBuffer(mimeType);
        this.sourceBuffer.addEventListener("updateend", () =>
          this._pushQueue(),
        );
        this._flushQueue(); // Отправляем уже пришедшие чанки
      });
    } else {
      console.warn(
        `⚠️ MediaSource не поддерживает ${mimeType}. Переключаюсь на стандартное воспроизведение.`,
      );
      this.useMSE = false;
      this.onStatus?.("active", "Буферизация аудио...");
    }
  }

  _handleIncomingChunk(chunk) {
    this.collectedChunks.push(chunk);

    if (this.useMSE && this.sourceBuffer) {
      if (!this.sourceBuffer.updating && this.queue.length === 0) {
        this.sourceBuffer.appendBuffer(chunk);
      } else {
        this.queue.push(chunk);
      }
    }
  }

  _flushQueue() {
    if (!this.sourceBuffer || this.sourceBuffer.updating) return;
    while (this.queue.length > 0) {
      this.sourceBuffer.appendBuffer(this.queue.shift());
      if (this.sourceBuffer.updating) break;
    }
  }

  _pushQueue() {
    if (this.queue.length > 0 && !this.sourceBuffer.updating) {
      this.sourceBuffer.appendBuffer(this.queue.shift());
    }
  }

  async _streamUpload(file) {
    const CHUNK_SIZE = 32768;
    for (let i = 0; i < file.size; i += CHUNK_SIZE) {
      const chunk = file.slice(i, i + CHUNK_SIZE);
      const buffer = await chunk.arrayBuffer();
      if (this.socket && this.socket.readyState === WebSocket.OPEN) {
        this.socket.send(buffer);
      }
      await new Promise((r) => setTimeout(r, 5));
    }
    this.socket.send(JSON.stringify({ type: "end" }));
  }

  _finalize() {
    if (this.collectedChunks.length === 0) return;

    if (this.useMSE && this.mediaSource) {
      try {
        if (this.mediaSource.readyState === "open") {
          this.mediaSource.endOfStream();
        }
      } catch (e) {}
      this.onStatus?.("waiting", "Обработка завершена успешно");
    } else {
      // Fallback: собираем всё в Blob и воспроизводим стандартно
      const mimeType = this.mimeMap[this.currentExt] || "audio/wav";
      const blob = new Blob(this.collectedChunks, { type: mimeType });
      const url = URL.createObjectURL(blob);

      this.audioElement.src = url;
      this.onStatus?.("waiting", "Обработка завершена успешно");
      this.onFinish?.(blob, this.currentExt);
    }
  }
}
