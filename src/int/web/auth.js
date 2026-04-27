// auth.js - Gestión de Autenticación para el Robot

const authManager = {
    // Alternar entre formularios de Login y Registro
    toggleAuth: function(isRegister) {
        document.getElementById('login-form').style.display = isRegister ? 'none' : 'block';
        document.getElementById('register-form').style.display = isRegister ? 'block' : 'none';
        document.getElementById('auth-title').innerText = isRegister ? 'Nuevo Usuario' : 'Control de Acceso';
        document.getElementById('auth-message').innerText = '';
    },

    // Gestion de sesion volatil
    setSession: function(username) {
        sessionStorage.setItem('robot_session', JSON.stringify({
            user: username,
            timestamp: new Date().getTime()
        }));
        document.getElementById('auth-overlay').style.display = 'none';
        // Disparar la conexión WebSocket una vez autenticado
        if (typeof connect === 'function') connect();
    },

    // Verificar si existe una sesión válida al cargar
    checkAuth: function() {
        const session = sessionStorage.getItem('robot_session');
        if (session) {
            document.getElementById('auth-overlay').style.display = 'none';
            return true;
        }
        return false;
    },

    logout: async function() {
        // Obtener el token antes de limpiar la sesion
        const session = JSON.parse(sessionStorage.getItem('robot_session'));
        const token = session? session.token : null;

        try {
            // Notificar al servidor para invalidar el token
            if (token) {
                await fetch('/logout', {
                    method: 'POST',
                    headers: { 'Authorization': token}
                });
            }
        } catch(err) {
            console.error("Error al cerrar sesión en el servidor:", err)
        } finally {
            // Cierre esplicito del WebSocket
            if (window.closeRobotConnection) {
                 window.closeRobotConnection();
            }

            // Limpiar almacenamiento local y recargar
            sessionStorage.removeItem('robot_session');
            location.reload();
        }
    }
}; 

// Hacerlo visible para el HTML
window.authManager = authManager;

function toggleAuth(isRegister) {
    authManager.toggleAuth(isRegister);
}

function togglePassword(inputId) {
    const input = document.getElementById(inputId);
    if (inputId.type === "password") {
        input.type = "text";
    } else {
        input.type = "password";
    }
}

document.addEventListener('DOMContentLoaded', () => {
    const regPassword = document.getElementById('reg-password');

    if (regPassword) {
        regPassword.addEventListener('input', function() {
            const val = this.value;

            const rules = {
                'reg-length':   val.length >= 8,
                'reg-upper':    /[A-Z]/.test(val),
                'reg-number':   /\d/.test(val),
                'reg-special':  /[@$!%*?&#./]/.test(val)
            };

            for (const [id, met] of Object.entries(rules)) {
                const el = document.getElementById(id);
                if (el) {
                    el.className = met ? 'valid' : 'invalid';
                    el.innerText = (met ? '✔ ' : '✖ ') + el.innerText.substring(2);
                }
            }
        });
    }
});

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

    // Validación final antes de enviar
    const passRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&#./])[A-Za-z\d@$!%*?&#./]{8,}$/;
    if (!passRegex.test(pass)) {
        msg.style.color = "#ff4d4d";
        msg.innerText = "La clave no cumple los requisitos de seguridad";
        return;
    }

    try {
        const response = await fetch('/register', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username: user, password: pass })
        });

        const data = await response.json();
        if (response.ok) {
            msg.style.color = "#2ecc71";
            msg.innerText = data.message;
            setTimeout(() => authManager.toggleAuth(false), 1500);
        } else {
            msg.style.color = "#ff4d4d";
            msg.innerText = data.message || "Error al registrar";
        }
    } catch (err) {
        msg.innerText = "Error de comunicación con el servidor";
    }
}

// Exponer funciones al scope global explícitamente
window.authManager = authManager;
window.toggleAuth = (isReg) => authManager.toggleAuth(isReg);
window.togglePassword = togglePassword;
window.handleLogin = handleLogin;
window.handleRegister = handleRegister;