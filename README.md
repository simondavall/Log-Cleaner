# Log-Cleaner
This project was inspired by excessive noise in my neovim logs. Specifically the lsp.log. 
For me, there are certain errors, particularly relating to deprecations, that are described
as errors that are of no consequence, and should be ignored.
I am of the opinion that if an error message should be ignored, it shouldn't be there in the first
place. (Or be able to be 'switched off').
I like clean logs that report issues that should (and can) be addressed.

So this log-cleaner, allows the user to remove log noise. I wanted to keep the implementation simple;
provide some mechanism to identify noisy log entries, and remove them.

I wanted to avoid using regex as a means of identifying the items, as regex has lead me down too
many dark paths in the past. So I have devised a simple config sttucture allowing the user to setup
a series of strings, for a given log entry. If all strings are present in a given log entry, then the log entry is a match
and is removed.

Note. This is a Linux only implementation.

# Acknowledgments #
cJSON - Copyright (c) 2009-2017 Dave Gamble and cJSON contributors\
Shout out to: Dave Gamble et al for their work on cJSON\
https://github.com/DaveGamble/cJSON

# Config structure #
As an example, I have the following repeated entry in my lsp.log.\
```text
[ERROR][2026-01-30 11:44:08] ...p/_transport.lua:36	"rpc"	"clangd"	"stderr"	"E[11:44:08.506] offsetEncoding capability
is a deprecated clangd extension that'll go away with clangd 23. Migrate to standard positionEncodings capability introduced by LSP 3.17"
```

According to some digging...It’s not an error that breaks anything. It’s a deprecation warning coming from clangd, forwarded 
into Neovim’s lsp.log. Neovim still works fine, which is why this shows up as stderr noise, not a crash.

I want to remove these entries, so that other legitimate errors don't get hidden.

My config setup:

```json
{
    "lsp.log" : [
        "clangd",
        "sdterr",
        "offsetEncoding capability is a deprecated clangd extension that'll go away with clangd 23"
    ]
}
```
If all three strings are present in the log entry, it will be removed from the log.

Note. It is clear that this is not perfect, and if string items are not defined specifically enough, it could produce
false positives.

There is a --retain (-r) option defined to store all removed items in a newly created and timestamped log file. (See Usage section)

# Usage #
To build the executable, use:
```bash
make log-cleaner
```
Copy the 'log-cleaner' executable to a directory in your $PATH.
I used '~/.local/bin'
Copy the sample 'log-cleaner-config.json' file to the same location, and change its contents
to match your own log file and search requirements.

To execute:
```bash
log-cleaner [options] <log_file_path> <config_file_path>
```

### Options

- **`--help`, `-h`**  
  Show this help message.

- **`--version`, `-v`**  
  Show version information.

- **`--retain`, `-r`**  
  Saves the removed log entries to a separate file in the same directory as the original log file.  
  File name format: `removed_<log_file_name>_<timestamp>.log`  
  Default: `false`

# Example #
A full example:
```bash
log-cleaner --retain ~/.local/state/nvim/lsp.log ~/.local/bin/log-cleaner-config.json
```
This will take all the search criteria defined in log-cleaner-config.json in the "lsp.log" section
and match it against all log entries. It it finds a match it will print the line to the console. Because the --retain option
was set the log enrty will be add it to a newly created file 'removed_lsp_\<timestamp\>.log'

