#!/usr/bin/env node
/**
 * Generate offline config for Trio Pebble (following timeboxed-watchface pattern)
 * Uses data:text/html;charset=utf-8, + encodeURIComponent (more reliable than base64)
 */

const fs = require('fs');
const path = require('path');

const htmlPath = path.join(__dirname, '..', 'config', 'index.html');
const html = fs.readFileSync(htmlPath, 'utf8');

// Use encodeURIComponent (like timeboxed) instead of base64
const encoded = encodeURIComponent(html);

const moduleContent = `module.exports = function() {
    return "${encoded}";
};
`;

// Main app
const mainDir = path.join(__dirname, '..', 'src', 'pkjs', 'settings');
fs.mkdirSync(mainDir, { recursive: true });
fs.writeFileSync(path.join(mainDir, 'generated.js'), moduleContent);
console.log('Updated main: src/js/settings/generated.js');

// Remote app
const remoteDir = path.join(__dirname, '..', 'remote-app', 'src', 'pkjs', 'settings');
fs.mkdirSync(remoteDir, { recursive: true });
fs.writeFileSync(path.join(remoteDir, 'generated.js'), moduleContent);
console.log('Updated remote-app: remote-app/src/js/settings/generated.js');

console.log('Encoded length:', encoded.length);
console.log('Done. Rebuild both apps.');
