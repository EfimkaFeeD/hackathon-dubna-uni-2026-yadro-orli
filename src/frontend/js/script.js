document.addEventListener('DOMContentLoaded', () => {
    const audioInput = document.getElementById('audioInput');
    const audioPlayer = document.getElementById('audioPlayer');
    const fileListEl = document.getElementById('fileList');
    const playbackRateSelect = document.getElementById('playbackRate');

    const WS_URL = "ws://localhost:8080/ws";
    
    const SESSION_ID = "sess_" + Math.random().toString(36).substring(2, 15);
    console.log('ID сессии:', SESSION_ID);
    
    const fileStreams = [];

    function formatTime(seconds) {
        if (!seconds || seconds <= 0) return '0 сек';
        if (seconds < 60) return seconds.toFixed(1) + ' сек';
        const mins = Math.floor(seconds / 60);
        const secs = Math.floor(seconds % 60);
        return `${mins} мин ${secs} сек`;
    }

    function getAudioDuration(file) {
        return new Promise((resolve) => {
            const audio = new Audio();
            const url = URL.createObjectURL(file);
            audio.preload = "metadata"; 
            audio.src = url;

            audio.onloadedmetadata = () => {
                URL.revokeObjectURL(url);
                resolve(audio.duration);
            };

            audio.onerror = () => {
                URL.revokeObjectURL(url);
                resolve(null);
            };
        });
    }

    playbackRateSelect.addEventListener('change', (e) => {
        audioPlayer.playbackRate = parseFloat(e.target.value);
    });

    audioInput.addEventListener('change', async () => {
        const files = Array.from(audioInput.files);
        if (files.length === 0) return;

        for (const file of files) {
            const duration = await getAudioDuration(file);
            
            if (duration === null) {
                alert(`Файл "${file.name}" не поддерживается или поврежден.`);
                continue;
            }

            const streamer = new AudioStreamer(WS_URL, SESSION_ID, file);
            streamer.originalDuration = duration; // Сохраняем длительность
            
            fileStreams.push(streamer);
            
            streamer.onStatusChange = () => renderFileList();
            streamer.start(); 
        }

        audioInput.value = '';
        renderFileList();
    });

    function renderFileList() {
        fileListEl.innerHTML = '';
        
        fileStreams.forEach(streamer => {
            const li = document.createElement('li');
            li.className = 'file-item';

            const infoDiv = document.createElement('div');
            infoDiv.className = 'file-info';
            
            const nameSpan = document.createElement('span');
            nameSpan.className = 'file-name';
            nameSpan.textContent = streamer.file.name;
            
            const statusSpan = document.createElement('span');
            statusSpan.className = `status-badge badge-${streamer.status}`;
            statusSpan.textContent = getStatusText(streamer.status);

        
            const timeDiffSpan = document.createElement('span');
            timeDiffSpan.className = 'size-info';
            
            if (streamer.status === 'finished') {
                const diff = streamer.originalDuration - streamer.processedDuration;
                if (diff > 0.5) {
                    timeDiffSpan.textContent = `Сэкономлено времени: ${formatTime(diff)}`;
                    timeDiffSpan.style.color = 'rgb(22, 101, 52)';
                } else {
                    timeDiffSpan.textContent = `Длительность: ${formatTime(streamer.processedDuration)}`;
                }
            } else {
                timeDiffSpan.textContent = `Исходная длина: ${formatTime(streamer.originalDuration)}`;
            }

            infoDiv.appendChild(nameSpan);
            infoDiv.appendChild(statusSpan);
            infoDiv.appendChild(timeDiffSpan);

            const actionsDiv = document.createElement('div');
            actionsDiv.className = 'file-actions';

            const playBtn = document.createElement('button');
            playBtn.className = 'btn-play';
            playBtn.textContent = 'Слушать';
            playBtn.onclick = () => {
                streamer.attachToPlayer(audioPlayer);
                audioPlayer.playbackRate = parseFloat(playbackRateSelect.value);
            };

            const downloadBtn = document.createElement('button');
            downloadBtn.className = 'btn-download';
            downloadBtn.textContent = 'Скачать';
            downloadBtn.disabled = streamer.status !== 'finished';
            
            if (streamer.status === 'finished') {
                downloadBtn.onclick = () => {
                    const a = document.createElement('a');
                    a.href = streamer.blobUrl;
                    a.download = `processed_${streamer.file.name}`;
                    a.click();
                };
            }

            actionsDiv.appendChild(playBtn);
            actionsDiv.appendChild(downloadBtn);

            li.appendChild(infoDiv);
            li.appendChild(actionsDiv);
            fileListEl.appendChild(li);
        });
    }

    function getStatusText(status) {
        const map = {
            'waiting': 'Ожидание',
            'active': 'Обработка',
            'finished': 'Готово',
            'error': 'Ошибка'
        };
        return map[status] || status;
    }
});