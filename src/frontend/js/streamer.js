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

        this.originalDuration = 0; 
        this.processedDuration = 0;        
    }

    start() {
        this._reset();
        this._updateStatus('active');
        
        this.socket = new WebSocket(this.wsUrl);
        this.socket.binaryType = 'arraybuffer';

        this.socket.onopen = () => {
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
                    if (msg && msg.extension) {
                        this.currentExt = msg.extension.replace('.', '').toLowerCase();
                    }
                } catch (e) {}
            } else {
                this._handleIncomingChunk(event.data);
            }
        };

        this.socket.onclose = () => {
            this._finalize();
        };

        this.socket.onerror = () => {
            this._updateStatus('error');
        };
    }

    attachToPlayer(audioElement) {
        this.audioElement = audioElement;
        if (this.status === 'finished' && this.blobUrl) {
            audioElement.src = this.blobUrl;
            audioElement.play().catch(e => console.warn(e));
            return;
        }

        const mimeType = this.mimeMap[this.currentExt] || `audio/${this.currentExt}`;
        if (this.currentExt === 'wav') {
            this.useMSE = false;
            return;
        }
        
        if (this.useMSE && 'MediaSource' in window && MediaSource.isTypeSupported(mimeType)) {
            this.mediaSource = new MediaSource();
            audioElement.src = URL.createObjectURL(this.mediaSource);
            this.mediaSource.addEventListener('sourceopen', () => {
                this.sourceBuffer = this.mediaSource.addSourceBuffer(mimeType);
                this.sourceBuffer.addEventListener('updateend', () => this._pushQueue());
                this._flushQueue();
            });
            audioElement.play().catch(e => console.warn(e));
        } else {
            this.useMSE = false;
        }
    }

    _reset() {
        this.collectedChunks = [];
        this.queue = [];
    }

    _handleIncomingChunk(chunk) {
        this.collectedChunks.push(chunk);
        if (this.useMSE && this.sourceBuffer) {
            if (!this.sourceBuffer.updating && this.queue.length === 0) {
                try { this.sourceBuffer.appendBuffer(chunk); } catch (e) {}
            } else {
                this.queue.push(chunk);
            }
        }
    }

    _flushQueue() {
        if (!this.sourceBuffer || this.sourceBuffer.updating) return;
        while (this.queue.length > 0) {
            try { this.sourceBuffer.appendBuffer(this.queue.shift()); } catch (e) { break; }
            if (this.sourceBuffer.updating) break;
        }
    }

    _pushQueue() {
        if (this.queue.length > 0 && !this.sourceBuffer.updating) {
            try { this.sourceBuffer.appendBuffer(this.queue.shift()); } catch (e) {}
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
            } else break;
            offset += CHUNK_SIZE;
            await new Promise(r => setTimeout(r, 5));
        }
        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(JSON.stringify({ type: 'end' }));
        }
    }

    async _finalize() {
        if (this.collectedChunks.length === 0) {
            this._updateStatus('finished');
            return;
        }
        const mimeType = this.mimeMap[this.currentExt] || 'audio/wav';
        const blob = new Blob(this.collectedChunks, { type: mimeType });
        
        const tempUrl = URL.createObjectURL(blob);
        const tempAudio = new Audio();
        tempAudio.src = tempUrl;

        tempAudio.addEventListener('loadedmetadata', () => {
            this.processedDuration = tempAudio.duration;
            URL.revokeObjectURL(tempUrl);
            this.blobUrl = URL.createObjectURL(blob);
            this._updateStatus('finished');
        });

        tempAudio.addEventListener('error', () => {
            URL.revokeObjectURL(tempUrl);
            this.blobUrl = URL.createObjectURL(blob);
            this._updateStatus('finished');
        });
    }

    _updateStatus(newStatus) {
        this.status = newStatus;
        if (this.onStatusChange) this.onStatusChange();
    }
}