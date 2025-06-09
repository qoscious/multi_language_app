use actix_web::{web, App, HttpServer, HttpResponse, Responder};
use actix_cors::Cors;
use mongodb::{
    bson::{doc, oid::ObjectId},
    options::ClientOptions,
    Client, Collection,
};
use serde::{Deserialize, Serialize};
use futures::StreamExt;
use std::sync::Arc;

#[derive(Debug, Serialize, Deserialize)]
struct ListItem {
    // This field is stored as an ObjectId in MongoDB.
    #[serde(rename = "_id", skip_serializing_if = "Option::is_none")]
    id: Option<ObjectId>,
    list: String,
}

/// Response type that converts the ObjectId into a plain string.
#[derive(Debug, Serialize)]
struct ListItemResponse {
    #[serde(rename = "_id")]
    id: String,
    list: String,
}

impl From<ListItem> for ListItemResponse {
    fn from(item: ListItem) -> Self {
        ListItemResponse {
            id: item.id.expect("id must be present").to_hex(),
            list: item.list,
        }
    }
}

struct AppState {
    collection: Arc<Collection<ListItem>>,
}

// GET /lists – Retrieve all list items.
async fn get_all_items(data: web::Data<AppState>) -> impl Responder {
    let collection = &data.collection;
    let mut cursor = collection.find(None, None).await.unwrap();
    let mut items: Vec<ListItem> = Vec::new();
    while let Some(result) = cursor.next().await {
        match result {
            Ok(item) => items.push(item),
            Err(e) => return HttpResponse::InternalServerError().body(e.to_string()),
        }
    }
    // Convert each ListItem into our response type.
    let response_items: Vec<ListItemResponse> =
        items.into_iter().map(ListItemResponse::from).collect();
    HttpResponse::Ok().json(response_items)
}

// POST /lists – Create a new list item.
async fn add_item(
    data: web::Data<AppState>,
    item: web::Json<ListItem>,
) -> impl Responder {
    let collection = &data.collection;
    let new_item = ListItem {
        id: None,
        list: item.list.clone(),
    };
    let insert_result = collection.insert_one(new_item, None).await.unwrap();
    // Return the inserted id as a plain string.
    let inserted_id = insert_result.inserted_id
        .as_object_id()
        .expect("should be an ObjectId")
        .to_hex();
    HttpResponse::Created().json(serde_json::json!({ "_id": inserted_id }))
}

// GET /lists/{id} – Retrieve a specific list item.
async fn get_item_by_id(
    data: web::Data<AppState>,
    id: web::Path<String>,
) -> impl Responder {
    let id_str = id.into_inner();
    let collection = &data.collection;
    let object_id = match ObjectId::parse_str(&id_str) {
        Ok(oid) => oid,
        Err(e) => return HttpResponse::BadRequest().body(e.to_string()),
    };
    let filter = doc! { "_id": object_id };
    let result = collection.find_one(filter, None).await.unwrap();
    match result {
        Some(item) => {
            let response_item: ListItemResponse = item.into();
            HttpResponse::Ok().json(response_item)
        }
        None => HttpResponse::NotFound().body("Item not found"),
    }
}

// PUT /lists/{id} – Update a specific list item.
async fn update_item(
    data: web::Data<AppState>,
    id: web::Path<String>,
    item: web::Json<ListItem>,
) -> impl Responder {
    let id_str = id.into_inner();
    let collection = &data.collection;
    let object_id = match ObjectId::parse_str(&id_str) {
        Ok(oid) => oid,
        Err(e) => return HttpResponse::BadRequest().body(e.to_string()),
    };
    let filter = doc! { "_id": object_id };
    let update = doc! { "$set": { "list": &item.list } };
    let result = collection.update_one(filter, update, None).await.unwrap();
    if result.matched_count > 0 {
        HttpResponse::Ok().json("Item updated")
    } else {
        HttpResponse::NotFound().body("Item not found")
    }
}

// DELETE /lists/{id} – Delete a specific list item.
async fn delete_item(
    data: web::Data<AppState>,
    id: web::Path<String>,
) -> impl Responder {
    let id_str = id.into_inner();
    let collection = &data.collection;
    let object_id = match ObjectId::parse_str(&id_str) {
        Ok(oid) => oid,
        Err(e) => return HttpResponse::BadRequest().body(e.to_string()),
    };
    let filter = doc! { "_id": object_id };
    let result = collection.delete_one(filter, None).await.unwrap();
    if result.deleted_count > 0 {
        HttpResponse::Ok().json("Item deleted")
    } else {
        HttpResponse::NotFound().body("Item not found")
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Set up MongoDB connection.
    let client_options = ClientOptions::parse("mongodb://localhost:27017").await.unwrap();
    let client = Client::with_options(client_options).unwrap();
    let database = client.database("listdb");
    let collection = database.collection::<ListItem>("lists");

    let app_state = web::Data::new(AppState {
        collection: Arc::new(collection),
    });

    // Start the HTTP server with CORS enabled.
    HttpServer::new(move || {
        App::new()
            .wrap(
                Cors::default()
                    .allow_any_origin()
                    .allowed_methods(vec!["GET", "POST", "PUT", "DELETE", "OPTIONS"])
                    .allowed_headers(vec![
                        actix_web::http::header::CONTENT_TYPE,
                        actix_web::http::header::ACCEPT,
                    ])
                    .max_age(3600),
            )
            .app_data(app_state.clone())
            .route("/lists", web::get().to(get_all_items))
            .route("/lists", web::post().to(add_item))
            .route("/lists/{id}", web::get().to(get_item_by_id))
            .route("/lists/{id}", web::put().to(update_item))
            .route("/lists/{id}", web::delete().to(delete_item))
    })
    .bind("127.0.0.1:3000")?
    .run()
    .await
}
