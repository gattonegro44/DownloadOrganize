DownloadOrganizer – Организатор загрузок на 7 языках
Автоматическая сортировка файлов в папке загрузок по типам, датам или пользовательским правилам.
Поддерживает настраиваемые категории, перемещение/копирование, режим симуляции, логирование и обработку вложенных папок.
Реализован на семи языках программирования с единым интерфейсом командной строки.

🚀 Возможности
Сортировка по типу – изображения, документы, архивы, видео, музыка, программы и другие.

Настраиваемые правила – файл конфигурации (JSON/YAML) для определения категорий и расширений.

Организация по дате – создание подпапок по году/месяцу (например, 2025/03).

Режимы – перемещение (move) или копирование (copy).

Симуляция – опция --dry-run для предварительного просмотра.

Логирование – запись всех действий в файл.

Рекурсивная обработка – сортировка файлов во всех подпапках.

Цветной вывод – наглядные сообщения о действиях.

📖 Использование
Синтаксис (единый для всех версий):

bash
<команда> [путь_к_папке] [опции]
Основные опции
Опция	Описание
-c, --config <file>	Путь к файлу конфигурации (JSON)
-m, --mode <move|copy>	Режим: переместить или скопировать (по умолчанию move)
-d, --date-folders	Создавать подпапки по дате (год/месяц)
-r, --recursive	Обрабатывать подпапки рекурсивно
-n, --dry-run	Только показать, что будет сделано
-l, --log <file>	Записать лог в файл
-v, --verbose	Подробный вывод
Примеры
bash
# Организовать папку ~/Downloads по умолчанию
python organizer.py

# Использовать пользовательский конфиг и режим копирования
python organizer.py -c rules.json -m copy

# Создать папки по дате и показать, что будет сделано
python organizer.py -d -n

# Обработать другую папку рекурсивно
python organizer.py ~/Documents -r
🛠 Установка и запуск
Python
bash
pip install pyyaml  # если нужен YAML
python organizer.py [путь] [опции]
Go
bash
go build organizer.go
./organizer [путь] [опции]
JavaScript (Node.js)
bash
npm install fs path
node organizer.js [путь] [опции]
C++
bash
g++ -std=c++11 organizer.cpp -o organizer
./organizer [путь] [опции]
C#
bash
csc organizer.cs
mono organizer.exe [путь] [опции]   # или dotnet run
Java
bash
javac organizer.java
java organizer [путь] [опции]
Ruby
bash
ruby organizer.rb [путь] [опции]
🧠 Формат конфигурации (JSON)
Пример rules.json:

json
{
  "Images": ["jpg", "jpeg", "png", "gif", "bmp", "svg"],
  "Documents": ["pdf", "doc", "docx", "txt", "xls", "xlsx", "ppt", "pptx"],
  "Archives": ["zip", "rar", "7z", "tar", "gz"],
  "Videos": ["mp4", "avi", "mkv", "mov", "wmv"],
  "Music": ["mp3", "wav", "flac", "aac", "ogg"],
  "Programs": ["exe", "msi", "dmg", "deb", "rpm"],
  "Other": []
}
Если файл не указан, используется встроенная конфигурация по умолчанию.

📂 Состав репозитория
Язык	Файл	Статус
Python	organizer.py	✅
Go	organizer.go	✅
JavaScript	organizer.js	✅
C++	organizer.cpp	✅
C#	organizer.cs	✅
Java	organizer.java	✅
Ruby	organizer.rb	✅
🤝 Вклад в проект
Приветствуются улучшения:

Поддержка YAML конфигурации.

Интеграция с системными уведомлениями.

Графический интерфейс.

Создавайте Issues и Pull Requests.

📜 Лицензия
MIT License – свободное использование, модификация и распространение.
