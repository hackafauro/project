# Shopify Semantic Search

Componente astratto e riutilizzabile per aggiungere **ricerca semantica AI** a qualsiasi store Shopify.

L'utente descrive cosa cerca in linguaggio naturale (es. "ho bisogno di qualcosa per correre"), l'AI identifica i tag rilevanti nel catalogo, e restituisce i prodotti corrispondenti con una spiegazione trasparente.

## Architettura

```
┌──────────────────────────────────────────────────┐
│  Shopify Theme / Qualsiasi sito web              │
│  ┌────────────────────────────────────────────┐  │
│  │  <shopify-semantic-search>                 │  │
│  │  Web Component autocontenuto               │  │
│  └──────────────────┬─────────────────────────┘  │
└─────────────────────┼────────────────────────────┘
                      │ POST /api/search
                      │ POST /api/details
                      ▼
┌──────────────────────────────────────────────────┐
│  Backend (Node.js / Express)                     │
│  ┌──────────┐  ┌───────────┐  ┌──────────────┐  │
│  │ Shopify  │  │ AI Engine │  │ Express API  │  │
│  │  Sync    │  │ (OpenAI / │  │  /search     │  │
│  │  Module  │  │ OpenRouter)│  │  /details    │  │
│  └──────────┘  └───────────┘  └──────────────┘  │
└──────────────────────────────────────────────────┘
```

## Quick Start (5 minuti)

### 1. Configura il backend

```bash
cd backend
cp .env.example .env
# Modifica .env con le tue credenziali
npm install
npm start
```

Variabili `.env` richieste:

| Variabile | Descrizione |
|-----------|-------------|
| `SHOPIFY_DOMAIN` | Il dominio del tuo store (es. `my-store.myshopify.com`) |
| `SHOPIFY_ACCESS_TOKEN` | Token Admin API (Shopify Admin → Apps → Custom app) |
| `AI_API_KEY` | API key OpenRouter o OpenAI |
| `AI_MODEL` | Modello AI (default: `openai/gpt-4.1-nano`) |

### 2. Aggiungi il widget al tuo sito

#### Opzione A: HTML puro (qualsiasi sito)

```html
<script src="https://your-cdn.com/semantic-search.js"></script>

<shopify-semantic-search
  api-url="https://your-backend.com"
  shop-url="https://your-store.myshopify.com"
  locale="it"
  currency="EUR"
  theme="dark"
></shopify-semantic-search>
```

#### Opzione B: Shopify Liquid (tema Shopify)

1. Carica `widget/semantic-search.js` in **Assets** del tema
2. Carica `widget/semantic-search.liquid` in **Snippets** del tema
3. Includi dove vuoi:

```liquid
{% render 'semantic-search',
   api_url: 'https://your-backend.com',
   locale: 'it',
   currency: 'EUR',
   theme: 'dark'
%}
```

## Configurazione

### Attributi HTML

| Attributo | Default | Descrizione |
|-----------|---------|-------------|
| `api-url` | — | **Obbligatorio.** URL del backend |
| `shop-url` | — | URL dello store per link diretti ai prodotti |
| `locale` | `en` | Lingua UI (`it`, `en`, `es`, `fr`, `de`) |
| `currency` | `EUR` | Valuta (`EUR`, `USD`, `GBP`, `JPY`, etc.) |
| `theme` | `dark` | Tema colori (`dark` / `light`) |
| `max-results` | `10` | Numero massimo risultati |
| `placeholder` | auto | Testo placeholder della barra di ricerca |

### CSS Custom Properties (tema avanzato)

Puoi sovrascrivere qualsiasi colore del widget con CSS custom properties:

```css
shopify-semantic-search {
    --semantic-search-bg: #1a1a2e;
    --semantic-search-surface: #16213e;
    --semantic-search-text: #eee;
    --semantic-search-accent: #e94560;
    --semantic-search-accent-text: #fff;
    --semantic-search-radius: 12px;
}
```

### Eventi

Il componente emette eventi custom per integrazione con il tuo tema:

```javascript
document.querySelector('shopify-semantic-search')
  .addEventListener('add-to-cart', (e) => {
    console.log('Prodotto:', e.detail);
    // { productId, name, price, image, handle }
  });
```

## API Backend

### `POST /api/search`

```json
// Request
{ "query": "ho bisogno di qualcosa per correre" }

// Response
{
  "tags": ["running", "sport", "fitness"],
  "products": [
    {
      "ID": "123",
      "name": "Scarpe Running Pro",
      "price": 89.99,
      "image": "https://...",
      "description": "...",
      "aiWhy": "Queste scarpe sono perfette per la corsa..."
    }
  ]
}
```

### `POST /api/details`

```json
// Request
{ "productId": "123", "tags": ["running"], "searchQuery": "..." }

// Response
{ "ID": "123", "name": "...", "aiWhy": "...", ... }
```

### `GET /api/tags`

Restituisce tutti i tag unici dei prodotti nel catalogo.

### `GET /api/health`

Health check del server.

## Deploy

Il backend puo' essere deployato su qualsiasi piattaforma Node.js:

- **Railway** / **Render** / **Fly.io** — deploy diretto da Git
- **Vercel** — con adapter serverless
- **VPS** — con PM2 o Docker

Il widget JS puo' essere servito da:

- **Shopify Assets** (caricato nel tema)
- **CDN** (Cloudflare, jsDelivr, etc.)
- **Lo stesso backend** servendo file statici

## Struttura File

```
shopify-semantic-search/
├── backend/
│   ├── server.js           # Express API server
│   ├── shopify-sync.js     # Sync prodotti da Shopify Admin API
│   ├── ai-engine.js        # Motore AI (tag extraction + spiegazioni)
│   ├── package.json
│   └── .env.example
├── widget/
│   ├── semantic-search.js      # Web Component (frontend)
│   └── semantic-search.liquid  # Snippet Liquid per Shopify
└── README.md
```
