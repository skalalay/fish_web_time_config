#include "nvs_db.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include "esp_log.h"

#define NVS_DB_PARTITION "storage"

static nvs_handle_t nvs_handle = 0;
static bool initialized = false;

esp_err_t nvs_db_init(void)
{
    if (initialized) return ESP_OK;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) return err;
        err = nvs_flash_init();
        if (err != ESP_OK) return err;
    }

    err = nvs_open(NVS_DB_PARTITION, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) initialized = true;
    return err;
}

esp_err_t nvs_db_deinit(void)
{
    if (!initialized) return ESP_OK;
    nvs_close(nvs_handle);
    initialized = false;
    return ESP_OK;
}

esp_err_t nvs_db_set_string(const char *key, const char *value)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    esp_err_t err = nvs_set_str(nvs_handle, key, value);
    if (err == ESP_OK) err = nvs_commit(nvs_handle);
    return err;
}

esp_err_t nvs_db_get_string(const char *key, char *buffer, size_t *len)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    return nvs_get_str(nvs_handle, key, buffer, len);
}

esp_err_t nvs_db_set_int(const char *key, int32_t value)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    esp_err_t err = nvs_set_i32(nvs_handle, key, value);
    if (err == ESP_OK) err = nvs_commit(nvs_handle);
    return err;
}

esp_err_t nvs_db_get_int(const char *key, int32_t *out_value)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    return nvs_get_i32(nvs_handle, key, out_value);
}

esp_err_t nvs_db_set_blob(const char *key, const void *data, size_t len)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    esp_err_t err = nvs_set_blob(nvs_handle, key, data, len);
    if (err == ESP_OK) err = nvs_commit(nvs_handle);
    return err;
}

esp_err_t nvs_db_get_blob(const char *key, void *out_data, size_t *len)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    return nvs_get_blob(nvs_handle, key, out_data, len);
}

esp_err_t nvs_db_delete(const char *key)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    esp_err_t err = nvs_erase_key(nvs_handle, key);
    if (err == ESP_OK) err = nvs_commit(nvs_handle);
    return err;
}

esp_err_t nvs_db_commit(void)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    return nvs_commit(nvs_handle);
}

esp_err_t nvs_db_print_all(const char *tag)
{
    if (!initialized) return ESP_ERR_NVS_NOT_INITIALIZED;
    if (tag == NULL) tag = "NVS_DB";

    nvs_iterator_t it = NULL;
    esp_err_t err = nvs_entry_find(NVS_DB_PARTITION, NULL, NVS_TYPE_ANY, &it);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(tag, "No entries found in partition '%s'", NVS_DB_PARTITION);
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(tag, "nvs_entry_find failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(tag, "=== Dump of NVS partition '%s' ===", NVS_DB_PARTITION);

    while (it != NULL) {
        nvs_entry_info_t info;
        err = nvs_entry_info(it, &info);
        if (err != ESP_OK) {
            ESP_LOGE(tag, "nvs_entry_info failed: %s", esp_err_to_name(err));
            nvs_entry_iterator_done(it);
            return err;
        }

        const char *type_str = "UNKNOWN";
        switch (info.type) {
            case NVS_TYPE_U8: type_str = "U8"; break;
            case NVS_TYPE_I8: type_str = "I8"; break;
            case NVS_TYPE_U16: type_str = "U16"; break;
            case NVS_TYPE_I16: type_str = "I16"; break;
            case NVS_TYPE_U32: type_str = "U32"; break;
            case NVS_TYPE_I32: type_str = "I32"; break;
            case NVS_TYPE_U64: type_str = "U64"; break;
            case NVS_TYPE_I64: type_str = "I64"; break;
            case NVS_TYPE_STR: type_str = "STR"; break;
            case NVS_TYPE_BLOB: type_str = "BLOB"; break;
            case NVS_TYPE_BLOB_DATA: type_str = "BLOB_DATA"; break;
            case NVS_TYPE_BLOB_LEGACY: type_str = "BLOB_LEGACY"; break;
            default: break;
        }

        if (info.type == NVS_TYPE_STR) {
            size_t len = 0;
            err = nvs_get_str(nvs_handle, info.key, NULL, &len);
            if (err == ESP_OK && len > 0) {
                char *buf = malloc(len);
                if (buf) {
                    err = nvs_get_str(nvs_handle, info.key, buf, &len);
                    if (err == ESP_OK) {
                        ESP_LOGI(tag, "  %s (STR) = %s", info.key, buf);
                    } else {
                        ESP_LOGI(tag, "  %s (STR) = <error reading>", info.key);
                    }
                    free(buf);
                } else {
                    ESP_LOGI(tag, "  %s (STR) = <out of memory>", info.key);
                }
            } else {
                ESP_LOGI(tag, "  %s (STR) = <empty or error>", info.key);
            }
        } else if (info.type == NVS_TYPE_I32) {
            int32_t val;
            err = nvs_get_i32(nvs_handle, info.key, &val);
            if (err == ESP_OK) {
                ESP_LOGI(tag, "  %s (I32) = %" PRId32, info.key, val);
            } else {
                ESP_LOGI(tag, "  %s (I32) = <error reading>", info.key);
            }
        } else if (info.type == NVS_TYPE_U32) {
            uint32_t val;
            err = nvs_get_u32(nvs_handle, info.key, &val);
            if (err == ESP_OK) {
                ESP_LOGI(tag, "  %s (U32) = %" PRIu32, info.key, val);
            } else {
                ESP_LOGI(tag, "  %s (U32) = <error reading>", info.key);
            }
        } else if (info.type == NVS_TYPE_I64) {
            int64_t val;
            err = nvs_get_i64(nvs_handle, info.key, &val);
            if (err == ESP_OK) {
                ESP_LOGI(tag, "  %s (I64) = %" PRId64, info.key, val);
            } else {
                ESP_LOGI(tag, "  %s (I64) = <error reading>", info.key);
            }
        } else if (info.type == NVS_TYPE_U64) {
            uint64_t val;
            err = nvs_get_u64(nvs_handle, info.key, &val);
            if (err == ESP_OK) {
                ESP_LOGI(tag, "  %s (U64) = %" PRIu64, info.key, val);
            } else {
                ESP_LOGI(tag, "  %s (U64) = <error reading>", info.key);
            }
        } else if (info.type == NVS_TYPE_BLOB || info.type == NVS_TYPE_BLOB_DATA || info.type == NVS_TYPE_BLOB_LEGACY) {
            size_t len = 0;
            err = nvs_get_blob(nvs_handle, info.key, NULL, &len);
            if (err == ESP_OK) {
                ESP_LOGI(tag, "  %s (BLOB) size = %zu bytes", info.key, len);
                if (len > 0 && len <= 64) {
                    uint8_t *blob = malloc(len);
                    if (blob) {
                        err = nvs_get_blob(nvs_handle, info.key, blob, &len);
                        if (err == ESP_OK) {
                            char hex[128] = {0};
                            for (size_t i = 0; i < len && i < 32; i++) {
                                sprintf(hex + i*3, "%02X ", blob[i]);
                            }
                            ESP_LOGI(tag, "  %s (BLOB) data: %s", info.key, hex);
                        }
                        free(blob);
                    }
                }
            } else {
                ESP_LOGI(tag, "  %s (BLOB) = <error reading size>", info.key);
            }
        } else {
            ESP_LOGI(tag, "  %s (type %d) = <not supported for display>", info.key, info.type);
        }

        err = nvs_entry_next(&it);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(tag, "nvs_entry_next failed: %s", esp_err_to_name(err));
            nvs_entry_iterator_done(it);
            return err;
        }
    }
    nvs_entry_iterator_done(it);
    ESP_LOGI(tag, "=== Dump complete ===");
    return ESP_OK;
}
