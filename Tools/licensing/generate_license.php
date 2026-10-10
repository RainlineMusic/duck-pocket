<?php
// Server-side library, never expose the private PEM to a browser/client.
function duckPocketDeviceCode(string $input): string {
    $code=preg_replace('/[ \t\r\n-]/', '', $input);
    if (!preg_match('/^2[0-9]{41}$/D', $code)) throw new InvalidArgumentException('Expected 42-digit device code');
    if (strcmp(substr($code,1,39),'340282366920938463463374607431768211455')>0) throw new InvalidArgumentException('Invalid device code');
    $checksum=hexdec(substr(hash('sha256',substr($code,0,40)),0,4))%97;
    if (substr($code,40)!==sprintf('%02d',$checksum)) throw new InvalidArgumentException('Device code checksum failed');
    return $code;
}
function duckPocketLicense(string $privatePem, string $licenseId, string $deviceCode): string {
    $id=strtolower($licenseId);$code=duckPocketDeviceCode($deviceCode);
    if (!preg_match('/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/D',$id)) throw new InvalidArgumentException('Expected canonical UUID');
    $key=openssl_pkey_get_private($privatePem);$details=$key ? openssl_pkey_get_details($key) : false;
    if (!$details || $details['type']!==OPENSSL_KEYTYPE_RSA || $details['bits']!==2048) throw new RuntimeException('Expected RSA-2048 private key');
    if (!openssl_sign('DuckPocket|2|'.$id.'|'.$code,$signature,$key,OPENSSL_ALGO_SHA256)) throw new RuntimeException('License signing failed');
    return 'DP2.'.$id.'.'.$code.'.'.bin2hex($signature);
}
// Store only a hash of the short key in the purchase database.
function duckPocketShortKey(): string {
    $alphabet='0123456789ABCDEFGHJKMNPQRSTVWXYZ';$data='';
    for ($i=0;$i<16;$i++) $data.=$alphabet[random_int(0,31)];
    return 'DUCK-'.implode('-',str_split($data,4));
}
