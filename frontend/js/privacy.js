function initPrivacyPopup() {
    if (!localStorage.getItem('urStore_privacy_accepted')) {
        const popup = document.getElementById('privacyPopup');
        setTimeout(() => {
            popup.classList.remove('opacity-0', 'pointer-events-none', 'translate-y-10');
        }, 1000);
    }
}

function acceptPrivacy() {
    localStorage.setItem('urStore_privacy_accepted', 'true');
    const popup = document.getElementById('privacyPopup');
    popup.classList.add('opacity-0', 'pointer-events-none', 'translate-y-10');
}
