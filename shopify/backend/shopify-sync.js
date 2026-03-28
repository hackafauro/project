const https = require('https');
const http = require('http');

class ShopifySync {
    constructor({ shopDomain, accessToken, cacheTTL = 15 * 60 * 1000 }) {
        this.shopDomain = shopDomain.replace(/^https?:\/\//, '').replace(/\/$/, '');
        this.accessToken = accessToken;
        this.cacheTTL = cacheTTL;

        this.products = [];
        this.tags = [];
        this.lastSync = 0;
    }

    async fetchAllProducts() {
        let allProducts = [];
        let url = `https://${this.shopDomain}/admin/api/2024-10/products.json?limit=250&status=active`;

        while (url) {
            const { data, nextLink } = await this._shopifyGet(url);
            if (data.products) {
                allProducts = allProducts.concat(data.products);
            }
            url = nextLink;
        }

        return allProducts;
    }

    _shopifyGet(url) {
        return new Promise((resolve, reject) => {
            const protocol = url.startsWith('https') ? https : http;
            const req = protocol.get(url, {
                headers: {
                    'X-Shopify-Access-Token': this.accessToken,
                    'Content-Type': 'application/json'
                }
            }, (res) => {
                let body = '';
                res.on('data', chunk => body += chunk);
                res.on('end', () => {
                    if (res.statusCode !== 200) {
                        return reject(new Error(`Shopify API ${res.statusCode}: ${body}`));
                    }

                    let nextLink = null;
                    const linkHeader = res.headers['link'];
                    if (linkHeader) {
                        const match = linkHeader.match(/<([^>]+)>;\s*rel="next"/);
                        if (match) nextLink = match[1];
                    }

                    resolve({ data: JSON.parse(body), nextLink });
                });
            });
            req.on('error', reject);
        });
    }

    _normalizeProduct(shopifyProduct) {
        const variant = shopifyProduct.variants?.[0];
        const image = shopifyProduct.image?.src || shopifyProduct.images?.[0]?.src || '';

        return {
            ID: String(shopifyProduct.id),
            name: shopifyProduct.title,
            tags: shopifyProduct.tags
                ? shopifyProduct.tags.split(',').map(t => t.trim().toLowerCase()).filter(Boolean)
                : [],
            price: variant ? parseFloat(variant.price) : 0,
            description: shopifyProduct.body_html
                ? shopifyProduct.body_html.replace(/<[^>]*>/g, '').trim()
                : '',
            image: image,
            handle: shopifyProduct.handle,
            vendor: shopifyProduct.vendor,
            productType: shopifyProduct.product_type,
            reviews: []
        };
    }

    _extractAllTags(products) {
        const tagSet = new Set();
        products.forEach(p => p.tags.forEach(t => tagSet.add(t)));
        return Array.from(tagSet).sort();
    }

    async syncProducts() {
        console.log(`[ShopifySync] Sincronizzazione prodotti da ${this.shopDomain}...`);
        const raw = await this.fetchAllProducts();
        this.products = raw.map(p => this._normalizeProduct(p));
        this.tags = this._extractAllTags(this.products);
        this.lastSync = Date.now();
        console.log(`[ShopifySync] ${this.products.length} prodotti, ${this.tags.length} tag unici.`);
        return { products: this.products, tags: this.tags };
    }

    async getProducts() {
        if (Date.now() - this.lastSync > this.cacheTTL) {
            await this.syncProducts();
        }
        return this.products;
    }

    async getTags() {
        if (Date.now() - this.lastSync > this.cacheTTL) {
            await this.syncProducts();
        }
        return this.tags;
    }

    async getProductById(id) {
        const products = await this.getProducts();
        return products.find(p => p.ID === String(id)) || null;
    }

    async getProductsByTags(tags) {
        const products = await this.getProducts();
        const tagSet = new Set(tags.map(t => t.toLowerCase().trim()));
        return products.filter(p =>
            p.tags.some(t => tagSet.has(t.toLowerCase()))
        );
    }
}

module.exports = ShopifySync;
