const authModal = document.getElementById('authModal');
const loginSection = document.getElementById('loginSection');
const profileSection = document.getElementById('profileSection');
const loginError = document.getElementById('loginError');

function initAuth() {
    const savedToken = localStorage.getItem('urStore_token');
    const savedUser = localStorage.getItem('urStore_user');

    if (savedToken && savedUser) {
        window.urStore.token = savedToken;
        window.urStore.user = JSON.parse(savedUser);
        document.getElementById('authIndicator').classList.remove('hidden');

        if (window.urStore.user.discountPlayable == true) {
            document.getElementById('consoleBtn').classList.remove('hidden');
        }
    }
}

function toggleAuthModal() {
    const isHidden = authModal.classList.contains('opacity-0');

    if (isHidden) {
        loginError.classList.add('hidden');
        document.getElementById('loginForm').reset();

        if (window.urStore.user) {
            loginSection.classList.add('hidden');
            profileSection.classList.remove('hidden');
            document.getElementById('profileInitial').textContent = window.urStore.user.name.charAt(0).toUpperCase();
            document.getElementById('profileName').textContent = `${window.urStore.user.name} ${window.urStore.user.surname}`;
            document.getElementById('profileEmail').textContent = window.urStore.user.email;

            const savedLimits = JSON.parse(localStorage.getItem('urStore_limits') || '{}');
            document.getElementById('spendingLimit').value = savedLimits.spending || '';
            document.getElementById('itemLimit').value = savedLimits.items || '';
        } else {
            profileSection.classList.add('hidden');
            loginSection.classList.remove('hidden');
        }

        authModal.classList.remove('opacity-0', 'pointer-events-none');
    } else {
        authModal.classList.add('opacity-0', 'pointer-events-none');
    }
}

async function handleLoginSubmit(e) {
    e.preventDefault();

    const email = document.getElementById('loginEmail').value.trim();
    const password = document.getElementById('loginPassword').value;
    const btnText = document.getElementById('loginBtnText');
    const spinner = document.getElementById('loginSpinner');
    const btn = document.getElementById('loginBtn');

    btn.disabled = true;
    btnText.classList.add('hidden');
    spinner.classList.remove('hidden');
    loginError.classList.add('hidden');

    try {
        const token = await sha256(email + password);

        const response = await fetch('https://urstore.dev/api/self', {
            method: 'GET',
            headers: {
                'Accept': 'application/json',
                'Authorization': token
            }
        });

        if (!response.ok) throw new Error();

        const userData = await response.json();

        window.urStore.token = token;
        window.urStore.user = userData;

        localStorage.setItem('urStore_token', token);
        localStorage.setItem('urStore_user', JSON.stringify(userData));

        document.getElementById('authIndicator').classList.remove('hidden');

        const consoleBtn = document.getElementById('consoleBtn');
        if (userData.discountPlayable == true) {
            consoleBtn.classList.remove('hidden');
        } else {
            consoleBtn.classList.add('hidden');
        }

        toggleAuthModal();
        showStatus(`Benvenuto, ${userData.name}!`);
    } catch (error) {
        loginError.textContent = "Autenticazione fallita. Riprova.";
        loginError.classList.remove('hidden');
    } finally {
        btn.disabled = false;
        btnText.classList.remove('hidden');
        spinner.classList.add('hidden');
    }
}

function logout() {
    window.urStore.token = null;
    window.urStore.user = null;
    localStorage.removeItem('urStore_token');
    localStorage.removeItem('urStore_user');
    localStorage.removeItem('urStore_limits');

    document.getElementById('authIndicator').classList.add('hidden');
    document.getElementById('consoleBtn').classList.add('hidden');

    toggleAuthModal();
    showStatus("Ti sei disconnesso con successo.");
}

async function syncUserData(retries = 1) {
    if (!window.urStore.token) return;

    try {
        const response = await fetch('https://urstore.dev/api/self', {
            method: 'GET',
            headers: getApiHeaders()
        });

        if (response.ok) {
            const oldUser = window.urStore.user;
            const userData = await response.json();

            window.urStore.user = userData;
            localStorage.setItem('urStore_user', JSON.stringify(userData));

            const consoleBtn = document.getElementById('consoleBtn');
            if (userData.discountPlayable == true) {
                consoleBtn.classList.remove('hidden');
            } else {
                consoleBtn.classList.add('hidden');
            }

            const isNowAvailable = (userData.discountAvailable == true);
            const wasAvailableBefore = oldUser ? (oldUser.discountAvailable == true) : false;

            if (isNowAvailable && !wasAvailableBefore) {
                triggerDiscountCelebration();
                return;
            }

            if (!isNowAvailable && retries > 1) {
                setTimeout(() => syncUserData(retries - 1), 1500);
            }
        }
    } catch (error) {
        console.error(error);
    }
}

function triggerDiscountCelebration() {
    if (typeof confetti === 'function') {
        confetti({
            particleCount: 120,
            spread: 80,
            origin: { y: 0.6 },
            colors: ['#a855f7', '#ec4899', '#3b82f6', '#10b981', '#f59e0b']
        });
    }

    const discountPopup = document.getElementById('discountPopup');
    const discountPopupInner = document.getElementById('discountPopupInner');
    discountPopup.classList.remove('opacity-0', 'pointer-events-none');
    setTimeout(() => {
        discountPopupInner.classList.remove('scale-95');
        discountPopupInner.classList.add('scale-100');
    }, 50);
}

function closeDiscountPopup() {
    const discountPopup = document.getElementById('discountPopup');
    const discountPopupInner = document.getElementById('discountPopupInner');
    discountPopupInner.classList.remove('scale-100');
    discountPopupInner.classList.add('scale-95');
    setTimeout(() => {
        discountPopup.classList.add('opacity-0', 'pointer-events-none');
    }, 300);
}

function saveLimits() {
    const limits = {
        spending: document.getElementById('spendingLimit').value,
        items: document.getElementById('itemLimit').value
    };
    localStorage.setItem('urStore_limits', JSON.stringify(limits));
}
