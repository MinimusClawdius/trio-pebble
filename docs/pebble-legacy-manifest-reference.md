# Legacy Pebble manifest fields (reference only)

Human-readable legacy fields that mirror `package.json` / historical `appinfo.json`. **Do not** use this file as a second manifest: CloudPebble may merge multiple JSON sources and duplicate resources.

```json
{
  "uuid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "shortName": "Trio",
  "longName": "Trio Pebble",
  "companyName": "MinimusClawdius",
  "versionLabel": "(use package.json version)",
  "sdkVersion": "3",
  "targetPlatforms": ["aplite", "basalt", "chalk", "diorite", "emery"],
  "watchapp": {
    "watchface": true
  },
  "enableMultiJS": true,
  "capabilities": ["configurable", "health", "location"]
}
```

**Menu icon:** This repo keeps `pebble.resources.media` **empty** in `package.json` so GitHub-linked CloudPebble does not merge a second `IMAGE_MENU_ICON` with the Resources tab. See `docs/CLOUDPEBBLE_AND_DEPLOY.md`.
