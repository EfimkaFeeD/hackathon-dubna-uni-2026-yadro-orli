document.addEventListener('DOMContentLoaded', () => {
    const audioInput = document.getElementById('audioInput');
    const audioPlayer = document.getElementById('audioPlayer');
    const fileListEl = document.getElementById('fileList');
    const playbackRateSelect = document.getElementById('playbackRate');

    const WS_URL = "ws://localhost:8080/ws";
    
    const SESSION_ID = "sess_" + Math.random().toString(36).substring(2, 15);
    console.log('ID сессии:', SESSION_ID);
    
    const fileStreams = [];

    playbackRateSelect.addEventListener('change', (e) => {
        audioPlayer.playbackRate = parseFloat(e.target.value);
    });

    audioInput.addEventListener('change', () => {
        const files = Array.from(audioInput.files);
        if (files.length === 0) return;

        files.forEach(file => {
            const streamer = new AudioStreamer(WS_URL, SESSION_ID, file);
            fileStreams.push(streamer);
            
            streamer.onStatusChange = () => renderFileList();
            streamer.start(); 
        });

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
            'waiting': '⏳ Ожидание',
            'active': '🔄 Обработка',
            'finished': '✅ Готово',
            'error': '❌ Ошибка'
        };
        return map[status] || status;
    }
});