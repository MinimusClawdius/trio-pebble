#!/usr/bin/env node
/**
 * Generate offline config base64 for Trio Pebble settings.
 * Run: node scripts/generate-offline-config.js
 * This updates src/pkjs/config_offline.js with base64 of config/index.html
 */

const fs = require('fs');
const path = require('path');

const htmlPath = path.join(__dirname, '..', 'config', 'index.html');
const outputPath = path.join(__dirname, '..', 'src', 'pkjs', 'config_offline.js');

console.log('Reading HTML from:', htmlPath);
const html = fs.readFileSync(htmlPath, 'utf8');

// Base64 encode the HTML
const b64 = Buffer.from(html, 'utf8').toString('base64');

const jsContent = `var OFFLINE_CONFIG_BASE64 = "${b64}";
`;

fs.writeFileSync(outputPath, jsContent);
console.log('Updated offline config base64 in:', outputPath);
console.log('Base64 length:', b64.length);
