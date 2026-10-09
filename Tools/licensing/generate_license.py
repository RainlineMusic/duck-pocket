#!/usr/bin/env python3
"""pip install cryptography; keep the signing PEM outside the web root and Git."""
import argparse,uuid
from pathlib import Path
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import padding
p=argparse.ArgumentParser()
p.add_argument('--private-key',required=True)
p.add_argument('--license-id',default=None,help='Persist one UUID per successful order; reuse for retries')
a=p.parse_args()
license_id=str(uuid.UUID(a.license_id)) if a.license_id else str(uuid.uuid4())
k=serialization.load_pem_private_key(Path(a.private_key).read_bytes(),password=None)
signature=k.sign(('DuckPocket|1|'+license_id).encode('ascii'),padding.PKCS1v15(),hashes.SHA256())
print('DP1.'+license_id+'.'+signature.hex())
