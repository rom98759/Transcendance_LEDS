async function sendRequest(url) {
    try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 2000);

        const response = await fetch(url, {
            method: 'GET',
            signal: controller.signal,
            mode: 'no-cors'
        });

        clearTimeout(timeoutId);
        console.log('✓ Requête envoyée:', url);
        return true;
    } catch (error) {
        if (error.name === 'AbortError') {
            console.error('✗ Timeout (2s):', url);
        } else {
            console.error('✗ Erreur:', error.message, url);
        }
        return false;
    }
}

chrome.runtime.onMessage.addListener((msg, sender) => {
    chrome.storage.sync.get("arduinoIP", ({ arduinoIP }) => {
        if (!arduinoIP) {
            console.error('IP non configurée');
            return;
        }

        if (msg.type === "UPDATE_SCORE") {
            const url = `${arduinoIP}/score?a=${msg.a}&b=${msg.b}`;
            console.log('📤 Envoi score:', msg.a, '/', msg.b);
            sendRequest(url);
        } else if (msg.type === "RESET") {
            const url = `${arduinoIP}/reset`;
            console.log('📤 Envoi RESET');
            sendRequest(url);
        } else if (msg.type === "SET_COLORS") {
            const url = `${arduinoIP}/color?ar=${msg.colorA.r}&ag=${msg.colorA.g}&ab=${msg.colorA.b}&br=${msg.colorB.r}&bg=${msg.colorB.g}&bb=${msg.colorB.b}`;
            console.log('🎨 Envoi couleurs:', msg.colorA, msg.colorB);
            sendRequest(url);
        } else if (msg.type === "SET_BRIGHTNESS") {
            const url = `${arduinoIP}/brightness?value=${msg.brightness}`;
            console.log('💡 Envoi luminosité:', msg.brightness);
            sendRequest(url);
        }
    });
});
