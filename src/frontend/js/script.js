document.addEventListener('DOMContentLoaded', () => {
    const audioInput = document.getElementById('audioInput');
    const audioPlayer = document.getElementById('audioPlayer');
    const fileListEl = document.getElementById('fileList');
    const playbackRateSelect = document.getElementById('playbackRate');

    const WS_URL = "ws://localhost:8080/ws";
    const SESSION_ID = "sess_" + Math.random().toString(36).substring(2, 15);
    
    const fileStreams = [];

    async function isValidAudio(file) {
        const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        
        try {
            const headerBlob = file.slice(0, 512 * 1024); 
            const arrayBuffer = await headerBlob.arrayBuffer();

            await audioCtx.decodeAudioData(arrayBuffer);
            return true; 
        } catch (e) {
            console.error("Валидация Web Audio API не удалась:", e);
            return false; 
        } finally {
            await audioCtx.close();
        }
    }

    playbackRateSelect.addEventListener('change', (e) => {
        audioPlayer.playbackRate = parseFloat(e.target.value);
    });

    audioInput.addEventListener('change', async () => {
        const files = Array.from(audioInput.files);
        if (files.length === 0) return;

        for (const file of files) {
            console.log(`Проверка файла: ${file.name}`);
            
            const isValid = await isValidAudio(file);

            if (!isValid) {
                alert(`Файл "${file.name}" поврежден или не является поддерживаемым аудио.`);
                continue;
            }

            const streamer = new AudioStreamer(WS_URL, SESSION_ID, file);
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

            infoDiv.appendChild(nameSpan);
            infoDiv.appendChild(statusSpan);

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