(function () {
    'use strict';

    const TEMPLATE = document.createElement('template');
    TEMPLATE.innerHTML = `
<style>
:host {
    --ss-bg: var(--semantic-search-bg, #262628);
    --ss-surface: var(--semantic-search-surface, #34363a);
    --ss-surface-alt: var(--semantic-search-surface-alt, #2e3034);
    --ss-border: var(--semantic-search-border, #5f6368);
    --ss-text: var(--semantic-search-text, #e8eaed);
    --ss-text-muted: var(--semantic-search-text-muted, #9aa0a6);
    --ss-accent: var(--semantic-search-accent, #f4f4f3);
    --ss-accent-text: var(--semantic-search-accent-text, #262628);
    --ss-radius: var(--semantic-search-radius, 16px);
    --ss-font: var(--semantic-search-font, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif);

    display: block;
    font-family: var(--ss-font);
    color: var(--ss-text);
    width: 100%;
    box-sizing: border-box;
}

:host([theme="light"]) {
    --ss-bg: #ffffff;
    --ss-surface: #f5f5f5;
    --ss-surface-alt: #eeeeee;
    --ss-border: #d0d0d0;
    --ss-text: #1a1a1a;
    --ss-text-muted: #666666;
    --ss-accent: #1a1a1a;
    --ss-accent-text: #ffffff;
}

* { box-sizing: border-box; margin: 0; padding: 0; }

.ss-container {
    width: 100%;
    max-width: 900px;
    margin: 0 auto;
    padding: 20px;
}

.ss-search-wrap {
    position: relative;
    width: 100%;
    max-width: 640px;
    margin: 0 auto 24px;
}

.ss-search-icon {
    position: absolute;
    left: 16px;
    top: 50%;
    transform: translateY(-50%);
    width: 20px;
    height: 20px;
    color: var(--ss-text-muted);
    pointer-events: none;
}

.ss-input {
    width: 100%;
    background: transparent;
    border: 1px solid var(--ss-border);
    border-radius: 9999px;
    padding: 14px 20px 14px 48px;
    color: var(--ss-text);
    font-size: 15px;
    font-family: var(--ss-font);
    outline: none;
    transition: border-color .2s, box-shadow .2s;
}

.ss-input::placeholder { color: var(--ss-text-muted); }
.ss-input:focus {
    border-color: var(--ss-text);
    box-shadow: 0 0 0 1px var(--ss-text);
}
.ss-input:disabled { opacity: .5; cursor: not-allowed; }

.ss-status {
    text-align: center;
    font-size: 12px;
    color: var(--ss-text-muted);
    margin-top: 8px;
    min-height: 18px;
    transition: opacity .3s;
}

.ss-status.error { color: #ef4444; }

.ss-results { display: none; }
.ss-results.visible { display: block; }

.ss-results-header {
    font-size: 17px;
    font-weight: 300;
    letter-spacing: .02em;
    margin-bottom: 16px;
    color: var(--ss-text);
}

.ss-card {
    display: flex;
    flex-direction: column;
    border-radius: var(--ss-radius);
    overflow: hidden;
    margin-bottom: 12px;
    cursor: pointer;
    transition: box-shadow .3s, transform .15s;
    box-shadow: 0 2px 8px rgba(0,0,0,.12);
}

.ss-card:hover {
    box-shadow: 0 4px 20px rgba(0,0,0,.25);
    transform: translateY(-1px);
}

@media (min-width: 640px) {
    .ss-card { flex-direction: row; }
}

.ss-card-img {
    flex: 0 0 auto;
    width: 100%;
    background: var(--ss-accent);
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 20px;
    min-height: 120px;
}

@media (min-width: 640px) {
    .ss-card-img { width: 25%; }
}

.ss-card-img img {
    max-height: 80px;
    max-width: 100%;
    object-fit: contain;
}

.ss-card-info {
    flex: 0 0 auto;
    width: 100%;
    background: var(--ss-surface-alt);
    padding: 20px;
    display: flex;
    flex-direction: column;
    justify-content: center;
    gap: 4px;
}

@media (min-width: 640px) {
    .ss-card-info { width: 25%; }
}

.ss-card-name {
    font-size: 14px;
    font-weight: 500;
    color: var(--ss-text);
    line-height: 1.4;
}

.ss-card-price {
    font-size: 14px;
    color: var(--ss-text);
    letter-spacing: .03em;
}

.ss-card-why {
    flex: 1 1 auto;
    width: 100%;
    background: var(--ss-surface);
    padding: 20px;
    display: flex;
    align-items: center;
}

@media (min-width: 640px) {
    .ss-card-why { width: 50%; }
}

.ss-card-why p {
    font-size: 14px;
    line-height: 1.6;
    color: var(--ss-text-muted);
    font-weight: 300;
}

.ss-empty {
    text-align: center;
    padding: 40px 20px;
    background: var(--ss-surface);
    border-radius: var(--ss-radius);
    border: 1px solid var(--ss-border);
}

.ss-empty-icon {
    width: 40px;
    height: 40px;
    color: var(--ss-text-muted);
    margin: 0 auto 12px;
}

.ss-empty p {
    color: var(--ss-text-muted);
    font-weight: 300;
    font-size: 14px;
}

.ss-detail { display: none; }
.ss-detail.visible { display: block; }

.ss-back {
    background: none;
    border: none;
    color: var(--ss-text-muted);
    font-size: 13px;
    cursor: pointer;
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 0;
    margin-bottom: 16px;
    font-family: var(--ss-font);
    transition: color .2s;
}

.ss-back:hover { color: var(--ss-text); }

.ss-back svg { width: 16px; height: 16px; }

.ss-detail-layout {
    display: flex;
    flex-direction: column;
    gap: 32px;
}

@media (min-width: 768px) {
    .ss-detail-layout { flex-direction: row; }
}

.ss-detail-left, .ss-detail-right { flex: 1; }

.ss-detail-img-wrap {
    background: var(--ss-surface);
    border-radius: var(--ss-radius);
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 40px;
    min-height: 350px;
    margin-bottom: 16px;
}

.ss-detail-img-wrap img {
    max-height: 100%;
    max-width: 100%;
    object-fit: contain;
    cursor: pointer;
    transition: transform .3s;
}

.ss-detail-img-wrap img:hover { transform: scale(1.05); }

.ss-detail-title {
    font-size: 26px;
    font-weight: 400;
    letter-spacing: .02em;
    margin-bottom: 4px;
}

.ss-detail-price {
    font-size: 18px;
    font-weight: 300;
    color: var(--ss-text-muted);
    margin-bottom: 8px;
}

.ss-detail-handle a {
    font-size: 13px;
    color: var(--ss-text-muted);
    text-decoration: underline;
    transition: color .2s;
}
.ss-detail-handle a:hover { color: var(--ss-text); }

.ss-detail-block {
    background: var(--ss-surface);
    border-radius: var(--ss-radius);
    padding: 24px;
    margin-bottom: 16px;
}

.ss-detail-block h3 {
    font-size: 13px;
    font-weight: 500;
    letter-spacing: .05em;
    margin-bottom: 8px;
    color: var(--ss-text);
}

.ss-detail-block p {
    font-size: 14px;
    line-height: 1.7;
    font-weight: 300;
    color: var(--ss-text-muted);
}

.ss-add-cart {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    background: var(--ss-accent);
    color: var(--ss-accent-text);
    border: none;
    border-radius: 9999px;
    padding: 14px 28px;
    font-size: 14px;
    font-weight: 500;
    font-family: var(--ss-font);
    cursor: pointer;
    transition: opacity .2s, transform .15s;
    margin-top: 12px;
}

.ss-add-cart:hover { opacity: .9; transform: scale(1.02); }

.ss-add-cart svg { width: 18px; height: 18px; }

.ss-fade-in {
    animation: ssFadeIn .4s ease;
}

@keyframes ssFadeIn {
    from { opacity: 0; transform: translateY(8px); }
    to   { opacity: 1; transform: translateY(0); }
}
</style>

<div class="ss-container">
    <form class="ss-search-wrap" id="searchForm">
        <svg class="ss-search-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" d="m21 21-5.197-5.197m0 0A7.5 7.5 0 1 0 5.196 5.196a7.5 7.5 0 0 0 10.607 10.607Z"/>
        </svg>
        <input type="text" class="ss-input" id="searchInput" autocomplete="off">
        <div class="ss-status" id="status"></div>
    </form>

    <div class="ss-results" id="results">
        <div class="ss-results-header" id="resultsHeader"></div>
        <div id="resultsList"></div>
    </div>

    <div class="ss-detail" id="detail">
        <button class="ss-back" id="backBtn">
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M10.5 19.5 3 12m0 0 7.5-7.5M3 12h18"/></svg>
            <span id="backText">Back to results</span>
        </button>
        <div class="ss-detail-layout">
            <div class="ss-detail-left">
                <div class="ss-detail-img-wrap">
                    <img id="detailImg" src="" alt="">
                </div>
                <h1 class="ss-detail-title" id="detailTitle"></h1>
                <p class="ss-detail-price" id="detailPrice"></p>
                <div class="ss-detail-handle" id="detailHandle"></div>
                <button class="ss-add-cart" id="addCartBtn">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M15.75 10.5V6a3.75 3.75 0 1 0-7.5 0v4.5m11.356-1.993 1.263 12c.07.665-.45 1.243-1.119 1.243H4.25a1.125 1.125 0 0 1-1.12-1.243l1.264-12A1.125 1.125 0 0 1 5.513 7.5h12.974c.576 0 1.059.435 1.119 1.007Z"/></svg>
                    <span id="addCartText">Add to cart</span>
                </button>
            </div>
            <div class="ss-detail-right">
                <div class="ss-detail-block">
                    <h3 id="whyLabel">Why we suggest it</h3>
                    <p id="detailWhy"></p>
                </div>
                <div class="ss-detail-block">
                    <h3 id="specsLabel">Description</h3>
                    <p id="detailSpecs"></p>
                </div>
            </div>
        </div>
    </div>
</div>
`;

    const LOCALES = {
        en: {
            placeholder: 'Describe a product you are looking for...',
            searching: 'Searching...',
            resultsFor: 'Results for:',
            noResults: 'Sorry, no products match your description.',
            back: 'Back to results',
            whyLabel: 'Why we suggest it',
            specsLabel: 'Description',
            addCart: 'Add to cart',
            loading: 'Loading details...',
            error: 'An error occurred. Please try again.'
        },
        it: {
            placeholder: 'Descrivi un prodotto che stai cercando...',
            searching: 'Ricerca in corso...',
            resultsFor: 'Risultati per:',
            noResults: 'Ci dispiace, nessun prodotto corrisponde alla tua descrizione.',
            back: 'Torna ai risultati',
            whyLabel: 'Perché lo suggeriamo',
            specsLabel: 'Descrizione',
            addCart: 'Aggiungi al carrello',
            loading: 'Caricamento dettagli...',
            error: 'Si è verificato un errore. Riprova.'
        },
        es: {
            placeholder: 'Describe un producto que estás buscando...',
            searching: 'Buscando...',
            resultsFor: 'Resultados para:',
            noResults: 'Lo sentimos, ningún producto coincide con tu descripción.',
            back: 'Volver a los resultados',
            whyLabel: 'Por qué lo sugerimos',
            specsLabel: 'Descripción',
            addCart: 'Añadir al carrito',
            loading: 'Cargando detalles...',
            error: 'Ocurrió un error. Inténtalo de nuevo.'
        },
        fr: {
            placeholder: 'Décrivez un produit que vous recherchez...',
            searching: 'Recherche en cours...',
            resultsFor: 'Résultats pour :',
            noResults: 'Désolé, aucun produit ne correspond à votre description.',
            back: 'Retour aux résultats',
            whyLabel: 'Pourquoi nous le suggérons',
            specsLabel: 'Description',
            addCart: 'Ajouter au panier',
            loading: 'Chargement des détails...',
            error: 'Une erreur est survenue. Veuillez réessayer.'
        },
        de: {
            placeholder: 'Beschreiben Sie ein Produkt, das Sie suchen...',
            searching: 'Suche läuft...',
            resultsFor: 'Ergebnisse für:',
            noResults: 'Leider stimmt kein Produkt mit Ihrer Beschreibung überein.',
            back: 'Zurück zu den Ergebnissen',
            whyLabel: 'Warum wir es vorschlagen',
            specsLabel: 'Beschreibung',
            addCart: 'In den Warenkorb',
            loading: 'Details werden geladen...',
            error: 'Ein Fehler ist aufgetreten. Bitte versuchen Sie es erneut.'
        }
    };

    const CURRENCY_SYMBOLS = {
        EUR: '€', USD: '$', GBP: '£', JPY: '¥', CHF: 'CHF',
        CAD: 'CA$', AUD: 'A$', CNY: '¥', KRW: '₩', BRL: 'R$'
    };

    class ShopifySemanticSearch extends HTMLElement {
        static get observedAttributes() {
            return ['api-url', 'placeholder', 'theme', 'max-results', 'currency', 'locale', 'shop-url'];
        }

        constructor() {
            super();
            this.attachShadow({ mode: 'open' });
            this.shadowRoot.appendChild(TEMPLATE.content.cloneNode(true));

            this._state = {
                lastQuery: '',
                lastResults: null,
                cachedDetails: {},
                currentProduct: null
            };

            this._els = {
                form: this.shadowRoot.getElementById('searchForm'),
                input: this.shadowRoot.getElementById('searchInput'),
                status: this.shadowRoot.getElementById('status'),
                results: this.shadowRoot.getElementById('results'),
                resultsHeader: this.shadowRoot.getElementById('resultsHeader'),
                resultsList: this.shadowRoot.getElementById('resultsList'),
                detail: this.shadowRoot.getElementById('detail'),
                backBtn: this.shadowRoot.getElementById('backBtn'),
                backText: this.shadowRoot.getElementById('backText'),
                detailImg: this.shadowRoot.getElementById('detailImg'),
                detailTitle: this.shadowRoot.getElementById('detailTitle'),
                detailPrice: this.shadowRoot.getElementById('detailPrice'),
                detailHandle: this.shadowRoot.getElementById('detailHandle'),
                detailWhy: this.shadowRoot.getElementById('detailWhy'),
                detailSpecs: this.shadowRoot.getElementById('detailSpecs'),
                whyLabel: this.shadowRoot.getElementById('whyLabel'),
                specsLabel: this.shadowRoot.getElementById('specsLabel'),
                addCartBtn: this.shadowRoot.getElementById('addCartBtn'),
                addCartText: this.shadowRoot.getElementById('addCartText')
            };
        }

        connectedCallback() {
            this._locale = LOCALES[this.getAttribute('locale') || 'en'] || LOCALES.en;
            this._currencySymbol = CURRENCY_SYMBOLS[
                (this.getAttribute('currency') || 'EUR').toUpperCase()
            ] || '€';
            this._apiUrl = (this.getAttribute('api-url') || '').replace(/\/$/, '');
            this._shopUrl = (this.getAttribute('shop-url') || '').replace(/\/$/, '');
            this._maxResults = parseInt(this.getAttribute('max-results') || '10');

            this._applyLocale();
            this._bindEvents();
        }

        attributeChangedCallback(name) {
            if (name === 'locale') {
                this._locale = LOCALES[this.getAttribute('locale') || 'en'] || LOCALES.en;
                this._applyLocale();
            }
        }

        _applyLocale() {
            const l = this._locale;
            this._els.input.placeholder = this.getAttribute('placeholder') || l.placeholder;
            this._els.backText.textContent = l.back;
            this._els.whyLabel.textContent = l.whyLabel;
            this._els.specsLabel.textContent = l.specsLabel;
            this._els.addCartText.textContent = l.addCart;
        }

        _bindEvents() {
            this._els.form.addEventListener('submit', (e) => {
                e.preventDefault();
                const q = this._els.input.value.trim();
                if (q) this._search(q);
            });

            this._els.backBtn.addEventListener('click', () => this._showResults());

            this._els.addCartBtn.addEventListener('click', () => {
                if (this._state.currentProduct) {
                    this._addToCart(this._state.currentProduct);
                }
            });
        }

        async _search(query) {
            this._els.input.disabled = true;
            this._showStatus(this._locale.searching);

            try {
                const res = await fetch(`${this._apiUrl}/api/search`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ query })
                });

                if (!res.ok) throw new Error('search failed');
                const data = await res.json();

                const products = (data.products || []).slice(0, this._maxResults);
                this._state.lastQuery = query;
                this._state.lastResults = products;

                this._renderResults(products, query);
                this._showResults();
            } catch (err) {
                console.error('[SemanticSearch]', err);
                this._state.lastResults = [];
                this._renderResults([], query);
                this._showResults();
            } finally {
                this._els.input.disabled = false;
                this._els.input.focus();
                this._clearStatus();
            }
        }

        async _loadDetails(productId, tags) {
            this._showStatus(this._locale.loading);

            try {
                let product = this._state.cachedDetails[productId];
                if (!product) {
                    const res = await fetch(`${this._apiUrl}/api/details`, {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            productId,
                            tags,
                            searchQuery: this._state.lastQuery
                        })
                    });
                    if (!res.ok) throw new Error('details failed');
                    product = await res.json();
                    this._state.cachedDetails[productId] = product;
                }

                this._renderDetail(product);
                this._showDetail();
            } catch (err) {
                console.error('[SemanticSearch]', err);
                this._showStatus(this._locale.error, true);
            } finally {
                this._clearStatus();
            }
        }

        _addToCart(product) {
            this.dispatchEvent(new CustomEvent('add-to-cart', {
                bubbles: true,
                composed: true,
                detail: {
                    productId: product.ID,
                    name: product.name,
                    price: product.price,
                    image: product.image,
                    handle: product.handle
                }
            }));

            if (this._shopUrl && product.handle) {
                window.location.href = `${this._shopUrl}/products/${product.handle}`;
            }
        }

        _renderResults(products, query) {
            this._els.resultsHeader.textContent = `${this._locale.resultsFor} ${query}`;

            if (!products || products.length === 0) {
                this._els.resultsList.innerHTML = `
                    <div class="ss-empty ss-fade-in">
                        <svg class="ss-empty-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
                            <path stroke-linecap="round" stroke-linejoin="round" d="m21 21-5.197-5.197m0 0A7.5 7.5 0 1 0 5.196 5.196a7.5 7.5 0 0 0 10.607 10.607Z"/>
                        </svg>
                        <p>${this._locale.noResults}</p>
                    </div>
                `;
                return;
            }

            this._els.resultsList.innerHTML = products.map((p, i) => `
                <div class="ss-card ss-fade-in" data-id="${p.ID}" data-tags="${encodeURIComponent(JSON.stringify(p.tags || []))}" style="animation-delay: ${i * 60}ms">
                    <div class="ss-card-img">
                        <img src="${this._escHtml(p.image)}" alt="${this._escHtml(p.name)}" onerror="this.style.display='none'">
                    </div>
                    <div class="ss-card-info">
                        <div class="ss-card-name">${this._escHtml(p.name)}</div>
                        <div class="ss-card-price">${this._currencySymbol}${p.price.toFixed(2)}</div>
                    </div>
                    <div class="ss-card-why">
                        <p>${this._escHtml(p.aiWhy || p.description || '')}</p>
                    </div>
                </div>
            `).join('');

            this._els.resultsList.querySelectorAll('.ss-card').forEach(card => {
                card.addEventListener('click', () => {
                    const id = card.dataset.id;
                    const tags = JSON.parse(decodeURIComponent(card.dataset.tags));
                    this._loadDetails(id, tags);
                });
            });
        }

        _renderDetail(product) {
            this._state.currentProduct = product;
            this._els.detailImg.src = product.image || '';
            this._els.detailImg.alt = product.name || '';
            this._els.detailTitle.textContent = product.name || '';
            this._els.detailPrice.textContent = `${this._currencySymbol}${product.price.toFixed(2)}`;
            this._els.detailWhy.textContent = product.aiWhy || '';
            this._els.detailSpecs.textContent = product.description || '';

            if (this._shopUrl && product.handle) {
                this._els.detailHandle.innerHTML = `<a href="${this._shopUrl}/products/${product.handle}" target="_blank">View on store →</a>`;
            } else {
                this._els.detailHandle.innerHTML = '';
            }
        }

        _showResults() {
            this._els.detail.classList.remove('visible');
            this._els.results.classList.add('visible');
            this._els.form.style.display = '';
        }

        _showDetail() {
            this._els.results.classList.remove('visible');
            this._els.form.style.display = 'none';
            this._els.detail.classList.add('visible');
        }

        _showStatus(msg, isError = false) {
            this._els.status.textContent = msg;
            this._els.status.className = `ss-status${isError ? ' error' : ''}`;
        }

        _clearStatus() {
            setTimeout(() => { this._els.status.textContent = ''; }, 2000);
        }

        _escHtml(str) {
            if (!str) return '';
            const div = document.createElement('div');
            div.textContent = str;
            return div.innerHTML;
        }
    }

    if (!customElements.get('shopify-semantic-search')) {
        customElements.define('shopify-semantic-search', ShopifySemanticSearch);
    }
})();
