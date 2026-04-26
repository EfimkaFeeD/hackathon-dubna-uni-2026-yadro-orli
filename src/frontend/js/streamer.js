class AudioStreamer {
    constructor(wsUrl, sessionId, file) {
        this.wsUrl = wsUrl;
        this.sessionId = sessionId;
        this.file = file;
        this.id = Math.random().toString(36).substring(2, 9);
        
        this.socket = null;
        this.collectedChunks = [];
        this.mediaSource = null;
        this.sourceBuffer = null;
        this.queue = [];
        
        this.mimeMap = {
            'mp3': 'audio/mpeg',
            'wav': 'audio/wav',
            'ogg': 'audio/ogg',
            'm4a': 'audio/mp4',
            'aac': 'audio/aac',
            'flac': 'audio/flac',
            'opus': 'audio/ogg; codecs=opus',
            'webm': 'audio/webm; codecs=opus'
        };

        this.currentExt = 'wav';
        this.useMSE = true;
        this.audioElement = null;
        this.status = 'waiting';
        this.blobUrl = null;
        this.onStatusChange = null;
    }

    start() {
        this._reset();
        this._updateStatus('active');
        
        this.socket = new WebSocket(this.wsUrl);
        this.socket.binaryType = 'arraybuffer';

        this.socket.onopen = () => {
            console.log("WebSocket opened, sending init...");
            this.socket.send(JSON.stringify({ 
                type: 'init',  
                sessionId: this.sessionId,
                fileId: this.id,
                fileName: this.file.name 
            }));
            this._streamUpload(this.file);
        };

        this.socket.onmessage = (event) => {
            if (typeof event.data === 'string') {
                try {
                    const msg = JSON.parse(event.data);
                    console.log("Received message:", msg);
                    if (msg && msg.extension) {
                        this.currentExt = msg.extension.replace('.', '').toLowerCase();
                        console.log("Extension received:", this.currentExt);
                    } else if (msg && msg.error) {
                        console.error("Server error:", msg.error);
                        this._updateStatus('error');
                    }
                } catch (e) {
                    console.warn("Failed to parse JSON (ignoring):", event.data, e);
                    // Невалидный JSON игнорируем
                }
            } else {
                console.log("Received binary chunk, size:", event.data.byteLength);
                this._handleIncomingChunk(event.data);
            }
        };

        this.socket.onclose = (event) => {
            console.log("WebSocket closed, code:", event.code, "reason:", event.reason);
            this._finalize();
            this._updateStatus('finished');
        };

        this.socket.onerror = (error) => {
            console.error("WebSocket error:", error);
            this._updateStatus('error');
        };
    }

    attachToPlayer(audioElement) {
        this.audioElement = audioElement;
        
        if (this.status === 'finished' && this.blobUrl && this.blobUrl !== 'null') {
            console.log("Playing finished file from blob:", this.blobUrl);
            audioElement.src = this.blobUrl;
            audioElement.play().catch(e => console.warn("Play error:", e));
            return;
        }

        const mimeType = this.mimeMap[this.currentExt] || `audio/${this.currentExt}`;
        console.log("MIME type for playback:", mimeType);
        
        // WAV не поддерживается MSE → ждём полной загрузки
        if (this.currentExt === 'wav') {
            this.useMSE = false;
            console.log("WAV detected, will play after full download");
            return;
        }
        
        if (this.useMSE && 'MediaSource' in window && MediaSource.isTypeSupported(mimeType)) {
            console.log("Initializing MediaSource for", mimeType);
            this.mediaSource = new MediaSource();
            audioElement.src = URL.createObjectURL(this.mediaSource);
            
            this.mediaSource.addEventListener('sourceopen', () => {
                console.log("MediaSource opened, adding source buffer");
                this.sourceBuffer = this.mediaSource.addSourceBuffer(mimeType);
                this.sourceBuffer.addEventListener('updateend', () => this._pushQueue());
                this._flushQueue();
            });
            
            audioElement.play().catch(e => console.warn("Play error:", e));
        } else {
            console.warn(`⚠️ MediaSource не поддерживает ${mimeType}. Буферизация...`);
            this.useMSE = false;
        }
    }

    _reset() {
        this.collectedChunks = [];
        this.queue = [];
        if (this.mediaSource && this.mediaSource.readyState === 'open') {
            try { this.mediaSource.endOfStream(); } catch(e) {}
        }
    }

    _handleIncomingChunk(chunk) {
        this.collectedChunks.push(chunk);

        if (this.useMSE && this.sourceBuffer) {
            if (!this.sourceBuffer.updating && this.queue.length === 0) {
                try {
                    this.sourceBuffer.appendBuffer(chunk);
                } catch (e) {
                    console.error("Error appending to buffer:", e);
                }
            } else {
                this.queue.push(chunk);
            }
        }
    }

    _flushQueue() {
        if (!this.sourceBuffer || this.sourceBuffer.updating) return;
        while (this.queue.length > 0) {
            try {
                this.sourceBuffer.appendBuffer(this.queue.shift());
            } catch (e) {
                console.error("Error flushing queue:", e);
                break;
            }
            if (this.sourceBuffer.updating) break;
        }
    }

    _pushQueue() {
        if (this.queue.length > 0 && !this.sourceBuffer.updating) {
            try {
                this.sourceBuffer.appendBuffer(this.queue.shift());
            } catch (e) {
                console.error("Error in pushQueue:", e);
            }
        }
    }

    async _streamUpload(file) {
        const CHUNK_SIZE = 32768; 
        let offset = 0;
        while (offset < file.size) {
            const chunk = file.slice(offset, offset + CHUNK_SIZE);
            const buffer = await chunk.arrayBuffer();
            if (this.socket && this.socket.readyState === WebSocket.OPEN) {
                this.socket.send(buffer);
            } else {
                console.warn("WebSocket not open, stopping upload");
                break;
            }
            offset += CHUNK_SIZE;
            await new Promise(r => setTimeout(r, 5));
        }
        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(JSON.stringify({ type: 'end' }));
            console.log("Sent end message");
        }
    }

    _finalize() {
        if (this.collectedChunks.length === 0) {
            console.warn("No chunks collected for file:", this.file.name);
            return;
        }

        console.log("Finalizing, chunks count:", this.collectedChunks.length);
        
        const mimeType = this.mimeMap[this.currentExt] || 'audio/wav';
        const blob = new Blob(this.collectedChunks, { type: mimeType });
        
        // Освобождаем старый blob URL, если есть
        if (this.blobUrl && this.blobUrl !== 'null') {
            URL.revokeObjectURL(this.blobUrl);
        }
        
        this.blobUrl = URL.createObjectURL(blob);
        console.log("Blob URL created:", this.blobUrl);
        
        if (this.useMSE && this.mediaSource) {
            try {
                if (this.mediaSource.readyState === 'open') {
                    this.mediaSource.endOfStream();
                    console.log("MediaSource ended");
                }
            } catch(e) {
                console.warn("Error ending MediaSource:", e);
            }
        } else {
            // WAV или другой неподдерживаемый формат — проигрываем сразу, если плеер уже привязан
            if (this.audioElement && this.blobUrl && this.blobUrl !== 'null') {
                console.log("Setting audio src to blob URL");
                this.audioElement.src = this.blobUrl;
                this.audioElement.play().catch(e => console.warn("Auto-play error:", e));
            }
        }
    }

    _updateStatus(newStatus) {
        this.status = newStatus;
        console.log("Status updated:", newStatus);
        if (this.onStatusChange) this.onStatusChange();
    }
}