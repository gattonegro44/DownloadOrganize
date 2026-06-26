// organizer.js
#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');
const os = require('os');

const COLORS = {
    green: '\x1b[92m',
    red: '\x1b[91m',
    yellow: '\x1b[93m',
    blue: '\x1b[94m',
    reset: '\x1b[0m'
};

function colorize(text, color) {
    return COLORS[color] + text + COLORS.reset;
}

const DEFAULT_RULES = {
    Images: ['jpg', 'jpeg', 'png', 'gif', 'bmp', 'svg', 'webp', 'ico'],
    Documents: ['pdf', 'doc', 'docx', 'txt', 'xls', 'xlsx', 'ppt', 'pptx', 'odt', 'ods', 'odp', 'rtf'],
    Archives: ['zip', 'rar', '7z', 'tar', 'gz', 'bz2', 'xz', 'tgz'],
    Videos: ['mp4', 'avi', 'mkv', 'mov', 'wmv', 'flv', 'webm', 'm4v'],
    Music: ['mp3', 'wav', 'flac', 'aac', 'ogg', 'wma', 'm4a'],
    Programs: ['exe', 'msi', 'dmg', 'deb', 'rpm', 'sh', 'bat', 'cmd'],
    Other: []
};

function loadConfig(configPath) {
    if (!configPath || !fs.existsSync(configPath)) return DEFAULT_RULES;
    const content = fs.readFileSync(configPath, 'utf8');
    const rules = JSON.parse(content);
    // Merge with defaults
    const merged = { ...DEFAULT_RULES };
    for (const [cat, exts] of Object.entries(rules)) {
        merged[cat] = exts;
    }
    return merged;
}

function getCategory(filename, rules) {
    const ext = path.extname(filename).slice(1).toLowerCase();
    for (const [cat, exts] of Object.entries(rules)) {
        if (exts.includes(ext)) return cat;
    }
    return 'Other';
}

function copyFile(src, dest) {
    return new Promise((resolve, reject) => {
        const readStream = fs.createReadStream(src);
        const writeStream = fs.createWriteStream(dest);
        readStream.pipe(writeStream);
        readStream.on('error', reject);
        writeStream.on('error', reject);
        writeStream.on('finish', resolve);
    });
}

async function organize(folder, rules, mode, dateFolders, recursive, dryRun, verbose, logFile) {
    folder = path.resolve(folder);
    if (!fs.existsSync(folder)) throw new Error(`Folder not found: ${folder}`);
    const stats = fs.statSync(folder);
    if (!stats.isDirectory()) throw new Error(`${folder} is not a directory`);

    const logLines = [];
    let processed = 0;

    const walk = async (dir) => {
        const entries = fs.readdirSync(dir, { withFileTypes: true });
        for (const entry of entries) {
            const fullPath = path.join(dir, entry.name);
            if (entry.isDirectory()) {
                if (recursive) await walk(fullPath);
                continue;
            }
            if (entry.name.startsWith('.')) continue;
            const cat = getCategory(entry.name, rules);
            let targetDir = path.join(folder, cat);
            if (dateFolders) {
                const mtime = fs.statSync(fullPath).mtime;
                const dateSub = mtime.toISOString().slice(0, 7); // YYYY-MM
                targetDir = path.join(targetDir, dateSub);
            }
            if (!dryRun) {
                fs.mkdirSync(targetDir, { recursive: true });
            }
            let dest = path.join(targetDir, entry.name);

            // Conflict resolution
            if (fs.existsSync(dest)) {
                const ext = path.extname(entry.name);
                const base = path.basename(entry.name, ext);
                let counter = 1;
                while (fs.existsSync(path.join(targetDir, `${base}_${counter}${ext}`))) {
                    counter++;
                }
                dest = path.join(targetDir, `${base}_${counter}${ext}`);
                if (verbose) console.log(colorize(`Conflict, new name: ${path.basename(dest)}`, 'yellow'));
            }

            const action = mode === 'move' ? 'Move' : 'Copy';
            if (!dryRun) {
                if (mode === 'move') {
                    fs.renameSync(fullPath, dest);
                } else {
                    await copyFile(fullPath, dest);
                }
                processed++;
                logLines.push(`${action}: ${fullPath} -> ${dest}`);
                if (verbose) console.log(colorize(`${action}: ${entry.name} -> ${cat}/`, 'green'));
            } else {
                logLines.push(`[DRY RUN] ${action}: ${fullPath} -> ${dest}`);
                if (verbose) console.log(colorize(`[DRY RUN] ${action}: ${entry.name} -> ${cat}/`, 'blue'));
                processed++;
            }
        }
    };

    await walk(folder);

    if (logFile) {
        fs.writeFileSync(logFile, logLines.join('\n'), 'utf8');
        console.log(colorize(`Log saved to ${logFile}`, 'green'));
    }
    return processed;
}

async function main() {
    const args = require('minimist')(process.argv.slice(2), {
        string: ['c', 'm', 'l'],
        boolean: ['d', 'r', 'n', 'v'],
        alias: { c: 'config', m: 'mode', d: 'date-folders', r: 'recursive', n: 'dry-run', l: 'log', v: 'verbose' },
        default: { m: 'move' }
    });

    let folder = args._[0] || path.join(os.homedir(), 'Downloads');
    const configPath = args.c;
    const mode = args.m;
    const dateFolders = args.d;
    const recursive = args.r;
    const dryRun = args.n;
    const verbose = args.v;
    const logFile = args.l;

    if (mode !== 'move' && mode !== 'copy') {
        console.log(colorize('Error: mode must be move or copy', 'red'));
        process.exit(1);
    }

    try {
        const rules = loadConfig(configPath);
        const count = await organize(folder, rules, mode, dateFolders, recursive, dryRun, verbose, logFile);
        if (dryRun) {
            console.log(colorize(`Dry run completed. Would process ${count} files.`, 'green'));
        } else {
            console.log(colorize(`Processed ${count} files.`, 'green'));
        }
    } catch (err) {
        console.log(colorize(`Error: ${err.message}`, 'red'));
        process.exit(1);
    }
}

main();
