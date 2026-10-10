#!/usr/bin/env python3
"""Issue a device-bound offline file. pip install cryptography."""
import argparse,hashlib,re,uuid
from pathlib import Path
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import padding,rsa

def device_code(value):
    code=re.sub(r'[ \t\r\n-]', '', value)
    if not re.fullmatch(r'2[0-9]{41}',code):
        raise ValueError('Expected the 42-digit device code shown by the plugin')
    if int(code[1:40]) >= 2**128:
        raise ValueError('Invalid device code')
    checksum=int(hashlib.sha256(code[:40].encode('ascii')).hexdigest()[:4],16)%97
    if code[40:] != f'{checksum:02d}':
        raise ValueError('Device code checksum failed; ask the customer to copy it again')
    return code

def issue(private_pem,license_id,code):
    uid=str(uuid.UUID(license_id));code=device_code(code)
    key=serialization.load_pem_private_key(private_pem,password=None)
    if not isinstance(key,rsa.RSAPrivateKey) or key.key_size!=2048:
        raise ValueError('Expected RSA-2048 private key')
    signature=key.sign(f'DuckPocket|2|{uid}|{code}'.encode('ascii'),padding.PKCS1v15(),hashes.SHA256())
    return f'DP2.{uid}.{code}.{signature.hex()}'

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--private-key',required=True)
    parser.add_argument('--device-code',required=True,help='42 digits; grouped code from email is accepted')
    parser.add_argument('--license-id',default=None,help='Persist one UUID per purchase, reuse for other devices/retries')
    parser.add_argument('--output',help='Save a .ducklicense file; without this print the signed license')
    args=parser.parse_args()
    license=issue(Path(args.private_key).read_bytes(),args.license_id or str(uuid.uuid4()),args.device_code)
    if args.output:
        Path(args.output).write_text(license+'\n',encoding='ascii');print('Saved device license:',args.output)
    else:print(license)
