#!/usr/bin/env node
/**
 * Syncs the version from package.json into config/index.html
 * Run this before committing or as part of the build.
 */
const fs = require('fs');
const path = require('path');

const pkg = JSON.parse(fs.readFileSync('package.json', 'utf8'));
const version = pkg.version;

const htmlPath = path.join('config', 'index.html');
let html = fs.readFileSync(htmlPath, 'utf8');

// Replace any existing version string in the footer
html = html.replace(
    /Trio Pebble config v[0-9.]+/,
    `Trio Pebble config v${version}`
);

fs.writeFileSync(htmlPath, html);
console.log(`Synced version ${version} into config/index.html`);
