<?php
// Stateless POST-to-browser launch. No simulations or payloads stored server-side.
declare(strict_types=1);
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: strict-origin-when-cross-origin');
function reject(int $status, string $message): never {
    http_response_code($status); header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['error' => $message]); exit;
}
if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') reject(405, 'POST a JSON payload or form field named payload.');
if ((int)($_SERVER['CONTENT_LENGTH'] ?? 0) > 4 * 1024 * 1024) reject(413, 'Payload exceeds 4 MB.');
try {
    $raw = $_POST['payload'] ?? file_get_contents('php://input', false, null, 0, 4 * 1024 * 1024 + 1);
    if (!is_string($raw) || strlen($raw) > 4 * 1024 * 1024) reject(413, 'Payload exceeds 4 MB.');
    $payload = json_decode($raw, true, 128, JSON_THROW_ON_ERROR);
    if (!is_array($payload) || !is_array($payload['geojson'] ?? null) || !is_string($payload['timestamp'] ?? null)) reject(400, 'Provide geojson and an ISO timestamp.');
    if (!preg_match('/(Z|[+-]\d{2}:\d{2})$/', $payload['timestamp']) || strtotime($payload['timestamp']) === false) reject(400, 'Timestamp must include its timezone.');
    if (!in_array($payload['geojson']['type'] ?? '', ['FeatureCollection','Feature','Point','Polygon','MultiPolygon'], true)) reject(400, 'Unsupported GeoJSON type.');
    $json = json_encode($payload, JSON_HEX_TAG | JSON_HEX_AMP | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_THROW_ON_ERROR);
    $root = is_file(__DIR__ . '/index.html') ? __DIR__ : __DIR__ . '/www/forefire';
    $html = file_get_contents($root . '/index.html');
    if ($html === false) reject(503, 'ForeFire application unavailable.');
    // An explicit base preserves relative module URLs on the POST route.
    $base = htmlspecialchars(rtrim(dirname($_SERVER['SCRIPT_NAME'] ?? '/start.php'), '/') . '/', ENT_QUOTES, 'UTF-8');
    $html = str_replace('<head>', '<head><base href="' . $base . '">', $html);
    header('Content-Type: text/html; charset=utf-8');
    echo str_replace('<!--STARTUP-->', '<script type="application/json" id="startup">' . $json . '</script>', $html);
} catch (JsonException $e) { reject(400, 'Invalid JSON.'); }
