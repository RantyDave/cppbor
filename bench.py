import time
import csv
import cbor

# Get a source dataset
places={}
with open("kx-nz-place-names-nzgb-CSV/nz-place-names-nzgb.csv") as f:
    src_data = csv.DictReader(f)
    for row in src_data:
        places[row['name']] = {'id': int(row['feat_id']), 'X': float(row['ref_point_X']), 'Y': float(row['ref_point_Y'])}
print("Places: %d" % len(places))

# Encode into cbor
start = time.time()
cbor_data = cbor.dumps(places)
print('Encoding time: %f' % (time.time()-start))

# Write out (for C++)
with open("cbor-python-wrote", 'wb') as f:
    f.write(cbor_data)

# Parse back to source
start = time.time()
cbor_original = cbor.loads(cbor_data)
print('Parse time: %f' % (time.time()-start))
