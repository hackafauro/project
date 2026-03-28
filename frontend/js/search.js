async function handleSearch(query) {
    DOM.searchInput.disabled = true;
    showStatus("Ricerca in corso...");

    try {
        const response = await fetch('https://urstore.dev/api/search', {
            method: 'POST',
            headers: getApiHeaders(),
            body: JSON.stringify({ query: query })
        });

        let products = [];
        if (response.ok) {
            const data = await response.json();
            products = data.products || [];
        }

        window.urStore.lastSearchResults = products;
        window.urStore.lastSearchQuery = query;

        renderResults(products, query);
        showResultsView();
    } catch (error) {
        window.urStore.lastSearchResults = [];
        window.urStore.lastSearchQuery = query;
        renderResults([], query);
        showResultsView();
    } finally {
        DOM.searchInput.disabled = false;
        DOM.searchInput.focus();
        clearTimeout(statusTimeout);
        DOM.statusMessage.classList.remove('opacity-100');
        DOM.statusMessage.classList.add('opacity-0');
    }
}

function renderResults(products, query) {
    DOM.resultsHeader.innerHTML = `<p class="text-gray-100 text-lg font-light tracking-wide">Risultati per: ${query}</p>`;

    if (!products || products.length === 0) {
        DOM.resultsList.innerHTML = `
            <div class="flex flex-col items-center justify-center p-10 mt-6 bg-[#34363a]/30 rounded-2xl border border-[#3f4042]">
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-10 h-10 text-gray-500 mb-4">
                  <path stroke-linecap="round" stroke-linejoin="round" d="m21 21-5.197-5.197m0 0A7.5 7.5 0 1 0 5.196 5.196a7.5 7.5 0 0 0 10.607 10.607Z" />
                </svg>
                <p class="text-center text-gray-400 font-light tracking-wide">Ci dispiace, non ci sono elementi che possono tornarti utili.</p>
            </div>
        `;
        return;
    }

    DOM.resultsList.innerHTML = products.map(product => {
        const safeTags = encodeURIComponent(JSON.stringify(product.tags || []));
        const whySuggest = product.aiWhy && product.aiWhy.trim() !== "" ? product.aiWhy : product.description;

        return `
        <article onclick="loadProductDetails('${product.ID}', '${safeTags}')" class="flex flex-col md:flex-row w-full rounded-2xl overflow-hidden shadow-md cursor-pointer hover:ring-1 hover:ring-gray-500 transition-all duration-300">
            <div class="w-full md:w-1/4 bg-[#f4f4f3] flex items-center justify-center p-6 min-h-[140px]">
                <img src="${product.image}" class="max-h-24 w-auto object-contain mix-blend-multiply" onerror="this.style.display='none'">
            </div>
            <div class="w-full md:w-1/4 bg-[#2e3034] p-6 flex flex-col justify-center space-y-1">
                <h3 class="text-gray-200 text-sm font-medium leading-snug">${product.name}</h3>
                <p class="text-gray-300 text-sm tracking-wide">\u20AC${product.price.toFixed(2)}</p>
            </div>
            <div class="w-full md:w-2/4 bg-[#34363a] p-6 flex flex-col justify-center">
                <p class="text-gray-300 text-sm leading-relaxed font-light">${whySuggest}</p>
            </div>
        </article>
        `;
    }).join('');
}

async function loadProductDetails(productId, encodedTags) {
    showStatus("Caricamento dettagli...");
    const tags = JSON.parse(decodeURIComponent(encodedTags));
    const requestBody = { productId: productId, tags: tags, searchQuery: window.urStore.lastSearchQuery };

    try {
        let product;
        if (window.urStore.cachedDetails[productId]) {
            product = window.urStore.cachedDetails[productId];
        } else {
            const response = await fetch('https://urstore.dev/api/details', {
                method: 'POST',
                headers: getApiHeaders(),
                body: JSON.stringify(requestBody)
            });
            if (!response.ok) throw new Error();
            product = await response.json();
            window.urStore.cachedDetails[productId] = product;
        }

        DOM.dImg.src = product.image;
        DOM.dTitle.textContent = product.name;
        DOM.dPrice.textContent = `\u20AC${product.price.toFixed(2)}`;
        DOM.dWhy.textContent = product.aiWhy && product.aiWhy.trim() !== "" ? product.aiWhy : "Nessuna motivazione generata.";
        DOM.dSpecs.textContent = product.description;

        if (product.reviews && product.reviews.length > 0) {
            DOM.reviewsList.innerHTML = product.reviews.map(r => `
                <div class="bg-[#2e3034] p-5 rounded-xl border border-[#3f4042]/50">
                    <div class="flex items-center space-x-2 mb-2">
                        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" class="w-4 h-4 text-yellow-500"><path fill-rule="evenodd" d="M10.788 3.21c.448-1.077 1.976-1.077 2.424 0l2.082 5.006 5.404.434c1.164.093 1.636 1.545.749 2.305l-4.117 3.527 1.257 5.273c.271 1.136-.964 2.033-1.96 1.425L12 18.354 7.373 21.18c-.996.608-2.231-.29-1.96-1.425l1.257-5.273-4.117-3.527c-.887-.76-.415-2.212.749-2.305l5.404-.434 2.082-5.005Z" clip-rule="evenodd" /></svg>
                        <span class="text-[#f4f4f3] text-sm font-medium">${r.rating}/5</span>
                    </div>
                    <div class="text-gray-300 text-sm leading-relaxed font-light">"${r.text}"</div>
                </div>
            `).join('');
            DOM.reviewsContainer.classList.remove('hidden');
        } else {
            DOM.reviewsContainer.classList.add('hidden');
        }

        clearTimeout(statusTimeout);
        DOM.statusMessage.classList.remove('opacity-100');
        DOM.statusMessage.classList.add('opacity-0');
        showDetailsView();
    } catch (error) {
        showStatus("Errore durante il caricamento del prodotto.", true);
    }
}
