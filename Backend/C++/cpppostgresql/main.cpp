#include "include/crow.h"   // Crow single-header
#include <pqxx/pqxx>        // libpqxx for PostgreSQL
#include <mutex>
#include <sstream>
#include <boost/asio.hpp>

const std::string db_conn_str = "postgresql://listuser:listpassword@localhost:5432/listdb";
std::mutex db_mutex;

int main()
{
    crow::SimpleApp app;

    // OPTIONS route for CORS (preflight requests)
    CROW_ROUTE(app, "/<path>")
        .methods("OPTIONS"_method)
    ([](const crow::request&, crow::response& res, std::string) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.code = 200;
        res.end();
    });


    // POST /lists – Create a new list item.
    CROW_ROUTE(app, "/lists").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, "Invalid JSON");
        if (!body.has("list"))
            return crow::response(400, "Missing 'list' field");

        std::string list_val = body["list"].s();
        crow::json::wvalue result;

        try {
            std::lock_guard<std::mutex> lock(db_mutex);
            pqxx::connection c(db_conn_str);
            pqxx::work txn(c);
            pqxx::result r = txn.exec_params(
                "INSERT INTO lists (list) VALUES ($1) RETURNING id, list",
                list_val
            );
            txn.commit();
            if (r.size() == 1) {
                int id = r[0]["id"].as<int>();
                std::string list_str = r[0]["list"].c_str();
                result["id"] = id;
                result["list"] = list_str;
            }
        } catch (const std::exception &e) {
            return crow::response(500, std::string("Database error: ") + e.what());
        }

        crow::response res(result);
        res.code = 201;
        return res;
    });

    // GET /lists – Retrieve all list items.
    CROW_ROUTE(app, "/lists").methods("GET"_method)
    ([]() {
        std::vector<crow::json::wvalue> items;
        try {
            std::lock_guard<std::mutex> lock(db_mutex);
            pqxx::connection c(db_conn_str);
            pqxx::nontransaction txn(c);
            pqxx::result r = txn.exec("SELECT id, list FROM lists ORDER BY id");
            for (const auto &row : r) {
                crow::json::wvalue item;
                item["id"] = row["id"].as<int>();
                item["list"] = std::string(row["list"].c_str());
                items.push_back(std::move(item));
            }
        } catch (const std::exception &e) {
            return crow::response(500, std::string("Database error: ") + e.what());
        }
        // Construct a JSON array from the vector.
        crow::json::wvalue result(std::move(items));
        crow::response res(result);
        return res;
    });
    

    // GET /lists/<id> – Retrieve a specific list item.
    CROW_ROUTE(app, "/lists/<int>").methods("GET"_method)
    ([](int id) {
        crow::json::wvalue result;
        try {
            std::lock_guard<std::mutex> lock(db_mutex);
            pqxx::connection c(db_conn_str);
            pqxx::nontransaction txn(c);
            std::stringstream ss;
            ss << "SELECT id, list FROM lists WHERE id = " << id;
            pqxx::result r = txn.exec(ss.str());
            if (r.size() == 1) {
                result["id"] = r[0]["id"].as<int>();
                result["list"] = std::string(r[0]["list"].c_str());
            } else {
                return crow::response(404, "Item not found");
            }
        } catch (const std::exception &e) {
            return crow::response(500, std::string("Database error: ") + e.what());
        }
        crow::response res(result);
        return res;
    });

    // PUT /lists/<id> – Update a specific list item.
    CROW_ROUTE(app, "/lists/<int>").methods("PUT"_method)
    ([](const crow::request& req, int id) {
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, "Invalid JSON");
        if (!body.has("list"))
            return crow::response(400, "Missing 'list' field");

        std::string new_list = body["list"].s();
        crow::json::wvalue result;
        try {
            std::lock_guard<std::mutex> lock(db_mutex);
            pqxx::connection c(db_conn_str);
            pqxx::work txn(c);
            pqxx::result r = txn.exec_params(
                "UPDATE lists SET list = $1 WHERE id = $2 RETURNING id, list",
                new_list, id
            );
            txn.commit();
            if (r.size() == 1) {
                result["id"] = r[0]["id"].as<int>();
                result["list"] = std::string(r[0]["list"].c_str());
            } else {
                return crow::response(404, "Item not found");
            }
        } catch (const std::exception &e) {
            return crow::response(500, std::string("Database error: ") + e.what());
        }
        crow::response res(result);
        return res;
    });

    // DELETE /lists/<id> – Delete a specific list item.
    CROW_ROUTE(app, "/lists/<int>").methods("DELETE"_method)
    ([](int id) {
        try {
            std::lock_guard<std::mutex> lock(db_mutex);
            pqxx::connection c(db_conn_str);
            pqxx::work txn(c);
            pqxx::result r = txn.exec_params("DELETE FROM lists WHERE id = $1", id);
            txn.commit();
            if (r.affected_rows() > 0)
                return crow::response(200, "Item deleted");
            else
                return crow::response(404, "Item not found");
        } catch (const std::exception &e) {
            return crow::response(500, std::string("Database error: ") + e.what());
        }
    });

    app.port(3000).multithreaded().run();
    return 0;
}
