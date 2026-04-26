document.addEventListener("DOMContentLoaded", () => {
  const audioInput = document.getElementById("audioInput");
  const startBtn = document.getElementById("startBtn");
  const audioPlayer = document.getElementById("audioPlayer");
  const statusLabel = document.getElementById("status");
  const resultSection = document.getElementById("resultSection");
  const downloadLink = document.getElementById("downloadLink");

  // Конфигурация (заменить на адрес сервера)
  const WS_URL = "ws://localhost:8080/ws";

  const streamer = new AudioStreamer(WS_URL);

  // Обработка смены статусов
  streamer.onStatus = (type, message) => {
    statusLabel.textContent = message;
    statusLabel.className = `status-${type}`; // active, waiting, error
  };

  streamer.onFinish = (blob, extension) => {
    streamer.onStatus("waiting", "Обработка завершена успешно");

    const url = URL.createObjectURL(blob);
    downloadLink.href = url;
    downloadLink.download = `processed_audio_${Date.now()}.${extension}`;

    // Показываем блок скачивания
    resultSection.style.display = "block";
  };

  // Клик по кнопке
  startBtn.addEventListener("click", () => {
    const file = audioInput.files[0];
    if (!file) {
      alert("Пожалуйста, выберите аудиофайл");
      return;
    }

    // Скрываем прошлый результат, если он был
    resultSection.style.display = "none";

    streamer.start(file, audioPlayer);
  });
});
