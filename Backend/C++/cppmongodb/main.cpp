#include "include/crow.h"   // Crow single-header (download from https://github.com/ipkn/crow)
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mutex>
#include <sstream>
#include <vector>
#include <boost/asio.hpp>

// For convenience in building BSON documents.
using namespace bsoncxx::builder::stream;

std::mutex mongo_mutex;

// Initialize the MongoDB C++ driver instance and client.
// The instance must be created before using any MongoDB operations.
mongocxx::instance instance{};
mongocxx::client mongo_client{mongocxx::uri{"mongodb://localhost:27017"}};
// Obtain the "lists" collection from the "listdb" database.
auto list_collection = mongo_client["listdb"]["lists"];

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
            std::lock_guard<std::mutex> lock(mongo_mutex);
            // Build BSON document with the new list item.
            auto doc = document{} << "list" << list_val << finalize;
            auto insert_result = list_collection.insert_one(doc.view());
            if (insert_result) {
                // Retrieve the inserted _id as a string.
                auto id = insert_result->inserted_id().get_oid().value.to_string();
                result["_id"] = id;
                result["list"] = list_val;
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
            std::lock_guard<std::mutex> lock(mongo_mutex);
            auto cursor = list_collection.find({});
            for (auto&& doc : cursor) {
                crow::json::wvalue item;
                // Convert ObjectId to string.
                item["_id"] = doc["_id"].get_oid().value.to_string();
                if (doc["list"] && doc["list"].type() == bsoncxx::type::k_utf8) {
                    item["list"] = std::string(doc["list"].get_utf8().value.to_string());
                } else {
                    item["list"] = "";
                }
                items.push_back(std::move(item));
            }
        } catch (const std::exception &e) {
            return crow::response(500, std::string("Database error: ") + e.what());
        }
        // Build a JSON array from the vector.
        crow::json::wvalue result(std::move(items));
        crow::response res(result);
        return res;
    });

    // GET /lists/<id> – Retrieve a specific list item.
    CROW_ROUTE(app, "/lists/<string>").methods("GET"_method)
    ([](std::string id_str) {
        crow::json::wvalue result;
        try {
            std::lock_guard<std::mutex> lock(mongo_mutex);
            // Convert the string to a bsoncxx::oid.
            bsoncxx::oid id(id_str);
            auto filter = document{} << "_id" << id << finalize;
            auto maybe_doc = list_collection.find_one(filter.view());
            if (maybe_doc) {
                auto doc = maybe_doc->view();
                result["_id"] = doc["_id"].get_oid().value.to_string();
                if (doc["list"] && doc["list"].type() == bsoncxx::type::k_utf8) {
                    result["list"] = std::string(doc["list"].get_utf8().value.to_string());
                } else {
                    result["list"] = "";
                }
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
    CROW_ROUTE(app, "/lists/<string>").methods("PUT"_method)
    ([](const crow::request& req, std::string id_str) {
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, "Invalid JSON");
        if (!body.has("list"))
            return crow::response(400, "Missing 'list' field");

        std::string new_list = body["list"].s();
        crow::json::wvalue result;
        try {
            std::lock_guard<std::mutex> lock(mongo_mutex);
            bsoncxx::oid id(id_str);
            auto filter = document{} << "_id" << id << finalize;
            auto update = document{} << "$set" << open_document << "list" << new_list << close_document << finalize;
            auto update_result = list_collection.update_one(filter.view(), update.view());
            if (update_result && update_result->modified_count() == 1) {
                // Retrieve and return the updated document.
                auto maybe_doc = list_collection.find_one(filter.view());
                if (maybe_doc) {
                    auto doc = maybe_doc->view();
                    result["_id"] = doc["_id"].get_oid().value.to_string();
                    if (doc["list"] && doc["list"].type() == bsoncxx::type::k_utf8) {
                        result["list"] = std::string(doc["list"].get_utf8().value.to_string());
                    } else {
                        result["list"] = "";
                    }
                }
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
    CROW_ROUTE(app, "/lists/<string>").methods("DELETE"_method)
    ([](std::string id_str) {
        try {
            std::lock_guard<std::mutex> lock(mongo_mutex);
            bsoncxx::oid id(id_str);
            auto filter = document{} << "_id" << id << finalize;
            auto del_result = list_collection.delete_one(filter.view());
            if (del_result && del_result->deleted_count() == 1)
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
