<?php
use Psr\Http\Message\ResponseInterface as Response;
use Psr\Http\Message\ServerRequestInterface as Request;
use Psr\Http\Server\RequestHandlerInterface;
use Slim\Factory\AppFactory;
use MongoDB\Client;
use MongoDB\BSON\ObjectId;

require __DIR__ . '/vendor/autoload.php';

$app = AppFactory::create();

// CORS Middleware: adds CORS headers to every response.
$app->add(function (Request $request, RequestHandlerInterface $handler): Response {
    // For preflight OPTIONS requests, create a new empty response.
    if ($request->getMethod() === 'OPTIONS') {
        $response = new \Slim\Psr7\Response();
    } else {
        $response = $handler->handle($request);
    }
    return $response
        ->withHeader('Access-Control-Allow-Origin', '*')
        ->withHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        ->withHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
});

// Create a MongoDB client and get the 'lists' collection from 'listdb'
$client = new Client("mongodb://localhost:27017");
$collection = $client->listdb->lists;

// POST /lists – Create a new list item.
$app->post('/lists', function (Request $request, Response $response) use ($collection) {
    $data = json_decode($request->getBody()->getContents(), true);
    if (!isset($data['list'])) {
        $error = ['error' => "Missing 'list' field"];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(400)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    try {
        $insertResult = $collection->insertOne(['list' => $data['list']]);
        $insertedId = (string)$insertResult->getInsertedId();
        $result = ['_id' => $insertedId, 'list' => $data['list']];
    } catch (Exception $e) {
        $error = ['error' => 'Database error: ' . $e->getMessage()];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(500)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    $response->getBody()->write(json_encode($result));
    return $response->withStatus(201)
                    ->withHeader('Content-Type', 'application/json');
});

// GET /lists – Retrieve all list items.
$app->get('/lists', function (Request $request, Response $response) use ($collection) {
    $cursor = $collection->find([]);
    $lists = [];
    foreach ($cursor as $doc) {
        $lists[] = [
            '_id'  => (string)$doc['_id'],
            'list' => $doc['list']
        ];
    }
    $response->getBody()->write(json_encode($lists));
    return $response->withHeader('Content-Type', 'application/json');
});

// GET /lists/{id} – Retrieve a specific list item.
$app->get('/lists/{id}', function (Request $request, Response $response, array $args) use ($collection) {
    try {
        $doc = $collection->findOne(['_id' => new ObjectId($args['id'])]);
    } catch (Exception $e) {
        $error = ['error' => 'Invalid ID format'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(400)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    if (!$doc) {
        $error = ['error' => 'Item not found'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(404)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    $result = [
        '_id'  => (string)$doc['_id'],
        'list' => $doc['list']
    ];
    $response->getBody()->write(json_encode($result));
    return $response->withHeader('Content-Type', 'application/json');
});

// PUT /lists/{id} – Update a specific list item.
$app->put('/lists/{id}', function (Request $request, Response $response, array $args) use ($collection) {
    $data = json_decode($request->getBody()->getContents(), true);
    if (!isset($data['list'])) {
        $error = ['error' => "Missing 'list' field"];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(400)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    try {
        $objectId = new ObjectId($args['id']);
    } catch (Exception $e) {
        $error = ['error' => 'Invalid ID format'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(400)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    $updateResult = $collection->updateOne(
        ['_id' => $objectId],
        ['$set' => ['list' => $data['list']]]
    );
    
    if ($updateResult->getModifiedCount() === 0) {
        $error = ['error' => 'Item not found or no change made'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(404)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    // Retrieve the updated document.
    $doc = $collection->findOne(['_id' => $objectId]);
    $result = [
        '_id'  => (string)$doc['_id'],
        'list' => $doc['list']
    ];
    $response->getBody()->write(json_encode($result));
    return $response->withHeader('Content-Type', 'application/json');
});

// DELETE /lists/{id} – Delete a specific list item.
$app->delete('/lists/{id}', function (Request $request, Response $response, array $args) use ($collection) {
    try {
        $objectId = new ObjectId($args['id']);
    } catch (Exception $e) {
        $error = ['error' => 'Invalid ID format'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(400)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    $deleteResult = $collection->deleteOne(['_id' => $objectId]);
    
    if ($deleteResult->getDeletedCount() === 0) {
        $error = ['error' => 'Item not found'];
        $response->getBody()->write(json_encode($error));
        return $response->withStatus(404)
                        ->withHeader('Content-Type', 'application/json');
    }
    
    $success = ['message' => 'Item deleted'];
    $response->getBody()->write(json_encode($success));
    return $response->withHeader('Content-Type', 'application/json');
});

$app->run();
