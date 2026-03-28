const gameModal = document.getElementById('gameModal');
const gameIframe = document.getElementById('gameIframe');

function openGameModal() {
    const isDino = Math.random() < 0.5;
    gameIframe.src = isDino ? "https://urstore.dev/game/dino/game.html" : "https://urstore.dev/game/flappy/game.html";
    gameModal.classList.remove('opacity-0', 'pointer-events-none');
}

function closeGameModal() {
    gameModal.classList.add('opacity-0', 'pointer-events-none');
    setTimeout(() => {
        gameIframe.src = "";
    }, 300);
}

const imageModal = document.getElementById('imageModal');
const zoomedImg = document.getElementById('zoomedImg');

function openImageModal() {
    zoomedImg.src = DOM.dImg.src;
    imageModal.classList.remove('opacity-0', 'pointer-events-none');
}

function closeImageModal() {
    imageModal.classList.add('opacity-0', 'pointer-events-none');
}
