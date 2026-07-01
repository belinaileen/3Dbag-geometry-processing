import json
import subprocess
import sys
import tempfile
from pathlib import Path

EXPECTED_IDS = [
    "NL.IMBAG.Pand.0503100000000010",
    "NL.IMBAG.Pand.0503100000002263",
    "NL.IMBAG.Pand.0503100000014169",
    "NL.IMBAG.Pand.0503100000014271",
    "NL.IMBAG.Pand.0503100000018536",
    "NL.IMBAG.Pand.0503100000022524",
    "NL.IMBAG.Pand.0503100000025169",
    "NL.IMBAG.Pand.0503100000031316",
    "NL.IMBAG.Pand.0503100000031927",
    "NL.IMBAG.Pand.0503100000032914",
]

EXPECTED_ATTRS = {
    "NL.IMBAG.Pand.0503100000000010": {
        "geo1004_volume": 617669.926,
        "geo1004_total_roof_area": 48078.923,
    },
    "NL.IMBAG.Pand.0503100000002263": {
        "geo1004_volume": 4577.551,
        "geo1004_total_roof_area": 505.462,
    },
    "NL.IMBAG.Pand.0503100000014169": {
        "geo1004_volume": 11891.344,
        "geo1004_total_roof_area": 1327.866,
    },
    "NL.IMBAG.Pand.0503100000014271": {
        "geo1004_volume": 770.265,
        "geo1004_total_roof_area": 80.635,
    },
    "NL.IMBAG.Pand.0503100000018536": {
        "geo1004_volume": 1009.64,
        "geo1004_total_roof_area": 181.218,
    },
    "NL.IMBAG.Pand.0503100000022524": {
        "geo1004_volume": 15827.593,
        "geo1004_total_roof_area": 1236.315,
    },
    "NL.IMBAG.Pand.0503100000025169": {
        "geo1004_volume": 5290.096,
        "geo1004_total_roof_area": 568.168,
    },
    "NL.IMBAG.Pand.0503100000031316": {
        "geo1004_volume": 32597.056,
        "geo1004_total_roof_area": 2256.203,
    },
    "NL.IMBAG.Pand.0503100000031927": {
        "geo1004_volume": 1331.834,
        "geo1004_total_roof_area": 233.65,
    },
    "NL.IMBAG.Pand.0503100000032914": {
        "geo1004_volume": 154856.043,
        "geo1004_total_roof_area": 11899.273,
    },
}

SKIP_VOLUME = {"NL.IMBAG.Pand.0503100000032914"}


def check(ok, description, detail=""):
    status = "OK" if ok else "FAIL"
    detail = f" -- {detail}" if detail else ""
    print(f"| {status} | {description}{detail} |")
    return ok


def md_table(rows, headers):
    sep = "| " + " | ".join("---" for _ in headers) + " |"
    hdr = "| " + " | ".join(headers) + " |"
    print(hdr)
    print(sep)
    for row in rows:
        print("| " + " | ".join(str(c) for c in row) + " |")


def run_validation(target_path):
    target = Path(target_path)
    if not target.exists():
        print(f"\nError: file not found: {target}")
        return False

    with open(target) as f:
        data = json.load(f)

    results = []
    cos = data.get("CityObjects", {})

    print(f"# Validation report for: {target.name}")
    print()

    # --- 1. Structure ---
    print("## 1. CityJSON structure")
    print("| Result | Check |")
    print("|---|---|")
    r = check(
        data.get("type") == "CityJSON",
        "type is CityJSON",
        f"got '{data.get('type')}'",
    )
    results.append(r)
    r = check(
        data.get("version") == "2.0",
        "version is 2.0",
        f"got '{data.get('version')}'",
    )
    results.append(r)
    print()

    # --- 2. CityObjects ---
    print("## 2. CityObjects")
    print("| Result | Check |")
    print("|---|---|")
    r = check(
        len(cos) == 10,
        "there are exactly 10 city objects",
        f"got {len(cos)}",
    )
    results.append(r)
    actual_ids = sorted(cos.keys())
    extra = set(actual_ids) - set(EXPECTED_IDS)
    missing = set(EXPECTED_IDS) - set(actual_ids)
    detail_parts = []
    if extra:
        detail_parts.append(f"extra: {extra}")
    if missing:
        detail_parts.append(f"missing: {missing}")
    r = check(
        actual_ids == EXPECTED_IDS,
        "object IDs match b10.city.json",
        "; ".join(detail_parts),
    )
    results.append(r)
    print()

    # --- 3. Geometry ---
    print("## 3. Geometry checks (per object)")
    print()
    print("| ID | 1 geometry | lod=2.2 | all triangles |")
    print("|---|---|---|---|")
    all_one_geom = True
    all_lod_22 = True
    all_triangles = True
    geo_rows = []
    for oid in EXPECTED_IDS:
        obj = cos.get(oid, {})
        geoms = obj.get("geometry", [])
        short_id = oid.split(".")[-1]

        ok1 = len(geoms) == 1
        if not ok1:
            all_one_geom = False
        mark1 = "OK" if ok1 else f"FAIL ({len(geoms)})"

        ok2 = False
        mark2 = "FAIL"
        ok3 = False
        mark3 = "FAIL"
        if geoms:
            ok2 = geoms[0].get("lod") == "2.2"
            mark2 = "OK" if ok2 else f"FAIL ({geoms[0].get('lod')})"

            boundaries = geoms[0].get("boundaries", [])
            tri_errors = []
            for fi, face in enumerate(boundaries[0]):
                for ri, ring in enumerate(face):
                    n_verts = len(ring)
                    if n_verts != 3:
                        tri_errors.append(f"face {fi}:{ri}({n_verts})")
            ok3 = len(tri_errors) == 0
            mark3 = "OK" if ok3 else f"FAIL ({'; '.join(tri_errors[:3])})"
            if not ok3:
                all_triangles = False
        else:
            all_lod_22 = False
            all_triangles = False

        geo_rows.append([short_id, mark1, mark2, mark3])
    for row in geo_rows:
        print("| " + " | ".join(str(c) for c in row) + " |")
    results.append(all_one_geom)
    results.append(all_lod_22)
    results.append(all_triangles)
    print()

    # --- 4. Semantic surfaces ---
    print("## 4. Semantic surfaces")
    print()
    print("| ID | RoofSurface | GroundSurface | WallSurface |")
    print("|---|---|---|---|")
    REQUIRED_SURFACES = {"RoofSurface", "GroundSurface", "WallSurface"}
    all_semantics_ok = True
    for oid in EXPECTED_IDS:
        obj = cos.get(oid, {})
        geoms = obj.get("geometry", [])
        short_id = oid.split(".")[-1]
        surface_types = set()
        if geoms:
            sem = geoms[0].get("semantics", {})
            surfaces = sem.get("surfaces", [])
            surface_types = {s.get("type") for s in surfaces if s.get("type")}
        marks = {}
        for stype in REQUIRED_SURFACES:
            if stype in surface_types:
                marks[stype] = "OK"
            else:
                marks[stype] = "FAIL"
                all_semantics_ok = False
        print(
            f"| {short_id} | {marks['RoofSurface']} | {marks['GroundSurface']} | {marks['WallSurface']} |"
        )
    results.append(all_semantics_ok)
    print()

    # --- 5. Attributes ---
    print("## 5. Attribute values")
    print()
    print("| ID | Attribute | Expected | Got | Result |")
    print("|---|---|---|---|---|")
    all_attrs_ok = True
    for oid in EXPECTED_IDS:
        obj = cos.get(oid, {})
        attrs = obj.get("attributes", {})
        short_id = oid.split(".")[-1]
        # for attr_name in ("geo1004_volume", "geo1004_total_roof_area"):
        attr_name = "geo1004_volume"
        if attr_name == "geo1004_volume" and oid in SKIP_VOLUME:
            continue
        expected = EXPECTED_ATTRS[oid][attr_name]
        actual = attrs.get(attr_name)
        ok = actual is not None and abs(actual - expected) <= 0.1
        if not ok:
            all_attrs_ok = False
        result = "OK" if ok else "FAIL"
        print(f"| {short_id} | {attr_name} | {expected} | {actual} | {result} |")
    print("| --- | --- | --- | --- | --- |")
    for oid in EXPECTED_IDS:
        obj = cos.get(oid, {})
        attrs = obj.get("attributes", {})
        short_id = oid.split(".")[-1]
        # for attr_name in ("geo1004_volume", "geo1004_total_roof_area"):
        attr_name = "geo1004_total_roof_area"
        expected = EXPECTED_ATTRS[oid][attr_name]
        actual = attrs.get(attr_name)
        ok = actual is not None and abs(actual - expected) <= 0.1
        if not ok:
            all_attrs_ok = False
        result = "OK" if ok else "FAIL"
        print(f"| {short_id} | {attr_name} | {expected} | {actual} | {result} |")
    results.append(all_attrs_ok)
    print()

    # --- 6. cjval ---
    print("## 6. cjval validation")
    print("| Result | Check |")
    print("|---|---|")
    try:
        proc = subprocess.run(
            ["cjval", "-q", str(target)],
            capture_output=True,
            text=True,
            timeout=120,
        )
        cjval_output = proc.stdout + proc.stderr
        cjval_ok = proc.returncode == 0
        if (
            "Error" in cjval_output
            or "Invalid" in cjval_output
            or "invalid" in cjval_output
        ):
            cjval_ok = False
        r = check(
            cjval_ok,
            "File is valid according to cjval",
            "" if cjval_ok else f"exit code {proc.returncode}",
        )
        results.append(r)
        if not cjval_ok:
            print(f"  cjval output: {cjval_output.strip()}")
    except FileNotFoundError:
        r = check(False, "cjval is available in PATH", "not found")
        results.append(r)
    except subprocess.TimeoutExpired:
        r = check(False, "cjval completed within timeout", "timed out after 120s")
        results.append(r)
    print()

    # --- 7. val3dity ---
    print("## 7. val3dity validation")
    print("| Result | Check |")
    print("|---|---|")
    EXPECTED_INVALID = {"NL.IMBAG.Pand.0503100000032914"}
    try:
        with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as tmp:
            report_path = tmp.name
        proc = subprocess.run(
            ["val3dity", str(target), "--report", report_path],
            capture_output=True,
            text=True,
            timeout=300,
        )
        with open(report_path) as rf:
            report = json.load(rf)
        Path(report_path).unlink(missing_ok=True)
        features = report.get("features", [])
        EXPECTED_INVALID = {"NL.IMBAG.Pand.0503100000032914"}
        checked_ids = {f["id"] for f in features} - EXPECTED_INVALID
        invalid_ids = {f["id"] for f in features if f.get("validity") is False}
        unexpected_invalid = invalid_ids - EXPECTED_INVALID
        ok = len(features) == 10 and len(unexpected_invalid) == 0
        detail_parts = []
        if unexpected_invalid:
            detail_parts.append(f"unexpected invalid: {unexpected_invalid}")
        if len(features) != 10:
            detail_parts.append(f"features={len(features)}")
        r = check(
            ok,
            "{}/9 objects valid (ignoring 0503100000032914)".format(
                9 - len(unexpected_invalid)
            ),
            "; ".join(detail_parts) if not ok else "",
        )
        results.append(r)
    except FileNotFoundError:
        r = check(False, "val3dity is available in PATH", "not found")
        results.append(r)
    except subprocess.TimeoutExpired:
        r = check(False, "val3dity completed within timeout", "timed out after 5min")
        results.append(r)
    print()

    # --- Summary ---
    passed = sum(results)
    n_total = len(results)
    print("## Summary")
    print()
    print(f"{'✅' if passed == n_total else '❌'} -- {passed}/{n_total} ")
    print()

    return all(results)


if __name__ == "__main__":
    target = (
        sys.argv[1]
        if len(sys.argv) == 2
        else str(Path(__file__).parent / "b10_out.city.json")
    )
    ok = run_validation(target)
    sys.exit(0 if ok else 1)
