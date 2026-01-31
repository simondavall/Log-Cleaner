#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE
#define INIT_LINES_SIZE 1000
#define MAX_LOG_FILES 100
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

int read_file(char *str[], const char *filepath);
void dispose_read_file(char *str[], int count);
void *m_alloc(void *ptr, size_t size, const char *err_msg);
Config *get_config(char *log_file_name, char *config_file);
void delete_config(Config *config);

void clean_file(char *file_path) {
  char *lines[INIT_LINES_SIZE];

  int line_count = read_file(lines, file_path);

  for (int j = 0; j < line_count; j++) {
    // test line against match criteria
    bool match = true;
    if (match) {
      // write to removed entries file
    } else {
      // write to cleaned log file
    }

    // replace existing log file with cleaned log file

    printf("Line %2d: '%s'\n", j, lines[j]);
  }

  dispose_read_file(lines, line_count);
}

int main(int argc, char *argv[]) {

  // todo: check args and report usage
  // todo: strip log file name from path
  printf("Args count: %d\n", argc);
  char *filename = argv[1];
  char *config_file = argv[2];
  Config *config = get_config(filename, config_file);

  if (config == NULL){
    printf("Could not find config information for log file '%s'.\nCheck the '%s' file for a '%s' section.\n", filename, config_file, filename);
    exit(EXIT_FAILURE);
  }

  printf("Log_File: %s\n", config->log_file);
  printf("Identifier count: %d\n", config->identifier_count);
  for (int i = 0; i < config->identifier_count; i++) {
    printf("Identifier item count %d\n", config->identifiers[i]->length);
    for (int j = 0; j < config->identifiers[i]->length; j++) {
      printf("Identifier item: %s\n", config->identifiers[i]->items[j]);
      printf("\tIdentifier: %s\n", config->identifiers[i]->items[j]);
    }
  }

  delete_config(config);

  for (int i = 1; i < argc; ++i) {
    clean_file(filename);
  }

  printf("Hello %s\n", "World");
  return EXIT_SUCCESS;
}

Config *get_config(char *log_file_name, char *config_file) {
  FILE *fp = fopen(config_file, "r");
  if (!fp) {
    printf("Error: Unable to open the file %s. Check spelling and that it "
           "exists.\n",
           config_file);
    exit(1);
  }

  char json_string[CONFIG_FILE_SIZE];
  int len = fread(json_string, 1, sizeof(json_string), fp);
  fclose(fp);

  cJSON *root = cJSON_Parse(json_string);
  if (!root) {
    printf("Parse error: %s\n", cJSON_GetErrorPtr());
    exit(1);
  }

  cJSON *files = cJSON_GetObjectItemCaseSensitive(root, "files");
  if (!cJSON_IsObject(files)) {
    printf("Invalid 'files' object.\n");
    cJSON_Delete(root);
    exit(1);
  }

  Config *config = NULL;

  cJSON *log_file;
  int log_file_id = 0;
  cJSON_ArrayForEach(log_file, files) {
    if (strcmp(log_file->string, log_file_name) != 0)
      continue;

    config = m_alloc(config, sizeof(Config), "config item");
    config->log_file = NULL;
    config->log_file =
        m_alloc(config->log_file, sizeof(strlen(log_file->string)) + 1,
                "log file name in config");
    strcpy(config->log_file, log_file->string);
    config->identifiers = NULL;

    cJSON *array = log_file;
    int size = cJSON_GetArraySize(array);
    if (size <= 0) { // error in config
      printf("No identifier items set for %s in config", log_file_name);
      exit(1);
    }

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
      if (inner_size <= 0)
        continue;

      identifier->items = NULL;
      identifier->items = m_alloc(
          identifier->items, inner_size * sizeof(char *), "identifier items");
      identifier->length = inner_size;

      for (int j = 0; j < inner_size; j++) {
        cJSON *item = cJSON_GetArrayItem(inner_array, j);
        if (cJSON_IsString(item)) {
          char *str = NULL;
          str = m_alloc(str, sizeof(strlen(item->valuestring) + 1),
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

// This function should be called in order to free memory
// allocatied during the read_file function above.
void dispose_read_file(char *str[], int count) {
  for (int i = 0; i < count; i++) {
    free(str[i]);
  }
}

void *m_alloc(void *ptr, size_t size, const char *field_name) {
  if (ptr == NULL) {
    ptr = malloc(size);
  } else {
    ptr = realloc(ptr, size);
  }

  if (ptr == NULL) {
    printf("Unable to allocate memory for %s\n", field_name);
    exit(EXIT_FAILURE);
  }

  return ptr;
}
