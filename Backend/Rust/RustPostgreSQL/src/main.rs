use actix_web::{web, App, HttpServer, HttpResponse, Responder};
use actix_cors::Cors;
use serde::{Deserialize, Serialize};
use sqlx::PgPool;
use std::env;

#[derive(Debug, Serialize, Deserialize)]
struct NewListItem {
    list: String,
}

#[derive(Debug, Serialize, sqlx::FromRow)]
struct ListItem {
    id: i32,
    list: String,
}

struct AppState {
    pool: PgPool,
}

// POST /lists – Create a new list item.
async fn create_list_item(
    data: web::Data<AppState>,
    new_item: web::Json<NewListItem>,
) -> impl Responder {
    let pool = &data.pool;
    let rec = sqlx::query_as::<_, ListItem>(
        "INSERT INTO lists (list) VALUES ($1) RETURNING id, list"
    )
    .bind(&new_item.list)
    .fetch_one(pool)
    .await;

    match rec {
        Ok(item) => HttpResponse::Created().json(item),
        Err(e) => {
            eprintln!("Error inserting item: {}", e);
            HttpResponse::InternalServerError().body("Error inserting item")
        }
    }
}

// GET /lists – Retrieve all list items.
async fn get_all_list_items(data: web::Data<AppState>) -> impl Responder {
    let pool = &data.pool;
    let items = sqlx::query_as::<_, ListItem>("SELECT id, list FROM lists ORDER BY id")
        .fetch_all(pool)
        .await;
    match items {
        Ok(list_items) => HttpResponse::Ok().json(list_items),
        Err(e) => {
            eprintln!("Error fetching items: {}", e);
            HttpResponse::InternalServerError().body("Error fetching items")
        }
    }
}

// GET /lists/{id} – Retrieve a specific list item.
async fn get_list_item_by_id(
    data: web::Data<AppState>,
    id: web::Path<i32>,
) -> impl Responder {
    let pool = &data.pool;
    let item = sqlx::query_as::<_, ListItem>("SELECT id, list FROM lists WHERE id = $1")
        .bind(*id)
        .fetch_optional(pool)
        .await;
    match item {
        Ok(Some(list_item)) => HttpResponse::Ok().json(list_item),
        Ok(None) => HttpResponse::NotFound().body("Item not found"),
        Err(e) => {
            eprintln!("Error fetching item: {}", e);
            HttpResponse::InternalServerError().body("Error fetching item")
        }
    }
}

// PUT /lists/{id} – Update a specific list item.
async fn update_list_item(
    data: web::Data<AppState>,
    id: web::Path<i32>,
    new_item: web::Json<NewListItem>,
) -> impl Responder {
    let pool = &data.pool;
    let updated = sqlx::query_as::<_, ListItem>(
        "UPDATE lists SET list = $1 WHERE id = $2 RETURNING id, list"
    )
    .bind(&new_item.list)
    .bind(*id)
    .fetch_optional(pool)
    .await;
    
    match updated {
        Ok(Some(item)) => HttpResponse::Ok().json(item),
        Ok(None) => HttpResponse::NotFound().body("Item not found"),
        Err(e) => {
            eprintln!("Error updating item: {}", e);
            HttpResponse::InternalServerError().body("Error updating item")
        }
    }
}

// DELETE /lists/{id} – Delete a specific list item.
async fn delete_list_item(
    data: web::Data<AppState>,
    id: web::Path<i32>,
) -> impl Responder {
    let pool = &data.pool;
    let result = sqlx::query("DELETE FROM lists WHERE id = $1")
        .bind(*id)
        .execute(pool)
        .await;
    match result {
        Ok(res) => {
            if res.rows_affected() > 0 {
                HttpResponse::Ok().json("Item deleted")
            } else {
                HttpResponse::NotFound().body("Item not found")
            }
        }
        Err(e) => {
            eprintln!("Error deleting item: {}", e);
            HttpResponse::InternalServerError().body("Error deleting item")
        }
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Connection string per the specification.
    let database_url = "postgresql://listuser:listpassword@localhost:5432/listdb";
    let pool = PgPool::connect(database_url)
        .await
        .expect("Failed to connect to PostgreSQL");

    let app_state = web::Data::new(AppState { pool });

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
                    .max_age(3600)
            )
            .app_data(app_state.clone())
            .route("/lists", web::post().to(create_list_item))
            .route("/lists", web::get().to(get_all_list_items))
            .route("/lists/{id}", web::get().to(get_list_item_by_id))
            .route("/lists/{id}", web::put().to(update_list_item))
            .route("/lists/{id}", web::delete().to(delete_list_item))
    })
    .bind("127.0.0.1:3000")?
    .run()
    .await
}
