document.getElementById("save-ip").addEventListener("click", () => {
    let ip = document.getElementById("arduino-ip").value.trim();

    // Ajouter http:// automatiquement si absent
    if (ip && !ip.startsWith('http://') && !ip.startsWith('https://')) {
        ip = 'http://' + ip;
    }

    chrome.storage.sync.set({ arduinoIP: ip });
    document.getElementById("arduino-ip").value = ip;
    alert("IP enregistrée : " + ip);
});

document.getElementById("send-score").addEventListener("click", () => {
    const a = parseInt(document.getElementById("manual-a").value);
    const b = parseInt(document.getElementById("manual-b").value);

    chrome.runtime.sendMessage({
        type: "UPDATE_SCORE",
        a: a,
        b: b
    });

    // Détection de victoire
    if (a >= 5) {
        showVictory('A', 'blue');
    } else if (b >= 5) {
        showVictory('B', 'red');
    }
});

// Fonction d'animation de victoire
function showVictory(player, color) {
    const overlay = document.getElementById('victory-overlay');
    const title = overlay.querySelector('.victory-title');
    const content = overlay.querySelector('.victory-content');

    title.textContent = `PLAYER ${player} GAGNE !`;
    content.style.borderColor = color === 'blue' ? '#00d4ff' : '#ff0080';
    content.style.boxShadow = color === 'blue'
        ? '0 0 50px rgba(0, 212, 255, 0.8), inset 0 0 30px rgba(0, 212, 255, 0.3)'
        : '0 0 50px rgba(255, 0, 128, 0.8), inset 0 0 30px rgba(255, 0, 128, 0.3)';
    title.style.color = color === 'blue' ? '#00d4ff' : '#ff0080';
    title.style.textShadow = color === 'blue'
        ? '0 0 20px rgba(0, 212, 255, 1)'
        : '0 0 20px rgba(255, 0, 128, 1)';

    overlay.classList.remove('hidden');

    // Masquer après 3 secondes
    setTimeout(() => {
        overlay.classList.add('hidden');
    }, 3000);
}

document.getElementById("reset").addEventListener("click", () => {
    chrome.runtime.sendMessage({
        type: "RESET"
    });
});

document.getElementById("toggle-auto").addEventListener("click", () => {
    chrome.storage.sync.get("autoMode", ({ autoMode }) => {
        const newMode = !autoMode;
        chrome.storage.sync.set({ autoMode: newMode });
        const btn = document.getElementById("toggle-auto");
        btn.textContent = newMode ? "✓ Mode Auto Actif" : "Mode Auto Inactif";
        btn.classList.toggle("active", newMode);
    });
});

document.getElementById("reset").addEventListener("click", () => {
    chrome.runtime.sendMessage({ type: "RESET" });
    // Reset visuel des inputs
    document.getElementById("manual-a").value = "0";
    document.getElementById("manual-b").value = "0";
});

// Fonction pour convertir hex en RGB
function hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}

// Appliquer les couleurs
document.getElementById("apply-colors").addEventListener("click", () => {
    const colorA = hexToRgb(document.getElementById("color-a").value);
    const colorB = hexToRgb(document.getElementById("color-b").value);

    chrome.runtime.sendMessage({
        type: "SET_COLORS",
        colorA: colorA,
        colorB: colorB
    });
});

// Réinitialiser les couleurs
document.getElementById("reset-colors").addEventListener("click", () => {
    document.getElementById("color-a").value = "#0000ff";
    document.getElementById("color-b").value = "#ff0000";
    document.getElementById("brightness").value = "128";
    document.getElementById("brightness-value").textContent = "50%";

    chrome.runtime.sendMessage({
        type: "SET_COLORS",
        colorA: { r: 0, g: 0, b: 255 },
        colorB: { r: 255, g: 0, b: 0 }
    });

    chrome.runtime.sendMessage({
        type: "SET_BRIGHTNESS",
        brightness: 128
    });
});

// Gestion du slider de luminosité
document.getElementById("brightness").addEventListener("input", (e) => {
    const value = e.target.value;
    const percent = Math.round((value / 255) * 100);
    document.getElementById("brightness-value").textContent = percent + "%";
});

document.getElementById("brightness").addEventListener("change", (e) => {
    const brightness = parseInt(e.target.value);
    chrome.runtime.sendMessage({
        type: "SET_BRIGHTNESS",
        brightness: brightness
    });
});

async function checkPing() {
    chrome.storage.sync.get(["arduinoIP"], async ({ arduinoIP }) => {
        if (!arduinoIP) return;

        const statusDot = document.querySelector(".status-dot");
        const statusText = document.querySelector(".status-text");
        const pingDisplay = document.querySelector(".ping-display");

        const startTime = performance.now();

        try {
            const response = await fetch(`${arduinoIP}/health`, {
                method: 'GET',
                mode: 'no-cors'
            });

            const endTime = performance.now();
            const ping = Math.round(endTime - startTime);

            statusDot.classList.remove("offline");
            statusText.classList.remove("offline");
            statusText.textContent = "ONLINE";
            pingDisplay.textContent = `PING: ${ping}ms`;
        } catch (error) {
            statusDot.classList.add("offline");
            statusText.classList.add("offline");
            statusText.textContent = "OFFLINE";
            pingDisplay.textContent = "PING: --ms";
        }
    });
}

// Charger l'IP et l'état au démarrage
chrome.storage.sync.get(["arduinoIP", "autoMode"], ({ arduinoIP, autoMode }) => {
    if (arduinoIP) {
        document.getElementById("arduino-ip").value = arduinoIP;
        checkPing();
        // Vérifier le ping toutes les 3 secondes
        setInterval(checkPing, 3000);
    }
    const btn = document.getElementById("toggle-auto");
    btn.textContent = autoMode ? "✓ Mode Auto Actif" : "Mode Auto Inactif";
    btn.classList.toggle("active", autoMode);
});
