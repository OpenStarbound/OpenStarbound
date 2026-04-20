#ifndef _GL4ES_CONFIG_H_
#define _GL4ES_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_NGG_DIRECTORY_PATH "/sdcard/NGG"
#define CONFIG_FILE_PATH "/config.json"
#define LOG_FILE_PATH "/latest.log"
    
void config_refresh();
int config_get_int(char* name);
char* config_get_string(char* name);
void config_cleanup();

#ifdef __cplusplus
}
#endif

#endif // _GL4ES_CONFIG_H_
