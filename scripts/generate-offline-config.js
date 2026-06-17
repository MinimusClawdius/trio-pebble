#!/usr/bin/env node
/**
 * Generate offline config base64 for Trio Pebble settings (main watchface + remote-app).
 * Run: node scripts/generate-offline-config.js
 * This keeps both apps in sync with config/index.html
 */

const fs = require('fs');
const path = require('path');

const htmlPath = path.join(__dirname, '..', 'config', 'index.html');
const html = fs.readFileSync(htmlPath, 'utf8');
const b64 = Buffer.from(html, 'utf8').toString('base64');

const jsContent = `var OFFLINE_CONFIG_BASE64 = "${b64}";
`;

// Main watchface
const mainOutput = path.join(__dirname, '..', 'src', 'pkjs', 'config_offline.js');
fs.writeFileSync(mainOutput, jsContent);
console.log('Updated main:', mainOutput);

// Remote app
const remoteOutput = path.join(__dirname, '..', 'remote-app', 'src', 'pkjs', 'config_offline.js');
fs.writeFileSync(remoteOutput, jsContent);
console.log('Updated remote-app:', remoteOutput);

console.log('Base64 length:', b64.length);
console.log('Done. Rebuild both apps after running this.');
