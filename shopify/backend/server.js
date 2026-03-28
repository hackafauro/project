require('dotenv').config();

const express = require('express');
const cors = require('cors');
const ShopifySync = require('./shopify-sync');
const AIEngine = require('./ai-engine');

const app = express();
app.use(cors());
app.use(express.json());

const shopify = new ShopifySync({
    shopDomain: process.env.SHOPIFY_DOMAIN,
    accessToken: process.env.SHOPIFY_ACCESS_TOKEN,
    cacheTTL: parseInt(process.env.CACHE_TTL_MS || '900000')
});

const ai = new AIEngine({
    apiKey: process.env.AI_API_KEY,
    apiUrl: process.env.AI_API_URL || 'https://openrouter.ai/api/v1/chat/completions',
    model: process.env.AI_MODEL || 'openai/gpt-4.1-nano'
});

app.get('/api/health', (req, res) => {
    res.json({ status: 'ok', shop: process.env.SHOPIFY_DOMAIN });
});

app.post('/api/search', async (req, res) => {
    try {
        const { query } = req.body;
        if (!query) return res.status(400).json({ error: 'query is required' });

        const dbTags = await shopify.getTags();
        if (!dbTags.length) {
            return res.json({ tags: [], products: [] });
        }

        const matchedTags = await ai.extractTags(dbTags, query);
        const products = await shopify.getProductsByTags(matchedTags);

        const enriched = await Promise.all(
            products.slice(0, 10).map(async (p) => {
                try {
                    const aiWhy = await ai.explainMatch(query, matchedTags, p.name, p.description);
                    return { ...p, aiWhy };
                } catch {
                    return { ...p, aiWhy: '' };
                }
            })
        );

        res.json({ tags: matchedTags, products: enriched });
    } catch (err) {
        console.error('[Search Error]', err.message);
        res.status(500).json({ error: 'search failed' });
    }
});

app.post('/api/details', async (req, res) => {
    try {
        const { productId, tags, searchQuery } = req.body;
        if (!productId) return res.status(400).json({ error: 'productId is required' });

        const product = await shopify.getProductById(productId);
        if (!product) return res.status(404).json({ error: 'product not found' });

        const aiWhy = await ai.explainMatch(
            searchQuery || '',
            tags || [],
            product.name,
            product.description
        );

        res.json({ ...product, aiWhy });
    } catch (err) {
        console.error('[Details Error]', err.message);
        res.status(500).json({ error: 'details failed' });
    }
});

app.get('/api/tags', async (req, res) => {
    try {
        const tags = await shopify.getTags();
        res.json({ tags });
    } catch (err) {
        res.status(500).json({ error: 'failed to get tags' });
    }
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`[SemanticSearch] Server avviato su http://localhost:${PORT}`);
    shopify.syncProducts().catch(err => {
        console.error('[Sync Error] Primo sync fallito:', err.message);
    });
});
