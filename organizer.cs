// organizer.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

class Organizer
{
    static string Colorize(string text, string color)
    {
        string col = color switch
        {
            "green" => "\x1b[92m",
            "red" => "\x1b[91m",
            "yellow" => "\x1b[93m",
            "blue" => "\x1b[94m",
            _ => "\x1b[0m"
        };
        return col + text + "\x1b[0m";
    }

    static Dictionary<string, List<string>> defaultRules = new()
    {
        ["Images"] = new List<string>{"jpg","jpeg","png","gif","bmp","svg","webp","ico"},
        ["Documents"] = new List<string>{"pdf","doc","docx","txt","xls","xlsx","ppt","pptx","odt","ods","odp","rtf"},
        ["Archives"] = new List<string>{"zip","rar","7z","tar","gz","bz2","xz","tgz"},
        ["Videos"] = new List<string>{"mp4","avi","mkv","mov","wmv","flv","webm","m4v"},
        ["Music"] = new List<string>{"mp3","wav","flac","aac","ogg","wma","m4a"},
        ["Programs"] = new List<string>{"exe","msi","dmg","deb","rpm","sh","bat","cmd"},
        ["Other"] = new List<string>()
    };

    static Dictionary<string, List<string>> LoadConfig(string path)
    {
        if (string.IsNullOrEmpty(path) || !File.Exists(path)) return defaultRules;
        string json = File.ReadAllText(path);
        var rules = JsonSerializer.Deserialize<Dictionary<string, List<string>>>(json);
        // Merge
        var merged = new Dictionary<string, List<string>>(defaultRules);
        foreach (var kv in rules)
        {
            merged[kv.Key] = kv.Value;
        }
        return merged;
    }

    static string GetCategory(string filename, Dictionary<string, List<string>> rules)
    {
        string ext = Path.GetExtension(filename).TrimStart('.').ToLower();
        foreach (var kv in rules)
        {
            if (kv.Value.Contains(ext)) return kv.Key;
        }
        return "Other";
    }

    static void Organize(string folder, Dictionary<string, List<string>> rules, string mode,
                         bool dateFolders, bool recursive, bool dryRun, bool verbose, string logFile)
    {
        var di = new DirectoryInfo(folder);
        if (!di.Exists) throw new Exception($"Folder not found: {folder}");
        var logLines = new List<string>();
        int processed = 0;

        var options = recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;
        foreach (var file in di.GetFiles("*", options))
        {
            if (file.Name.StartsWith('.')) continue;
            string cat = GetCategory(file.Name, rules);
            string targetDir = Path.Combine(folder, cat);
            if (dateFolders)
            {
                string dateSub = file.LastWriteTime.ToString("yyyy-MM");
                targetDir = Path.Combine(targetDir, dateSub);
            }
            if (!dryRun) Directory.CreateDirectory(targetDir);
            string dest = Path.Combine(targetDir, file.Name);

            // Conflict resolution
            if (File.Exists(dest))
            {
                string baseName = Path.GetFileNameWithoutExtension(file.Name);
                string ext = Path.GetExtension(file.Name);
                int counter = 1;
                while (File.Exists(Path.Combine(targetDir, $"{baseName}_{counter}{ext}"))) counter++;
                dest = Path.Combine(targetDir, $"{baseName}_{counter}{ext}");
                if (verbose) Console.WriteLine(Colorize($"Conflict, new name: {Path.GetFileName(dest)}", "yellow"));
            }

            string action = mode == "move" ? "Move" : "Copy";
            if (!dryRun)
            {
                if (mode == "move") File.Move(file.FullName, dest);
                else File.Copy(file.FullName, dest);
                processed++;
                logLines.Add($"{action}: {file.FullName} -> {dest}");
                if (verbose) Console.WriteLine(Colorize($"{action}: {file.Name} -> {cat}/", "green"));
            }
            else
            {
                logLines.Add($"[DRY RUN] {action}: {file.FullName} -> {dest}");
                if (verbose) Console.WriteLine(Colorize($"[DRY RUN] {action}: {file.Name} -> {cat}/", "blue"));
                processed++;
            }
        }

        if (!string.IsNullOrEmpty(logFile))
        {
            File.WriteAllLines(logFile, logLines);
            Console.WriteLine(Colorize($"Log saved to {logFile}", "green"));
        }
        if (dryRun)
            Console.WriteLine(Colorize($"Dry run completed. Would process {processed} files.", "green"));
        else
            Console.WriteLine(Colorize($"Processed {processed} files.", "green"));
    }

    static void Main(string[] args)
    {
        string folder = null, configPath = null, mode = "move", logFile = null;
        bool dateFolders = false, recursive = false, dryRun = false, verbose = false;

        for (int i = 0; i < args.Length; i++)
        {
            string arg = args[i];
            if (arg == "-c" && i+1 < args.Length) configPath = args[++i];
            else if (arg == "-m" && i+1 < args.Length) mode = args[++i];
            else if (arg == "-d") dateFolders = true;
            else if (arg == "-r") recursive = true;
            else if (arg == "-n") dryRun = true;
            else if (arg == "-l" && i+1 < args.Length) logFile = args[++i];
            else if (arg == "-v") verbose = true;
            else if (arg == "-h") { Console.WriteLine("Help..."); return; }
            else if (folder == null) folder = arg;
        }
        if (folder == null) folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads");

        try
        {
            var rules = LoadConfig(configPath);
            Organize(folder, rules, mode, dateFolders, recursive, dryRun, verbose, logFile);
        }
        catch (Exception e)
        {
            Console.WriteLine(Colorize($"Error: {e.Message}", "red"));
        }
    }
}
