// auth.js - Gestión de autenticación para el panel del robot.

const authManager = {
    // Alterna entre los formularios de login y registro sin recargar la página.
    toggleAuth: function(isRegister) {
        document.getElementById('login-form').style.display = isRegister ? 'none' : 'block';
        document.getElementById('register-form').style.display = isRegister ? 'block' : 'none';
        document.getElementById('auth-title').innerText = isRegister ? 'Nuevo Usuario' : 'Control de Acceso';
        document.getElementById('auth-message').innerText = '';
    },

    // Persiste una sesión volátil en sessionStorage y activa la UI protegida.
    setSession: function(username, token) {
        sessionStorage.setItem('robot_session', JSON.stringify({
            user: username,
            token: token,
            timestamp: new Date().getTime()
        }));

        document.getElementById('auth-overlay').style.display = 'none';
        // El canal WebSocket solo se abre después de autenticar correctamente al usuario.
        if (typeof connect === 'function') connect();
    },

    // Verifica si existe una sesión en memoria al cargar la interfaz.
    checkAuth: function() {
        const session = sessionStorage.getItem('robot_session');
        if (session) {
            document.getElementById('auth-overlay').style.display = 'none';
            return true;
        }
        return false;
    },

    // Informa al backend antes de limpiar el estado local para cerrar sesión de forma consistente.
    logout: async function() {
        const session = JSON.parse(sessionStorage.getItem('robot_session'));
        const token = session? session.token : null;

        try {
            if (token) {
                await fetch('/api/audio/stop', {
                    method: 'GET',
                    headers: { 'Authorization': token}
                });

                await fetch('/api/move/stop', {
                    method: 'GET',
                    headers: { 'Authorization': token}
                });

                await fetch('/logout', {
                    method: 'POST',
                    headers: { 'Authorization': token}
                });
            }
        } catch(err) {
            console.error("Error al cerrar sesión en el servidor:", err)
        } finally {
            // El cierre explícito del WebSocket evita mensajes tardíos después del logout.
            if (window.closeRobotConnection) {
                 window.closeRobotConnection();
            }

            // Se limpia la sesión del navegador y se reinicia la vista para un estado limpio.
            sessionStorage.removeItem('robot_session');
            location.reload();
        }
    }
}; 

// Se expone el gestor para que HTML y otros scripts puedan invocar sus métodos.
window.authManager = authManager;

function toggleAuth(isRegister) {
    authManager.toggleAuth(isRegister);
}

function togglePassword(inputId, button) {
    const input = document.getElementById(inputId);
    if (input.type === "password") {
        input.type = "text";
        button.innerText = "✘";
    } else {
        input.type = "password";
        button.innerText = "👁️"
    }
}

document.addEventListener('DOMContentLoaded', () => {
    const regPassword = document.getElementById('reg-password');

    if (regPassword) {
        // Valida la contraseña incrementalmente para actualizar el checklist en vivo.
        regPassword.addEventListener('input', function() {
            const pass = this.value;

            const requirements = {
                'req-length':   pass.length >= 8,
                'req-upper':    /[A-Z]/.test(pass),
                'req-lower':    /[a-z]/.test(pass),
                'req-number':   /\d/.test(pass),
                'req-special':  /[@$!%*?&#./]/.test(pass)
            };

            for (const [id, isValid] of Object.entries(requirements)) {
                const el = document.getElementById(id);
                if (el) {
                    el.className = isValid ? 'valid' : 'invalid';
                    el.innerText = (isValid ? '✔ ' : '✘ ') + el.innerText.substring(2);
                }
            }
        });
    }
});

// Realiza login remoto y almacena la sesión si el backend devuelve un token válido.
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
            authManager.setSession(user, data.token || "token_simulado");
        } else {
            msg.innerText = data.message || "Error al iniciar sesión";
        }
    } catch (err) {
        msg.innerText = "Error de conexión con el servidor";
    }
}

// Ejecuta el registro remoto tras validar localmente la política mínima de contraseña.
async function handleRegister() {
    const user = document.getElementById('reg-username').value;
    const pass = document.getElementById('reg-password').value;
    const msg = document.getElementById('auth-message');

    // Validación final antes de enviar para evitar round-trips innecesarios al servidor.
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

// Se exponen los handlers al scope global porque el HTML los invoca desde atributos onclick.
window.authManager = authManager;
window.toggleAuth = (isReg) => authManager.toggleAuth(isReg);
window.togglePassword = togglePassword;
window.handleLogin = handleLogin;
window.handleRegister = handleRegister;