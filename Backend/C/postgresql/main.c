#include <microhttpd.h>
#include <libpq-fe.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 3000

// Global PostgreSQL connection pointer.
PGconn *pg_conn = NULL;

/* -------------------- PostgreSQL Controller Functions ---------------------------- */

// Insert a new item into PostgreSQL.
void pg_add_item(const char *list_text) {
    const char *paramValues[1] = { list_text };
    PGresult *res = PQexecParams(pg_conn,
                                 "INSERT INTO lists (list) VALUES ($1)",
                                 1,
                                 NULL,   // Let PostgreSQL deduce parameter types.
                                 paramValues,
                                 NULL,
                                 NULL,
                                 0);     // Text results.
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert failed: %s\n", PQerrorMessage(pg_conn));
    }
    PQclear(res);
}

// Update an existing item identified by id.
int pg_update_item(const char *id_str, const char *new_text) {
    char *endptr;
    long id = strtol(id_str, &endptr, 10);
    if (*endptr != '\0') {
         fprintf(stderr, "Invalid id: %s\n", id_str);
         return -1;
    }
    char idBuffer[32];
    snprintf(idBuffer, sizeof(idBuffer), "%ld", id);
    const char *paramValues[2] = { new_text, idBuffer };
    PGresult *res = PQexecParams(pg_conn,
                                 "UPDATE lists SET list = $1 WHERE id = $2",
                                 2,
                                 NULL,
                                 paramValues,
                                 NULL,
                                 NULL,
                                 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
         fprintf(stderr, "Update failed: %s\n", PQerrorMessage(pg_conn));
         PQclear(res);
         return -1;
    }
    PQclear(res);
    return 0;
}

// Delete an item identified by id.
int pg_delete_item(const char *id_str) {
    char *endptr;
    long id = strtol(id_str, &endptr, 10);
    if (*endptr != '\0') {
         fprintf(stderr, "Invalid id: %s\n", id_str);
         return -1;
    }
    char idBuffer[32];
    snprintf(idBuffer, sizeof(idBuffer), "%ld", id);
    const char *paramValues[1] = { idBuffer };
    PGresult *res = PQexecParams(pg_conn,
                                 "DELETE FROM lists WHERE id = $1",
                                 1,
                                 NULL,
                                 paramValues,
                                 NULL,
                                 NULL,
                                 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
         fprintf(stderr, "Delete failed: %s\n", PQerrorMessage(pg_conn));
         PQclear(res);
         return -1;
    }
    PQclear(res);
    return 0;
}

// Retrieve all items from PostgreSQL and return a JSON array string.
char* pg_get_items_json() {
    PGresult *res = PQexec(pg_conn, "SELECT id, list FROM lists ORDER BY id");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
         fprintf(stderr, "Select failed: %s\n", PQerrorMessage(pg_conn));
         PQclear(res);
         return strdup("[]");
    }
    int rows = PQntuples(res);
    json_t *json_arr = json_array();  // Renamed variable to avoid conflict
    for (int i = 0; i < rows; i++) {
         const char *id_str = PQgetvalue(res, i, 0);
         const char *list_val = PQgetvalue(res, i, 1);
         json_t *json_obj = json_object();
         json_object_set_new(json_obj, "id", json_string(id_str));
         json_object_set_new(json_obj, "list", json_string(list_val));
         json_array_append_new(json_arr, json_obj);
    }
    char *ret = json_dumps(json_arr, 0);
    json_decref(json_arr);
    PQclear(res);
    return ret;
}


/* -------------------- HTTP Server & Request Handling -------------------- */

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

/* Helper: Extract the dynamic id from URL.
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

    // Accumulate incoming POST/PUT data.
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
        char *json = pg_get_items_json();
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
                    pg_add_item(list_text);
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
            const char *paramValues[1];
            char idBuffer[32];
            snprintf(idBuffer, sizeof(idBuffer), "%s", id_str);
            paramValues[0] = idBuffer;
            PGresult *res = PQexecParams(pg_conn,
                "SELECT id, list FROM lists WHERE id = $1",
                1,
                NULL,
                paramValues,
                NULL,
                NULL,
                0);
            if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
                ret = send_response(connection, "{\"error\":\"Resource not found\"}", MHD_HTTP_NOT_FOUND);
            } else {
                json_t *json_obj = json_object();
                json_object_set_new(json_obj, "id", json_string(PQgetvalue(res, 0, 0)));
                json_object_set_new(json_obj, "list", json_string(PQgetvalue(res, 0, 1)));
                char *json_str = json_dumps(json_obj, 0);
                ret = send_response(connection, json_str, MHD_HTTP_OK);
                free(json_str);
                json_decref(json_obj);
            }
            PQclear(res);
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
                    if (pg_update_item(id_str, new_list) == 0)
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
            if (pg_delete_item(id_str) == 0)
                ret = send_response(connection, "{\"status\":\"item deleted\"}", MHD_HTTP_OK);
            else
                ret = send_response(connection, "{\"error\":\"Delete failed\"}", MHD_HTTP_INTERNAL_SERVER_ERROR);
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
    // Connect to PostgreSQL using the provided connection string.
    const char *conninfo = "postgresql://listuser:listpassword@localhost:5432/listdb";
    pg_conn = PQconnectdb(conninfo);
    if (PQstatus(pg_conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to PostgreSQL failed: %s\n", PQerrorMessage(pg_conn));
        PQfinish(pg_conn);
        return 1;
    }

    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                                                 &answer_to_connection, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start HTTP server.\n");
        PQfinish(pg_conn);
        return 1;
    }

    printf("Server running on port %d. Press Enter to stop.\n", PORT);
    getchar();

    MHD_stop_daemon(daemon);
    PQfinish(pg_conn);

    return 0;
}
