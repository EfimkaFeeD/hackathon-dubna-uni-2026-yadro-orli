document.addEventListener('DOMContentLoaded', () => {
    const audioInput = document.getElementById('audioInput');
    const audioPlayer = document.getElementById('audioPlayer');
    const fileListEl = document.getElementById('fileList');

    function getSessionId() {
        const KEY = 'app_session_id';
        const TTL = 7 * 24 * 60 * 60 * 1000; 
        const now = Date.now();
        
        const saved = localStorage.getItem(KEY);
        if (saved) {
            const data = JSON.parse(saved);
            if (now < data.expiry) return data.id;
        }

        const newId = "sess_" + Math.random().toString(36).substring(2, 15);
        localStorage.setItem(KEY, JSON.stringify({
            id: newId,
            expiry: now + TTL
        }));
        return newId;
    }

    const SESSION_ID = getSessionId();
    const WS_URL = "ws://localhost:8080/ws";
    const fileStreams = [];

    audioInput.addEventListener('change', async () => {
        for (const file of audioInput.files) {
            const streamer = new AudioStreamer(WS_URL, SESSION_ID, file);
            fileStreams.push(streamer);
            streamer.onStatusChange = () => renderList();
            streamer.start();
        }
        renderList();
    });

    function renderList() {
        fileListEl.innerHTML = '';
        fileStreams.forEach(s => {
            const li = document.createElement('li');
            li.innerHTML = `
                ${s.file.name} [${s.status}] 
                <button onclick="playStream('${s.id}')">Играть</button>
            `;
            fileListEl.appendChild(li);
        });
    }

    window.playStream = (id) => {
        const s = fileStreams.find(x => x.id === id);
        if (s) s.attachToPlayer(audioPlayer);
    };
});