initAuth();

document.getElementById('loginForm').addEventListener('submit', handleLoginSubmit);

DOM.searchForm.addEventListener('submit', function(event) {
    event.preventDefault();
    const query = DOM.searchInput.value.trim();
    if (query) {
        DOM.resultsContainer.classList.add('opacity-0');
        setTimeout(() => handleSearch(query), 300);
    }
});

document.getElementById('spendingLimit').addEventListener('input', saveLimits);
document.getElementById('itemLimit').addEventListener('input', saveLimits);

window.addEventListener('message', function(event) {
    if (typeof event.data === 'string' && event.data.toLowerCase().includes('close')) {
        closeGameModal();
        setTimeout(() => syncUserData(3), 1000);
    }
});

document.addEventListener('keydown', function(event) {
    if (event.key === "Escape" && !imageModal.classList.contains('opacity-0')) {
        closeImageModal();
    }
});

initPrivacyPopup();
