const canvas = document.getElementById('mapCanvas');
const ctx = canvas.getContext('2d');
const statusDot = document.querySelector('.status-dot');
const statusText = document.getElementById('status');

const CELL_SIZE = 20;
const GRID_SIZE = 20;

// Estados de conexión
let socket;
let isConnected = false;

// Inicializar WebSocket
function connect() {
    socket = new WebSocket('ws://${window.location.host}/ws');

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
        console.log("Datos recibidos del servidor:", data);
        renderMap(data.grid, data.robot);
    };

    socket.onclose = () => {
        isConnected = false;
        statusDot.className = "status-dot offline";
        statusText.innerHTML = '<span class="status-dot offline"></span> Desconectado';

        startSimulation(); // Como no hay robot aun, simulamos para probar visuales
    };
}

// Dibujar el mapa en el Canvas
function renderMap(grid, robot) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (let y = 0; y < grid.length; y++) {
        for (let x = 0; x < grid[y].length; x++) {
            switch(gird[y][x]) {
                case 0: ctx.fillStyle = '#111'; break;      // Desconocido
                case 1: ctx.fillStyle = '#444'; break;      // Libre
                case 2: ctx.fillStyle = '#e74c3c'; break;   // Obstaculo
            }
            ctx.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1);
        }
    }

    // Dibujar Robot 
    ctx.fillStyle = '#00f2ff';
    ctx.beginPath();
    ctx.arc(robot[0] * CELL_SIZE + CELL_SIZE / 2, robot[1] * CELL_SIZE + CELL_SIZE / 2, 7, 0, Math.PI * 2);
    ctx.fill();
}

//Funciones de Control (API Fetch)
async function sendAction(endpoint) {
    console.log(`Enviando a /api/${endpoint}`);
    try {
        await fetch(`/api/${endpoint}`)
    } catch (err) {
        console.warn("Servidor no disponible para fetch.");
    }
}

// Para actualizacion cada 10s
setInterval(() => {
    pedirActualizacion();
}, 1000);

function move(dir) { sendAction(`move/${dir}`); }
function setMode(m) { sendAction(`mode/${m}`); }
function playMusic(track) { sendAction(`audio/play/${track}`); }
function stopMusic() { sendAction(`audio/stop`); }

function pedirActualizacion() {
    // Verificamos que el WebSocket esté conectado antes de enviar
    if (socket && socket.readyState === WebSocket.OPEN) {
        console.log("Solicitando actualización de mapa al servidor...");
        socket.send("update"); // Enviamos la cadena "update" que el C++ espera
    } else {
        console.error("No se puede actualizar: WebSocket desconectado.");
        alert("El robot no esta conectado.");
    }
}

// --- LOGICA DE PRUEBA (SIMULACION) ---
function startSimulation() {
    let mockRobot = [10, 10];
    let mockGrid = Array(GRID_SIZE).fill().map(() => Array(GRID_SIZE).fill(0));

    setInterval(() => {
        if (isConnected) return;
        // Mover aleatoriamente para probar el dibujo
        mockRobot[0] = Math.max(0, Math.min(19, mockRobot[0] + (Math.floor(Math.random() * 3) - 1)));
        mockRobot[1] = Math.max(0, Math.min(19, mockRobot[1] + (Math.floor(Math.random() * 3) - 1)));
        mockGrid[mockRobot[1]][mockRobot[0]] = 1;

        // Simular un obstaculo aleatorio
        if(Math.random() > 0.9) {
            mockGrid[Math.floor(Math.random() * 20)][Math.floor(Math.random() * 20)] = 2;
        }

        renderMap(mockGrid, mockRobot);
    }, 500);

}

// Iniciar 
connect();