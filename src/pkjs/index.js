// ============================================================
// Trio Pebble — PebbleKit JS Bridge (primary transport)
// ============================================================
//
// **Supported data path:** HTTP GET Trio's loopback snapshot (`/api/all` or `/api/pebble/v1/snapshot`),
// then Pebble.sendAppMessage → watch C `inbox_received`. Community workflows extend this JS layer.
//
// Trio's optional native iOS PebbleKit BLE push is **off by default**. We no longer slow polling when
// `blePushActive` is true — that caused stale UI when native BLE was flaky.

var POLL_INTERVAL_MS = 20000;
var POLL_JITTER_MS = 4000;
var POLL_BACKOFF_CAP_MS = 120000;
var WEATHER_INTERVAL_MS = 1800000; // 30 min

/** Informational: Trio reports whether native iOS BLE push is active (does not throttle polling). */
var blePushActive = false;
/** Consecutive failed Trio HTTP snapshots (reset on success). */
var failStreak = 0;
/** Last time `fetchTrio` returned usable data (ms since epoch). */
var lastTrioOkAt = 0;
var pollTimer = null;

// AppMessage keys (must match C enums)
var K = {
    GLUCOSE: 0, TREND: 1, DELTA: 2, IOB: 3, COB: 4,
    LAST_LOOP: 5, GLUCOSE_STALE: 6, CMD_TYPE: 7, CMD_AMOUNT: 8,
    CMD_STATUS: 9, GRAPH_DATA: 10, GRAPH_COUNT: 11, LOOP_STATUS: 12,
    UNITS: 13, PUMP_STATUS: 14, RESERVOIR: 15,
    CONFIG_FACE_TYPE: 16, CONFIG_DATA_SOURCE: 17,
    CONFIG_HIGH_THRESHOLD: 18, CONFIG_LOW_THRESHOLD: 19,
    CONFIG_ALERT_HIGH_ENABLED: 20, CONFIG_ALERT_LOW_ENABLED: 21,
    CONFIG_ALERT_URGENT_LOW: 22, CONFIG_ALERT_SNOOZE_MIN: 23,
    CONFIG_COLOR_SCHEME: 24,
    BATTERY_PHONE: 25, WEATHER_TEMP: 26, WEATHER_ICON: 27,
    STEPS: 28, HEART_RATE: 29,
    PREDICTIONS_DATA: 30, PREDICTIONS_COUNT: 31,
    PUMP_BATTERY: 32, SENSOR_AGE: 33,
    CONFIG_CHANGED: 34, TAP_ACTION: 35,
    CONFIG_WEATHER_ENABLED: 36,
    CONFIG_COMP_SLOT_0: 37, CONFIG_COMP_SLOT_1: 38,
    CONFIG_COMP_SLOT_2: 39, CONFIG_COMP_SLOT_3: 40,
    CONFIG_CLOCK_24H: 41,
    CONFIG_GRAPH_SCALE_MODE: 42,
    CONFIG_    GRAPH_TIME_RANGE: 43,
    TRIO_LINK: 44,
    SUGGESTED_BOLUS_TENTHS: 45
};

// ---------- Settings ----------
var settings = {
    dataSource: 0,        // 0=Trio, 1=Dexcom, 2=Nightscout, 3=Health/CGM via Trio (same HTTP)
    trioHost: 'http://127.0.0.1:8080',
    nightscoutUrl: '',
    nightscoutToken: '',
    dexcomUsername: '',
    dexcomPassword: '',
    dexcomServer: 'us',   // 'us' or 'ous' (outside US)
    faceType: 0,
    colorScheme: 0,
    highThreshold: 180,
    lowThreshold: 70,
    urgentLow: 55,
    alertHighEnabled: true,
    alertLowEnabled: true,
    alertSnoozeMin: 15,
    weatherEnabled: true,
    weatherUnits: 'f',    // 'f' or 'c'
    glucoseUnits: 'mgdl',  // 'mgdl' | 'mmol' — watchface display + threshold entry
    /* Bottom bar left→right; values match watch ComplicationSlotKind */
    compSlot0: 1,
    compSlot1: 5,
    compSlot2: 6,
    compSlot3: 0,
    clock24h: true,
    graphScaleMode: 0,
    graphTimeRange: 0
};

var GRAPH_SEND_MAX = 48;

function graphHoursFromMode(mode) {
    var m = mode | 0;
    if (m === 1) return 6;
    if (m === 2) return 12;
    if (m === 3) return 24;
    return 3;
}

function maxRawGraphSamplesForHours(hours) {
    return Math.min(500, Math.max(GRAPH_SEND_MAX, Math.ceil(hours * 60 / 5) + 10));
}

function downsampleGraphHistory(arr, targetLen) {
    if (!arr || arr.length === 0) return [];
    if (targetLen <= 1) return [arr[arr.length - 1]];
    if (arr.length <= targetLen) return arr.slice();
    var out = [];
    for (var j = 0; j < targetLen; j++) {
        var idx = Math.round((j * (arr.length - 1)) / (targetLen - 1));
        out.push(arr[idx]);
    }
    return out;
}

function loadSettings() {
    try {
        var saved = localStorage.getItem('trio_settings');
        if (saved) {
            var parsed = JSON.parse(saved);
            for (var key in parsed) {
                if (parsed.hasOwnProperty(key)) settings[key] = parsed[key];
            }
        }
        if (settings.clock24h !== true && settings.clock24h !== false) {
            settings.clock24h = true;
        }
        var gsm = settings.graphScaleMode | 0;
        if (gsm < 0 || gsm > 2) {
            settings.graphScaleMode = 0;
        }
        var gtr = settings.graphTimeRange | 0;
        if (gtr < 0 || gtr > 3) {
            settings.graphTimeRange = 0;
        }
        settings.compSlot3 = 0;
    } catch (e) {
        console.log('Trio: settings load error: ' + e);
    }
}

function saveSettings() {
    localStorage.setItem('trio_settings', JSON.stringify(settings));
}

// ---------- Data Source: Trio Local API ----------
function fetchTrio(callback) {
    httpGet(settings.trioHost + '/api/all', function (data) {
        if (!data) return callback(null);
        try {
            var parsed = JSON.parse(data);
            var wasActive = blePushActive;
            blePushActive = parsed.blePushActive === true;
            if (blePushActive !== wasActive) {
                console.log('Trio: native iOS BLE push (informational): ' + blePushActive);
            }
            if (parsed.stateRevision !== undefined) {
                console.log('Trio: stateRevision=' + parsed.stateRevision + ' proto=' + (parsed.pebbleProtocolVersion || 0));
            }
            callback(normalizeTrio(parsed));
        } catch (e) {
            console.log('Trio: parse error: ' + e);
            callback(null);
        }
    });
}

function trioParseGlucoseNumber(raw) {
    if (raw == null || raw === '') return NaN;
    if (typeof raw === 'number') return raw;
    return parseFloat(String(raw).replace(',', '.'));
}

/**
 * Use cgm.units when present; if missing/wrong, infer from value scale (Trio mmol ~2–30, mg/dL ~40–400).
 */
function trioInferSourceIsMmol(unitsStr, glucoseRaw, historyArr) {
    var u = String(unitsStr || '').toLowerCase();
    if (u.indexOf('mmol') >= 0) return true;
    if (u.indexOf('mg') >= 0 && u.indexOf('dl') >= 0) return false;
    var g = trioParseGlucoseNumber(glucoseRaw);
    if (!isNaN(g) && g > 0) {
        if (g < 35) return true;
        if (g >= 40) return false;
    }
    if (historyArr && historyArr.length > 0) {
        var v = trioParseGlucoseNumber(historyArr[0]);
        if (!isNaN(v) && v > 0 && v < 35) return true;
        if (!isNaN(v) && v >= 40) return false;
    }
    return false;
}

function dexcomTrendToArrow(trend) {
    var map = {
        'TripleUp': '↑↑', 'DoubleUp': '↑↑', 'SingleUp': '↑', 'FortyFiveUp': '↗',
        'Flat': '→', 'FortyFiveDown': '↘', 'SingleDown': '↓',
        'DoubleDown': '↓↓', 'TripleDown': '↓↓', 'None': '--', 'NotComputable': '?',
        'RateOutOfRange': '⚠'
    };
    if (typeof trend === 'number') {
        var numMap = ['--', '↑↑', '↑', '↗', '→', '↘', '↓', '↓↓', '?', '⚠'];
        return numMap[trend] || '--';
    }
    return map[trend] || '--';
}

/**
 * Trio /api/all often sends human-readable trend strings ("Flat", "flat", "SingleUp").
 * Map to compact arrows so the watch layout does not crowd the glucose value.
 */
function trioTrendToArrow(raw) {
    if (raw === null || raw === undefined) return '--';
    if (typeof raw === 'number') return dexcomTrendToArrow(raw);
    var s = String(raw).trim();
    if (!s) return '--';
    var compact = s.replace(/\s+/g, '');
    var m = dexcomTrendToArrow(compact);
    if (m !== '--') return m;
    var lc = compact.toLowerCase();
    var toPascal = {
        flat: 'Flat',
        singleup: 'SingleUp',
        doubleup: 'DoubleUp',
        tripleup: 'TripleUp',
        fortyfiveup: 'FortyFiveUp',
        fortyfivedown: 'FortyFiveDown',
        singledown: 'SingleDown',
        doubledown: 'DoubleDown',
        tripledown: 'TripleDown',
        none: 'None',
        notcomputable: 'NotComputable',
        rateoutofrange: 'RateOutOfRange',
        unknown: 'NotComputable',
        stable: 'Flat',
        steady: 'Flat'
    };
    if (toPascal[lc]) return dexcomTrendToArrow(toPascal[lc]);
    var first = s.split(/[\s,;(]+/)[0];
    if (first && first !== s) {
        compact = first.replace(/\s+/g, '');
        m = dexcomTrendToArrow(compact);
        if (m !== '--') return m;
        lc = compact.toLowerCase();
        if (toPascal[lc]) return dexcomTrendToArrow(toPascal[lc]);
    }
    return '--';
}

/**
 * Watch AppMessage uses mg/dL integers for KEY_GLUCOSE + graph. Trio JSON may be mmol/L.
 * BUGFIX: previously returned raw 4.7 mmol as glucose → watch saw ~4 mg/dL → 0.2 mmol display + false urgent-low vibes.
 */
function normalizeTrio(data) {
    var cgm = data.cgm || {};
    var loop = data.loop || {};
    var histIn = loop.glucoseHistory || [];
    var isMmol = trioInferSourceIsMmol(cgm.units, cgm.glucose, histIn);

    var pump = data.pump || {};
    var reservoir = 0;
    var pumpBattery = 0;
    var resNum = Number(pump.reservoir);
    if (isFinite(resNum) && resNum >= 0) {
        reservoir = Math.round(resNum);
    }
    var batNum = Number(pump.battery);
    if (isFinite(batNum) && batNum >= 0) {
        pumpBattery = Math.round(batNum);
    }

    var gNum = trioParseGlucoseNumber(cgm.glucose);
    var glucoseMgdl = 0;
    if (!isNaN(gNum) && gNum > 0) {
        glucoseMgdl = isMmol ? Math.round(gNum * 18.0) : Math.round(gNum);
    }

    var historyMgdl = [];
    for (var i = 0; i < histIn.length; i++) {
        var v = trioParseGlucoseNumber(histIn[i]);
        if (isNaN(v) || v <= 0) continue;
        var mg = isMmol ? Math.round(v * 18.0) : Math.round(v);
        if (mg > 0 && mg <= 65535) historyMgdl.push(mg);
    }

    var gh = graphHoursFromMode(settings.graphTimeRange);
    var maxHist = Math.min(500, Math.ceil(gh * 60 / 5) + 8);
    if (historyMgdl.length > maxHist) {
        historyMgdl = historyMgdl.slice(historyMgdl.length - maxHist);
    }

    return {
        glucose: glucoseMgdl,
        trend: trioTrendToArrow(cgm.trend),
        delta: cgm.delta || '',
        isStale: cgm.isStale || false,
        iob: loop.iob || '',
        cob: loop.cob || '',
        lastLoop: loop.lastLoopTime || '',
        history: historyMgdl,
        pumpStatus: typeof pump.status === 'string' ? pump.status : '',
        reservoir: reservoir,
        pumpBattery: pumpBattery,
        sensorAge: ''
    };
}

// ---------- Data Source: Nightscout ----------
function fetchNightscout(callback) {
    var url = settings.nightscoutUrl.replace(/\/$/, '');
    var tokenQs = settings.nightscoutToken ? '&token=' + encodeURIComponent(settings.nightscoutToken) : '';
    var hc = maxRawGraphSamplesForHours(graphHoursFromMode(settings.graphTimeRange));

    httpGet(url + '/api/v1/entries/sgv.json?count=' + hc + tokenQs, function (sgvData) {
        if (!sgvData) return callback(null);
        try {
            var entries = JSON.parse(sgvData);
            httpGet(url + '/api/v1/properties/iob,cob,loop' + tokenParam, function (propData) {
                var props = {};
                try { props = JSON.parse(propData || '{}'); } catch (e) { /* ok */ }
                callback(normalizeNightscout(entries, props));
            });
        } catch (e) {
            console.log('Trio: Nightscout parse error: ' + e);
            callback(null);
        }
    });
}

function normalizeNightscout(entries, props) {
    if (!entries || entries.length === 0) return null;
    var latest = entries[0];

    var delta = '';
    if (entries.length >= 2) {
        var d = latest.sgv - entries[1].sgv;
        delta = (d >= 0 ? '+' : '') + d;
    }

    var iob = props.iob && props.iob.iob ? props.iob.iob.iob.toFixed(1) : '';
    var cob = props.cob && props.cob.cob ? Math.round(props.cob.cob.cob).toString() : '';
    var lastLoop = '';
    if (props.loop && props.loop.loop && props.loop.loop.lastLoop) {
        var loopAge = Math.round((Date.now() - new Date(props.loop.loop.lastLoop.timestamp).getTime()) / 60000);
        lastLoop = loopAge + ' min';
    }

    var lim = maxRawGraphSamplesForHours(graphHoursFromMode(settings.graphTimeRange));
    var history = entries.slice(0, Math.min(entries.length, lim)).map(function (e) { return e.sgv; }).reverse();

    return {
        glucose: latest.sgv || 0,
        trend: directionToArrow(latest.direction),
        delta: delta,
        isStale: (Date.now() - latest.date) > 15 * 60 * 1000,
        units: 'mgdL',
        iob: iob,
        cob: cob,
        lastLoop: lastLoop,
        history: history,
        pumpStatus: '',
        reservoir: 0,
        pumpBattery: 0,
        sensorAge: ''
    };
}

// ---------- Data Source: Dexcom Share ----------
var dexcomSessionId = null;

function fetchDexcom(callback) {
    var server = settings.dexcomServer === 'ous'
        ? 'https://shareous1.dexcom.com'
        : 'https://share2.dexcom.com';

    var loginUrl = server + '/ShareWebServices/Services/General/LoginPublisherAccountByName';
    var readUrl = server + '/ShareWebServices/Services/Publisher/ReadPublisherLatestGlucoseValues';

    function doRead(sessionId) {
        var hours = graphHoursFromMode(settings.graphTimeRange);
        var maxCount = maxRawGraphSamplesForHours(hours);
        httpPost(readUrl + '?sessionId=' + sessionId + '&minutes=' + (hours * 60) + '&maxCount=' + maxCount, '', function (data) {
            if (!data) return callback(null);
            try {
                var entries = JSON.parse(data);
                callback(normalizeDexcom(entries));
            } catch (e) {
                console.log('Trio: Dexcom parse error: ' + e);
                callback(null);
            }
        });
    }

    if (dexcomSessionId) {
        doRead(dexcomSessionId);
        return;
    }

    var loginBody = JSON.stringify({
        accountName: settings.dexcomUsername,
        password: settings.dexcomPassword,
        applicationId: 'd89443d2-327c-4a6f-89e5-496bbb0317db'
    });

    httpPost(loginUrl, loginBody, function (data) {
        if (!data) return callback(null);
        try {
            dexcomSessionId = JSON.parse(data);
            doRead(dexcomSessionId);
        } catch (e) {
            console.log('Trio: Dexcom login error: ' + e);
            callback(null);
        }
    });
}

function normalizeDexcom(entries) {
    if (!entries || entries.length === 0) return null;
    var latest = entries[0];
    var glucose = latest.Value || 0;

    var delta = '';
    if (entries.length >= 2) {
        var d = glucose - (entries[1].Value || 0);
        delta = (d >= 0 ? '+' : '') + d;
    }

    var dateMatch = (latest.ST || latest.WT || '').match(/\d+/);
    var timestamp = dateMatch ? parseInt(dateMatch[0], 10) : Date.now();
    var isStale = (Date.now() - timestamp) > 15 * 60 * 1000;

    var history = entries.map(function (e) { return e.Value; }).reverse();

    return {
        glucose: glucose,
        trend: dexcomTrendToArrow(latest.Trend),
        delta: delta,
        isStale: isStale,
        units: 'mgdL',
        iob: '', cob: '', lastLoop: '',
        history: history,
        pumpStatus: '',
        reservoir: 0,
        pumpBattery: 0,
        sensorAge: ''
    };
}

function directionToArrow(direction) {
    var map = {
        'DoubleUp': '↑↑', 'SingleUp': '↑', 'FortyFiveUp': '↗',
        'Flat': '→', 'FortyFiveDown': '↘', 'SingleDown': '↓',
        'DoubleDown': '↓↓', 'NONE': '--', 'NOT COMPUTABLE': '?'
    };
    return map[direction] || direction || '--';
}

// ---------- Fetch Dispatcher ----------
function isTrioDataSource() {
    var d = settings.dataSource | 0;
    return d === 0 || d === 3;
}

function sendTrioLinkHint(text) {
    var msg = {};
    msg[K.TRIO_LINK] = text;
    Pebble.sendAppMessage(msg,
        function () { },
        function (e) { console.log('Trio: link hint send failed: ' + JSON.stringify(e)); }
    );
}

/** `done` optional — called after fetch attempt finishes (success or failure). */
function fetchData(done) {
    var finish = typeof done === 'function' ? done : function () {};
    var fetcher;
    switch (settings.dataSource) {
        case 1:  fetcher = fetchDexcom; break;
        case 2:  fetcher = fetchNightscout; break;
        case 3:  fetcher = fetchTrio; break;
        default: fetcher = fetchTrio; break;
    }

    fetcher(function (data) {
        if (data) {
            lastTrioOkAt = Date.now();
            failStreak = 0;
            sendToWatch(data);
            finish();
            return;
        }
        if (isTrioDataSource()) {
            failStreak++;
            httpPing(settings.trioHost + '/api/pebble/v1/ping', function (pingOk) {
                if (failStreak >= 2) {
                    var hint = '';
                    if (!pingOk) {
                        hint = 'No phone';
                    } else if (lastTrioOkAt > 0) {
                        var m = Math.min(99, Math.floor((Date.now() - lastTrioOkAt) / 60000));
                        hint = m > 0 ? ('Old ' + m + 'm') : 'Busy?';
                    } else {
                        hint = 'Trio?';
                    }
                    if (hint.length > 15) hint = hint.substring(0, 15);
                    sendTrioLinkHint(hint);
                }
                finish();
            });
            return;
        }
        finish();
    });
}

function fetchDataThenSchedule() {
    fetchData(function () { scheduleNextPoll(); });
}

function msUntilNextPoll() {
    var jitter = Math.floor(Math.random() * POLL_JITTER_MS);
    if (failStreak === 0) {
        return POLL_INTERVAL_MS + jitter;
    }
    var backoff = Math.min(POLL_BACKOFF_CAP_MS, 8000 + failStreak * 11000);
    return backoff + jitter;
}

function scheduleNextPoll() {
    if (pollTimer) clearTimeout(pollTimer);
    pollTimer = setTimeout(function () {
        runPollTick();
    }, msUntilNextPoll());
}

function runPollTick() {
    fetchData(function () { scheduleNextPoll(); });
}

// ---------- Send to Watch ----------
function displayUnitsForWatch() {
    return settings.glucoseUnits === 'mmol' ? 'mmol/L' : 'mg/dL';
}

function sendToWatch(data) {
    var msg = {};

    /* Watch number format only — source values are always mg/dL after normalizeTrio */
    msg[K.UNITS] = displayUnitsForWatch();

    var gInt = Math.round(Number(data.glucose) || 0);
    if (gInt > 0) {
        msg[K.GLUCOSE] = gInt;
    }
    if (data.trend) {
        var tr = String(data.trend);
        var out = '';
        for (var i = 0; i < tr.length && out.length < 8; ) {
            var cp = tr.codePointAt(i);
            out += String.fromCodePoint(cp);
            i += cp > 0xffff ? 2 : 1;
        }
        msg[K.TREND] = out;
    }
    if (data.delta) msg[K.DELTA] = data.delta.substring(0, 15);
    if (data.iob) msg[K.IOB] = data.iob.substring(0, 15);
    if (data.cob) msg[K.COB] = data.cob.substring(0, 15);
    if (data.lastLoop) msg[K.LAST_LOOP] = data.lastLoop.substring(0, 15);
    if (data.pumpStatus) msg[K.PUMP_STATUS] = data.pumpStatus.substring(0, 15);
    if (data.sensorAge) msg[K.SENSOR_AGE] = data.sensorAge.substring(0, 15);

    msg[K.GLUCOSE_STALE] = data.isStale ? 1 : 0;
    var resN = Number(data.reservoir);
    if (isFinite(resN) && resN >= 0) {
        msg[K.RESERVOIR] = Math.round(resN);
    }
    var batN = Number(data.pumpBattery);
    if (isFinite(batN) && batN >= 0) {
        msg[K.PUMP_BATTERY] = Math.round(batN);
    }

    // Graph: mg/dL uint16 LE (already converted in normalizeTrio)
    var history = downsampleGraphHistory(data.history || [], GRAPH_SEND_MAX);
    if (history.length > 0) {
        var count = history.length;
        var bytes = [];
        for (var i = 0; i < count; i++) {
            var val = Math.round(Number(history[i]) || 0);
            if (val < 0) val = 0;
            if (val > 65535) val = 65535;
            bytes.push(val & 0xFF);
            bytes.push((val >> 8) & 0xFF);
        }
        msg[K.GRAPH_DATA] = bytes;
        msg[K.GRAPH_COUNT] = count;
    }

    Pebble.sendAppMessage(msg,
        function () { /* success */ },
        function (e) { console.log('Trio: send failed: ' + JSON.stringify(e)); }
    );
}

// ---------- Weather ----------
var lastWeatherFetch = 0;

function fetchWeather() {
    if (!settings.weatherEnabled) return;
    if (Date.now() - lastWeatherFetch < WEATHER_INTERVAL_MS) return;

    navigator.geolocation.getCurrentPosition(function (pos) {
        var url = 'https://api.open-meteo.com/v1/forecast?latitude=' +
            pos.coords.latitude + '&longitude=' + pos.coords.longitude +
            '&current_weather=true&temperature_unit=' +
            (settings.weatherUnits === 'c' ? 'celsius' : 'fahrenheit');

        httpGet(url, function (data) {
            if (!data) return;
            try {
                var w = JSON.parse(data);
                if (w.current_weather) {
                    lastWeatherFetch = Date.now();
                    var msg = {};
                    msg[K.WEATHER_TEMP] = Math.round(w.current_weather.temperature);
                    msg[K.WEATHER_ICON] = weatherCodeToIcon(w.current_weather.weathercode);
                    Pebble.sendAppMessage(msg);
                }
            } catch (e) {
                console.log('Trio: weather parse error: ' + e);
            }
        });
    }, function () {
        console.log('Trio: geolocation unavailable');
    }, { timeout: 15000, maximumAge: 600000 });
}

/** WMO weather codes → short tags for watch (see weather_background.c). Max 15 chars. */
function weatherCodeToIcon(code) {
    if (code === 0) return 'clear';
    if (code === 1) return 'mainly_clr';
    if (code === 2) return 'partly';
    if (code === 3) return 'overcast';
    if (code <= 48) return 'fog';
    if (code <= 67) return 'rain';
    if (code <= 77) return 'snow';
    if (code <= 82) return 'rain';
    if (code <= 86) return 'snow';
    return 'storm';
}

// ---------- Commands from Watch ----------
/** Rebble / PebbleKit may expose dict keys as numbers or strings — normalize. */
function payloadGet(p, keyNum) {
    if (!p) return undefined;
    var k = keyNum | 0;
    if (p[k] !== undefined && p[k] !== null) return p[k];
    if (p[String(k)] !== undefined && p[String(k)] !== null) return p[String(k)];
    return undefined;
}

function trioStatusFromHttpResponse(resp, fallbackOk) {
    if (resp == null) {
        return 'Trio unreachable';
    }
    try {
        var r = JSON.parse(resp || '{}');
        if (r.status === 'delivered') {
            if (r.type === 'carbEntry') {
                return 'Carbs saved';
            }
            if (r.type === 'bolus') {
                return 'Bolus sent';
            }
        }
        return r.message || r.status || fallbackOk;
    } catch (e) {
        return fallbackOk;
    }
}

function sendCarbsThenSuggestBolus(amountGrams) {
    var host = settings.trioHost;
    var recUrl = host + '/api/pebble/v1/bolus_recommendation?grams=' + encodeURIComponent(amountGrams);
    httpGet(recUrl, function (recBody) {
        var tenths = 0;
        if (recBody) {
            try {
                var jr = JSON.parse(recBody);
                if (typeof jr.recommendedUnitsTenths === 'number') {
                    tenths = jr.recommendedUnitsTenths | 0;
                }
            } catch (e1) { /* ignore */ }
        }
        var carbBody = JSON.stringify({ grams: amountGrams, absorptionHours: 3 });
        httpPost(host + '/api/carbs', carbBody, function (resp) {
            var statusMsg = trioStatusFromHttpResponse(resp, 'Sent');
            var msg = {};
            msg[K.CMD_STATUS] = statusMsg.substring(0, 63);
            if (tenths > 0) {
                msg[K.SUGGESTED_BOLUS_TENTHS] = tenths;
            }
            Pebble.sendAppMessage(msg);
        });
    });
}

function sendCommand(type, amount) {
    if (settings.dataSource !== 0 && settings.dataSource !== 3) {
        var msg = {};
        msg[K.CMD_STATUS] = 'Commands need Trio API';
        Pebble.sendAppMessage(msg);
        return;
    }

    if ((type | 0) === 2) {
        sendCarbsThenSuggestBolus(amount | 0);
        return;
    }

    var endpoint = '/api/bolus';
    var body = JSON.stringify({ units: amount / 10.0 });

    httpPost(settings.trioHost + endpoint, body, function (resp) {
        var statusMsg = trioStatusFromHttpResponse(resp, 'Sent');
        var msg = {};
        msg[K.CMD_STATUS] = statusMsg.substring(0, 63);
        Pebble.sendAppMessage(msg);
    });
}

// ---------- Configuration Page ----------
// Must be an https URL that is served as Content-Type: text/html (not text/plain).
// jsDelivr and raw.githubusercontent.com send .html as text/plain + nosniff, so Rebble's
// WebView shows raw source instead of a UI. Use GitHub Pages (recommended) or any host
// that serves this file as real HTML.
// Enable: repo Settings → Pages → Build from branch "main", folder "/ (root)".
var TRIO_CONFIG_PAGE_URL =
    'https://minimusclawdius.github.io/trio-pebble/config/index.html';

Pebble.addEventListener('showConfiguration', function () {
    var params = encodeURIComponent(JSON.stringify(settings));
    Pebble.openURL(TRIO_CONFIG_PAGE_URL + '#' + params);
});

Pebble.addEventListener('webviewclosed', function (e) {
    if (e && e.response) {
        try {
            var newSettings = JSON.parse(decodeURIComponent(e.response));
            for (var key in newSettings) {
                if (newSettings.hasOwnProperty(key)) settings[key] = newSettings[key];
            }
            saveSettings();
            dexcomSessionId = null; // reset on credential change

            // Push config to watch
            var msg = {};
            msg[K.CONFIG_CHANGED] = 1;
            msg[K.CONFIG_FACE_TYPE] = settings.faceType;
            msg[K.CONFIG_DATA_SOURCE] = settings.dataSource;
            msg[K.CONFIG_HIGH_THRESHOLD] = settings.highThreshold;
            msg[K.CONFIG_LOW_THRESHOLD] = settings.lowThreshold;
            msg[K.CONFIG_ALERT_URGENT_LOW] = settings.urgentLow;
            msg[K.CONFIG_ALERT_HIGH_ENABLED] = settings.alertHighEnabled ? 1 : 0;
            msg[K.CONFIG_ALERT_LOW_ENABLED] = settings.alertLowEnabled ? 1 : 0;
            msg[K.CONFIG_ALERT_SNOOZE_MIN] = settings.alertSnoozeMin;
            msg[K.CONFIG_COLOR_SCHEME] = settings.colorScheme;
            msg[K.CONFIG_WEATHER_ENABLED] = settings.weatherEnabled ? 1 : 0;
            msg[K.CONFIG_COMP_SLOT_0] = settings.compSlot0 | 0;
            msg[K.CONFIG_COMP_SLOT_1] = settings.compSlot1 | 0;
            msg[K.CONFIG_COMP_SLOT_2] = settings.compSlot2 | 0;
            msg[K.CONFIG_COMP_SLOT_3] = 0;
            msg[K.CONFIG_CLOCK_24H] = settings.clock24h ? 1 : 0;
            msg[K.CONFIG_GRAPH_SCALE_MODE] = settings.graphScaleMode | 0;
            msg[K.CONFIG_GRAPH_TIME_RANGE] = settings.graphTimeRange | 0;
            msg[K.UNITS] = displayUnitsForWatch();
            if (!settings.weatherEnabled) {
                msg[K.WEATHER_TEMP] = 0;
                msg[K.WEATHER_ICON] = 'off';
            }
            Pebble.sendAppMessage(msg);

            fetchDataThenSchedule();
            if (settings.weatherEnabled) fetchWeather();
            else {
                var w = {};
                w[K.WEATHER_TEMP] = 0;
                w[K.WEATHER_ICON] = 'off';
                Pebble.sendAppMessage(w);
            }
        } catch (ex) {
            console.log('Trio: config parse error: ' + ex);
        }
    }
});

// ---------- Watch Messages ----------
Pebble.addEventListener('appmessage', function (e) {
    var p = e.payload;
    var cmdType = payloadGet(p, K.CMD_TYPE);
    var cmdAmt = payloadGet(p, K.CMD_AMOUNT);
    if (cmdType !== undefined && cmdAmt !== undefined) {
        console.log('Trio: watch command type=' + cmdType + ' amount=' + cmdAmt);
        sendCommand(cmdType | 0, cmdAmt | 0);
    } else if (p[K.TAP_ACTION] !== undefined) {
        /* Includes watch tick refresh (KEY_TAP_ACTION = refresh) — do not use KEY_GLUCOSE (0) for pings */
        if (p[K.TAP_ACTION] === 4) fetchDataThenSchedule();
    } else {
        /* Legacy firmware used dict key 0 for poll; never treat as CGM update */
        fetchDataThenSchedule();
    }
});

// ---------- Ready ----------
Pebble.addEventListener('ready', function () {
    console.log('Trio Pebble pkjs v2.16 (direct delivery + carb bolus hint) ready');
    loadSettings();

    var msg = {};
    msg[K.CONFIG_FACE_TYPE] = settings.faceType;
    msg[K.CONFIG_DATA_SOURCE] = settings.dataSource;
    msg[K.CONFIG_HIGH_THRESHOLD] = settings.highThreshold;
    msg[K.CONFIG_LOW_THRESHOLD] = settings.lowThreshold;
    msg[K.CONFIG_ALERT_URGENT_LOW] = settings.urgentLow;
    msg[K.CONFIG_ALERT_HIGH_ENABLED] = settings.alertHighEnabled ? 1 : 0;
    msg[K.CONFIG_ALERT_LOW_ENABLED] = settings.alertLowEnabled ? 1 : 0;
    msg[K.CONFIG_ALERT_SNOOZE_MIN] = settings.alertSnoozeMin;
    msg[K.CONFIG_COLOR_SCHEME] = settings.colorScheme;
    msg[K.CONFIG_WEATHER_ENABLED] = settings.weatherEnabled ? 1 : 0;
    msg[K.CONFIG_COMP_SLOT_0] = settings.compSlot0 | 0;
    msg[K.CONFIG_COMP_SLOT_1] = settings.compSlot1 | 0;
    msg[K.CONFIG_COMP_SLOT_2] = settings.compSlot2 | 0;
    msg[K.CONFIG_COMP_SLOT_3] = 0;
    msg[K.CONFIG_CLOCK_24H] = settings.clock24h ? 1 : 0;
    msg[K.CONFIG_GRAPH_SCALE_MODE] = settings.graphScaleMode | 0;
    msg[K.CONFIG_GRAPH_TIME_RANGE] = settings.graphTimeRange | 0;
    msg[K.UNITS] = displayUnitsForWatch();
    Pebble.sendAppMessage(msg);

    runPollTick();
    if (settings.weatherEnabled) fetchWeather();
    else {
        var w = {};
        w[K.WEATHER_TEMP] = 0;
        w[K.WEATHER_ICON] = 'off';
        Pebble.sendAppMessage(w);
    }
    setInterval(function () {
        if (settings.weatherEnabled) fetchWeather();
    }, WEATHER_INTERVAL_MS);
});

// ---------- HTTP Helpers ----------
function httpPing(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.timeout = 4000;
    xhr.onload = function () {
        callback(xhr.status === 200);
    };
    xhr.onerror = function () { callback(false); };
    xhr.ontimeout = function () { callback(false); };
    xhr.send();
}

function httpGet(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.timeout = 15000;
    xhr.onload = function () {
        callback(xhr.status === 200 ? xhr.responseText : null);
    };
    xhr.onerror = function () { callback(null); };
    xhr.ontimeout = function () { callback(null); };
    xhr.send();
}

function httpPost(url, body, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('POST', url, true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.timeout = 15000;
    xhr.onload = function () {
        callback(xhr.responseText);
    };
    xhr.onerror = function () { callback(null); };
    xhr.ontimeout = function () { callback(null); };
    xhr.send(body);
}
