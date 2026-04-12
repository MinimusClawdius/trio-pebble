# Build and Deploy Workflow

Two Pebble apps live in this workspace. Both target all Pebble hardware (aplite, basalt, chalk, diorite, emery).

| App | Type | Source directory | UUID |
|-----|------|------------------|------|
| **Trio Pebble** | Watchface | `trio-pebble/` (repo root) | `a1b2c3d4-e5f6-7890-abcd-ef1234567890` |
| **Trio Remote** | Watch app | `trio-pebble/remote-app/` | `fdd1d4b6-f2f2-4819-a26b-8ccad4264feb` |

---

## Prerequisites

### 1. Docker Desktop (local builds)

Install Docker Desktop for Windows:
<https://docs.docker.com/desktop/setup/install/windows-install/>

After install, pull the Pebble SDK image once:

```powershell
docker pull rebble/pebble-sdk:4.5-2
```

### 2. Phone setup

- Install the **Rebble** app (iOS or Android) — <https://rebble.io/howto/>
- Pair your **Pebble** watch via Bluetooth
- Enable **Developer Connection** in the Rebble/Pebble app (Settings > Developer) and note the phone's IP address

### 3. Trio iOS app (for Trio data source)

- Enable **Pebble integration** in Trio: Settings > Services > Pebble / Rebble
- The local API starts on `127.0.0.1:8080` by default

---

## Option A: Local Docker Build (recommended)

### Build both apps

```powershell
cd c:\EclipseWorkspace\TrioWorkspace\trio-pebble
.\scripts\pebble-build.ps1
```

### Build only the watchface

```powershell
.\scripts\pebble-build.ps1 -Target watchface
```

### Build only Trio Remote

```powershell
.\scripts\pebble-build.ps1 -Target remote
```

### Clean build

```powershell
.\scripts\pebble-build.ps1 -Clean
```

### Output

Built `.pbw` files appear in:
- `trio-pebble/build/*.pbw` (watchface)
- `trio-pebble/remote-app/build/*.pbw` (Trio Remote)

---

## Install to watch

### Via Developer Connection (direct)

```powershell
.\scripts\pebble-install.ps1 -Target watchface -PhoneIP 192.168.1.42
.\scripts\pebble-install.ps1 -Target remote -PhoneIP 192.168.1.42
```

Replace `192.168.1.42` with your phone's IP from the Developer Connection screen.

### Via sideload (manual transfer)

1. Find the `.pbw` in the `build/` directory
2. Transfer it to your phone (AirDrop, iCloud Drive, email, USB, Google Drive)
3. Open the file on the phone and choose **Open in Rebble**
4. Confirm the install — the app/watchface appears on the watch

### Via emulator

```powershell
.\scripts\pebble-install.ps1 -Target watchface -Emulator -Platform basalt
```

Platforms: `aplite`, `basalt`, `chalk`, `diorite`, `emery`

---

## Option B: CloudPebble (web IDE)

CloudPebble is back at <https://cloudpebble.repebble.com/>.

**Note:** GitHub syncing is not yet operational in the new CloudPebble. You need to import projects manually.

### Import the watchface

1. Go to CloudPebble > **Create** > **Import**
2. Upload a `.zip` of the `trio-pebble/` directory (or import from GitHub if syncing is restored)
3. In **Settings**, enable: Configurable, Uses Health, Uses Location
4. **Compile** then **Install and Run**

### Import Trio Remote

1. Create a **second** CloudPebble project
2. Import from the `trio-pebble-remote/` repo (separate UUID)
3. In **Settings**, enable: Configurable only
4. **Compile** then **Install and Run**

**Important:** The two apps have different UUIDs. Installing one does NOT overwrite the other.

---

## Keeping Trio Remote in sync

When you modify watchface code that affects shared message keys, the remote app, or pkjs:

```powershell
cd c:\EclipseWorkspace\TrioWorkspace\trio-pebble
pwsh -File scripts\sync-trio-pebble-remote.ps1
```

This copies `remote-app/package.json`, `wscript`, and `src/` into the sibling `trio-pebble-remote/` directory. See `SYNC_WITH_TRIO_PEBBLE.md` in that repo for the full checklist.

---

## Quick reference: one-liner build and deploy

```powershell
# Build everything, then install watchface to phone
cd c:\EclipseWorkspace\TrioWorkspace\trio-pebble
.\scripts\pebble-build.ps1 -Target watchface; .\scripts\pebble-install.ps1 -Target watchface -PhoneIP <YOUR_PHONE_IP>

# Build and install Trio Remote
.\scripts\pebble-build.ps1 -Target remote; .\scripts\pebble-install.ps1 -Target remote -PhoneIP <YOUR_PHONE_IP>
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `docker` not recognized | Install Docker Desktop and restart your terminal |
| Build fails on missing `menu_icon.png` | Copy `trio-pebble/resources/images/menu_icon.png` to the project's `resources/images/` |
| Settings page shows raw HTML | Host `config/index.html` via GitHub Pages (see `CLOUDPEBBLE_AND_DEPLOY.md`) |
| Trio HTTP unreachable from watch | Keep Trio in the foreground on iPhone; iOS suspends background apps |
| `pebble install` times out | Use manual sideload instead (`.pbw` file transfer) |
| Watch shows old version | Uninstall the app from Rebble app first, then reinstall the `.pbw` |

---

## File layout

```
trio-pebble/                    ← watchface project root
├── scripts/
│   ├── pebble-build.ps1        ← Docker build script (this workflow)
│   ├── pebble-install.ps1      ← Deploy/install helper
│   └── sync-trio-pebble-remote.ps1
├── src/                        ← watchface C sources + pkjs
├── remote-app/                 ← Trio Remote (monorepo copy)
│   ├── src/                    ← remote app C sources + pkjs
│   ├── package.json
│   └── wscript
├── config/index.html           ← settings page (served via GitHub Pages)
├── resources/images/           ← PNGs (menu icon, trend arrows)
├── docs/                       ← documentation
├── package.json                ← Pebble manifest (SDK 4.x)
└── wscript                     ← waf build config

trio-pebble-remote/             ← standalone repo for CloudPebble import
├── src/                        ← mirror of remote-app/src
├── resources/images/
│   └── menu_icon.png           ← must exist (copied from trio-pebble)
├── package.json
└── wscript
```
