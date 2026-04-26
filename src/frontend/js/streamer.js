class AudioStreamer {
    constructor(wsUrl, sessionId, file) {
        this.wsUrl = wsUrl;
        this.sessionId = sessionId;
        this.file = file;
        this.id = Math.random().toString(36).substring(2, 9);
        
        this.socket = null;
        this.collectedChunks = [];
        
        this.sampleRate = 44100;
        this.channels = 1;
        this.bitsPerSample = 16;
        
        this.status = 'waiting';
        this.blobUrl = null;
        this.onStatusChange = null;
        this.originalDuration = 0; 
        this.processedDuration = 0;
    }

    start() {
        this.status = 'active';
        this.socket = new WebSocket(this.wsUrl);
        this.socket.binaryType = 'arraybuffer';

        this.socket.onopen = () => {
            this.socket.send(JSON.stringify({ 
                type: 'init', sessionId: this.sessionId, fileId: this.id, fileName: this.file.name 
            }));
            this._streamUpload();
        };

        this.socket.onmessage = (event) => {
            if (typeof event.data === 'string') {
                try {
                    const msg = JSON.parse(event.data);
                    // Синхронизируем параметры аудио с сервером
                    if (msg.sample_rate) this.sampleRate = msg.sample_rate;
                    if (msg.channels) this.channels = msg.channels;
                    if (msg.bits_per_sample) this.bitsPerSample = msg.bits_per_sample;
                } catch (e) {}
            } else {
                this.collectedChunks.push(new Uint8Array(event.data));
            }
        };

        this.socket.onclose = () => this._finalize();
        this.socket.onerror = () => { this.status = 'error'; this.onStatusChange(); };
    }

    async _streamUpload() {
        const CHUNK_SIZE = 32768;
        let offset = 0;
        while (offset < this.file.size) {
            const chunk = await this.file.slice(offset, offset + CHUNK_SIZE).arrayBuffer();
            if (this.socket.readyState !== WebSocket.OPEN) break;
            this.socket.send(chunk);
            offset += CHUNK_SIZE;
            await new Promise(r => setTimeout(r, 5));
        }
        if (this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(JSON.stringify({ type: 'end' }));
        }
    }

    _createWavBlob() {
        let dataLen = 0;
        this.collectedChunks.forEach(c => dataLen += c.byteLength);

        const buffer = new ArrayBuffer(44 + dataLen);
        const view = new DataView(buffer);
        const bytesPerSample = this.bitsPerSample / 8;

        const writeStr = (off, s) => { for(let i=0; i<s.length; i++) view.setUint8(off+i, s.charCodeAt(i)); };

        writeStr(0, 'RIFF');
        view.setUint32(4, 36 + dataLen, true);
        writeStr(8, 'WAVE');
        writeStr(12, 'fmt ');
        view.setUint32(16, 16, true);
        view.setUint16(20, 1, true); 
        view.setUint16(22, this.channels, true);
        view.setUint32(24, this.sampleRate, true);
        view.setUint32(28, this.sampleRate * this.channels * bytesPerSample, true);
        view.setUint16(32, this.channels * bytesPerSample, true);
        view.setUint16(34, this.bitsPerSample, true);
        writeStr(36, 'data');
        view.setUint32(40, dataLen, true);

        const finalBuffer = new Uint8Array(buffer);
        let offset = 44;
        this.collectedChunks.forEach(chunk => {
            finalBuffer.set(chunk, offset);
            offset += chunk.byteLength;
        });

        return new Blob([buffer], { type: 'audio/wav' });
    }

    async _finalize() {
        if (this.collectedChunks.length === 0) {
            this.status = 'finished';
            if (this.onStatusChange) this.onStatusChange();
            return;
        }

        const blob = this._createWavBlob();
        this.blobUrl = URL.createObjectURL(blob);
        
        const tempAudio = new Audio(this.blobUrl);
        tempAudio.onloadedmetadata = () => {
            this.processedDuration = tempAudio.duration;
            this.status = 'finished';
            if (this.onStatusChange) this.onStatusChange();
        };
    }

    attachToPlayer(player) {
        if (this.blobUrl) {
            player.src = this.blobUrl;
            player.play();
        }
    }
}