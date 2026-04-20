#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "logs.h"
#include "gl4es.h"

static cJSON *config_json = NULL;

void config_refresh() {
    char* path = malloc(strlen(NGGDirectory) + strlen(CONFIG_FILE_PATH) + 1);
    strcpy(path, NGGDirectory);
    strcat(path, CONFIG_FILE_PATH);

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        DBG(SHUT_LOGE("Unable to open config file %s", path);)
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size + 1);
    if (file_content == NULL) {
        SHUT_LOGE("Unable to allocate memory for file content");
        fclose(file);
        return;
    }

    fread(file_content, 1, file_size, file);
    fclose(file);
    file_content[file_size] = '\0';

    config_json = cJSON_Parse(file_content);
    if (config_json == NULL) {
        SHUT_LOGE("Error parsing config JSON: %s", cJSON_GetErrorPtr());
    }

    free(file_content);
    free(path);
}

int config_get_int(char* name) {
    if (config_json == NULL) {
        return -1;
    }

    cJSON *item = cJSON_GetObjectItem(config_json, name);
    if (item == NULL || !cJSON_IsNumber(item)) {
        DBG(SHUT_LOGD("Config item '%s' not found or not an integer.\n", name);)
        return -1;
    }

    return item->valueint;
}

char* config_get_string(char* name) {
    if (config_json == NULL) {
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItem(config_json, name);
    if (item == NULL || !cJSON_IsString(item)) {
        DBG(SHUT_LOGD("Config item '%s' not found or not a string.\n", name);)
        return NULL; 
    }

    return item->valuestring;
}

void config_cleanup() {
    if (config_json != NULL) {
        cJSON_Delete(config_json);
        config_json = NULL;
    }
}
