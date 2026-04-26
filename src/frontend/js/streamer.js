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
        
        // Параметры аудио (получаем с сервера или используем дефолтные)
        this.sampleRate = 44100;
        this.channels = 1;
        this.bitsPerSample = 16;
        
        this.currentExt = 'wav';
        this.useMSE = false; // Используем сборку WAV, не MSE
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
                    } else if (msg && msg.sample_rate) {
                        this.sampleRate = msg.sample_rate;
                        this.channels = msg.channels;
                        this.bitsPerSample = msg.bits_per_sample;
                    }
                } catch (e) {
                    console.warn("Failed to parse JSON:", event.data, e);
                }
            } else {
                console.log("Received PCM chunk, size:", event.data.byteLength);
                this._handleIncomingChunk(new Uint8Array(event.data));
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

    _createWavBlob() {
        const bytesPerSample = this.bitsPerSample / 8;
        let totalSamples = 0;
        for (const chunk of this.collectedChunks) {
            totalSamples += chunk.byteLength / bytesPerSample;
        }
        
        const dataSize = totalSamples * bytesPerSample;
        const fileSize = 36 + dataSize;
        const byteRate = this.sampleRate * this.channels * bytesPerSample;
        const blockAlign = this.channels * bytesPerSample;
        
        const buffer = new ArrayBuffer(44 + dataSize);
        const view = new DataView(buffer);
        
        // RIFF chunk
        this._writeString(view, 0, 'RIFF');
        view.setUint32(4, fileSize, true);
        this._writeString(view, 8, 'WAVE');
        
        // fmt chunk
        this._writeString(view, 12, 'fmt ');
        view.setUint32(16, 16, true);
        view.setUint16(20, 1, true);
        view.setUint16(22, this.channels, true);
        view.setUint32(24, this.sampleRate, true);
        view.setUint32(28, byteRate, true);
        view.setUint16(32, blockAlign, true);
        view.setUint16(34, this.bitsPerSample, true);
        
        // data chunk
        this._writeString(view, 36, 'data');
        view.setUint32(40, dataSize, true);
        
        // Копируем PCM данные
        let offset = 44;
        for (const chunk of this.collectedChunks) {
            const chunkView = new Uint8Array(chunk);
            for (let i = 0; i < chunkView.length; i++) {
                view.setUint8(offset + i, chunkView[i]);
            }
            offset += chunkView.length;
        }
        
        return new Blob([buffer], { type: 'audio/wav' });
    }

    _writeString(view, offset, str) {
        for (let i = 0; i < str.length; i++) {
            view.setUint8(offset + i, str.charCodeAt(i));
        }
    }

    _finalize() {
        if (this.collectedChunks.length === 0) {
            console.warn("No chunks collected for file:", this.file.name);
            return;
        }

        console.log("Finalizing, chunks count:", this.collectedChunks.length);
        
        // Создаём корректный WAV файл из PCM чанков
        const blob = this._createWavBlob();
        
        if (this.blobUrl && this.blobUrl !== 'null') {
            URL.revokeObjectURL(this.blobUrl);
        }
        
        this.blobUrl = URL.createObjectURL(blob);
        console.log("Blob URL created:", this.blobUrl);
        
        if (this.audioElement && this.blobUrl && this.blobUrl !== 'null') {
            console.log("Setting audio src to blob URL");
            this.audioElement.src = this.blobUrl;
            this.audioElement.play().catch(e => console.warn("Auto-play error:", e));
        }
    }

    _updateStatus(newStatus) {
        this.status = newStatus;
        console.log("Status updated:", newStatus);
        if (this.onStatusChange) this.onStatusChange();
    }
}