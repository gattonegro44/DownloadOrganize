#!/usr/bin/env ruby
# organizer.rb
# encoding: UTF-8

require 'json'
require 'fileutils'
require 'date'
require 'optparse'

COLORS = {
  green: "\e[92m",
  red: "\e[91m",
  yellow: "\e[93m",
  blue: "\e[94m",
  reset: "\e[0m"
}

def colorize(text, color)
  "#{COLORS[color]}#{text}#{COLORS[:reset]}"
end

DEFAULT_RULES = {
  'Images' => %w[jpg jpeg png gif bmp svg webp ico],
  'Documents' => %w[pdf doc docx txt xls xlsx ppt pptx odt ods odp rtf],
  'Archives' => %w[zip rar 7z tar gz bz2 xz tgz],
  'Videos' => %w[mp4 avi mkv mov wmv flv webm m4v],
  'Music' => %w[mp3 wav flac aac ogg wma m4a],
  'Programs' => %w[exe msi dmg deb rpm sh bat cmd],
  'Other' => []
}

def load_config(path)
  return DEFAULT_RULES unless path && File.exist?(path)
  data = JSON.parse(File.read(path))
  rules = DEFAULT_RULES.dup
  data.each { |cat, exts| rules[cat] = exts }
  rules
end

def get_category(filename, rules)
  ext = File.extname(filename).downcase[1..-1] || ''
  rules.each do |cat, exts|
    return cat if exts.include?(ext)
  end
  'Other'
end

def organize(folder, rules, mode: 'move', date_folders: false, recursive: false,
             dry_run: false, verbose: false, log_file: nil)
  folder = File.expand_path(folder)
  unless Dir.exist?(folder)
    raise "Folder not found: #{folder}"
  end

  log_lines = []
  processed = 0

  entries = if recursive
              Dir.glob(File.join(folder, '**/*'), File::FNM_DOTMATCH)
            else
              Dir.glob(File.join(folder, '*'), File::FNM_DOTMATCH)
            end

  entries.each do |entry|
    next unless File.file?(entry)
    fname = File.basename(entry)
    next if fname.start_with?('.')
    cat = get_category(fname, rules)
    target_dir = File.join(folder, cat)
    if date_folders
      mtime = File.mtime(entry)
      date_sub = mtime.strftime('%Y-%m')
      target_dir = File.join(target_dir, date_sub)
    end
    unless dry_run
      FileUtils.mkdir_p(target_dir)
    end
    dest = File.join(target_dir, fname)

    # Conflict
    if File.exist?(dest)
      base = File.basename(fname, '.*')
      ext = File.extname(fname)
      counter = 1
      while File.exist?(File.join(target_dir, "#{base}_#{counter}#{ext}"))
        counter += 1
      end
      dest = File.join(target_dir, "#{base}_#{counter}#{ext}")
      if verbose
        puts colorize("Conflict, new name: #{File.basename(dest)}", :yellow)
      end
    end

    action = mode == 'move' ? 'Move' : 'Copy'
    unless dry_run
      if mode == 'move'
        FileUtils.mv(entry, dest)
      else
        FileUtils.cp(entry, dest)
      end
      processed += 1
      log_lines << "#{action}: #{entry} -> #{dest}"
      if verbose
        puts colorize("#{action}: #{fname} -> #{cat}/", :green)
      end
    else
      log_lines << "[DRY RUN] #{action}: #{entry} -> #{dest}"
      if verbose
        puts colorize("[DRY RUN] #{action}: #{fname} -> #{cat}/", :blue)
      end
      processed += 1
    end
  end

  if log_file
    File.write(log_file, log_lines.join("\n"))
    puts colorize("Log saved to #{log_file}", :green)
  end

  processed
end

options = {
  folder: nil,
  config: nil,
  mode: 'move',
  date_folders: false,
  recursive: false,
  dry_run: false,
  log_file: nil,
  verbose: false
}

parser = OptionParser.new do |opts|
  opts.banner = "Usage: organizer.rb [folder] [options]"
  opts.on("-c", "--config FILE", "Config file") { |v| options[:config] = v }
  opts.on("-m", "--mode MODE", "move or copy") { |v| options[:mode] = v }
  opts.on("-d", "--date-folders", "Date folders") { options[:date_folders] = true }
  opts.on("-r", "--recursive", "Recursive") { options[:recursive] = true }
  opts.on("-n", "--dry-run", "Dry run") { options[:dry_run] = true }
  opts.on("-l", "--log FILE", "Log file") { |v| options[:log_file] = v }
  opts.on("-v", "--verbose", "Verbose") { options[:verbose] = true }
  opts.on("-h", "--help", "Help") { puts opts; exit }
end
parser.parse!

options[:folder] = ARGV[0] || File.join(Dir.home, 'Downloads')

begin
  rules = load_config(options[:config])
  count = organize(options[:folder], rules,
                   mode: options[:mode],
                   date_folders: options[:date_folders],
                   recursive: options[:recursive],
                   dry_run: options[:dry_run],
                   verbose: options[:verbose],
                   log_file: options[:log_file])
  if options[:dry_run]
    puts colorize("Dry run completed. Would process #{count} files.", :green)
  else
    puts colorize("Processed #{count} files.", :green)
  end
rescue => e
  puts colorize("Error: #{e.message}", :red)
  exit 1
end
