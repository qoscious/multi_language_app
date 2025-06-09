#include <microhttpd.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 3000

// Global MongoDB client and collection pointers.
mongoc_client_t *mongo_client = NULL;
mongoc_collection_t *mongo_collection = NULL;

/* ------------------------ Custom BSON to JSON Conversion ---------------------------- */
// Converts a BSON document into a JSON string with the format:
// { "_id": "<oid_as_string>", "list": "<value of list field>" }
char *mongo_doc_to_custom_json(const bson_t *doc) {
    json_t *json_obj = json_object();
    bson_iter_t iter;
    if (bson_iter_init(&iter, doc)) {
        while (bson_iter_next(&iter)) {
            const char *key = bson_iter_key(&iter);
            if (strcmp(key, "_id") == 0) {
                if (BSON_ITER_HOLDS_OID(&iter)) {
                    const bson_oid_t *oid = bson_iter_oid(&iter);
                    char oid_str[25] = {0};
                    bson_oid_to_string(oid, oid_str);
                    json_object_set_new(json_obj, "_id", json_string(oid_str));
                }
            } else if (strcmp(key, "list") == 0) {
                if (BSON_ITER_HOLDS_UTF8(&iter)) {
                    uint32_t len;
                    const char *value = bson_iter_utf8(&iter, &len);
                    json_object_set_new(json_obj, "list", json_string(value));
                }
            }
        }
    }
    char *ret = json_dumps(json_obj, 0);
    json_decref(json_obj);
    return ret;
}

/* -------------------- MongoDB Controller Functions ---------------------------- */

// Insert a new document into MongoDB with field "list".
void mongo_add_item(const char *list_text) {
    bson_t *doc = BCON_NEW("list", BCON_UTF8(list_text));
    bson_error_t error;
    if (!mongoc_collection_insert_one(mongo_collection, doc, NULL, NULL, &error)) {
         fprintf(stderr, "Insert failed: %s\n", error.message);
    }
    bson_destroy(doc);
}

// Update the document identified by ObjectId string by setting its "list" field.
int mongo_update_item(const char *oid_str, const char *new_text) {
    bson_oid_t oid;
    if (!bson_oid_is_valid(oid_str, strlen(oid_str))) {
         fprintf(stderr, "Invalid ObjectId string: %s\n", oid_str);
         return -1;
    }
    bson_oid_init_from_string(&oid, oid_str);
    bson_t *query = BCON_NEW("_id", BCON_OID(&oid));
    bson_t *update = BCON_NEW("$set", "{", "list", BCON_UTF8(new_text), "}");
    bson_error_t error;
    bool success = mongoc_collection_update_one(mongo_collection, query, update, NULL, NULL, &error);
    bson_destroy(query);
    bson_destroy(update);
    if (!success) {
         fprintf(stderr, "Update failed: %s\n", error.message);
         return -1;
    }
    return 0;
}

// Delete the document identified by ObjectId string.
int mongo_delete_item(const char *oid_str) {
    bson_oid_t oid;
    if (!bson_oid_is_valid(oid_str, strlen(oid_str))) {
         fprintf(stderr, "Invalid ObjectId string: %s\n", oid_str);
         return -1;
    }
    bson_oid_init_from_string(&oid, oid_str);
    bson_t *query = BCON_NEW("_id", BCON_OID(&oid));
    bson_error_t error;
    bson_t reply;
    bool success = mongoc_collection_delete_one(mongo_collection, query, NULL, &reply, &error);
    bson_destroy(query);
    bson_destroy(&reply);
    if (!success) {
         fprintf(stderr, "Delete failed: %s\n", error.message);
         return -1;
    }
    return 0;
}

// Retrieve all documents from MongoDB and return a JSON array string.
char* mongo_get_items_json() {
    bson_t *query = bson_new();
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(mongo_collection, query, NULL, NULL);
    
    char *buffer = malloc(4096);
    if (!buffer) {
        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        return NULL;
    }
    buffer[0] = '\0';
    strcat(buffer, "[");
    
    const bson_t *doc;
    int first = 1;
    while (mongoc_cursor_next(cursor, &doc)) {
         char *json_str = mongo_doc_to_custom_json(doc);
         if (!first) {
             strcat(buffer, ",");
         }
         strcat(buffer, json_str);
         free(json_str);
         first = 0;
    }
    strcat(buffer, "]");
    
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    return buffer;
}

/* -------------------- HTTP Server & Router Implementation -------------------- */
// Structure to accumulate incoming request data.
struct connection_info_struct {
    char *data;
    size_t data_size;
};

static int send_response(struct MHD_Connection *connection, const char *response_text, int status_code) {
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                                    (void*)response_text, MHD_RESPMEM_MUST_COPY);
    if (!response)
        return MHD_NO;
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

/* Helper: Manually extract the dynamic id from the URL.
   Expects URL format "/lists/<id>" and returns the substring after "/lists/".
*/
const char *get_dynamic_url_id(const char *url) {
    const char *prefix = "/lists/";
    size_t prefix_len = strlen(prefix);
    if (strncmp(url, prefix, prefix_len) != 0)
        return NULL;
    return url + prefix_len;
}

static enum MHD_Result answer_to_connection(void *cls, struct MHD_Connection *connection,
                                            const char *url, const char *method,
                                            const char *version, const char *upload_data,
                                            unsigned long *upload_data_size, void **con_cls) {

    (void)cls;
    (void)version;

    // Handle OPTIONS preflight requests.
    if (strcmp(method, "OPTIONS") == 0) {
        return send_response(connection, "", MHD_HTTP_OK);
    }

    if (*con_cls == NULL) {
        struct connection_info_struct *con_info = malloc(sizeof(struct connection_info_struct));
        con_info->data = NULL;
        con_info->data_size = 0;
        *con_cls = con_info;
        return MHD_YES;
    }
    struct connection_info_struct *con_info = *con_cls;

    if ((strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) && *upload_data_size != 0) {
        con_info->data = realloc(con_info->data, con_info->data_size + *upload_data_size + 1);
        memcpy(con_info->data + con_info->data_size, upload_data, *upload_data_size);
        con_info->data_size += *upload_data_size;
        con_info->data[con_info->data_size] = '\0';
        *upload_data_size = 0;
        return MHD_YES;
    }

    int ret;
    // Handle GET /lists (retrieve all items)
    if (strcmp(method, "GET") == 0 && strcmp(url, "/lists") == 0) {
        char *json = mongo_get_items_json();
        ret = send_response(connection, json, MHD_HTTP_OK);
        free(json);
    }
    // Handle POST /lists (create a new item)
    else if (strcmp(method, "POST") == 0 && strcmp(url, "/lists") == 0) {
        if (con_info->data && con_info->data_size > 0) {
            json_error_t json_error;
            json_t *json_in = json_loads(con_info->data, 0, &json_error);
            if (!json_in) {
                ret = send_response(connection, "{\"error\":\"Invalid JSON\"}", MHD_HTTP_BAD_REQUEST);
            } else {
                const char *list_text = json_string_value(json_object_get(json_in, "list"));
                if (!list_text) {
                    ret = send_response(connection, "{\"error\":\"Missing list field\"}", MHD_HTTP_BAD_REQUEST);
                } else {
                    mongo_add_item(list_text);
                    ret = send_response(connection, "{\"status\":\"item added\"}", MHD_HTTP_CREATED);
                }
                json_decref(json_in);
            }
        } else {
            ret = send_response(connection, "{\"error\":\"no data provided\"}", MHD_HTTP_BAD_REQUEST);
        }
    }
    // Handle GET /lists/<id> (retrieve one item)
    else if (strcmp(method, "GET") == 0 && strncmp(url, "/lists/", 7) == 0) {
        const char *id_str = get_dynamic_url_id(url);
        if (!id_str) {
            ret = send_response(connection, "{\"error\":\"Missing id parameter\"}", MHD_HTTP_BAD_REQUEST);
        } else {
            char query[256];
            snprintf(query, sizeof(query), "{ \"_id\": { \"$oid\": \"%s\" } }", id_str);
            bson_t *query_bson = bson_new_from_json((const uint8_t*)query, -1, NULL);
            mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(mongo_collection, query_bson, NULL, NULL);
            bson_destroy(query_bson);
            const bson_t *doc;
            if (mongoc_cursor_next(cursor, &doc)) {
                char *json_str = mongo_doc_to_custom_json(doc);
                ret = send_response(connection, json_str, MHD_HTTP_OK);
                free(json_str);
            } else {
                ret = send_response(connection, "{\"error\":\"Resource not found\"}", MHD_HTTP_NOT_FOUND);
            }
            mongoc_cursor_destroy(cursor);
        }
    }
    // Handle PUT /lists/<id> (update an item)
    else if (strcmp(method, "PUT") == 0 && strncmp(url, "/lists/", 7) == 0) {
        const char *id_str = get_dynamic_url_id(url);
        if (!id_str) {
            ret = send_response(connection, "{\"error\":\"Missing id parameter\"}", MHD_HTTP_BAD_REQUEST);
        } else {
            json_error_t json_error;
            json_t *json_in = json_loads(con_info->data, 0, &json_error);
            if (!json_in) {
                ret = send_response(connection, "{\"error\":\"Invalid JSON\"}", MHD_HTTP_BAD_REQUEST);
            } else {
                const char *new_list = json_string_value(json_object_get(json_in, "list"));
                if (!new_list) {
                    ret = send_response(connection, "{\"error\":\"Missing list field\"}", MHD_HTTP_BAD_REQUEST);
                } else {
                    if (mongo_update_item(id_str, new_list) == 0)
                        ret = send_response(connection, "{\"status\":\"item updated\"}", MHD_HTTP_OK);
                    else
                        ret = send_response(connection, "{\"error\":\"Update failed\"}", MHD_HTTP_INTERNAL_SERVER_ERROR);
                }
                json_decref(json_in);
            }
        }
    }
    // Handle DELETE /lists/<id> (delete an item)
    else if (strcmp(method, "DELETE") == 0 && strncmp(url, "/lists/", 7) == 0) {
        const char *id_str = get_dynamic_url_id(url);
        if (!id_str) {
            ret = send_response(connection, "{\"error\":\"Missing id parameter\"}", MHD_HTTP_BAD_REQUEST);
        } else {
            char query[256];
            snprintf(query, sizeof(query), "{ \"_id\": { \"$oid\": \"%s\" } }", id_str);
            bson_t *query_bson = bson_new_from_json((const uint8_t*)query, -1, NULL);
            bson_error_t error;
            bson_t reply;
            bool success = mongoc_collection_delete_one(mongo_collection, query_bson, NULL, &reply, &error);
            bson_destroy(query_bson);
            bson_destroy(&reply);
            if (!success)
                ret = send_response(connection, "{\"error\":\"Delete failed\"}", MHD_HTTP_INTERNAL_SERVER_ERROR);
            else
                ret = send_response(connection, "{\"status\":\"item deleted\"}", MHD_HTTP_OK);
        }
    }
    else {
        ret = send_response(connection, "{\"error\":\"unknown endpoint\"}", MHD_HTTP_NOT_FOUND);
    }

    if (con_info) {
        if (con_info->data)
            free(con_info->data);
        free(con_info);
        *con_cls = NULL;
    }
    return ret;
}

/* ------------------------------ Main ------------------------------- */
int main() {
    mongoc_init();
    mongo_client = mongoc_client_new("mongodb://localhost:27017");
    if (!mongo_client) {
        fprintf(stderr, "Failed to connect to MongoDB.\n");
        return 1;
    }
    mongo_collection = mongoc_client_get_collection(mongo_client, "mydb", "items");

    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                                                 &answer_to_connection, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start HTTP server.\n");
        return 1;
    }

    printf("Server running on port %d. Press Enter to stop.\n", PORT);
    getchar();

    MHD_stop_daemon(daemon);

    mongoc_collection_destroy(mongo_collection);
    mongoc_client_destroy(mongo_client);
    mongoc_cleanup();

    return 0;
}
