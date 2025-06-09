<?php
use Psr\Http\Message\ResponseInterface as Response;
use Psr\Http\Message\ServerRequestInterface as Request;
use Psr\Http\Server\RequestHandlerInterface;
use Slim\Factory\AppFactory;

require __DIR__ . '/vendor/autoload.php';

$app = AppFactory::create();

// CORS Middleware: Adds CORS headers to every response.
$app->add(function (Request $request, RequestHandlerInterface $handler): Response {
    // Handle OPTIONS requests directly for preflight
    if ($request->getMethod() === 'OPTIONS') {
        $response = new Response();
    } else {
        $response = $handler->handle($request);
    }
    // Add CORS headers to the response
    return $response
        ->withHeader('Access-Control-Allow-Origin', '*')
        ->withHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        ->withHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
});

// Create a PDO instance for PostgreSQL connection.
$dsn = 'pgsql:host=localhost;port=5432;dbname=listdb;';
$user = 'listuser';
$pass = 'listpassword';
try {
    $pdo = new PDO($dsn, $user, $pass, [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION
    ]);
} catch (PDOException $e) {
    die('Database connection failed: ' . $e->getMessage());
}

// POST /lists – Create a new list item.
$app->post('/lists', function (Request $request, Response $response) use ($pdo) {
    $data = json_decode($request->getBody()->getContents(), true);
    if (!isset($data['list'])) {
        $error = ['error' => "Missing 'list' field"];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(400)->withHeader('Content-Type', 'application/json');
    }
    
    $stmt = $pdo->prepare("INSERT INTO lists (list) VALUES (:list) RETURNING id, list");
    $stmt->bindParam(':list', $data['list']);
    $stmt->execute();
    $result = $stmt->fetch(PDO::FETCH_ASSOC);
    
    $response->getBody()->write(json_encode($result));
    return $response->withStatus(201)->withHeader('Content-Type', 'application/json');
});

// GET /lists – Retrieve all list items.
$app->get('/lists', function (Request $request, Response $response) use ($pdo) {
    $stmt = $pdo->query("SELECT id, list FROM lists ORDER BY id");
    $lists = $stmt->fetchAll(PDO::FETCH_ASSOC);
    
    $response->getBody()->write(json_encode($lists));
    return $response->withHeader('Content-Type', 'application/json');
});

// GET /lists/{id} – Retrieve a specific list item.
$app->get('/lists/{id}', function (Request $request, Response $response, array $args) use ($pdo) {
    $stmt = $pdo->prepare("SELECT id, list FROM lists WHERE id = :id");
    $stmt->bindParam(':id', $args['id']);
    $stmt->execute();
    $list = $stmt->fetch(PDO::FETCH_ASSOC);
    
    if (!$list) {
        $error = ['error' => 'Item not found'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(404)->withHeader('Content-Type', 'application/json');
    }
    
    $response->getBody()->write(json_encode($list));
    return $response->withHeader('Content-Type', 'application/json');
});

// PUT /lists/{id} – Update a specific list item.
$app->put('/lists/{id}', function (Request $request, Response $response, array $args) use ($pdo) {
    $data = json_decode($request->getBody()->getContents(), true);
    if (!isset($data['list'])) {
        $error = ['error' => "Missing 'list' field"];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(400)->withHeader('Content-Type', 'application/json');
    }
    
    $stmt = $pdo->prepare("UPDATE lists SET list = :list WHERE id = :id RETURNING id, list");
    $stmt->bindParam(':list', $data['list']);
    $stmt->bindParam(':id', $args['id']);
    $stmt->execute();
    $updated = $stmt->fetch(PDO::FETCH_ASSOC);
    
    if (!$updated) {
        $error = ['error' => 'Item not found'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(404)->withHeader('Content-Type', 'application/json');
    }
    
    $response->getBody()->write(json_encode($updated));
    return $response->withHeader('Content-Type', 'application/json');
});

// DELETE /lists/{id} – Delete a specific list item.
$app->delete('/lists/{id}', function (Request $request, Response $response, array $args) use ($pdo) {
    $stmt = $pdo->prepare("DELETE FROM lists WHERE id = :id");
    $stmt->bindParam(':id', $args['id']);
    $stmt->execute();
    
    if ($stmt->rowCount() === 0) {
        $error = ['error' => 'Item not found'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(404)->withHeader('Content-Type', 'application/json');
    }
    
    $success = ['message' => 'Item deleted'];
    $response->getBody()->write(json_encode($success));
    return $response->withHeader('Content-Type', 'application/json');
});

$app->run();
