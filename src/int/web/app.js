const canvas = document.getElementById('mapCanvas');
const ctx = canvas.getContext('2d');
const statusDot = document.querySelector('.status-dot');
const statusText = document.getElementById('status');

const CELL_SIZE = 20;
const GRID_SIZE = 20;

// Estado de la conexión con el backend y del canal de telemetría.
let socket;
let isConnected = false;

// Estado local del reproductor; refleja la intención del usuario, no el backend por sí solo.
let isPlaying = false;

// Cache local de la playlist recibida desde el servidor.
let songs = [];

// Se intenta reconectar solo si la sesión ya fue validada por auth.js.
window.onload = () => {
    if (authManager.checkAuth()) {
        connect();
    }
};

// Abre el WebSocket y registra manejadores para mapa, audio y estado del sistema.
function connect() {
    if (socket && socket.readyState === WebSocket.OPEN) return;

    const wsUrl = `ws://127.0.0.1:8080/ws`;

    console.log("Conectando WebSocket a: ", wsUrl);
    socket = new WebSocket(wsUrl);

    socket.onopen = () => {
        isConnected = true;
        
        // El primer mensaje debe ser el token para autorizar la sesión en el servidor.
        const session = JSON.parse(sessionStorage.getItem('robot_session'));
        if (session && session.token) {
            socket.send(session.token);
        }

        statusDot.className = "status-dot online";
        statusText.innerHTML = '<span class="status-dot online"></span> Conectado al Robot';
    };

    socket.onmessage = (event) => {
        const data = JSON.parse(event.data);

        // El backend envía el mapa completo para evitar desincronización parcial.
        if (data.map) {
            renderMap(data.map.grid, data.map.robot, data.map.angle);
        }

        // El bloque de audio se refresca junto con la telemetría recibida por WebSocket.
        if (data.audio) {
            updateAudioUI(data.audio);
        }

        if (data.status) {
            updateUI(data.status);
        }

        socket.onclose = () => {
            isConnected = false;
            statusDot.className = "status-dot offline";
            statusText.innerHTML = '<span class="status-dot offline"></span> Desconectado';
            // La reconexión es diferida para evitar ciclos agresivos ante caídas transitorias.
            setTimeout(connect, 2000);
        }

    };
}

// Sincroniza los indicadores LED y el estado de los botones con el modo operativo recibido.
function updateUI(status) {
    document.getElementById('led-system').className = status.system ? 'led led-blue' : 'led led-off';
    document.getElementById('led-manual').className = status.manual ? 'led led-green' : 'led led-off';
    document.getElementById('led-auto').className = status.autonomous ? 'led led-green' : 'led led-off';
    document.getElementById('led-obstacle').className = status.obstacle ? 'led led-red' : 'led led-off';

    // La UI resalta el modo activo y deshabilita el control manual cuando el sistema entra en autónomo.
    const btnManual = document.getElementById('btn-manual');
    const btnAuto = document.getElementById('btn-auto');

    if (status.autonomous) {
        btnAuto.style.backgroundColor = '#2ecc71';
        btnManual.style.backgroundColor = '#666';
        disableManualControls(true);
    } else {
        btnAuto.style.backgroundColor = '#666';
        btnManual.style.backgroundColor = '#2ecc71';
        disableManualControls(false);
    }
}

// Habilita o bloquea la botonera manual para evitar comandos inconsistentes con el modo activo.
function disableManualControls(disabled) {
    const buttons = document.querySelectorAll('.control-btn');
    buttons.forEach(btn => {
        btn.disabled = disabled;
        btn.style.opacity = disabled ? "0.5" : "1.0";
        btn.style.cursor = disabled ? "not-allowed" : "pointer";
        btn.style.pointerEvents = disabled ? "none" : "auto";
    });
}

// --- LÓGICA DE INTERFAZ DE AUDIO ---
// Actualiza el panel del reproductor con la pista, el tiempo y el volumen reportados por el backend.
function updateAudioUI(audioData) {
    const trackInfo = document.getElementById('track-info');
    const timeCurrent = document.getElementById('time-current');
    const timeTotal = document.getElementById('time-total');
    const progressBar = document.getElementById('progress-bar');
    const volumeBar = document.getElementById('volume-bar');
    const volumeText = document.getElementById('volume-text');

    // La pista puede venir vacía si aún no hay reproducción activa.
    if (trackInfo) trackInfo.innerText = audioData.track || "Sin reproducción";

    // El tiempo se formatea en el cliente para mantener estable el contrato del backend.
    if (timeCurrent) timeCurrent.innerText = formatTime(audioData.current);
    if (timeTotal) timeTotal.innerText = formatTime(audioData.total);

    // El progreso depende del tiempo total reportado por el reproductor.
    if (progressBar && audioData.total > 0) {
        const percentage = (audioData.current / audioData.total) * 100
        progressBar.style.width = `${percentage}%`;
    }

    // El volumen se refleja como porcentaje para mantener sincronía con el backend.
    if (volumeBar && audioData.volume !== undefined) {
        volumeBar.style.width = `${audioData.volume}%`;
        if(volumeText) volumeText.innerText = `${audioData.volume}%`;

    }
}

// Convierte segundos a una cadena mm:ss para la interfaz de progreso.
function formatTime(secs) {
    const mins = Math.floor(secs / 60);
    const s = Math.floor(secs % 60);
    return `${mins}:${s < 10 ? '0' : ''}${s}`;
}

// --- ACCIONES HACIA EL SERVIDOR ---
// Envía una acción autenticada al prefijo /api y deja al backend resolver la operación exacta.
async function sendAction(endpoint) {
    const session = JSON.parse(sessionStorage.getItem('robot_session'));
    const token = session ? session.token : null;

    if (!token) return;

    try {
        const response = await fetch(`/api/${endpoint}` , {
            headers: { 'Authorization': token }
        });
        if (!response.ok) throw new Error('Error en la peticion');
        console.log(`Accion exitosa: ${endpoint}`);
    } catch (error) {
        console.error('Error al enviar accion:', error);
        if (error.message === 'No autorizado') authManager.logout();
    }
}

// Las acciones de movimiento y modo solo traducen eventos de UI a rutas del backend.
function move(dir) { sendAction(`move/${dir}`); }
function setMode(m) { sendAction(`mode/${m}`); }

// Cambia entre play y pause; el estado local solo refleja la intención del usuario.
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

// Carga la lista de reproducción bajo sesión autenticada y la pinta en un menú dinámico.
function togglePlaylist() {
    const menu = document.getElementById('playlist-menu');
    const listContainer = document.getElementById('songs-list');

    const session = JSON.parse(sessionStorage.getItem('robot_session'));
    const token = session ? session.token : null;

    if (menu.style.display === 'none' || menu.style.display === '') {
        fetch('/api/audio/playlist', {
            headers: { 'Authorization': token }
        })
        .then(response => response.json())
        .then(data => {
            // El backend expone la playlist como un arreglo simple de nombres de pista.
            const songsArray = data.songs;

            if (!Array.isArray(songsArray)) {
                console.error("La respuesta no contiene un arreglo de canciones:", data);
                return;
            }

            listContainer.innerHTML = '';
            
            songsArray.forEach(song => {
                const item = document.createElement('div');
                item.innerText = song;
                item.className = "playlist-item";
                
                // El estilo inline evita depender de una clase adicional para este menú dinámico.
                item.style.padding = "8px";
                item.style.cursor = "pointer";
                item.style.borderBottom = "1px solid #333";
                
                item.onmouseover = () => item.style.background = "#333";
                item.onmouseout = () => item.style.background = "transparent";
                
                item.onclick = () => {
                    // Se reutiliza el mismo token al pedir la reproducción de una pista concreta.
                    fetch(`/api/audio/play_specific?name=${encodeURIComponent(song)}`, { 
                        method: 'POST',
                        headers: { 'Authorization': token }
                    });
                    menu.style.display = 'none';
                };
                listContainer.appendChild(item);
            });
            menu.style.display = 'block';
        })
        .catch(err => console.error("Error al cargar la playlist:", err));
    } else {
        menu.style.display = 'none';
    }
}
// Detiene la reproducción sin asumir que el backend liberó la pista actual.
function stopMusic() { 
    isPlaying = false;
    sendAction('audio/stop'); 
    document.getElementById('btn-play-pause').innerText = "⏸";
}

// Avanza o retrocede la pista actual en la playlist del backend.
function nextSong() { 
    isPlaying = true;
    sendAction('audio/next'); 
}

function prevSong() { 
    isPlaying = true;
    sendAction('audio/prev'); 
}

// Salta tiempo relativo dentro de la pista actual; `dir` se espera como `forward` o `back`.
function skipTime(dir) { sendAction(`audio/${dir}`); }

// Ajusta el volumen por pasos discretos; `dir` se espera como `up` o `down`.
function changeVolume(dir) { sendAction(`audio/volume/${dir}`); }

// --- DIBUJO DEL MAPA ---
// Renderiza la grilla, los obstáculos y la posición/orientación del robot sobre el canvas.
function renderMap(grid, robot, angle) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // El mapa se pinta celda por celda para conservar una grilla visual consistente.
    for (let y = 0; y < GRID_SIZE; y++) {
        for (let x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x] === 1) {
                ctx.fillStyle = '#777';
                ctx.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            } else if (grid[y][x] == 2) {
                ctx.fillStyle = '#ff4d4d';
                ctx.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            }
            ctx.strokeStyle = '#222';
            ctx.strokeRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        }
    }

    // El triángulo se centra en la celda actual y rota según el ángulo reportado.
    const centerX = robot[0] * CELL_SIZE + CELL_SIZE / 2;
    const centerY = robot[1] * CELL_SIZE + CELL_SIZE / 2;

    ctx.save();
    ctx.translate(centerX, centerY);
    ctx.rotate((angle * Math.PI) / 180);

    ctx.fillStyle = "#00f2ff";
    ctx.beginPath();

    // La geometría apunta hacia arriba antes de aplicar la rotación del robot.
    ctx.moveTo(0, -CELL_SIZE / 2.5);
    ctx.lineTo(-CELL_SIZE / 3, CELL_SIZE / 3);
    ctx.lineTo(CELL_SIZE / 3, CELL_SIZE / 3);

    ctx.closePath();
    ctx.fill();

    ctx.restore();

}

// Cierre explícito de la conexión de telemetría al salir de la sesión.
window.closeRobotConnection = () => {
    if (socket) {
        console.log("Cerrando conexión WebSocket por Logout...");
        socket.close();
    }
};

// --- LOOP DE ACTUALIZACIÓN ---
// Solicita un snapshot periódico al backend mientras el canal siga abierto.
setInterval(() => {
    if (isConnected && socket.readyState === WebSocket.OPEN) {
        socket.send("update");
    }
}, 500);