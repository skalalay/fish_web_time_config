#ifndef NVS_DB_H
#define NVS_DB_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t nvs_db_init(void);
esp_err_t nvs_db_deinit(void);

esp_err_t nvs_db_set_string(const char *key, const char *value);
esp_err_t nvs_db_get_string(const char *key, char *buffer, size_t *len);

esp_err_t nvs_db_set_int(const char *key, int32_t value);
esp_err_t nvs_db_get_int(const char *key, int32_t *out_value);

esp_err_t nvs_db_set_blob(const char *key, const void *data, size_t len);
esp_err_t nvs_db_get_blob(const char *key, void *out_data, size_t *len);

esp_err_t nvs_db_delete(const char *key);
esp_err_t nvs_db_commit(void);

esp_err_t nvs_db_print_all(const char *tag);

#ifdef __cplusplus
}
#endif

#endif // NVS_DB_H
