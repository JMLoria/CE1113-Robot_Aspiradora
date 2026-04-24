// auth.js - Gestión de Autenticación para el Robot


const authManager = {
    // Alternar entre formularios de Login y Registro
    toggleAuth: function(isRegister) {
        document.getElementById('login-form').style.display = isRegister ? 'none' : 'block';
        document.getElementById('register-form').style.display = isRegister ? 'block' : 'none';
        document.getElementById('auth-title').innerText = isRegister ? 'Nuevo Usuario' : 'Control de Acceso';
        document.getElementById('auth-message').innerText = '';
    },

    // Guardar sesión y ocultar modal
    setSession: function(username) {
        localStorage.setItem('robot_session', JSON.stringify({
            user: username,
            timestamp: new Date().getTime()
        }));
        document.getElementById('auth-overlay').style.display = 'none';
        // Disparar la conexión WebSocket una vez autenticado
        if (typeof connect === 'function') connect();
    },

    // Verificar si existe una sesión válida al cargar
    checkAuth: function() {
        const session = localStorage.getItem('robot_session');
        if (session) {
            document.getElementById('auth-overlay').style.display = 'none';
            return true;
        }
        return false;
    },

    logout: function() {
        localStorage.removeItem('robot_session');
        location.reload(); // Recarga para bloquear todo de nuevo
    }
}; 

// Función para el Login
async function handleLogin() {
    const user = document.getElementById('login-username').value;
    const pass = document.getElementById('login-password').value;
    const msg = document.getElementById('auth-message');

    try {
        const response = await fetch('/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username: user, password: pass })
        });

        const data = await response.json();

        if (response.ok) {
            authManager.setSession(user);
        } else {
            msg.innerText = data.message || "Error al iniciar sesión";
        }
    } catch (err) {
        msg.innerText = "Error de conexión con el servidor";
    }
}

// Función para el Registro
async function handleRegister() {
    const user = document.getElementById('reg-username').value;
    const pass = document.getElementById('reg-password').value;
    const msg = document.getElementById('auth-message');

    // Validación básica en frontend antes de enviar
    const passRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]{8,}$/;
    if (!passRegex.test(pass)) {
        msg.innerText = "La clave no cumple los requisitos de seguridad";
        return;
    }

    try {
        const response = await fetch('/register', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username: user, password: pass })
        });

        if (response.ok) {
            msg.style.color = "#2ecc71";
            msg.innerText = "¡Registro exitoso! Ya puedes entrar.";
            setTimeout(() => authManager.toggleAuth(false), 2000);
        } else {
            msg.style.color = "#ff4d4d";
            msg.innerText = "Error: El usuario ya existe o es inválido";
        }
    } catch (err) {
        msg.innerText = "Error al conectar con el servidor";
    }
}