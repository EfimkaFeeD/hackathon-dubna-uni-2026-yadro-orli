document.addEventListener('DOMContentLoaded', () => {
    const audioInput = document.getElementById('audioInput');
    const audioPlayer = document.getElementById('audioPlayer');
    const fileListEl = document.getElementById('fileList');
    const playbackRateSelect = document.getElementById('playbackRate');

    const WS_URL = "ws://localhost:8080/ws";
    const SESSION_ID = "sess_" + Math.random().toString(36).substring(2, 15);
    const fileStreams = [];

    function formatBytes(bytes) {
        if (bytes === 0) return '0 Б';
        const k = 1024;
        const sizes = ['Б', 'КБ', 'МБ', 'ГБ'];
        const i = Math.floor(Math.log(Math.abs(bytes)) / Math.log(k));
        return parseFloat((Math.abs(bytes) / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    audioInput.addEventListener('change', async () => {
        const files = Array.from(audioInput.files);
        for (const file of files) {
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

            const sizeDiffSpan = document.createElement('span');
            sizeDiffSpan.className = 'size-info';
            
            if (streamer.status === 'finished') {
                const diff = streamer.originalSize - streamer.processedSize;
                const percent = ((Math.abs(diff) / streamer.originalSize) * 100).toFixed(1);
                
                if (diff > 0) {
                    sizeDiffSpan.textContent = `Сэкономлено: ${formatBytes(diff)} (${percent}%)`;
                    sizeDiffSpan.style.color = 'rgb(22, 101, 52)';
                } else if (diff < 0) {
                    sizeDiffSpan.textContent = `Размер вырос на: ${formatBytes(Math.abs(diff))}`;
                    sizeDiffSpan.style.color = 'rgb(153, 27, 27)';
                } else {
                    sizeDiffSpan.textContent = `Размер не изменился`;
                }
            } else {
                sizeDiffSpan.textContent = `Исходный: ${formatBytes(streamer.originalSize)}`;
            }

            infoDiv.appendChild(nameSpan);
            infoDiv.appendChild(statusSpan);
            infoDiv.appendChild(sizeDiffSpan);

            const actionsDiv = document.createElement('div');
            actionsDiv.className = 'file-actions';

            const playBtn = document.createElement('button');
            playBtn.className = 'btn-play';
            playBtn.textContent = 'Слушать';
            playBtn.onclick = () => streamer.attachToPlayer(audioPlayer);

            const downloadBtn = document.createElement('button');
            downloadBtn.className = 'btn-download';
            downloadBtn.textContent = 'Скачать';
            downloadBtn.disabled = streamer.status !== 'finished';
            if (streamer.status === 'finished') {
                downloadBtn.onclick = () => {
                    const a = document.createElement('a');
                    a.href = streamer.blobUrl;
                    a.download = `ready_${streamer.file.name}`;
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
        const map = {'waiting': 'Ожидание', 'active': 'В работе', 'finished': 'Готово', 'error': 'Ошибка'};
        return map[status] || status;
    }
});