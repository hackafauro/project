const https = require('https');

const TAG_SELECTOR_PROMPT = `
You are an intelligent tag selector. You have the following list of tags:

%TAGS%

When I give you a short description or a sentence about an item I need, you will return **only the relevant tags from the above list** that specifically describe the item, separated by commas, without extra explanation.

Example:
Input: "I need a new pair of running shoes for jogging"
Output: "fitness, sport, shoes"

Input: "I want a laptop with good battery life and lightweight"
Output: "laptop, ultrabook, battery, pc"

Now process the following input and give me only the matching tags:
%QUERY%`;

const TRANSPARENCY_PROMPT = `
You are giving an honest and transparent answer to the user.
The user searched: %QUERY%

The search matched these tags: %TAGS%
The suggested item is: %ITEM_NAME%
The item description: %ITEM_DESC%

Explain briefly (1-2 sentences max) why this item matches what the user is looking for. Write naturally, not as a list. Do not repeat the item name.`;

class AIEngine {
    constructor({ apiKey, apiUrl, model }) {
        this.apiKey = apiKey;
        this.apiUrl = apiUrl || 'https://openrouter.ai/api/v1/chat/completions';
        this.model = model || 'openai/gpt-4.1-nano';
    }

    async ask(prompt) {
        const payload = JSON.stringify({
            model: this.model,
            messages: [{ role: 'user', content: prompt }],
            temperature: 0.2
        });

        return new Promise((resolve, reject) => {
            const url = new URL(this.apiUrl);
            const req = https.request({
                hostname: url.hostname,
                port: url.port || 443,
                path: url.pathname,
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${this.apiKey}`,
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(payload)
                }
            }, (res) => {
                let body = '';
                res.on('data', chunk => body += chunk);
                res.on('end', () => {
                    if (res.statusCode !== 200) {
                        return reject(new Error(`AI API ${res.statusCode}: ${body}`));
                    }
                    try {
                        const parsed = JSON.parse(body);
                        const content = parsed.choices?.[0]?.message?.content || '';
                        resolve(content.trim());
                    } catch (e) {
                        reject(new Error('Failed to parse AI response'));
                    }
                });
            });
            req.on('error', reject);
            req.write(payload);
            req.end();
        });
    }

    async extractTags(dbTags, userQuery) {
        const prompt = TAG_SELECTOR_PROMPT
            .replace('%TAGS%', JSON.stringify(dbTags))
            .replace('%QUERY%', userQuery);

        const response = await this.ask(prompt);
        return this._splitTags(response);
    }

    async explainMatch(userQuery, tags, itemName, itemDescription) {
        const prompt = TRANSPARENCY_PROMPT
            .replace('%QUERY%', userQuery)
            .replace('%TAGS%', tags.join(', '))
            .replace('%ITEM_NAME%', itemName)
            .replace('%ITEM_DESC%', itemDescription);

        return this.ask(prompt);
    }

    _splitTags(raw) {
        if (!raw) return [];
        return raw
            .split(/[,\n]/)
            .map(t => t.trim().toLowerCase().replace(/^["']|["']$/g, ''))
            .filter(Boolean);
    }
}

module.exports = AIEngine;
