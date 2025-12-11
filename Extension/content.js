function scanScores() {
    // Vérifier si le contexte de l'extension est toujours valide
    if (!chrome.runtime?.id) {
        console.log('Extension context invalidé, arrêt du scan');
        clearInterval(scanInterval);
        return;
    }

    try {
        chrome.storage.sync.get(["autoMode"], ({ autoMode }) => {
            if (!autoMode) return;

            const elA = document.getElementById("player1-score");
            const elB = document.getElementById("player2-score");
            if (!elA || !elB) return;

            const a = parseInt(elA.innerText);
            const b = parseInt(elB.innerText);

            if (Number.isNaN(a) || Number.isNaN(b)) return;

            chrome.runtime.sendMessage({
                type: "UPDATE_SCORE",
                a, b
            }, (response) => {
                // Gérer l'erreur si le contexte est invalidé
                if (chrome.runtime.lastError) {
                    console.log('Message non envoyé:', chrome.runtime.lastError.message);
                }
            });
        });
    } catch (e) {
        console.log('Erreur dans scanScores:', e.message);
    }
}

const scanInterval = setInterval(scanScores, 500);
