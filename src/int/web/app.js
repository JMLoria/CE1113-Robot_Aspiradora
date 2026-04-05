const canvas = document.getElementById('mapCanvas');
const ctx = canvas.getContext('2d');
const statusDot = document.querySelector('.status-dot');
const statusText = document.getElementById('status');

const CELL_SIZE = 20;
const GRID_SIZE = 20;

// Estados de conexión
let socket;
let isConnected = false;

// Estados de reproduccion
let isPlaying = false;

// Conectar al servidor al cargar la pagina
window.onload = connect;

function connect() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    console.log("Intentando conectar a:", wsUrl);
    socket = new WebSocket(wsUrl);

    socket.onopen = () => {
        isConnected = true;
        statusDot.className = "status-dot online";
        statusText.innerHTML = '<span class="status-dot online"></span> Conectado al Robot';
    };

    socket.onmessage = (event) => {
        const data = JSON.parse(event.data);

        // Renderizar Mapa
        if (data.map) {
            renderMap(data.map.grid, data.map.robot, data.map.angle);
        }

        // Renderizar Audio
        if (data.audio) {
            updateAudioUI(data.audio);
        }

        socket.onclose = () => {
            isConnected = false;
            statusDot.className = "status-dot offline";
            statusText.innerHTML = '<span class="status-dot offline"></span> Desconectado';
        }

    };
}

// --- LOGICA DE INTERFAZ DE AUDIO ---
function updateAudioUI(audioData) {
    const trackInfo = document.getElementById('track-info');
    const timeCurrent = document.getElementById('time-current');
    const timeTotal = document.getElementById('time-total');
    const progressBar = document.getElementById('progress-bar');

    // Actualizar nombre de la cancion
    if (trackInfo) trackInfo.innerText = audioData.track || "Sin reproducción";

    // Actualizar tiempos formateados
    if (timeCurrent) timeCurrent.innerText = formatTime(audioData.current);
    if (timeTotal) timeTotal.innerText = formatTime(audioData.total);

    // Actualizar barra de progreso
    if (progressBar && audioData.total > 0) {
        const percentage = (audioData.current / audioData.total) * 100
        progressBar.style.width = `${percentage}%`;
    }
}

function formatTime(secs) {
    const mins = Math.floor(secs / 60);
    const s = Math.floor(secs % 60);
    return `${mins}:${s < 10 ? '0' : ''}${s}`;
}

// --- ACCIONES HACIA EL SEVIDOR ---
async function sendAction(endpoint) {
    try {
        const response = await fetch(`/api/${endpoint}`);
        if (!response.ok) throw new Error('Error en la peticion');
        console.log(`Accion exitosa: ${endpoint}`);
    } catch (error) {
        console.error('Error al enviar accion:', error);
    }
}

// Funciones de control vinculados a botones
function move(dir) { sendAction(`move/${dir}`); }
function setMode(m) { sendAction(`mode/${m}`); }

// Controles de musica
function togglePlay() { 
    isPlaying = !isPlaying;
    if (isPlaying) {
        sendAction('audio/play/current');
        document.getElementById('btn-play-pause').innerText = "▶";
    } else {
        sendAction('audio/pause'); 
        document.getElementById('btn-play-pause').innerText = "⏸";
    }
}

function stopMusic() { 
    isPlaying = false;
    sendAction('audio/stop'); 
    document.getElementById('btn-play-pause').innerText = "⏸";
}

function nextSong() { 
    isPlaying = true;
    sendAction('audio/next'); 
}

function prevSong() { 
    isPlaying = true;
    sendAction('audio/prev'); 
}

function skipTime(dir) { sendAction(`audio/${dir}`); } // 'forward' o 'back'

function changeVolume(dir) { sendAction(`audio/volume/${dir}`); } // 'up' o 'down'

// --- DIBUJO DEL MAPA ---
function renderMap(grid, robot, angle) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // 1. Dibujar celdas (Visitadas y Obstaculos)
    for (let y = 0; y < GRID_SIZE; y++) {
        for (let x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x] === 1) {
                ctx.fillStyle = '#777';     // VISITADA
                ctx.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            } else if (grid[y][x] == 2) {
                ctx.fillStyle = '#ff4d4d';  // OBSTACULO
                ctx.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            }
            ctx.strokeStyle = '#222';
            ctx.strokeRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        }
    }

    // Dibujar Robot
    // Calcula el cenbtro de la celda donde esta el robot
    const centerX = robot[0] * CELL_SIZE + CELL_SIZE / 2;
    const centerY = robot[1] * CELL_SIZE + CELL_SIZE / 2;

    ctx.save(); // Guarda el estado actual del canvas
    ctx.translate(centerX, centerY); // Mueve el origen al centro del robot
    ctx.rotate((angle * Math.PI) / 180);

    ctx.fillStyle = "#00f2ff";
    ctx.beginPath();

    // Dibuja el triangulo
    ctx.moveTo(0, -CELL_SIZE / 2.5);            // La punta en (0, -altura)
    ctx.lineTo(-CELL_SIZE / 3, CELL_SIZE / 3);  // Esquina inferior izquierda
    ctx.lineTo(CELL_SIZE / 3, CELL_SIZE / 3);   // Espina inferior derecha

    ctx.closePath();
    ctx.fill();

    ctx.restore(); // Restaura el canvas para que la matriz no se dibuje rotada

}

// --- LOOP DE ACTUALIZACION
setInterval(() => {
    if (isConnected && socket.readyState === WebSocket.OPEN) {
        socket.send("update");
    }
}, 500);