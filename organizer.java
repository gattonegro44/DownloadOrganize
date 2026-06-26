// organizer.java
import java.io.*;
import java.nio.file.*;
import java.nio.file.attribute.*;
import java.util.*;
import java.time.*;
import java.time.format.*;
import com.google.gson.*; // install gson

public class organizer {
    private static final String RESET = "\u001B[0m";
    private static final String GREEN = "\u001B[92m";
    private static final String RED = "\u001B[91m";
    private static final String YELLOW = "\u001B[93m";
    private static final String BLUE = "\u001B[94m";

    private static String colorize(String text, String color) {
        return color + text + RESET;
    }

    private static Map<String, List<String>> defaultRules = new HashMap<>();
    static {
        defaultRules.put("Images", Arrays.asList("jpg","jpeg","png","gif","bmp","svg","webp","ico"));
        defaultRules.put("Documents", Arrays.asList("pdf","doc","docx","txt","xls","xlsx","ppt","pptx","odt","ods","odp","rtf"));
        defaultRules.put("Archives", Arrays.asList("zip","rar","7z","tar","gz","bz2","xz","tgz"));
        defaultRules.put("Videos", Arrays.asList("mp4","avi","mkv","mov","wmv","flv","webm","m4v"));
        defaultRules.put("Music", Arrays.asList("mp3","wav","flac","aac","ogg","wma","m4a"));
        defaultRules.put("Programs", Arrays.asList("exe","msi","dmg","deb","rpm","sh","bat","cmd"));
        defaultRules.put("Other", new ArrayList<>());
    }

    private static Map<String, List<String>> loadConfig(String path) throws IOException {
        if (path == null || path.isEmpty() || !Files.exists(Paths.get(path)))
            return defaultRules;
        String json = new String(Files.readAllBytes(Paths.get(path)));
        Gson gson = new Gson();
        Type type = new com.google.gson.reflect.TypeToken<Map<String, List<String>>>(){}.getType();
        Map<String, List<String>> rules = gson.fromJson(json, type);
        // Merge
        Map<String, List<String>> merged = new HashMap<>(defaultRules);
        for (Map.Entry<String, List<String>> e : rules.entrySet()) {
            merged.put(e.getKey(), e.getValue());
        }
        return merged;
    }

    private static String getCategory(String filename, Map<String, List<String>> rules) {
        int dot = filename.lastIndexOf('.');
        if (dot == -1) return "Other";
        String ext = filename.substring(dot+1).toLowerCase();
        for (Map.Entry<String, List<String>> e : rules.entrySet()) {
            if (e.getValue().contains(ext)) return e.getKey();
        }
        return "Other";
    }

    private static void organize(String folder, Map<String, List<String>> rules, String mode,
                                 boolean dateFolders, boolean recursive, boolean dryRun,
                                 boolean verbose, String logFile) throws IOException {
        Path root = Paths.get(folder);
        if (!Files.exists(root)) throw new IOException("Folder not found: " + folder);
        if (!Files.isDirectory(root)) throw new IOException("Not a directory: " + folder);

        List<String> logLines = new ArrayList<>();
        int processed = 0;

        try (var stream = Files.walk(root)) {
            for (Path entry : (Iterable<Path>) stream::iterator) {
                if (Files.isDirectory(entry)) {
                    if (!recursive && !entry.equals(root)) continue;
                    continue;
                }
                String fname = entry.getFileName().toString();
                if (fname.startsWith(".")) continue;
                String cat = getCategory(fname, rules);
                Path targetDir = root.resolve(cat);
                if (dateFolders) {
                    FileTime ft = Files.getLastModifiedTime(entry);
                    Instant instant = ft.toInstant();
                    String dateSub = instant.atZone(ZoneId.systemDefault()).format(DateTimeFormatter.ofPattern("yyyy-MM"));
                    targetDir = targetDir.resolve(dateSub);
                }
                if (!dryRun) Files.createDirectories(targetDir);
                Path dest = targetDir.resolve(fname);

                // Conflict
                if (Files.exists(dest)) {
                    String base = dest.getFileName().toString();
                    int dot = base.lastIndexOf('.');
                    String namePart = dot > 0 ? base.substring(0, dot) : base;
                    String extPart = dot > 0 ? base.substring(dot) : "";
                    int counter = 1;
                    while (Files.exists(targetDir.resolve(namePart + "_" + counter + extPart))) counter++;
                    dest = targetDir.resolve(namePart + "_" + counter + extPart);
                    if (verbose) System.out.println(colorize("Conflict, new name: " + dest.getFileName(), YELLOW));
                }

                String action = mode.equals("move") ? "Move" : "Copy";
                if (!dryRun) {
                    if (mode.equals("move")) Files.move(entry, dest, StandardCopyOption.REPLACE_EXISTING);
                    else Files.copy(entry, dest, StandardCopyOption.REPLACE_EXISTING);
                    processed++;
                    logLines.add(action + ": " + entry + " -> " + dest);
                    if (verbose) System.out.println(colorize(action + ": " + fname + " -> " + cat + "/", GREEN));
                } else {
                    logLines.add("[DRY RUN] " + action + ": " + entry + " -> " + dest);
                    if (verbose) System.out.println(colorize("[DRY RUN] " + action + ": " + fname + " -> " + cat + "/", BLUE));
                    processed++;
                }
            }
        }

        if (logFile != null && !logFile.isEmpty()) {
            Files.write(Paths.get(logFile), logLines);
            System.out.println(colorize("Log saved to " + logFile, GREEN));
        }
        if (dryRun)
            System.out.println(colorize("Dry run completed. Would process " + processed + " files.", GREEN));
        else
            System.out.println(colorize("Processed " + processed + " files.", GREEN));
    }

    public static void main(String[] args) throws IOException {
        String folder = null, configPath = null, mode = "move", logFile = null;
        boolean dateFolders = false, recursive = false, dryRun = false, verbose = false;

        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            if (arg.equals("-c") && i+1 < args.length) configPath = args[++i];
            else if (arg.equals("-m") && i+1 < args.length) mode = args[++i];
            else if (arg.equals("-d")) dateFolders = true;
            else if (arg.equals("-r")) recursive = true;
            else if (arg.equals("-n")) dryRun = true;
            else if (arg.equals("-l") && i+1 < args.length) logFile = args[++i];
            else if (arg.equals("-v")) verbose = true;
            else if (arg.equals("-h")) { System.out.println("Help..."); return; }
            else if (folder == null) folder = arg;
        }
        if (folder == null) folder = System.getProperty("user.home") + "/Downloads";

        try {
            Map<String, List<String>> rules = loadConfig(configPath);
            organize(folder, rules, mode, dateFolders, recursive, dryRun, verbose, logFile);
        } catch (Exception e) {
            System.err.println(colorize("Error: " + e.getMessage(), RED));
        }
    }
}
