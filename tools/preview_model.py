#!/usr/bin/env python3
"""Render a .bfnmdl to a PNG with a tiny software rasterizer (verification aid).

Meshes are drawn in their stored vertex coordinates (mesh/node transforms are
not recovered yet). Textures are looked up by material texture name among the
unpacked .btga/.bpng/.bdds files, preferring the model's own textures/ folder.
"""
import argparse
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import convert_models as cm  # noqa: E402
import convert_textures as ct  # noqa: E402

TEXTURE_SUFFIXES = ('.btga', '.bpng', '.bdds')


def texture_index(root):
    index = {}
    for path in root.rglob('*'):
        if path.suffix in TEXTURE_SUFFIXES:
            index.setdefault(path.stem.lower(), []).append(path)
    return index


def find_texture(index, name, model_path):
    stem = Path(name.replace('\\', '/')).stem.lower()
    candidates = index.get(stem, [])
    for path in candidates:
        if path.parent == model_path.parent / 'textures':
            return path
    return candidates[0] if candidates else None


class Texture:
    def __init__(self, path):
        _, self.rows = ct.decode(path.read_bytes())
        self.h, self.w = len(self.rows), len(self.rows[0])

    def sample(self, u, v):
        x = int((u % 1.0) * self.w) % self.w
        y = int((1.0 - (v % 1.0)) * self.h) % self.h  # rows are top-down, v=0 is the bottom
        return self.rows[y][x]


def render(model, textures, size, yaw, pitch, exclude=()):
    tris = []
    for mesh in model['meshes']:
        for part in mesh['parts']:
            verts = cm.decode_vertices(part)
            names = part['material']['textures']
            if any(word in slot['name'].lower() for slot in names for word in exclude):
                continue
            tex = textures.get(names[0]['name']) if names else None
            for a, b, c in cm.triangles(part):
                tris.append((verts[a], verts[b], verts[c], tex))
    if not tris:
        raise ValueError("model has no triangles")
    points = [v['position'] for t in tris for v in t[:3]]
    lo = [min(p[i] for p in points) for i in range(3)]
    hi = [max(p[i] for p in points) for i in range(3)]
    centre = [(lo[i] + hi[i]) / 2 for i in range(3)]
    radius = max(math.dist(p, centre) for p in points) or 1.0
    cy, sy, cp, sp = math.cos(yaw), math.sin(yaw), math.cos(pitch), math.sin(pitch)
    dist = radius * 2.6
    focal = size / 2 / math.tan(math.radians(25))

    def project(p):
        x, y, z = (p[i] - centre[i] for i in range(3))
        x, z = x * cy - z * sy, x * sy + z * cy
        y, z = y * cp - z * sp, y * sp + z * cp
        z += dist
        return size / 2 + x * focal / z, size / 2 - y * focal / z, z

    light = (0.4, 0.8, 0.45)
    colour = [[(40, 44, 52)] * size for _ in range(size)]
    depth = [[math.inf] * size for _ in range(size)]
    for v0, v1, v2, tex in tris:
        p = [project(v['position']) for v in (v0, v1, v2)]
        area = (p[1][0] - p[0][0]) * (p[2][1] - p[0][1]) - (p[2][0] - p[0][0]) * (p[1][1] - p[0][1])
        if abs(area) < 1e-9 or min(q[2] for q in p) <= 0:
            continue
        x0, x1 = max(0, int(min(q[0] for q in p))), min(size - 1, int(max(q[0] for q in p)) + 1)
        y0, y1 = max(0, int(min(q[1] for q in p))), min(size - 1, int(max(q[1] for q in p)) + 1)
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                px, py = x + 0.5, y + 0.5
                w0 = ((p[1][0] - px) * (p[2][1] - py) - (p[2][0] - px) * (p[1][1] - py)) / area
                w1 = ((p[2][0] - px) * (p[0][1] - py) - (p[0][0] - px) * (p[2][1] - py)) / area
                w2 = 1 - w0 - w1
                if w0 < 0 or w1 < 0 or w2 < 0:
                    continue
                z = w0 * p[0][2] + w1 * p[1][2] + w2 * p[2][2]
                if z >= depth[y][x]:
                    continue
                rgba = [255, 255, 255, 255]
                if tex and 'uv0' in v0:
                    u = w0 * v0['uv0'][0] + w1 * v1['uv0'][0] + w2 * v2['uv0'][0]
                    v = w0 * v0['uv0'][1] + w1 * v1['uv0'][1] + w2 * v2['uv0'][1]
                    rgba = list(tex.sample(u, v))
                    if rgba[3] < 64:
                        continue
                if 'color' in v0:
                    for i in range(3):
                        c = w0 * v0['color'][i] + w1 * v1['color'][i] + w2 * v2['color'][i]
                        rgba[i] = rgba[i] * c / 255
                shade = 1.0
                if 'normal' in v0:
                    n = [w0 * v0['normal'][i] + w1 * v1['normal'][i] + w2 * v2['normal'][i] for i in range(3)]
                    length = math.sqrt(sum(c * c for c in n)) or 1
                    shade = 0.45 + 0.55 * max(0.0, sum(n[i] * light[i] for i in range(3)) / length)
                depth[y][x] = z
                colour[y][x] = tuple(min(255, int(rgba[i] * shade)) for i in range(3))
    return [[c + (255,) for c in row] for row in colour]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('model', type=Path)
    parser.add_argument('--assets', type=Path, required=True, help='unpacked .fib files/ directory')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--size', type=int, default=384)
    parser.add_argument('--yaw', type=float, default=35)
    parser.add_argument('--pitch', type=float, default=20)
    parser.add_argument('--exclude', action='append', default=[],
                        help='skip parts whose texture name contains this text (repeatable), e.g. sky')
    args = parser.parse_args()
    model = cm.parse_model(args.model.read_bytes())
    index = texture_index(args.assets)
    textures, missing = {}, set()
    for mesh in model['meshes']:
        for part in mesh['parts']:
            for slot in part['material']['textures']:
                if slot['name'] in textures or slot['name'] in missing:
                    continue
                path = find_texture(index, slot['name'], args.model)
                if path:
                    textures[slot['name']] = Texture(path)
                else:
                    missing.add(slot['name'])
    rows = render(model, textures, args.size, math.radians(args.yaw), math.radians(args.pitch),
                  [w.lower() for w in args.exclude])
    args.output.write_bytes(ct.png(rows))
    print('rendered %s (%d textures, missing: %s)' % (args.output, len(textures), sorted(missing) or 'none'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
