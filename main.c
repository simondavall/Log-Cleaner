#define _GNU_SOURCE
#include "cJSON.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define VERSION "v1.0.0"
// program limitation: max config size of 4k
#define MAX_CONFIG_FILE_SIZE 4096

typedef struct {
  char **items;
  int length;
} Identifier;

typedef struct {
  char *log_file;
  Identifier **identifiers;
  int identifier_count;
} Config;

typedef struct {
  char *file_path;
  char *config_file;
  bool saveRemovedItems;
} Settings;

void clean_file(const char *file_path, const Config *config, Settings settings);
char *create_timestamped_file_path(const char *filename, const char *prefix);
void delete_config(Config *config);
Config *get_config(const char *log_file_name, char *config_file);
const char *get_filename(const char *path);
void *m_alloc(void *ptr, size_t size, const char *err_msg);
void processArgs(int argc, char **argv, Settings *setttings);
void show_usage();

int main(int argc, char *argv[]) {

  Settings settings = {.saveRemovedItems = false};
  processArgs(argc, argv, &settings);

  char *file_path = settings.file_path;
  const char *filename = get_filename(file_path);
  char *config_file = settings.config_file;

  Config *config = get_config(filename, config_file);
  if (config == NULL) {
    printf("Could not find config information for log file '%s'.\nCheck the "
           "'%s' file for a '%s' section.\n",
           filename, config_file, filename);
    exit(EXIT_FAILURE);
  }

  clean_file(file_path, config, settings);
  delete_config(config);

  return EXIT_SUCCESS;
}

void clean_file(const char *file_path, const Config *config, Settings settings) {
  FILE *log_file_ptr;
  log_file_ptr = fopen(file_path, "rb");
  if (log_file_ptr == NULL) {
    printf("Error opening file: %s", file_path);
    exit(EXIT_FAILURE);
  }

  char *cleaned_filename = create_timestamped_file_path(file_path, "cleaned");
  FILE *cleaned_filePtr;
  cleaned_filePtr = fopen(cleaned_filename, "w");
  if (cleaned_filePtr == NULL) {
    printf("Error opening %s\n", cleaned_filename);
    exit(EXIT_FAILURE);
  }

  FILE *removed_filePtr = NULL;
  char *removed_filename = NULL;
  if (settings.saveRemovedItems) {
    removed_filename = create_timestamped_file_path(file_path, "removed");
    removed_filePtr = fopen(removed_filename, "w");
    if (removed_filePtr == NULL) {
      printf("Error opening %s\n", removed_filename);
      exit(EXIT_FAILURE);
    }
  }

  int str_len;
  char *log_entry = NULL;
  size_t len = 0;
  while ((str_len = (int)getline(&log_entry, &len, log_file_ptr)) != -1) {
    log_entry[str_len - 1] = '\0';
    if (strcmp(log_entry, "\0") == 0) // ignore empty strings
      continue;

    bool match = true;
    for (int k = 0; k < config->identifier_count; k++) {
      match = true;
      for (int l = 0; l < config->identifiers[k]->length; l++) {
        if (strstr(log_entry, config->identifiers[k]->items[l]) == NULL) {
          match = false;
          break;
        }
      }
      if (match)
        break;
    }

    if (match) {
      if (settings.saveRemovedItems)
        fprintf(removed_filePtr, "%s\n", log_entry);
      printf("Removed: %s\n", log_entry);
    } else {
      fprintf(cleaned_filePtr, "%s\n", log_entry);
    }
  }

  if (log_entry)
    free(log_entry);

  if (settings.saveRemovedItems)
    fclose(removed_filePtr);
  fclose(cleaned_filePtr);
  fclose(log_file_ptr);

  if (rename(cleaned_filename, file_path) != 0) {
    printf("Unable to replace '%s' with the cleaned log file '%s'.\nFile is "
           "likely locked by another process.\nThis file will need to be replaced manually.\n",
           file_path, cleaned_filename);
  }

  if (settings.saveRemovedItems)
    free(removed_filename);
  free(cleaned_filename);
}

Config *get_config(const char *log_file_name, char *config_file) {
  FILE *fp = fopen(config_file, "r");
  if (!fp) {
    printf("Error: Unable to open the file %s. Check spelling and that it "
           "exists.\n",
           config_file);
    exit(EXIT_FAILURE);
  }

  char json_string[MAX_CONFIG_FILE_SIZE];
  int len = fread(json_string, 1, sizeof(json_string) - 1, fp);
  fclose(fp);

  if (len <= 0) {
    printf("No config set in config file '%s'", config_file);
    exit(EXIT_FAILURE);
  }

  // null terminate json_string
  json_string[len] = '\0';

  cJSON *root = cJSON_Parse(json_string);
  if (!root) {
    printf("Parse error: %s\n", cJSON_GetErrorPtr());
    exit(EXIT_FAILURE);
  }
  
  cJSON *files = cJSON_GetObjectItemCaseSensitive(root, "files");
  if (!cJSON_IsObject(files)) {
    printf("Invalid 'files' object.\n");
    cJSON_Delete(root);
    exit(EXIT_FAILURE);
  }

  Config *config = NULL;

  cJSON *log_file;
  cJSON_ArrayForEach(log_file, files) {
    if (strcmp(log_file->string, log_file_name) != 0)
      continue;

    config = m_alloc(config, sizeof(Config), "config item");
    config->log_file = NULL;
    config->log_file = m_alloc(config->log_file, strlen(log_file->string) + 1, "log file name in config");
    strcpy(config->log_file, log_file->string);

    cJSON *array = log_file;
    int size = cJSON_GetArraySize(array);
    if (size <= 0) { // error in config
      printf("No identifier items set for %s in config", log_file_name);
      exit(EXIT_FAILURE);
    }

    config->identifiers = NULL;
    config->identifiers = m_alloc(config->identifiers, size * sizeof(Identifier *), "identifiers list");
    config->identifier_count = size;

    for (int i = 0; i < size; i++) {

      cJSON *inner_array = cJSON_GetArrayItem(array, i);
      if (!cJSON_IsArray(inner_array))
        continue;

      Identifier *identifier = NULL;
      identifier = m_alloc(identifier, sizeof(Identifier), "config identifier");
      config->identifiers[i] = identifier;

      int inner_size = cJSON_GetArraySize(inner_array);
      identifier->length = inner_size;
      if (inner_size <= 0)
        continue;

      identifier->items = NULL;
      identifier->items = m_alloc(identifier->items, inner_size * sizeof(char *), "identifier items");

      for (int j = 0; j < inner_size; j++) {
        cJSON *item = cJSON_GetArrayItem(inner_array, j);
        if (cJSON_IsString(item)) {
          char *str = NULL;
          str = m_alloc(str, strlen(item->valuestring) + 1, "item string in config");
          strcpy(str, item->valuestring);
          config->identifiers[i]->items[j] = str;
        }
      }
    }
    break;
  }

  cJSON_Delete(root);

  return config;
}

// Free the memory allocated to config
void delete_config(Config *config) {
  for (int i = 0; i < config->identifier_count; i++) {
    for (int j = 0; j < config->identifiers[i]->length; j++) {
      free(config->identifiers[i]->items[j]);
    }
    free(config->identifiers[i]->items);
    free(config->identifiers[i]);
  }
  free(config->log_file);
  free(config->identifiers);
  free(config);
}

void processArgs(int argc, char *argv[], Settings *settings) {
  int ch;

  // Define long options
  static struct option long_options[] = {
      {"help",    no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {"retain",  no_argument, NULL, 'r'},
      {0,         0,           0,    0  }
  };

  while ((ch = getopt_long(argc, argv, "hvr", long_options, NULL)) != -1) {
    switch (ch) {
    case 'r':
      settings->saveRemovedItems = true;
      break;
    case 'v':
      printf("%s\n", VERSION);
      exit(EXIT_SUCCESS);
    case 'h':
    default:
      show_usage();
    }
  }

  // Process positional arguments
  if (optind + 2 > argc) {
    fprintf(stderr, "Error: Two file paths are required.\n");
    show_usage();
  }
  
  settings->file_path = argv[optind];
  settings->config_file = argv[optind + 1];
}

void show_usage() {
  printf("Usage: log-cleaner [options] <log_filepath> <config_filepath>\n");
  printf("Options:\n");
  printf("  --help, -h     Show this help message\n");
  printf("  --version, -v  Show version information\n");
  printf("  --retain, -r   Saves the removed log entries to a separate file in the same directory\n\t\t as the "
         "original log "
         "file. 'removed_<log_file_name>_<timestamp>.log'\n\t\t Default: false\n");
  exit(EXIT_SUCCESS);
}

const char *get_filename(const char *path) {
  const char *last_slash = strrchr(path, '/');
  return last_slash ? last_slash + 1 : path;
}

char *create_timestamped_file_path(const char *file_path, const char *prefix) {
  char base[256] = {0};
  char timestamp[20];
  char *ext;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  const char *fname = get_filename(file_path);

  ext = strrchr(fname, '.');
  if (ext && strcmp(ext, ".log") == 0) {
    size_t len = ext - fname;
    strncpy(base, fname, len);
    base[len] = '\0';
  } else {
    strcpy(base, fname);
  }

  strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);

  size_t dir_len = fname - file_path;
  size_t new_len = dir_len + strlen(prefix) + 1 + strlen(base) + 1 + strlen(timestamp) + 5; // +5 for "_", ".log", \0
  char *new_file_path = NULL;
  new_file_path = m_alloc(new_file_path, new_len, "new file path");

  snprintf(new_file_path, new_len, "%.*s%s_%s_%s.log", (int)dir_len, file_path, prefix, base, timestamp);

  return new_file_path; // Caller must free()
}

void *m_alloc(void *ptr, size_t size, const char *field_name) {
  ptr = malloc(size);

  if (ptr == NULL) {
    printf("Unable to allocate memory for %s\n", field_name);
    exit(EXIT_FAILURE);
  }

  return ptr;
}
