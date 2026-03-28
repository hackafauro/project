let statusTimeout;

const DOM = {
    mainArea: document.getElementById('mainArea'),
    searchForm: document.getElementById('searchForm'),
    searchInput: document.getElementById('searchInput'),
    statusMessage: document.getElementById('statusMessage'),
    resultsContainer: document.getElementById('resultsContainer'),
    resultsList: document.getElementById('resultsList'),
    resultsHeader: document.getElementById('resultsHeader'),
    detailsContainer: document.getElementById('detailsContainer'),
    dImg: document.getElementById('detailImg'),
    dTitle: document.getElementById('detailTitle'),
    dPrice: document.getElementById('detailPrice'),
    dWhy: document.getElementById('detailWhy'),
    dSpecs: document.getElementById('detailSpecs'),
    reviewsContainer: document.getElementById('reviewsContainer'),
    reviewsList: document.getElementById('reviewsList')
};

function resetToHome() {
    DOM.resultsContainer.classList.add('hidden', 'opacity-0');
    DOM.detailsContainer.classList.add('hidden', 'opacity-0');
    DOM.searchForm.classList.remove('hidden');
    DOM.mainArea.classList.remove('justify-start', 'pt-10');
    DOM.mainArea.classList.add('justify-center');
    DOM.searchInput.value = "";
    window.urStore.lastSearchQuery = "";
}

function showResultsView() {
    DOM.detailsContainer.classList.add('hidden', 'opacity-0');
    DOM.mainArea.classList.remove('justify-center');
    DOM.mainArea.classList.add('justify-start', 'pt-10');
    DOM.searchForm.classList.remove('hidden');
    DOM.resultsContainer.classList.remove('hidden');
    setTimeout(() => DOM.resultsContainer.classList.remove('opacity-0'), 50);
}

function showDetailsView() {
    DOM.resultsContainer.classList.add('hidden', 'opacity-0');
    DOM.searchForm.classList.add('hidden');
    DOM.mainArea.classList.remove('justify-center');
    DOM.mainArea.classList.add('justify-start', 'pt-10');
    DOM.detailsContainer.classList.remove('hidden');
    setTimeout(() => DOM.detailsContainer.classList.remove('opacity-0'), 50);
}

function showStatus(message, isError = false) {
    clearTimeout(statusTimeout);
    DOM.statusMessage.textContent = message;
    DOM.statusMessage.className = `absolute -bottom-8 left-0 w-full text-center text-xs transition-opacity duration-300 opacity-100 ${isError ? 'text-red-400' : 'text-gray-500'}`;
    statusTimeout = setTimeout(() => {
        DOM.statusMessage.classList.remove('opacity-100');
        DOM.statusMessage.classList.add('opacity-0');
    }, 3000);
}
