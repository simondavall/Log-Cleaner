# Log-Cleaner
Allows the user to remove log noise.


# Acknowledgments #
cJSON - Copyright (c) 2009-2017 Dave Gamble and cJSON contributors\
Shout out to: Dave Gamble et al for their work on cJSON\
https://github.com/DaveGamble/cJSON

# Quick Start
1. Clone repository.
2. Navigate to the project root
3. Execute the makefile
```bash
make
```
4. Copy the log-cleaner file to a location covered by PATH (e.g. I have mine in .local/bin/log-cleaner)
5. Copy the sample log-cleaner-config.json file to the same location.
6. Configure the json file with log name to cleanse and strings to identify items to remove.
7. Execute log-cleaner with log and config.json as command line parameters.
```bash
log-cleaner <path to log file> <path to config file>
```

