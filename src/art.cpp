// art.cpp - see art.h. Everything is painted into a grey float buffer at twice the final size,
// softened, shrunk, and then given the look of an old print: contrast, grain, vignette, tint.
#include "art.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace art {
namespace {

// ---------------------------------------------------------------------------- randomness and noise
struct R {
    uint64_t s;
    explicit R(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) {}
    uint64_t next() {
        uint64_t z = (s += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    float f() { return (float)((next() >> 40) / 16777216.0); }
    float rnd(float a, float b) { return a + f() * (b - a); }
    int n(int k) { return k <= 0 ? 0 : (int)(next() % (uint64_t)k); }
    bool pct(float c) { return f() * 100 < c; }
    float gauss() { float u = std::max(f(), 1e-7f), v = f(); return std::sqrt(-2 * std::log(u)) * std::cos(6.2831853f * v); }
};

inline float cl(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float sstep(float a, float b, float x) { float t = cl((x - a) / (b - a)); return t * t * (3 - 2 * t); }
inline float mix(float a, float b, float t) { return a + (b - a) * t; }

inline uint32_t hash(int x, int y, uint32_t seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
inline float h01(int x, int y, uint32_t seed) { return (hash(x, y, seed) & 0xFFFFFF) / 16777216.f; }

/** Smooth value noise; if period > 0 it wraps, so the texture tiles. */
float vnoise(float x, float y, uint32_t seed, int period = 0) {
    int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
    float fx = x - x0, fy = y - y0;
    auto w = [&](int a) { return period > 0 ? ((a % period) + period) % period : a; };
    float a = h01(w(x0), w(y0), seed), b = h01(w(x0 + 1), w(y0), seed), c = h01(w(x0), w(y0 + 1), seed), d = h01(w(x0 + 1), w(y0 + 1), seed);
    float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    return mix(mix(a, b, sx), mix(c, d, sx), sy);
}
float fbm(float x, float y, uint32_t seed, int oct, int period = 0) {
    float sum = 0, amp = 0.5f, norm = 0;
    for (int i = 0; i < oct; i++) {
        sum += amp * vnoise(x, y, seed + i * 101, period);
        norm += amp; amp *= 0.5f; x *= 2; y *= 2; if (period) period *= 2;
    }
    return sum / norm;
}

// ---------------------------------------------------------------------------- a grey canvas
struct Buf {
    int w, h;
    std::vector<float> v;
    Buf(int w_, int h_, float fill = 0) : w(w_), h(h_), v((size_t)w_ * h_, fill) {}
    float& at(int x, int y) { return v[(size_t)y * w + x]; }
    float get(int x, int y) const { x = std::max(0, std::min(w - 1, x)); y = std::max(0, std::min(h - 1, y)); return v[(size_t)y * w + x]; }
};

void blur(Buf& b, float sigma) {
    if (sigma <= 0.05f) return;
    int r = (int)std::ceil(sigma * 2.5f);
    std::vector<float> k(2 * r + 1);
    float s = 0;
    for (int i = -r; i <= r; i++) { k[i + r] = std::exp(-(i * i) / (2 * sigma * sigma)); s += k[i + r]; }
    for (auto& x : k) x /= s;
    Buf t(b.w, b.h);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) { float a = 0; for (int i = -r; i <= r; i++) a += k[i + r] * b.get(x + i, y); t.at(x, y) = a; }
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) { float a = 0; for (int i = -r; i <= r; i++) a += k[i + r] * t.get(x, y + i); b.at(x, y) = a; }
}

Buf half(const Buf& b) {
    Buf o(b.w / 2, b.h / 2);
    for (int y = 0; y < o.h; y++)
        for (int x = 0; x < o.w; x++) o.at(x, y) = 0.25f * (b.get(2 * x, 2 * y) + b.get(2 * x + 1, 2 * y) + b.get(2 * x, 2 * y + 1) + b.get(2 * x + 1, 2 * y + 1));
    return o;
}

inline void put(Buf& b, int x, int y, float val, float a) {
    if (x < 0 || y < 0 || x >= b.w || y >= b.h || a <= 0) return;
    float& p = b.at(x, y);
    p = mix(p, val, cl(a));
}

// soft shapes, coordinates in pixels
void rect(Buf& b, float x0, float y0, float x1, float y1, float val, float a = 1, float soft = 1) {
    int ix0 = std::max(0, (int)(x0 - soft - 1)), ix1 = std::min(b.w - 1, (int)(x1 + soft + 1));
    int iy0 = std::max(0, (int)(y0 - soft - 1)), iy1 = std::min(b.h - 1, (int)(y1 + soft + 1));
    for (int y = iy0; y <= iy1; y++)
        for (int x = ix0; x <= ix1; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float m = sstep(x0 - soft, x0 + soft, px) * sstep(-x1 - soft, -x1 + soft, -px) * sstep(y0 - soft, y0 + soft, py) * sstep(-y1 - soft, -y1 + soft, -py);
            put(b, x, y, val, a * m);
        }
}
void ellipse(Buf& b, float cx, float cy, float rx, float ry, float val, float a = 1, float soft = 1) {
    int ix0 = std::max(0, (int)(cx - rx - soft - 1)), ix1 = std::min(b.w - 1, (int)(cx + rx + soft + 1));
    int iy0 = std::max(0, (int)(cy - ry - soft - 1)), iy1 = std::min(b.h - 1, (int)(cy + ry + soft + 1));
    for (int y = iy0; y <= iy1; y++)
        for (int x = ix0; x <= ix1; x++) {
            float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
            float d = (std::sqrt(dx * dx + dy * dy) - 1) * std::min(rx, ry);
            put(b, x, y, val, a * sstep(soft, -soft, d));
        }
}
/** A convex quadrilateral (corners in order), anti-aliased. */
void quad(Buf& b, const float* xs, const float* ys, float val, float a = 1, float soft = 1) {
    float minx = 1e9, maxx = -1e9, miny = 1e9, maxy = -1e9;
    for (int i = 0; i < 4; i++) { minx = std::min(minx, xs[i]); maxx = std::max(maxx, xs[i]); miny = std::min(miny, ys[i]); maxy = std::max(maxy, ys[i]); }
    float area = 0;
    for (int i = 0; i < 4; i++) { int j = (i + 1) % 4; area += xs[i] * ys[j] - xs[j] * ys[i]; }
    float sg = area > 0 ? 1.f : -1.f;
    for (int y = std::max(0, (int)miny - 2); y <= std::min(b.h - 1, (int)maxy + 2); y++)
        for (int x = std::max(0, (int)minx - 2); x <= std::min(b.w - 1, (int)maxx + 2); x++) {
            float px = x + 0.5f, py = y + 0.5f, dmin = 1e9;
            for (int i = 0; i < 4; i++) {
                int j = (i + 1) % 4;
                float ex = xs[j] - xs[i], ey = ys[j] - ys[i], len = std::sqrt(ex * ex + ey * ey) + 1e-6f;
                float d = sg * ((px - xs[i]) * ey - (py - ys[i]) * ex) / len;   // positive inside
                dmin = std::min(dmin, -d);
            }
            put(b, x, y, val, a * sstep(soft, -soft, -dmin));
        }
}
void line(Buf& b, float x0, float y0, float x1, float y1, float width, float val, float a = 1) {
    float dx = x1 - x0, dy = y1 - y0, len = std::sqrt(dx * dx + dy * dy) + 1e-6f;
    float nx = -dy / len * width / 2, ny = dx / len * width / 2;
    float xs[4] = {x0 + nx, x1 + nx, x1 - nx, x0 - nx}, ys[4] = {y0 + ny, y1 + ny, y1 - ny, y0 - ny};
    quad(b, xs, ys, val, a, 0.8f);
}

/** The look of a print: contrast, grain, vignette, dust, and a tint for the decade. */
Img finish(Buf& g, uint32_t seed, int era, float grain, float contrast, float vignette) {
    R r(seed ^ 0xA5A5);
    Img im;
    im.w = g.w; im.h = g.h; im.px.resize((size_t)g.w * g.h);
    // dust and a scratch or two
    int specks = 6 + r.n(10);
    for (int i = 0; i < specks; i++) {
        float x = r.f() * g.w, y = r.f() * g.h, rad = r.rnd(0.4f, 1.3f);
        ellipse(g, x, y, rad, rad * r.rnd(0.6f, 1.6f), r.pct(70) ? 0.97f : 0.08f, 0.8f, 0.5f);
    }
    if (r.pct(45)) { float x = r.f() * g.w; line(g, x, 0, x + r.rnd(-6, 6), (float)g.h, 0.6f, 0.95f, 0.35f); }
    for (int y = 0; y < g.h; y++)
        for (int x = 0; x < g.w; x++) {
            float v = g.at(x, y);
            v = 0.5f + (v - 0.5f) * contrast;
            v = cl(v);
            v = v * v * (3 - 2 * v) * 0.35f + v * 0.65f;                 // a gentle S curve
            float nx = (x + 0.5f) / g.w - 0.5f, ny = (y + 0.5f) / g.h - 0.5f;
            v *= 1 - vignette * (nx * nx + ny * ny) * 2.2f;
            v += r.gauss() * grain * (0.6f + 0.8f * v * (1 - v) * 2);
            v = cl(v);
            float rr, gg, bb;
            if (era == ERA_60S) { rr = v * 1.06f + 0.02f; gg = v * 0.98f + 0.01f; bb = v * 0.84f; }
            else if (era == ERA_70S) { rr = v * 1.03f + 0.01f; gg = v * 0.99f; bb = v * 0.92f; }
            else { rr = v * 0.98f; gg = v * 0.99f; bb = v * 1.02f; }
            uint32_t R8 = (uint32_t)(cl(rr) * 255 + 0.5f), G8 = (uint32_t)(cl(gg) * 255 + 0.5f), B8 = (uint32_t)(cl(bb) * 255 + 0.5f);
            im.px[(size_t)y * g.w + x] = 0xFF000000u | (B8 << 16) | (G8 << 8) | R8;
        }
    return im;
}

}  // namespace

// ============================================================================ the file photograph
Img portrait(const Face& f, int W, int H) {
    R r(f.seed * 2654435761u + 17);
    const int RW = W * 2, RH = H * 2;
    Buf g(RW, RH);
    const float S = RW / 2.f;                      // pixels per unit; x runs -1..1
    auto X = [&](float u) { return S + u * S; };
    auto Y = [&](float v) { return v * S; };

    // --- who is in front of the camera
    bool fem = f.female;
    float hx = r.rnd(-0.04f, 0.04f), hy = r.rnd(1.00f, 1.06f);
    float rx = fem ? r.rnd(0.38f, 0.42f) : r.rnd(0.41f, 0.47f);
    float ry = rx * (fem ? r.rnd(1.30f, 1.40f) : r.rnd(1.26f, 1.36f));
    float jaw = fem ? r.rnd(0.18f, 0.28f) : r.rnd(0.08f, 0.22f);
    float skin = f.skin + r.rnd(-0.04f, 0.04f);
    float age = (float)f.age;
    float grey = age < 42 ? r.rnd(0, 0.15f) : age < 52 ? r.rnd(0.15f, 0.55f) : r.rnd(0.5f, 0.95f);
    float hairDark = r.pct(fem ? 22 : 18) ? r.rnd(0.40f, 0.55f) : r.rnd(0.10f, 0.26f);
    float hairCol = mix(hairDark, 0.72f, grey);
    float recede = (!fem && age > 38) ? cl((age - 38) / 26 + r.rnd(-0.2f, 0.3f)) : 0;
    bool bald = !fem && age > 50 && r.pct(28);
    bool glasses = r.pct(age > 45 ? 45 : 22);
    bool moustache = !fem && !f.uniform && r.pct(age > 40 ? 30 : 15);
    float suit = r.pct(55) ? r.rnd(0.12f, 0.22f) : r.rnd(0.28f, 0.45f);
    float tie = r.rnd(0.12f, 0.55f);
    float lx = r.rnd(-0.75f, -0.55f), ly = -0.38f, lz = 0.60f;         // the studio lamp: upper left, well to the side
    float ll = std::sqrt(lx * lx + ly * ly + lz * lz); lx /= ll; ly /= ll; lz /= ll;
    float gaze = r.rnd(-0.02f, 0.02f);
    float smile = r.rnd(-0.015f, 0.01f);

    bool femDark = fem && r.pct(45);
    float femTop = femDark ? suit : r.rnd(0.72f, 0.86f);
    float eyeY = hy - 0.06f * ry, browY = hy - 0.27f * ry, noseY = hy + 0.32f * ry, mouthY = hy + 0.56f * ry;
    float eyeDX = 0.37f * rx;
    float sY = hy + ry * 1.18f;                     // where the shoulders begin

    // --- background: a plain studio wall, lighter where the lamp falls
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            float u = (x - S) / S, v = y / S;
            float bg = 0.80f - 0.07f * v + 0.06f * std::exp(-((u + 0.7f) * (u + 0.7f) + (v - 0.4f) * (v - 0.4f)) * 1.2f);
            bg += (fbm(x * 0.02f, y * 0.02f, f.seed, 3) - 0.5f) * 0.05f;
            g.at(x, y) = bg;
        }
    // the sitter's shadow on the wall
    ellipse(g, X(hx + 0.22f), Y(hy + 0.1f), rx * S * 1.1f, ry * S * 1.1f, 0.62f, 0.45f, S * 0.12f);

    // --- body
    for (int y = (int)Y(sY - 0.15f); y < RH; y++)
        for (int x = 0; x < RW; x++) {
            float u = (x - S) / S - hx, v = y / S;
            float t = v - sY;
            float halfW = 0.40f + 2.3f * std::max(0.f, t) * (fem ? 0.9f : 1.f);
            halfW = std::min(halfW, 1.35f);
            float shoulderTop = sY + 0.50f * std::pow(std::max(0.f, std::fabs(u) - 0.26f), 1.35f);
            float m = sstep(-0.02f, 0.02f, v - shoulderTop) * sstep(-0.02f, 0.02f, halfW - std::fabs(u));
            if (m <= 0) continue;
            float shade = 0.85f + 0.30f * cl(0.5f - u * 0.45f) - 0.10f * cl((v - sY) * 0.6f);
            float val = (fem ? femTop : suit) * shade;
            if (f.uniform) val = 0.30f * shade;
            put(g, x, y, val, m);
        }
    if (!fem) {
        // shirt, collar and tie in the opening of the jacket
        float bx[4] = {X(hx - 0.20f), X(hx + 0.20f), X(hx + 0.02f), X(hx - 0.02f)};
        float by[4] = {Y(sY - 0.06f), Y(sY - 0.06f), Y(sY + 0.62f), Y(sY + 0.62f)};
        quad(g, bx, by, 0.86f);
        float cxl[4] = {X(hx - 0.21f), X(hx - 0.01f), X(hx - 0.06f), X(hx - 0.25f)}, cyl[4] = {Y(sY - 0.10f), Y(sY + 0.02f), Y(sY + 0.14f), Y(sY + 0.02f)};
        float cxr[4] = {X(hx + 0.01f), X(hx + 0.21f), X(hx + 0.25f), X(hx + 0.06f)}, cyr[4] = {Y(sY + 0.02f), Y(sY - 0.10f), Y(sY + 0.02f), Y(sY + 0.14f)};
        quad(g, cxl, cyl, 0.93f); quad(g, cxr, cyr, 0.90f);
        float tx[4] = {X(hx - 0.045f), X(hx + 0.045f), X(hx + 0.075f), X(hx - 0.075f)}, ty[4] = {Y(sY + 0.03f), Y(sY + 0.03f), Y(sY + 0.66f), Y(sY + 0.66f)};
        quad(g, tx, ty, tie);
        ellipse(g, X(hx), Y(sY + 0.02f), 0.05f * S, 0.04f * S, tie * 0.85f);
        if (r.pct(35)) for (int k = 0; k < 7; k++) line(g, X(hx - 0.08f), Y(sY + 0.12f + k * 0.08f), X(hx + 0.08f), Y(sY + 0.06f + k * 0.08f), 0.012f * S, tie + 0.25f, 0.6f);
        // lapels
        line(g, X(hx - 0.20f), Y(sY - 0.04f), X(hx - 0.02f), Y(sY + 0.66f), 0.025f * S, suit * 0.6f, 0.9f);
        line(g, X(hx + 0.20f), Y(sY - 0.04f), X(hx + 0.02f), Y(sY + 0.66f), 0.025f * S, suit * 0.6f, 0.9f);
        line(g, X(hx - 0.22f), Y(sY + 0.02f), X(hx - 0.38f), Y(sY + 0.36f), 0.02f * S, suit * 0.65f, 0.8f);
        line(g, X(hx + 0.22f), Y(sY + 0.02f), X(hx + 0.38f), Y(sY + 0.36f), 0.02f * S, suit * 0.65f, 0.8f);
        if (f.uniform) {
            // shoulder boards
            for (int side = -1; side <= 1; side += 2) {
                float cx = hx + side * 0.78f, cy = sY + 0.30f;
                float a = side * 0.32f, ca = std::cos(a), sa = std::sin(a), hw = 0.17f, hh = 0.05f;
                float qx[4], qy[4];
                float px_[4] = {-hw, hw, hw, -hw}, py_[4] = {-hh, -hh, hh, hh};
                for (int i = 0; i < 4; i++) { qx[i] = X(cx + px_[i] * ca - py_[i] * sa); qy[i] = Y(cy + px_[i] * sa + py_[i] * ca); }
                quad(g, qx, qy, 0.68f);
                line(g, X(cx - hw * ca), Y(cy - hw * sa), X(cx + hw * ca), Y(cy + hw * sa), 0.02f * S, 0.40f);
                for (int k = 0; k < 2; k++) ellipse(g, X(cx + (k ? 0.06f : -0.06f) * ca), Y(cy + (k ? 0.06f : -0.06f) * sa - 0.025f), 0.016f * S, 0.016f * S, 0.9f);
            }
        }
    } else {
        // a blouse with a round collar, or a dark dress with a light one
        float collar = femDark ? 0.88f : 0.95f;
        ellipse(g, X(hx - 0.13f), Y(sY + 0.05f), 0.15f * S, 0.08f * S, collar);
        ellipse(g, X(hx + 0.13f), Y(sY + 0.05f), 0.15f * S, 0.08f * S, collar);
        if (r.pct(40)) ellipse(g, X(hx), Y(sY + 0.20f), 0.03f * S, 0.03f * S, 0.95f);
    }

    // --- hair behind the head (women)
    float hairBottom = hy + ry * r.rnd(0.35f, 0.75f);
    if (fem) {
        float hr = rx * r.rnd(1.20f, 1.34f), hry = ry * r.rnd(1.08f, 1.16f);
        for (int y = 0; y < RH; y++)
            for (int x = 0; x < RW; x++) {
                float u = (x - S) / S, v = y / S;
                float dx = (u - hx) / hr, dy = (v - (hy - 0.06f)) / hry;
                float d = std::sqrt(dx * dx + dy * dy);
                float m = sstep(1.02f, 0.97f, d) * sstep(hairBottom + 0.04f, hairBottom - 0.04f, v);
                if (m <= 0) continue;
                float tex = (vnoise(u * 90, v * 12, f.seed + 7) - 0.5f) * 0.10f;
                put(g, x, y, hairCol * (0.85f + 0.3f * cl(-dx * 0.5f + 0.4f)) + tex, m);
            }
    }

    // --- neck
    for (int y = (int)Y(hy + 0.4f * ry); y < (int)Y(sY + 0.15f); y++)
        for (int x = 0; x < RW; x++) {
            float u = (x - S) / S - hx, v = y / S;
            float nw = (fem ? 0.19f : 0.245f) * rx / 0.44f * (1 + 0.25f * cl((v - (hy + ry)) / 0.5f));
            float m = sstep(nw + 0.05f, nw - 0.05f, std::fabs(u));
            if (!fem) m *= sstep(sY + 0.02f, sY - 0.04f, v);
            if (m <= 0) continue;
            float nx = u / nw;
            float diff = cl(nx * lx * 0.8f + 0.55f);
            float under = sstep(hy + ry * 1.30f, hy + ry * 0.95f, v);          // the shadow of the chin
            put(g, x, y, skin * (0.38f + 0.40f * diff) * (1 - 0.45f * under), m);
        }

    // --- ears
    for (int side = -1; side <= 1; side += 2) {
        float ex = hx + side * rx * 0.96f, ey = hy + 0.10f * ry;
        float lit = side < 0 ? 0.80f : 0.55f;
        ellipse(g, X(ex), Y(ey), 0.10f * rx * S, 0.19f * ry * S, skin * lit * 0.85f, 1, 3);
        ellipse(g, X(ex - side * 0.02f), Y(ey), 0.05f * rx * S, 0.12f * ry * S, skin * lit * 0.55f, 0.7f, 3);
    }

    // --- the face
    float hairTop = hy - ry * (bald ? 0.95f : r.rnd(0.64f, 0.72f));
    for (int y = (int)Y(hy - ry * 1.1f); y < (int)Y(hy + ry * 1.05f); y++)
        for (int x = (int)X(hx - rx * 1.1f); x < (int)X(hx + rx * 1.1f); x++) {
            if (x < 0 || y < 0 || x >= RW || y >= RH) continue;
            float u = (x + 0.5f - S) / S, v = (y + 0.5f) / S;
            float dy = (v - hy) / ry;
            float taper = 1 - jaw * std::pow(std::max(0.f, dy), 1.6f);
            float dx = (u - hx) / (rx * taper);
            float d = std::sqrt(dx * dx + dy * dy);
            float m = sstep(1.0f + 0.012f / ry, 1.0f - 0.012f / ry, d);
            if (m <= 0) continue;
            float nz = std::sqrt(std::max(0.f, 1 - dx * dx * 0.92f - dy * dy * 0.85f));
            float diff = cl(dx * lx + dy * 0.30f * ly + nz * lz);
            float val = skin * (0.40f + 0.70f * diff) * (0.70f + 0.30f * std::sqrt(nz));
            val *= 1 - 0.22f * sstep(0.55f, 1.0f, dy) * sstep(0.2f, 0.9f, std::fabs(dx) + dy * 0.3f);   // under the jaw
            float fx = u - hx, fy = v;
            auto el = [&](float cx, float cy, float ax, float ay, float soft) {
                float qx = (fx - cx) / ax, qy = (fy - cy) / ay;
                return sstep(1 + soft, 1 - soft, std::sqrt(qx * qx + qy * qy));
            };
            // eye sockets and brow ridge
            for (int s = -1; s <= 1; s += 2) {
                val *= 1 - 0.30f * el(s * eyeDX, eyeY - 0.02f, 0.30f * rx, 0.14f * ry, 0.7f);
                val *= 1 + 0.05f * el(s * eyeDX, browY - 0.05f * ry, 0.30f * rx, 0.07f * ry, 0.7f);
            }
            float shadowSide = lx < 0 ? 1.f : -1.f;
            val *= 1 - 0.14f * el(shadowSide * 0.55f * rx, hy + 0.30f * ry, 0.30f * rx, 0.22f * ry, 0.8f);
            val *= 1 + 0.05f * el(0, hy + 0.86f * ry, 0.20f * rx, 0.08f * ry, 0.8f);
            // nose: the shadowed side, the tip, the nostrils, the shadow beneath
            float sideSh = lx < 0 ? 1.f : -1.f;
            val *= 1 - 0.17f * el(sideSh * 0.10f * rx, (eyeY + noseY) / 2, 0.06f * rx, 0.24f * ry, 0.7f);
            val *= 1 + 0.06f * el(-sideSh * 0.03f * rx, noseY - 0.02f * ry, 0.06f * rx, 0.05f * ry, 0.8f);
            for (int s = -1; s <= 1; s += 2) val *= 1 - 0.50f * el(s * 0.085f * rx, noseY + 0.05f * ry, 0.045f * rx, 0.022f * ry, 0.5f);
            val *= 1 - 0.18f * el(0.02f * rx, noseY + 0.11f * ry, 0.18f * rx, 0.035f * ry, 0.8f);
            // mouth
            float mw = (fem ? 0.27f : 0.31f) * rx;
            float mx = fx / mw;
            if (std::fabs(mx) < 1.15f) {
                float lipY = mouthY + smile * (1 - mx * mx) - 0.01f * ry * mx * mx;
                float endFade = sstep(1.15f, 0.85f, std::fabs(mx));
                float lineM = sstep(0.012f * ry, 0.003f * ry, std::fabs(fy - lipY)) * endFade;
                val *= 1 - 0.55f * lineM;
                float up = sstep(0.05f * ry, 0.0f, lipY - fy) * sstep(-0.002f, 0.004f * ry, lipY - fy) * endFade;
                val *= 1 - (fem ? 0.32f : 0.13f) * up;
                float low = sstep(0.07f * ry, 0.0f, fy - lipY) * sstep(-0.002f, 0.006f * ry, fy - lipY) * endFade;
                val *= 1 + (fem ? -0.20f : 0.03f) * low;
                val *= 1 - 0.16f * el(0, lipY + 0.11f * ry, 0.18f * rx, 0.03f * ry, 0.8f);
            }
            // lines of age
            if (age > 40) {
                float k = cl((age - 40) / 25);
                for (int s = -1; s <= 1; s += 2) {
                    float t = cl((fy - (noseY + 0.02f * ry)) / (mouthY + 0.06f * ry - noseY));
                    float lx_ = s * (0.16f + 0.10f * t) * rx;
                    float near = sstep(0.012f * rx, 0.0f, std::fabs(fx - lx_)) * (t > 0 && t < 1 ? 1.f : 0.f);
                    val *= 1 - 0.13f * k * near;
                }
                if (age > 52) for (int i = 0; i < 3; i++) {
                    float ly_ = hy - ry * (0.42f + i * 0.07f);
                    val *= 1 - 0.06f * k * sstep(0.006f * ry, 0.0f, std::fabs(fy - ly_)) * sstep(0.5f * rx, 0.3f * rx, std::fabs(fx));
                }
            }
            // eyes
            for (int s = -1; s <= 1; s += 2) {
                float ex = s * eyeDX;
                float alm = el(ex, eyeY, 0.15f * rx, 0.045f * ry, 0.35f);
                val = mix(val, 0.58f * skin / 0.74f, alm * 0.80f);
                float iris = el(ex + gaze, eyeY + 0.004f, 0.075f * rx, 0.075f * rx, 0.35f) * alm;
                val = mix(val, 0.20f, iris);
                val = mix(val, 0.06f, el(ex + gaze, eyeY + 0.005f, 0.028f * rx, 0.028f * rx, 0.3f) * alm);
                val = mix(val, 0.98f, el(ex + gaze - 0.022f * rx, eyeY - 0.012f * ry, 0.012f * rx, 0.012f * rx, 0.4f) * alm);
                float lid = el(ex, eyeY - 0.04f * ry, 0.17f * rx, 0.028f * ry, 0.4f) * (1 - el(ex, eyeY + 0.005f, 0.16f * rx, 0.05f * ry, 0.4f) * 0.5f);
                val *= 1 - (fem ? 0.62f : 0.45f) * lid;
                // eyebrows
                float t = (fx - ex) * s / (0.24f * rx) * 0.5f + 0.5f;           // 0 inner .. 1 outer
                if (t > -0.05f && t < 1.05f) {
                    float yc = browY - 0.03f * ry * std::sin(3.1416f * cl(t)) + (fem ? -0.01f * ry : 0);
                    float th = (fem ? 0.018f : 0.03f) * ry * (1.2f - 0.6f * cl(t));
                    float bm = sstep(th, th * 0.4f, std::fabs(fy - yc)) * sstep(-0.05f, 0.08f, t) * sstep(1.05f, 0.85f, t);
                    val = mix(val, hairCol * 0.75f, bm * 0.85f);
                }
            }
            // moustache
            if (moustache) {
                float mm = el(0, mouthY - 0.07f * ry, 0.26f * rx, 0.045f * ry, 0.35f);
                val = mix(val, hairCol * 0.8f + (vnoise(fx * 300, fy * 40, f.seed) - 0.5f) * 0.08f, mm * 0.9f);
            }
            put(g, x, y, val, m);
        }

    // --- hair on top (and the fringe)
    for (int y = 0; y < (int)Y(hy + ry * 0.75f); y++)
        for (int x = 0; x < RW; x++) {
            float u = (x + 0.5f - S) / S, v = (y + 0.5f) / S;
            float dxn = (u - hx) / rx;
            float cy = hy - 0.05f * ry;
            float dx = (u - hx) / (rx * (fem ? 1.10f : 1.06f)), dy = (v - cy) / (ry * (fem ? 1.10f : 1.05f));
            float d = std::sqrt(dx * dx + dy * dy);
            float fuzz = (vnoise(u * 60, v * 60, f.seed + 11) - 0.5f) * 0.05f;
            float cap = sstep(1.03f, 0.95f, d + fuzz);
            if (cap <= 0) continue;
            float line_;
            if (fem) line_ = hy - ry * (0.50f + 0.06f * std::cos(dxn * 7 + f.seed % 7) - 0.10f * dxn * dxn);
            else {
                line_ = hairTop + ry * 0.10f * dxn * dxn;
                line_ -= recede * ry * 0.30f * std::exp(-((std::fabs(dxn) - 0.5f) * (std::fabs(dxn) - 0.5f)) / 0.05f);
            }
            float onTop = sstep(line_ + 0.025f, line_ - 0.025f, v);
            float edge = fem ? 0.80f : 0.80f + 0.12f * cl((v - line_) / (0.55f * ry));       // the temples curve inwards and down
            float sides = sstep(edge, edge + 0.09f, std::fabs(dxn)) * sstep(hy + (fem ? 0.45f : 0.02f) * ry, hy - (fem ? 0.10f : 0.22f) * ry, v);
            float m = cap * std::max(onTop, sides);
            if (bald) m = cap * sides * sstep(hy - 0.6f * ry, hy - 0.4f * ry, v);
            if (m <= 0) continue;
            float nz = std::sqrt(std::max(0.f, 1 - dx * dx - dy * dy));
            float diff = cl(dx * lx + dy * ly + nz * lz);
            float sweep = u * 0.94f + v * 0.34f * (dxn < -0.38f ? -1.f : 1.f);
            float tex = (vnoise(sweep * (fem ? 90 : 170), (v - u * 0.3f) * 9, f.seed + 3) - 0.5f) * 0.16f;
            float val = hairCol * (0.55f + 0.55f * diff) + tex + 0.16f * std::pow(diff, 6.f) * (1 - grey * 0.5f);
            if (!fem && !bald) val += 0.10f * sstep(0.012f, 0.0f, std::fabs(dxn + 0.38f)) * onTop;    // the parting
            put(g, x, y, cl(val), m);
        }

    // --- spectacles
    if (glasses) {
        float rim = r.rnd(0.10f, 0.30f);
        bool heavy = r.pct(50);
        for (int s = -1; s <= 1; s += 2) {
            float cx = X(hx + s * eyeDX), cy = Y(eyeY + 0.01f);
            float ax = 0.25f * rx * S, ay = 0.15f * ry * S;
            for (int y = (int)(cy - ay - 4); y <= (int)(cy + ay + 4); y++)
                for (int x = (int)(cx - ax - 4); x <= (int)(cx + ax + 4); x++) {
                    if (x < 0 || y < 0 || x >= RW || y >= RH) continue;
                    float qx = (x + 0.5f - cx) / ax, qy = (y + 0.5f - cy) / ay;
                    float d = std::pow(std::pow(std::fabs(qx), 3.f) + std::pow(std::fabs(qy), 3.f), 1 / 3.f);
                    float w = (heavy ? 0.13f : 0.06f) * (qy < 0 ? 1.3f : 1.f);
                    put(g, x, y, rim, sstep(w, w * 0.4f, std::fabs(d - 1)));
                    if (d < 1) put(g, x, y, 0.95f, 0.10f * sstep(0.2f, 0.9f, -qx - qy));
                }
            line(g, X(hx + s * (eyeDX + 0.25f * rx)), Y(eyeY), X(hx + s * rx * 0.98f), Y(eyeY + 0.02f), (heavy ? 0.025f : 0.012f) * S, rim);
        }
        line(g, X(hx - eyeDX + 0.25f * rx), Y(eyeY - 0.01f), X(hx + eyeDX - 0.25f * rx), Y(eyeY - 0.01f), 0.018f * S, rim);
    }

    blur(g, 1.5f);
    Buf h = half(g);
    return finish(h, f.seed, f.era, f.era == ERA_60S ? 0.048f : 0.036f, f.era == ERA_80S ? 1.10f : 1.02f, 0.34f);
}

// ============================================================================ the covert photograph
namespace {
void person(Buf& b, R& r, float cx, float feet, float hgt, float tone, bool hat, bool facing = true) {
    float head = hgt * 0.065f;
    float shoulderY = feet - hgt * 0.80f, hemY = feet - hgt * (r.rnd(0.26f, 0.36f));
    float sw = hgt * r.rnd(0.12f, 0.15f), hw = hgt * r.rnd(0.14f, 0.19f);
    float lean = r.rnd(-0.03f, 0.03f) * hgt;
    // legs
    float step = r.rnd(0.0f, 0.06f) * hgt;
    rect(b, cx - hgt * 0.06f + lean - step * 0.3f, hemY - 2, cx - hgt * 0.015f + lean - step * 0.3f, feet, tone * 0.7f, 1, 1.2f);
    rect(b, cx + hgt * 0.015f + lean + step, hemY - 2, cx + hgt * 0.06f + lean + step, feet, tone * 0.7f, 1, 1.2f);
    // coat
    float xs[4] = {cx - sw * 0.85f + lean, cx + sw * 0.85f + lean, cx + hw, cx - hw}, ys[4] = {shoulderY + hgt * 0.02f, shoulderY + hgt * 0.02f, hemY, hemY};
    quad(b, xs, ys, tone, 1, 1.6f);
    ellipse(b, cx + lean, shoulderY + hgt * 0.05f, sw * 1.02f, hgt * 0.06f, tone, 1, 1.6f);
    line(b, cx - sw + lean, shoulderY + 6, cx - hw * 0.95f, hemY - hgt * 0.12f, hgt * 0.05f, tone * 0.85f);
    line(b, cx + sw + lean, shoulderY + 6, cx + hw * 0.95f, hemY - hgt * 0.12f, hgt * 0.05f, tone * 0.85f);
    if (r.pct(30)) rect(b, cx + hw * 0.8f, hemY - hgt * 0.14f, cx + hw * 0.8f + hgt * 0.11f, hemY - hgt * 0.04f, 0.12f, 1, 1);   // briefcase
    // head
    float hx = cx + lean, hy = shoulderY - head * 0.9f;
    rect(b, hx - head * 0.38f, hy, hx + head * 0.38f, shoulderY + 3, 0.45f, 1, 1);
    ellipse(b, hx, hy, head * 0.74f, head * 0.95f, facing ? 0.58f : 0.25f, 1, 1.4f);
    ellipse(b, hx + head * 0.25f, hy + head * 0.1f, head * 0.35f, head * 0.6f, facing ? 0.42f : 0.2f, 0.6f, 1.4f);
    if (hat) {
        ellipse(b, hx, hy - head * 0.55f, head * 1.35f, head * 0.28f, 0.10f, 1, 1);
        rect(b, hx - head * 0.75f, hy - head * 1.25f, hx + head * 0.75f, hy - head * 0.5f, 0.12f, 1, 1.2f);
    } else ellipse(b, hx, hy - head * 0.35f, head * 0.85f, head * 0.7f, 0.15f, 0.9f, 1.2f);
}
void car(Buf& b, R& r, float x, float ground, float len, float tone) {
    float hgt = len * 0.30f;
    float xs[4] = {x + len * 0.22f, x + len * 0.70f, x + len * 0.80f, x + len * 0.12f}, ys[4] = {ground - hgt * 1.45f, ground - hgt * 1.45f, ground - hgt * 0.85f, ground - hgt * 0.85f};
    quad(b, xs, ys, tone, 1, 1.5f);
    float wx[4] = {x + len * 0.26f, x + len * 0.67f, x + len * 0.75f, x + len * 0.17f}, wy[4] = {ground - hgt * 1.36f, ground - hgt * 1.36f, ground - hgt * 0.92f, ground - hgt * 0.92f};
    quad(b, wx, wy, 0.18f, 1, 1.2f);
    line(b, x + len * 0.30f, ground - hgt * 1.30f, x + len * 0.42f, ground - hgt * 0.95f, 2.5f, 0.55f, 0.5f);
    rect(b, x, ground - hgt * 0.95f, x + len, ground - hgt * 0.25f, tone, 1, 2);
    rect(b, x - 2, ground - hgt * 0.40f, x + len + 2, ground - hgt * 0.30f, 0.80f, 0.8f, 1);
    for (float wx_ : {x + len * 0.20f, x + len * 0.80f}) {
        ellipse(b, wx_, ground - hgt * 0.20f, hgt * 0.38f, hgt * 0.38f, 0.06f, 1, 1.5f);
        ellipse(b, wx_, ground - hgt * 0.20f, hgt * 0.16f, hgt * 0.16f, 0.55f, 0.8f, 1);
    }
    ellipse(b, x + len * 0.04f, ground - hgt * 0.70f, hgt * 0.10f, hgt * 0.10f, 0.85f, 0.8f, 1);
    (void)r;
}
void tree(Buf& b, R& r, float x, float ground, float hgt, uint32_t seed) {
    rect(b, x - hgt * 0.035f, ground - hgt * 0.55f, x + hgt * 0.035f, ground, 0.12f, 1, 1.5f);
    for (int i = 0; i < 9; i++) {
        float cx = x + r.rnd(-0.30f, 0.30f) * hgt, cy = ground - hgt * r.rnd(0.55f, 0.95f), rr = hgt * r.rnd(0.14f, 0.24f);
        int x0 = (int)(cx - rr - 2), x1 = (int)(cx + rr + 2), y0 = (int)(cy - rr - 2), y1 = (int)(cy + rr + 2);
        for (int y = std::max(0, y0); y <= std::min(b.h - 1, y1); y++)
            for (int xx = std::max(0, x0); xx <= std::min(b.w - 1, x1); xx++) {
                float dx = (xx - cx) / rr, dy = (y - cy) / rr;
                float n = fbm(xx * 0.08f, y * 0.08f, seed + i, 3);
                float d = std::sqrt(dx * dx + dy * dy) - (n - 0.5f) * 0.8f;
                put(b, xx, y, 0.16f + 0.22f * n, sstep(1.0f, 0.85f, d));
            }
    }
}
void facade(Buf& b, R& r, float x0, float x1, float top, float bottom, float tone, bool shop) {
    float fh = (bottom - top);
    // roof line: chimneys or a parapet
    for (int k = 0; k < 2 + r.n(3); k++) { float cx = r.rnd(x0, x1); rect(b, cx, top - fh * r.rnd(0.04f, 0.10f), cx + fh * 0.05f, top + 2, tone - 0.12f, 1, 1); }
    rect(b, x0, top, x1, bottom, tone, 1, 0.8f);
    for (int y = (int)std::max(0.f, top); y < (int)std::min((float)b.h, bottom); y++)          // weathering
        for (int x = (int)std::max(0.f, x0); x < (int)std::min((float)b.w, x1); x++) b.at(x, y) += (vnoise(x * 0.05f, y * 0.012f, (uint32_t)(x0 * 7)) - 0.5f) * 0.10f;
    rect(b, x0, top, x1, top + fh * 0.03f, tone + 0.12f, 1, 0.8f);
    for (int k = 1; k < 3; k++) if (r.pct(50)) rect(b, x0, top + fh * k * 0.28f, x1, top + fh * k * 0.28f + 2.5f, tone + 0.10f, 0.9f, 0.8f);
    int rows = 3 + r.n(3), cols = std::max(2, (int)((x1 - x0) / (fh * r.rnd(0.13f, 0.22f))));
    float tall = r.rnd(0.12f, 0.30f);
    float groundF = shop ? 0.30f : 0.18f;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) {
            float cw = (x1 - x0) / cols, rh = fh * (1 - groundF - 0.08f) / rows;
            float wx0 = x0 + j * cw + cw * 0.22f, wx1 = x0 + (j + 1) * cw - cw * 0.22f;
            float wy0 = top + fh * 0.08f + i * rh + rh * tall, wy1 = top + fh * 0.08f + (i + 1) * rh - rh * 0.14f;
            rect(b, wx0 - 1.5f, wy0 - 1.5f, wx1 + 1.5f, wy1 + 1.5f, tone + 0.14f, 0.9f, 0.6f);
            rect(b, wx0, wy0, wx1, wy1, r.pct(18) ? 0.42f : r.rnd(0.16f, 0.30f), 1, 0.6f);
            if (r.pct(40)) rect(b, wx0, wy0, wx1, wy0 + (wy1 - wy0) * r.rnd(0.2f, 0.6f), 0.55f, 0.5f, 0.6f);   // a blind half down
            line(b, (wx0 + wx1) / 2, wy0, (wx0 + wx1) / 2, wy1, 1.2f, tone + 0.15f, 0.8f);
        }
    if (shop) {
        float sy = bottom - fh * groundF;
        rect(b, x0, sy, x1, sy + fh * 0.05f, 0.85f, 1, 0.8f);                      // the sign board
        rect(b, x0 + (x1 - x0) * 0.06f, sy + fh * 0.07f, x0 + (x1 - x0) * 0.62f, bottom - fh * 0.02f, 0.16f, 1, 0.8f);
        for (int k = 0; k < 3; k++) line(b, x0 + (x1 - x0) * (0.12f + k * 0.16f), sy + fh * 0.08f, x0 + (x1 - x0) * (0.18f + k * 0.16f), bottom - fh * 0.04f, 3, 0.42f, 0.35f);
        rect(b, x0 + (x1 - x0) * 0.70f, sy + fh * 0.07f, x0 + (x1 - x0) * 0.86f, bottom, 0.10f, 1, 0.8f);
        float ax[4] = {x0 - 4, x1 + 4, x1 + 14, x0 - 14}, ay[4] = {sy + fh * 0.055f, sy + fh * 0.055f, sy + fh * 0.12f, sy + fh * 0.12f};
        quad(b, ax, ay, 0.30f, 0.95f, 1);
    } else rect(b, (x0 + x1) / 2 - fh * 0.06f, bottom - fh * 0.15f, (x0 + x1) / 2 + fh * 0.06f, bottom, 0.10f, 1, 0.8f);
}
}  // namespace

Img street(uint32_t seed, int scene, int era, int W, int H) {
    R r(seed * 40503u + scene * 7919u + 3);
    const int RW = W * 2, RH = H * 2;
    Buf g(RW, RH, 0.8f);
    float horizon = RH * r.rnd(0.18f, 0.30f), kerb = RH * r.rnd(0.70f, 0.78f);
    // sky, pavement, road
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            float v = y < horizon ? 0.86f - 0.08f * (y / horizon) : (y < kerb ? 0.62f : 0.40f + 0.06f * ((y - kerb) / (RH - kerb)));
            v += (fbm(x * 0.03f, y * 0.03f, seed + 1, 3) - 0.5f) * 0.06f;
            g.at(x, y) = v;
        }
    rect(g, 0, kerb - 3, (float)RW, kerb + 3, 0.78f, 0.9f, 1.5f);
    if (scene != SC_DROP && scene != SC_FLAT) {
        float x = -r.rnd(0, RW * 0.2f);
        while (x < RW) {
            float w = RW * r.rnd(0.22f, 0.45f);
            float top = horizon - RH * r.rnd(-0.05f, 0.18f);
            facade(g, r, x, x + w, std::max(2.f, top), kerb - RH * 0.05f, r.rnd(0.38f, 0.62f), scene == SC_MEET && x > RW * 0.1f && x < RW * 0.55f);
            x += w + r.rnd(0, 6);
        }
    }
    switch (scene) {
        case SC_DROP: {
            // a park wall, a bench, trees
            float wallTop = RH * r.rnd(0.38f, 0.48f);
            for (int t = 0; t < 4; t++) tree(g, r, RW * r.rnd(0.05f, 0.95f), wallTop + RH * 0.04f, RH * r.rnd(0.45f, 0.65f), seed + 50 + t);
            rect(g, 0, wallTop, (float)RW, kerb - RH * 0.02f, 0.50f, 1, 1);
            float bh = RH * 0.035f;
            for (int row = 0; row * bh < kerb - wallTop; row++)
                for (int k = -1; k < RW / (bh * 2.2f) + 1; k++) {
                    float bx = k * bh * 2.2f + (row % 2) * bh * 1.1f, by = wallTop + row * bh;
                    rect(g, bx + 1.5f, by + 1.5f, bx + bh * 2.2f - 1.5f, by + bh - 1.5f, r.rnd(0.30f, 0.46f), 1, 0.6f);
                }
            rect(g, 0, wallTop - RH * 0.02f, (float)RW, wallTop + 2, 0.70f, 1, 1);
            float bx = RW * r.rnd(0.3f, 0.6f), by = kerb - RH * 0.02f;
            for (int k = 0; k < 3; k++) rect(g, bx, by - RH * (0.10f - k * 0.025f), bx + RW * 0.28f, by - RH * (0.09f - k * 0.025f), 0.14f, 1, 1);
            rect(g, bx + 6, by - RH * 0.08f, bx + 12, by + 4, 0.1f); rect(g, bx + RW * 0.28f - 12, by - RH * 0.08f, bx + RW * 0.28f - 6, by + 4, 0.1f);
            break;
        }
        case SC_SIGNAL: {
            float px = RW * r.rnd(0.35f, 0.65f);
            rect(g, px - RW * 0.012f, RH * 0.08f, px + RW * 0.012f, kerb + 2, 0.12f, 1, 1.2f);
            ellipse(g, px, RH * 0.09f, RW * 0.05f, RH * 0.03f, 0.18f);
            rect(g, px - RW * 0.018f, kerb - RH * 0.05f, px + RW * 0.018f, kerb + 4, 0.1f);
            float bx = px + RW * r.rnd(0.12f, 0.25f) * (r.pct(50) ? 1 : -1);
            rect(g, bx - RW * 0.035f, kerb - RH * 0.20f, bx + RW * 0.035f, kerb, 0.20f, 1, 1.5f);
            ellipse(g, bx, kerb - RH * 0.20f, RW * 0.04f, RH * 0.02f, 0.25f);
            rect(g, bx - RW * 0.012f, kerb - RH * 0.17f, bx + RW * 0.012f, kerb - RH * 0.165f, 0.8f);
            rect(g, bx - RW * 0.035f + 3, kerb - RH * 0.20f, bx - RW * 0.035f + 7, kerb, 0.42f, 0.6f);
            for (int k = 0; k < 16; k++) { float rx_ = RW * 0.04f * k; rect(g, rx_, kerb - RH * 0.22f, rx_ + 3, kerb - RH * 0.02f, 0.15f, 0.9f, 0.8f); }
            rect(g, 0, kerb - RH * 0.22f, (float)RW, kerb - RH * 0.21f, 0.15f);
            break;
        }
        case SC_FLAT: {
            // a block of flats seen from across the street: rows of windows, the entrance, a lamp over the door
            float top = RH * r.rnd(0.02f, 0.08f), bottom = kerb - RH * 0.03f, x0 = RW * r.rnd(-0.05f, 0.05f), x1 = RW * r.rnd(0.95f, 1.05f);
            float tone = r.rnd(0.45f, 0.60f);
            rect(g, x0, top, x1, bottom, tone, 1, 0.8f);
            for (int y = (int)top; y < (int)bottom; y++)
                for (int x = (int)std::max(0.f, x0); x < (int)std::min((float)RW, x1); x++) g.at(x, y) += (vnoise(x * 0.04f, y * 0.01f, seed + 77) - 0.5f) * 0.08f;
            int rows = 5 + r.n(2), cols = 6 + r.n(3);
            float fh = bottom - top, cw = (x1 - x0) / cols, rh = fh * 0.80f / rows;
            for (int i = 0; i < rows; i++) {
                rect(g, x0, top + fh * 0.04f + (i + 1) * rh - rh * 0.08f, x1, top + fh * 0.04f + (i + 1) * rh - rh * 0.04f, tone + 0.08f, 0.8f, 0.6f);   // balconies' line
                for (int j = 0; j < cols; j++) {
                    float wx0 = x0 + j * cw + cw * 0.20f, wx1 = x0 + (j + 1) * cw - cw * 0.20f;
                    float wy0 = top + fh * 0.04f + i * rh + rh * 0.18f, wy1 = top + fh * 0.04f + (i + 1) * rh - rh * 0.16f;
                    bool lit = r.pct(22);
                    rect(g, wx0 - 1.5f, wy0 - 1.5f, wx1 + 1.5f, wy1 + 1.5f, tone + 0.12f, 0.9f, 0.6f);
                    rect(g, wx0, wy0, wx1, wy1, lit ? r.rnd(0.70f, 0.85f) : r.rnd(0.14f, 0.28f), 1, 0.6f);
                    if (r.pct(45)) rect(g, wx0, wy0, wx0 + (wx1 - wx0) * 0.35f, wy1, lit ? 0.55f : 0.40f, 0.6f, 0.6f);   // a curtain
                    line(g, (wx0 + wx1) / 2, wy0, (wx0 + wx1) / 2, wy1, 1.2f, tone + 0.14f, 0.8f);
                }
            }
            float dx = RW * r.rnd(0.35f, 0.55f), dw = RW * 0.09f;
            rect(g, dx - dw * 0.7f, bottom - fh * 0.20f, dx + dw * 0.7f, bottom, tone - 0.12f, 1, 0.8f);
            rect(g, dx - dw * 0.45f, bottom - fh * 0.16f, dx + dw * 0.45f, bottom, 0.10f, 1, 0.8f);                  // the door
            rect(g, dx - dw * 0.8f, bottom - fh * 0.21f, dx + dw * 0.8f, bottom - fh * 0.19f, 0.80f, 1, 0.8f);       // the canopy
            ellipse(g, dx, bottom - fh * 0.23f, RW * 0.012f, RH * 0.012f, 0.95f, 1, 1.5f);                             // the lamp
            for (int k = 0; k < 3; k++) rect(g, dx - dw * (0.9f + k * 0.12f), bottom + k * 3.f, dx + dw * (0.9f + k * 0.12f), bottom + (k + 1) * 3.f, 0.66f - k * 0.04f, 1, 0.6f);
            break;
        }
        default: break;
    }
    // traffic
    if (scene == SC_ROUTE || scene == SC_WATCHERS || scene == SC_MEET || scene == SC_ARREST || r.pct(40)) {
        int nc = scene == SC_WATCHERS ? 1 : 1 + r.n(2);
        for (int i = 0; i < nc; i++) {
            float len = RW * r.rnd(0.30f, 0.42f);
            car(g, r, RW * r.rnd(-0.1f, 0.75f), RH * r.rnd(0.86f, 0.96f), len, r.rnd(0.10f, 0.55f));
        }
    }
    // people
    int crowd = scene == SC_ROUTE ? 4 + r.n(3) : (scene == SC_DROP || scene == SC_SIGNAL || scene == SC_FLAT) ? r.n(2) : 1 + r.n(3);
    for (int i = 0; i < crowd; i++) {
        float feet = kerb - RH * r.rnd(0.0f, 0.04f);
        float tall = scene == SC_FLAT ? RH * r.rnd(0.13f, 0.16f) : RH * r.rnd(0.30f, 0.40f);
        person(g, r, RW * r.rnd(0.05f, 0.95f), feet, tall, r.pct(70) ? r.rnd(0.10f, 0.22f) : r.rnd(0.50f, 0.65f), r.pct(era == ERA_60S ? 70 : 35));
    }
    if (scene == SC_CANDID || scene == SC_WATCHERS || scene == SC_ARREST) {
        // the person the picture is about: nearer the camera
        float cx = RW * r.rnd(0.35f, 0.62f);
        person(g, r, cx, RH * r.rnd(0.90f, 0.97f), RH * r.rnd(0.58f, 0.72f), r.pct(60) ? r.rnd(0.10f, 0.20f) : r.rnd(0.50f, 0.62f), r.pct(era == ERA_60S ? 65 : 30));
        if (scene == SC_ARREST) person(g, r, cx + RW * 0.16f, RH * 0.95f, RH * 0.66f, 0.15f, true);
    }
    blur(g, 1.4f);
    // something in the way: a door frame, a curtain, the edge of a car window
    if (r.pct(75)) {
        bool left = r.pct(50);
        float wdt = RW * r.rnd(0.08f, 0.20f);
        Buf occ(RW, RH, 0);
        for (int y = 0; y < RH; y++)
            for (int x = 0; x < RW; x++) {
                float d = left ? x : RW - 1 - x;
                occ.at(x, y) = d < wdt ? 1.f : 0.f;
            }
        if (r.pct(50)) for (int y = 0; y < RH; y++) for (int x = 0; x < RW; x++) if (y > RH * 0.88f) occ.at(x, y) = 1;
        blur(occ, RW * 0.03f);
        float dark = r.rnd(0.04f, 0.14f);
        for (size_t i = 0; i < g.v.size(); i++) g.v[i] = mix(g.v[i], dark, occ.v[i]);
    }
    Buf h = half(g);
    blur(h, 0.5f);
    return finish(h, seed * 31 + scene, era, 0.075f, 1.35f, 0.55f);
}

// ============================================================================ textures
Img paperGrain(int N, uint32_t seed) {
    Img im; im.w = im.h = N; im.px.resize((size_t)N * N);
    R r(seed);
    std::vector<float> a((size_t)N * N, 0);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float n = fbm(x / 16.f, y / 16.f, seed, 4, N / 16);
            float fine = h01(x, y, seed + 9);
            a[(size_t)y * N + x] = cl((n - 0.45f) * 0.35f) + (fine > 0.985f ? 0.25f : 0) + (fine < 0.004f ? 0.4f : 0);
        }
    // fibres
    for (int k = 0; k < N / 6; k++) {
        float x = r.f() * N, y = r.f() * N, ang = r.f() * 6.283f, len = r.rnd(4, 16);
        for (float t = 0; t < len; t += 0.5f) {
            int ix = ((int)(x + std::cos(ang) * t) % N + N) % N, iy = ((int)(y + std::sin(ang) * t) % N + N) % N;
            a[(size_t)iy * N + ix] = std::max(a[(size_t)iy * N + ix], 0.22f);
            ang += r.rnd(-0.15f, 0.15f);
        }
    }
    for (size_t i = 0; i < a.size(); i++) im.px[i] = ((uint32_t)(cl(a[i]) * 255) << 24) | (0x2Au << 16) | (0x3Cu << 8) | 0x55u;
    return im;
}

Img wood(int N, uint32_t seed) {
    Img im; im.w = im.h = N; im.px.resize((size_t)N * N);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float warp = fbm(x / 64.f, y / 8.f, seed, 4, N / 64) * 3.0f;
            float grain = 0.5f + 0.5f * std::sin((y / (float)N) * 6.2831853f * 14 + warp * 3.1f);
            float fine = fbm(x / 4.f, y / 1.5f, seed + 5, 2, N / 4);
            float v = 0.55f + 0.25f * grain + 0.20f * (fine - 0.5f);
            float rr = 0.25f + 0.17f * v, gg = 0.16f + 0.11f * v, bb = 0.10f + 0.06f * v;
            im.px[(size_t)y * N + x] = 0xFF000000u | ((uint32_t)(cl(bb) * 255) << 16) | ((uint32_t)(cl(gg) * 255) << 8) | (uint32_t)(cl(rr) * 255);
        }
    return im;
}

Img wear(int N, uint32_t seed) {
    Img im; im.w = im.h = N; im.px.resize((size_t)N * N);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float n = fbm(x / 10.f, y / 10.f, seed, 4, N / 10 > 0 ? N / 10 : 1);
            float s = h01(x, y, seed + 3);
            float a = cl((n - 0.58f) * 5.0f) + (s > 0.93f ? 0.7f : 0);
            im.px[(size_t)y * N + x] = ((uint32_t)(cl(a) * 255) << 24) | 0x00FFFFFFu;
        }
    return im;
}

bool saveRaw(const Img& im, const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    int hdr[2] = {im.w, im.h};
    std::fwrite(hdr, sizeof hdr, 1, f);
    std::fwrite(im.px.data(), 4, im.px.size(), f);
    std::fclose(f);
    return true;
}

}  // namespace art
