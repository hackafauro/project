window.urStore = {
    lastSearchQuery: "",
    lastSearchResults: null,
    cachedDetails: {},
    token: null,
    user: null
};

function getApiHeaders() {
    const headers = {
        'Content-Type': 'application/json',
        'Accept': 'application/json'
    };
    if (window.urStore.token) {
        headers['Authorization'] = window.urStore.token;
    }
    return headers;
}

async function sha256(message) {
    const msgBuffer = new TextEncoder().encode(message);
    const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}
