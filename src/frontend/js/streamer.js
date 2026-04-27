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

        this.sampleRate = 44100;
        this.channels = 1;
        this.bitsPerSample = 16;

        this.currentExt = 'wav';
        this.useMSE = false;
        this.audioElement = null;
        this.status = 'waiting';
        this.blobUrl = null;
        this.onStatusChange = null;

        this.originalDuration = 0;
        this.processedDuration = 0;
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
        const audioCtx = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: 44100 });
        const arrayBuffer = await file.arrayBuffer();
        const audioBuffer = await audioCtx.decodeAudioData(arrayBuffer);
        const float32Data = audioBuffer.getChannelData(0);

        const int16Data = new Int16Array(float32Data.length);
        for (let i = 0; i < float32Data.length; i++) {
            const s = Math.max(-1, Math.min(1, float32Data[i]));
            int16Data[i] = s < 0 ? s * 0x8000 : s * 0x7FFF;
        }

        const CHUNK_SIZE = 65536;
        let offset = 0;
        const buffer = int16Data.buffer;

        while (offset < buffer.byteLength) {
            // Ускорение: если буфер сокета не переполнен, шлем без задержки
            if (this.socket.bufferedAmount > 2 * 1024 * 1024) { // 2MB лимит
                await new Promise(r => setTimeout(r, 10));
                continue;
            }

            const chunk = buffer.slice(offset, offset + CHUNK_SIZE);
            this.socket.send(chunk);
            offset += CHUNK_SIZE;
            if (offset % (CHUNK_SIZE * 4) === 0) {
                await new Promise(r => setTimeout(r, 0));
            }
        }
        this.socket.send(JSON.stringify({ type: 'end' }));
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

        this._writeString(view, 0, 'RIFF');
        view.setUint32(4, fileSize, true);
        this._writeString(view, 8, 'WAVE');

        this._writeString(view, 12, 'fmt ');
        view.setUint32(16, 16, true);
        view.setUint16(20, 1, true);
        view.setUint16(22, this.channels, true);
        view.setUint32(24, this.sampleRate, true);
        view.setUint32(28, byteRate, true);
        view.setUint16(32, blockAlign, true);
        view.setUint16(34, this.bitsPerSample, true);

        this._writeString(view, 36, 'data');
        view.setUint32(40, dataSize, true);

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
            this._updateStatus('finished');
            return;
        }

        console.log("Finalizing, chunks count:", this.collectedChunks.length);

        const blob = this._createWavBlob();

        if (this.blobUrl && this.blobUrl !== 'null') {
            URL.revokeObjectURL(this.blobUrl);
        }

        const tempUrl = URL.createObjectURL(blob);
        const tempAudio = new Audio();
        tempAudio.src = tempUrl;

        const onReady = () => {
            this.blobUrl = URL.createObjectURL(blob);
            console.log("Blob URL created:", this.blobUrl);

            if (this.audioElement && this.blobUrl && this.blobUrl !== 'null') {
                console.log("Setting audio src to blob URL");
                this.audioElement.src = this.blobUrl;
                this.audioElement.play().catch(e => console.warn("Auto-play error:", e));
            }

            this._updateStatus('finished');
        };

        tempAudio.addEventListener('loadedmetadata', () => {
            this.processedDuration = tempAudio.duration;
            URL.revokeObjectURL(tempUrl);
            onReady();
        });

        tempAudio.addEventListener('error', () => {
            URL.revokeObjectURL(tempUrl);
            onReady();
        });
    }

    _updateStatus(newStatus) {
        this.status = newStatus;
        console.log("Status updated:", newStatus);
        if (this.onStatusChange) this.onStatusChange();
    }
}