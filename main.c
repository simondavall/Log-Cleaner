#define _GNU_SOURCE
#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define INIT_LINES_SIZE 1000
#define CONFIG_FILE_SIZE 4096

typedef struct {
  char **items;
  int length;
} Identifier;

typedef struct {
  char *log_file;
  Identifier **identifiers;
  int identifier_count;
} Config;

void clean_file(const char *file_path, Config *config);
char *create_timestamped_file_path(const char *filename, const char *prefix);
void delete_config(Config *config);
void dispose_read_file(char *str[], int count);
Config *get_config(const char *log_file_name, char *config_file);
const char *get_filename(const char *path);
void *m_alloc(void *ptr, size_t size, const char *err_msg);
int read_file(char *str[], const char *filepath);

int main(int argc, char *argv[]) {

  if (argc < 3) {
    // todo: parse args and provide usage.
    printf("Print out Usage for user: Need filepath amd config_file");
    return EXIT_FAILURE;
  }

  char *file_path = argv[1];
  const char *filename = get_filename(file_path);
  char *config_file = argv[2];

  Config *config = get_config(filename, config_file);
  if (config == NULL) {
    printf("Could not find config information for log file '%s'.\nCheck the "
           "'%s' file for a '%s' section.\n",
           filename, config_file, filename);
    exit(EXIT_FAILURE);
  }

  clean_file(file_path, config);
  delete_config(config);

  return EXIT_SUCCESS;
}

Config *get_config(const char *log_file_name, char *config_file) {
  FILE *fp = fopen(config_file, "r");
  if (!fp) {
    printf("Error: Unable to open the file %s. Check spelling and that it "
           "exists.\n",
           config_file);
    exit(EXIT_FAILURE);
  }

  char json_string[CONFIG_FILE_SIZE];
  int len = fread(json_string, 1, sizeof(json_string), fp);
  fclose(fp);

  if (len <= 0){
    printf("No config set in config file '%s'", config_file);
    exit(EXIT_FAILURE);
  }
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
    config->log_file =
        m_alloc(config->log_file, sizeof(strlen(log_file->string)) + 1,
                "log file name in config");
    strcpy(config->log_file, log_file->string);

    cJSON *array = log_file;
    int size = cJSON_GetArraySize(array);
    if (size <= 0) { // error in config
      printf("No identifier items set for %s in config", log_file_name);
      exit(EXIT_FAILURE);
    }

    config->identifiers = NULL;
    config->identifiers = m_alloc(
        config->identifiers, size * sizeof(Identifier *), "identifiers list");
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
      identifier->items = m_alloc(
          identifier->items, inner_size * sizeof(char *), "identifier items");

      for (int j = 0; j < inner_size; j++) {
        cJSON *item = cJSON_GetArrayItem(inner_array, j);
        if (cJSON_IsString(item)) {
          char *str = NULL;
          str = m_alloc(str, strlen(item->valuestring) + 1,
                        "item string in config");
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
  free(config->identifiers);
  free(config);
}

void clean_file(const char *file_path, Config *config) {
  char *lines[INIT_LINES_SIZE];

  int line_count = read_file(lines, file_path);

  char *cleaned_filename = create_timestamped_file_path(file_path, "cleaned");
  FILE *cleaned_filePtr;
  cleaned_filePtr = fopen(cleaned_filename, "w");
  if (cleaned_filePtr == NULL) {
    printf("Error opening %s\n", cleaned_filename);
    exit(EXIT_FAILURE);
  }

  char *removed_filename = create_timestamped_file_path(file_path, "removed");
  FILE *removed_filePtr;
  removed_filePtr = fopen(removed_filename, "w");
  if (removed_filePtr == NULL) {
    printf("Error opening %s\n", removed_filename);
    exit(EXIT_FAILURE);
  }

  for (int j = 0; j < line_count; j++) {
    char *log_entry = lines[j];
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
      fprintf(removed_filePtr, "%s\n", lines[j]);
    } else {
      fprintf(cleaned_filePtr, "%s\n", lines[j]);
    }
  }

  fclose(removed_filePtr);
  fclose(cleaned_filePtr);

  printf("Replacing '%s' with '%s'", file_path, cleaned_filename);
  if (rename(cleaned_filename, file_path) != 0) {
    printf("Unable to replace '%s' with the cleaned log file '%s'.\nFile is "
           "likely locked by another process.\n",
           file_path, cleaned_filename);
  }
  // todo: decide what to do with the removed items file (options: retain,
  // delete, decide by config or command-line arg)

  free(cleaned_filename);
  free(removed_filename);
  dispose_read_file(lines, line_count);
}

// This function reads lines from a specified file into
// the array provided. After use the dispose_read_file function
// to free the memory allocated.
int read_file(char *str[], const char *filepath) {
  FILE *fptr;
  int line_count = 0;
  void *str_ptr;
  char *line = NULL;
  size_t len = 0;
  int str_len;

  fptr = fopen(filepath, "rb");
  if (fptr == NULL) {
    printf("Error opening file: %s", filepath);
    exit(EXIT_FAILURE);
  }

  while ((str_len = (int)getline(&line, &len, fptr)) != -1) {
    line[str_len - 1] = '\0';
    if (strcmp(line, "\0") == 0) // ignore empty strings
      continue;
    str_ptr = malloc(str_len);
    if (str_ptr == NULL) {
      printf("Error allocating memory during read line.\n");
      exit(EXIT_FAILURE);
    }
    str[line_count] = str_ptr;
    strncpy(str[line_count++], line, str_len);
  }

  fclose(fptr);
  if (line)
    free(line);
  return line_count;
}

const char *get_filename(const char *path) {
  const char *last_slash = strrchr(path, '/');
  return last_slash ? last_slash + 1 : path;
}

// This function should be called in order to free memory
// allocatied during the read_file function above.
void dispose_read_file(char *str[], int count) {
  for (int i = 0; i < count; i++) {
    free(str[i]);
  }
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
  size_t new_len = dir_len + strlen(prefix) + 1 + strlen(base) + 1 +
                   strlen(timestamp) + 5; // +5 for "_", ".log", \0
  char *new_file_path = NULL;
  new_file_path = m_alloc(new_file_path, new_len, "new file path");

  snprintf(new_file_path, new_len, "%.*s%s_%s_%s.log", (int)dir_len, file_path,
           prefix, base, timestamp);

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
