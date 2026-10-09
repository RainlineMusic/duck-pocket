<?php
// Library function, not a public unauthenticated HTTP endpoint.
function duckPocketLicense(string $privatePem, string $licenseId): string {
    $id = strtolower($licenseId);
    if (!preg_match('/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/D', $id)) {
        throw new InvalidArgumentException('Expected canonical UUID');
    }
    if (!openssl_sign('DuckPocket|1|' . $id, $signature, $privatePem, OPENSSL_ALGO_SHA256)) {
        throw new RuntimeException('License signing failed');
    }
    if (strlen($signature) !== 256) throw new RuntimeException('Expected RSA-2048 key');
    return 'DP1.' . $id . '.' . bin2hex($signature);
}
