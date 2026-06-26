# organizer.py
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import json
import argparse
import shutil
import datetime
from pathlib import Path
from collections import defaultdict

# ANSI colors
COLORS = {
    'green': '\033[92m',
    'red': '\033[91m',
    'yellow': '\033[93m',
    'blue': '\033[94m',
    'reset': '\033[0m'
}

def colorize(text, color):
    return f"{COLORS.get(color, '')}{text}{COLORS['reset']}"

# Встроенная конфигурация по умолчанию
DEFAULT_RULES = {
    "Images": ["jpg", "jpeg", "png", "gif", "bmp", "svg", "webp", "ico"],
    "Documents": ["pdf", "doc", "docx", "txt", "xls", "xlsx", "ppt", "pptx", "odt", "ods", "odp", "rtf"],
    "Archives": ["zip", "rar", "7z", "tar", "gz", "bz2", "xz", "tgz"],
    "Videos": ["mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v"],
    "Music": ["mp3", "wav", "flac", "aac", "ogg", "wma", "m4a"],
    "Programs": ["exe", "msi", "dmg", "deb", "rpm", "sh", "bat", "cmd"],
    "Other": []
}

def load_config(config_path):
    if not config_path or not os.path.exists(config_path):
        return DEFAULT_RULES
    with open(config_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    # Объединяем с дефолтными, если каких-то категорий нет
    rules = DEFAULT_RULES.copy()
    for cat, exts in data.items():
        rules[cat] = exts
    return rules

def get_category(filename, rules):
    ext = os.path.splitext(filename)[1].lower().lstrip('.')
    for cat, exts in rules.items():
        if ext in exts:
            return cat
    return "Other"

def organize(folder, rules, mode='move', date_folders=False, recursive=False, dry_run=False, verbose=False, log_file=None):
    folder = Path(folder).resolve()
    if not folder.exists():
        raise FileNotFoundError(f"Folder not found: {folder}")

    log_lines = []
    actions = 0

    # Сбор файлов
    if recursive:
        files = list(folder.rglob('*'))
    else:
        files = list(folder.glob('*'))

    for file_path in files:
        if not file_path.is_file():
            continue
        # Пропускаем скрытые файлы (опционально)
        if file_path.name.startswith('.'):
            continue

        cat = get_category(file_path.name, rules)
        target_dir = folder / cat
        if date_folders:
            # Добавляем подпапку по дате изменения
            mtime = file_path.stat().st_mtime
            dt = datetime.datetime.fromtimestamp(mtime)
            date_sub = dt.strftime('%Y-%m')
            target_dir = target_dir / date_sub

        if not target_dir.exists():
            if not dry_run:
                target_dir.mkdir(parents=True, exist_ok=True)

        dest = target_dir / file_path.name

        if dest.exists():
            # Конфликт – добавляем суффикс
            base = dest.stem
            ext = dest.suffix
            counter = 1
            while dest.exists():
                dest = target_dir / f"{base}_{counter}{ext}"
                counter += 1
            if verbose:
                print(colorize(f"Conflict, new name: {dest.name}", 'yellow'))

        action = "Move" if mode == 'move' else "Copy"
        if not dry_run:
            if mode == 'move':
                shutil.move(str(file_path), str(dest))
            else:
                shutil.copy2(str(file_path), str(dest))
            actions += 1
            log_lines.append(f"{action}: {file_path} -> {dest}")
            if verbose:
                print(colorize(f"{action}: {file_path.name} -> {cat}/", 'green'))
        else:
            log_lines.append(f"[DRY RUN] {action}: {file_path} -> {dest}")
            if verbose:
                print(colorize(f"[DRY RUN] {action}: {file_path.name} -> {cat}/", 'blue'))
            actions += 1

    # Логирование
    if log_file:
        with open(log_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(log_lines))
        print(colorize(f"Log saved to {log_file}", 'green'))

    return actions

def main():
    parser = argparse.ArgumentParser(description="DownloadOrganizer – сортировка файлов по типам")
    parser.add_argument('folder', nargs='?', default=os.path.expanduser('~/Downloads'),
                        help='Папка для организации (по умолчанию ~/Downloads)')
    parser.add_argument('-c', '--config', help='Файл конфигурации (JSON)')
    parser.add_argument('-m', '--mode', choices=['move', 'copy'], default='move',
                        help='Режим: move – переместить, copy – скопировать (по умолчанию move)')
    parser.add_argument('-d', '--date-folders', action='store_true',
                        help='Создавать подпапки по дате (год-месяц)')
    parser.add_argument('-r', '--recursive', action='store_true',
                        help='Обрабатывать подпапки рекурсивно')
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='Показать действия без фактического выполнения')
    parser.add_argument('-l', '--log', help='Сохранить лог в файл')
    parser.add_argument('-v', '--verbose', action='store_true', help='Подробный вывод')
    args = parser.parse_args()

    try:
        rules = load_config(args.config)
        count = organize(args.folder, rules, args.mode, args.date_folders,
                         args.recursive, args.dry_run, args.verbose, args.log)
        if args.dry_run:
            print(colorize(f"Dry run completed. Would process {count} files.", 'green'))
        else:
            print(colorize(f"Processed {count} files.", 'green'))
    except Exception as e:
        sys.exit(colorize(f"Error: {e}", 'red'))

if __name__ == '__main__':
    main()
