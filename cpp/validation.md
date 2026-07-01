# Validation report for: b10_out.city.json

## 1. CityJSON structure
| Result | Check |
|---|---|
| OK | type is CityJSON -- got 'CityJSON' |
| OK | version is 2.0 -- got '2.0' |

## 2. CityObjects
| Result | Check |
|---|---|
| OK | there are exactly 10 city objects -- got 10 |
| OK | object IDs match b10.city.json |

## 3. Geometry checks (per object)

| ID | 1 geometry | lod=2.2 | all triangles |
|---|---|---|---|
| 0503100000000010 | OK | OK | OK |
| 0503100000002263 | OK | OK | OK |
| 0503100000014169 | OK | OK | OK |
| 0503100000014271 | OK | OK | OK |
| 0503100000018536 | OK | OK | OK |
| 0503100000022524 | OK | OK | OK |
| 0503100000025169 | OK | OK | OK |
| 0503100000031316 | OK | OK | OK |
| 0503100000031927 | OK | OK | OK |
| 0503100000032914 | OK | OK | OK |

## 4. Semantic surfaces

| ID | RoofSurface | GroundSurface | WallSurface |
|---|---|---|---|
| 0503100000000010 | OK | OK | OK |
| 0503100000002263 | OK | OK | OK |
| 0503100000014169 | OK | OK | OK |
| 0503100000014271 | OK | OK | OK |
| 0503100000018536 | OK | OK | OK |
| 0503100000022524 | OK | OK | OK |
| 0503100000025169 | OK | OK | OK |
| 0503100000031316 | OK | OK | OK |
| 0503100000031927 | OK | OK | OK |
| 0503100000032914 | OK | OK | OK |

## 5. Attribute values

| ID | Attribute | Expected | Got | Result |
|---|---|---|---|---|
| 0503100000000010 | geo1004_volume | 617669.926 | 617669.9262729915 | OK |
| 0503100000002263 | geo1004_volume | 4577.551 | 4577.550598224712 | OK |
| 0503100000014169 | geo1004_volume | 11891.344 | 11891.432546125725 | OK |
| 0503100000014271 | geo1004_volume | 770.265 | 770.2726942759473 | OK |
| 0503100000018536 | geo1004_volume | 1009.64 | 1009.640174912956 | OK |
| 0503100000022524 | geo1004_volume | 15827.593 | 15827.589392716374 | OK |
| 0503100000025169 | geo1004_volume | 5290.096 | 5290.1151506849565 | OK |
| 0503100000031316 | geo1004_volume | 32597.056 | 32597.053647664132 | OK |
| 0503100000031927 | geo1004_volume | 1331.834 | 1331.8435965020144 | OK |
| --- | --- | --- | --- | --- |
| 0503100000000010 | geo1004_total_roof_area | 48078.923 | 48078.92319449945 | OK |
| 0503100000002263 | geo1004_total_roof_area | 505.462 | 505.4615818623802 | OK |
| 0503100000014169 | geo1004_total_roof_area | 1327.866 | 1327.8660882352526 | OK |
| 0503100000014271 | geo1004_total_roof_area | 80.635 | 80.63462171318196 | OK |
| 0503100000018536 | geo1004_total_roof_area | 181.218 | 181.21782426903488 | OK |
| 0503100000022524 | geo1004_total_roof_area | 1236.315 | 1236.3148630158437 | OK |
| 0503100000025169 | geo1004_total_roof_area | 568.168 | 568.1678859321532 | OK |
| 0503100000031316 | geo1004_total_roof_area | 2256.203 | 2256.2033199915923 | OK |
| 0503100000031927 | geo1004_total_roof_area | 233.65 | 233.650158097221 | OK |
| 0503100000032914 | geo1004_total_roof_area | 11899.273 | 11899.27382318282 | OK |

## 6. cjval validation
| Result | Check |
|---|---|
| OK | File is valid according to cjval |

## 7. val3dity validation
| Result | Check |
|---|---|
| OK | 9/9 objects valid (ignoring 0503100000032914) |

## Summary

✅ -- 11/11 

