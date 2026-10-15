#include "runtime/vehicle_box_contact.hpp"

#include "assets/assets.hpp"
#include "runtime/vehicle_collision.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr float kSimdEpsilon = 1.1920929e-07f;
constexpr float kBoxBoxFudge = 1.05f;
constexpr float kBoxBoxEdgeFudge = 1.0e-5f;
constexpr float kLineApproachLimit = 0.0001f;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kUpperLimit = 1e10f;
constexpr int kManifoldPoints = 4;
constexpr int kBoxBoxMaxContacts = 4;
// The float 0x376c5c (0x3dcccccd in the ELF data): 0x332368 scales a contact row's angular component of body A (flag word bit 0) or B (bit 1).
constexpr float kContactAngularScale = 0x1.99999ap-4f;

Vector3 add(const Vector3& a, const Vector3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
Vector3 sub(const Vector3& a, const Vector3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vector3 scale(const Vector3& a, float s) { return {a[0] * s, a[1] * s, a[2] * s}; }
Vector3 neg(const Vector3& a) { return {-a[0], -a[1], -a[2]}; }
Vector3 cross(const Vector3& a, const Vector3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
float dot(const Vector3& a, const Vector3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
float length2(const Vector3& a) { return dot(a, a); }

std::uint32_t bits(float value) {
    std::uint32_t out;
    std::memcpy(&out, &value, 4);
    return out;
}
bool aboveEpsilon(float value) { return std::int32_t(bits(value)) > std::int32_t(bits(kSimdEpsilon)); }

float dot14(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[4] + a[2] * b[8]; }
float dot41(const float* a, const float* b) { return a[0] * b[0] + a[4] * b[1] + a[8] * b[2]; }
float dot44(const float* a, const float* b) { return a[0] * b[0] + a[4] * b[4] + a[8] * b[8]; }

void lineClosestApproach(const Vector3& pa, const Vector3& ua, const Vector3& pb, const Vector3& ub, float& alpha, float& beta) {
    const Vector3 p = sub(pb, pa);
    const float uaub = dot(ua, ub);
    const float q1 = dot(ua, p);
    const float q2 = -dot(ub, p);
    float d = 1 - uaub * uaub;
    if (std::int32_t(bits(d)) <= std::int32_t(bits(kLineApproachLimit))) {
        alpha = 0;
        beta = 0;
    } else {
        d = 1.0f / d;
        alpha = (q1 + uaub * q2) * d;
        beta = (uaub * q1 + q2) * d;
    }
}

int intersectRectQuad(const float h[2], float p[8], float ret[16]) {
    int nq = 4, nr = 0;
    float buffer[16];
    float* q = p;
    float* r = ret;
    for (int dir = 0; dir <= 1; dir++) {
        for (int sign = -1; sign <= 1; sign += 2) {
            float* pq = q;
            float* pr = r;
            nr = 0;
            for (int i = nq; i > 0; i--) {
                if (float(sign) * pq[dir] < h[dir]) {
                    pr[0] = pq[0];
                    pr[1] = pq[1];
                    pr += 2;
                    nr++;
                    if (nr & 8) {
                        q = r;
                        goto done;
                    }
                }
                float* nextq = (i > 1) ? pq + 2 : q;
                if ((float(sign) * pq[dir] < h[dir]) ^ (float(sign) * nextq[dir] < h[dir])) {
                    pr[1 - dir] = pq[1 - dir] + (nextq[1 - dir] - pq[1 - dir]) / (nextq[dir] - pq[dir]) * (float(sign) * h[dir] - pq[dir]);
                    pr[dir] = float(sign) * h[dir];
                    pr += 2;
                    nr++;
                    if (nr & 8) {
                        q = r;
                        goto done;
                    }
                }
                pq += 2;
            }
            q = r;
            r = (q == ret) ? buffer : ret;
            nq = nr;
        }
    }
done:
    if (q != ret) std::memcpy(ret, q, std::size_t(nr) * 2 * sizeof(float));
    return nr;
}

void cullPoints(int n, const float p[], int m, int i0, int iret[]) {
    float a, cx, cy, q;
    if (n == 1) {
        cx = p[0];
        cy = p[1];
    } else if (n == 2) {
        cx = 0.5f * (p[0] + p[2]);
        cy = 0.5f * (p[1] + p[3]);
    } else {
        a = 0;
        cx = 0;
        cy = 0;
        for (int i = 0; i < (n - 1); i++) {
            q = p[i * 2] * p[i * 2 + 3] - p[i * 2 + 2] * p[i * 2 + 1];
            a += q;
            cx += q * (p[i * 2] + p[i * 2 + 2]);
            cy += q * (p[i * 2 + 1] + p[i * 2 + 3]);
        }
        q = p[n * 2 - 2] * p[1] - p[0] * p[n * 2 - 1];
        if (std::fabs(a + q) > kSimdEpsilon) a = 1.0f / (3.0f * (a + q));
        else a = kBulletLargeFloat;
        cx = a * (cx + q * (p[n * 2 - 2] + p[0]));
        cy = a * (cy + q * (p[n * 2 - 1] + p[1]));
    }
    float angle[8];
    for (int i = 0; i < n; i++) angle[i] = std::atan2(p[i * 2 + 1] - cy, p[i * 2] - cx);
    int avail[8];
    for (int i = 0; i < n; i++) avail[i] = 1;
    avail[i0] = 0;
    iret[0] = i0;
    iret++;
    for (int j = 1; j < m; j++) {
        a = float(j) * (2 * kPi / float(m)) + angle[i0];
        if (a > kPi) a -= 2 * kPi;
        float maxdiff = 1e9f, diff;
        *iret = i0;
        for (int i = 0; i < n; i++) {
            if (avail[i]) {
                diff = std::fabs(angle[i] - a);
                if (diff > kPi) diff = 2 * kPi - diff;
                if (diff < maxdiff) {
                    maxdiff = diff;
                    *iret = i;
                }
            }
        }
        avail[*iret] = 0;
        iret++;
    }
}

void emit(std::vector<BoxBoxContact>& out, const Vector3& normal, const Vector3& point, float depth) { out.push_back({neg(normal), point, depth}); }

int boxBox(const Vector3& p1, const float R1[12], const float side1[3], const Vector3& p2, const float R2[12], const float side2[3],
           std::vector<BoxBoxContact>& out) {
    int maxc = kBoxBoxMaxContacts;
    Vector3 p, pp, normalC = {0, 0, 0}, normal;
    const float* normalR = nullptr;
    float A[3], B[3], R11, R12, R13, R21, R22, R23, R31, R32, R33, Q11, Q12, Q13, Q21, Q22, Q23, Q31, Q32, Q33, s, s2, l;
    int i, j, invertNormal, code;

    p = sub(p2, p1);
    pp = {dot41(R1 + 0, p.data()), dot41(R1 + 1, p.data()), dot41(R1 + 2, p.data())};
    for (i = 0; i < 3; ++i) {
        A[i] = side1[i] * 0.5f;
        B[i] = side2[i] * 0.5f;
    }
    R11 = dot44(R1 + 0, R2 + 0);
    R12 = dot44(R1 + 0, R2 + 1);
    R13 = dot44(R1 + 0, R2 + 2);
    R21 = dot44(R1 + 1, R2 + 0);
    R22 = dot44(R1 + 1, R2 + 1);
    R23 = dot44(R1 + 1, R2 + 2);
    R31 = dot44(R1 + 2, R2 + 0);
    R32 = dot44(R1 + 2, R2 + 1);
    R33 = dot44(R1 + 2, R2 + 2);
    Q11 = std::fabs(R11);
    Q12 = std::fabs(R12);
    Q13 = std::fabs(R13);
    Q21 = std::fabs(R21);
    Q22 = std::fabs(R22);
    Q23 = std::fabs(R23);
    Q31 = std::fabs(R31);
    Q32 = std::fabs(R32);
    Q33 = std::fabs(R33);

    s = -3.4028234663852886e+38f;
    invertNormal = 0;
    code = 0;
    const auto faceTest = [&](float expr1, float expr2, const float* norm, int cc) {
        s2 = std::fabs(expr1) - expr2;
        if (s2 > 0) return false;
        if (s2 > s) {
            s = s2;
            normalR = norm;
            invertNormal = expr1 < 0;
            code = cc;
        }
        return true;
    };
    if (!faceTest(pp[0], A[0] + B[0] * Q11 + B[1] * Q12 + B[2] * Q13, R1 + 0, 1)) return 0;
    if (!faceTest(pp[1], A[1] + B[0] * Q21 + B[1] * Q22 + B[2] * Q23, R1 + 1, 2)) return 0;
    if (!faceTest(pp[2], A[2] + B[0] * Q31 + B[1] * Q32 + B[2] * Q33, R1 + 2, 3)) return 0;
    if (!faceTest(dot41(R2 + 0, p.data()), A[0] * Q11 + A[1] * Q21 + A[2] * Q31 + B[0], R2 + 0, 4)) return 0;
    if (!faceTest(dot41(R2 + 1, p.data()), A[0] * Q12 + A[1] * Q22 + A[2] * Q32 + B[1], R2 + 1, 5)) return 0;
    if (!faceTest(dot41(R2 + 2, p.data()), A[0] * Q13 + A[1] * Q23 + A[2] * Q33 + B[2], R2 + 2, 6)) return 0;

    const auto edgeTest = [&](float expr1, float expr2, float n1, float n2, float n3, int cc) {
        s2 = std::fabs(expr1) - expr2;
        if (aboveEpsilon(s2)) return false;
        l = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
        if (aboveEpsilon(l)) {
            s2 /= l;
            if (s2 * kBoxBoxFudge > s) {
                s = s2;
                normalR = nullptr;
                normalC = {n1 / l, n2 / l, n3 / l};
                invertNormal = expr1 < 0;
                code = cc;
            }
        }
        return true;
    };
    Q11 += kBoxBoxEdgeFudge;
    Q12 += kBoxBoxEdgeFudge;
    Q13 += kBoxBoxEdgeFudge;
    Q21 += kBoxBoxEdgeFudge;
    Q22 += kBoxBoxEdgeFudge;
    Q23 += kBoxBoxEdgeFudge;
    Q31 += kBoxBoxEdgeFudge;
    Q32 += kBoxBoxEdgeFudge;
    Q33 += kBoxBoxEdgeFudge;
    if (!edgeTest(pp[2] * R21 - pp[1] * R31, A[1] * Q31 + A[2] * Q21 + B[1] * Q13 + B[2] * Q12, 0, -R31, R21, 7)) return 0;
    if (!edgeTest(pp[2] * R22 - pp[1] * R32, A[1] * Q32 + A[2] * Q22 + B[0] * Q13 + B[2] * Q11, 0, -R32, R22, 8)) return 0;
    if (!edgeTest(pp[2] * R23 - pp[1] * R33, A[1] * Q33 + A[2] * Q23 + B[0] * Q12 + B[1] * Q11, 0, -R33, R23, 9)) return 0;
    if (!edgeTest(pp[0] * R31 - pp[2] * R11, A[0] * Q31 + A[2] * Q11 + B[1] * Q23 + B[2] * Q22, R31, 0, -R11, 10)) return 0;
    if (!edgeTest(pp[0] * R32 - pp[2] * R12, A[0] * Q32 + A[2] * Q12 + B[0] * Q23 + B[2] * Q21, R32, 0, -R12, 11)) return 0;
    if (!edgeTest(pp[0] * R33 - pp[2] * R13, A[0] * Q33 + A[2] * Q13 + B[0] * Q22 + B[1] * Q21, R33, 0, -R13, 12)) return 0;
    if (!edgeTest(pp[1] * R11 - pp[0] * R21, A[0] * Q21 + A[1] * Q11 + B[1] * Q33 + B[2] * Q32, -R21, R11, 0, 13)) return 0;
    if (!edgeTest(pp[1] * R12 - pp[0] * R22, A[0] * Q22 + A[1] * Q12 + B[0] * Q33 + B[2] * Q31, -R22, R12, 0, 14)) return 0;
    if (!edgeTest(pp[1] * R13 - pp[0] * R23, A[0] * Q23 + A[1] * Q13 + B[0] * Q32 + B[1] * Q31, -R23, R13, 0, 15)) return 0;

    if (!code) return 0;
    if (normalR) normal = {normalR[0], normalR[4], normalR[8]};
    else normal = {dot(Vector3{R1[0], R1[1], R1[2]}, normalC), dot(Vector3{R1[4], R1[5], R1[6]}, normalC), dot(Vector3{R1[8], R1[9], R1[10]}, normalC)};
    if (invertNormal) normal = neg(normal);
    const float depth = -s;

    if (code > 6) {
        Vector3 pa = p1;
        float sign;
        for (j = 0; j < 3; j++) {
            sign = (dot14(normal.data(), R1 + j) > 0) ? 1.0f : -1.0f;
            for (i = 0; i < 3; i++) pa[std::size_t(i)] += sign * A[j] * R1[i * 4 + j];
        }
        Vector3 pb = p2;
        for (j = 0; j < 3; j++) {
            sign = (dot14(normal.data(), R2 + j) > 0) ? -1.0f : 1.0f;
            for (i = 0; i < 3; i++) pb[std::size_t(i)] += sign * B[j] * R2[i * 4 + j];
        }
        float alpha, beta;
        Vector3 ua, ub;
        for (i = 0; i < 3; i++) ua[std::size_t(i)] = R1[((code)-7) / 3 + i * 4];
        for (i = 0; i < 3; i++) ub[std::size_t(i)] = R2[((code)-7) % 3 + i * 4];
        lineClosestApproach(pa, ua, pb, ub, alpha, beta);
        for (i = 0; i < 3; i++) pa[std::size_t(i)] += ua[std::size_t(i)] * alpha;
        for (i = 0; i < 3; i++) pb[std::size_t(i)] += ub[std::size_t(i)] * beta;
        emit(out, normal, pb, -depth);
        return 1;
    }

    const float *Ra, *Rb, *Sa, *Sb;
    Vector3 pa, pb;
    if (code <= 3) {
        Ra = R1;
        Rb = R2;
        pa = p1;
        pb = p2;
        Sa = A;
        Sb = B;
    } else {
        Ra = R2;
        Rb = R1;
        pa = p2;
        pb = p1;
        Sa = B;
        Sb = A;
    }
    const Vector3 normal2 = code <= 3 ? normal : neg(normal);
    const Vector3 nr = {dot41(Rb + 0, normal2.data()), dot41(Rb + 1, normal2.data()), dot41(Rb + 2, normal2.data())};
    const Vector3 anr = {std::fabs(nr[0]), std::fabs(nr[1]), std::fabs(nr[2])};
    int lanr, a1, a2;
    if (anr[1] > anr[0]) {
        if (anr[1] > anr[2]) {
            a1 = 0;
            lanr = 1;
            a2 = 2;
        } else {
            a1 = 0;
            a2 = 1;
            lanr = 2;
        }
    } else {
        if (anr[0] > anr[2]) {
            lanr = 0;
            a1 = 1;
            a2 = 2;
        } else {
            a1 = 0;
            a2 = 1;
            lanr = 2;
        }
    }
    Vector3 center;
    if (nr[std::size_t(lanr)] < 0) {
        for (i = 0; i < 3; i++) center[std::size_t(i)] = pb[std::size_t(i)] - pa[std::size_t(i)] + Sb[lanr] * Rb[i * 4 + lanr];
    } else {
        for (i = 0; i < 3; i++) center[std::size_t(i)] = pb[std::size_t(i)] - pa[std::size_t(i)] - Sb[lanr] * Rb[i * 4 + lanr];
    }
    int codeN, code1, code2;
    codeN = code <= 3 ? code - 1 : code - 4;
    if (codeN == 0) {
        code1 = 1;
        code2 = 2;
    } else if (codeN == 1) {
        code1 = 0;
        code2 = 2;
    } else {
        code1 = 0;
        code2 = 1;
    }
    float quad[8];
    const float c1 = dot14(center.data(), Ra + code1);
    const float c2 = dot14(center.data(), Ra + code2);
    float m11 = dot44(Ra + code1, Rb + a1);
    float m12 = dot44(Ra + code1, Rb + a2);
    float m21 = dot44(Ra + code2, Rb + a1);
    float m22 = dot44(Ra + code2, Rb + a2);
    {
        const float k1 = m11 * Sb[a1];
        const float k2 = m21 * Sb[a1];
        const float k3 = m12 * Sb[a2];
        const float k4 = m22 * Sb[a2];
        quad[0] = c1 - k1 - k3;
        quad[1] = c2 - k2 - k4;
        quad[2] = c1 - k1 + k3;
        quad[3] = c2 - k2 + k4;
        quad[4] = c1 + k1 + k3;
        quad[5] = c2 + k2 + k4;
        quad[6] = c1 + k1 - k3;
        quad[7] = c2 + k2 - k4;
    }
    const float rect[2] = {Sa[code1], Sa[code2]};
    float ret[16];
    const int n = intersectRectQuad(rect, quad, ret);
    if (n < 1) return 0;
    float point[3 * 8];
    float dep[8];
    const float det1 = 1.0f / (m11 * m22 - m12 * m21);
    m11 *= det1;
    m12 *= det1;
    m21 *= det1;
    m22 *= det1;
    int cnum = 0;
    for (j = 0; j < n; j++) {
        const float k1 = m22 * (ret[j * 2] - c1) - m12 * (ret[j * 2 + 1] - c2);
        const float k2 = -m21 * (ret[j * 2] - c1) + m11 * (ret[j * 2 + 1] - c2);
        for (i = 0; i < 3; i++) point[cnum * 3 + i] = center[std::size_t(i)] + k1 * Rb[i * 4 + a1] + k2 * Rb[i * 4 + a2];
        dep[cnum] = Sa[codeN] - dot(normal2, Vector3{point[cnum * 3], point[cnum * 3 + 1], point[cnum * 3 + 2]});
        if (dep[cnum] >= 0) {
            ret[cnum * 2] = ret[j * 2];
            ret[cnum * 2 + 1] = ret[j * 2 + 1];
            cnum++;
        }
    }
    if (cnum < 1) return 0;
    if (maxc > cnum) maxc = cnum;
    if (maxc < 1) maxc = 1;
    if (cnum <= maxc) {
        for (j = 0; j < cnum; j++) {
            Vector3 world;
            for (i = 0; i < 3; i++) world[std::size_t(i)] = point[j * 3 + i] + pa[std::size_t(i)];
            if (code >= 4) world = sub(world, scale(normal, dep[j]));
            emit(out, normal, world, -dep[j]);
        }
    } else {
        int i1 = 0;
        float maxdepth = dep[0];
        for (i = 1; i < cnum; i++) {
            if (dep[i] > maxdepth) {
                maxdepth = dep[i];
                i1 = i;
            }
        }
        int iret[8];
        cullPoints(cnum, ret, maxc, i1, iret);
        for (j = 0; j < maxc; j++) {
            Vector3 world;
            for (i = 0; i < 3; i++) world[std::size_t(i)] = point[iret[j] * 3 + i] + pa[std::size_t(i)];
            if (code >= 4) world = sub(world, scale(normal, dep[iret[j]]));
            emit(out, normal, world, -dep[iret[j]]);
        }
        cnum = maxc;
    }
    return cnum;
}

void toRows(const BulletTransform& t, float R[12]) {
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) R[4 * i + k] = t.basis[std::size_t(i)][std::size_t(k)];
        R[4 * i + 3] = 0.0f;
    }
}

void removeManifoldPoint(ContactManifold& manifold, std::size_t index) {
    const std::size_t last = manifold.points.size() - 1;
    if (index != last) {
        manifold.points[index] = manifold.points[last];
    }
    manifold.points.pop_back();
}

int cacheEntry(const ContactManifold& manifold, const ManifoldPoint& point) {
    float shortest = manifold.breakingThreshold * manifold.breakingThreshold;
    int nearest = -1;
    for (std::size_t i = 0; i < manifold.points.size(); ++i) {
        const Vector3 diff = sub(manifold.points[i].localA, point.localA);
        const float distance = dot(diff, diff);
        if (distance < shortest) {
            shortest = distance;
            nearest = int(i);
        }
    }
    return nearest;
}

int sortCachedPoints(const ContactManifold& manifold, const ManifoldPoint& point) {
    const std::vector<ManifoldPoint>& cache = manifold.points;
    int maxPenetrationIndex = -1;
    float maxPenetration = point.distance;
    for (int i = 0; i < 4; i++) {
        if (cache[std::size_t(i)].distance < maxPenetration) {
            maxPenetrationIndex = i;
            maxPenetration = cache[std::size_t(i)].distance;
        }
    }
    float res[4] = {0, 0, 0, 0};
    if (maxPenetrationIndex != 0) res[0] = length2(cross(sub(point.localA, cache[1].localA), sub(cache[3].localA, cache[2].localA)));
    if (maxPenetrationIndex != 1) res[1] = length2(cross(sub(point.localA, cache[0].localA), sub(cache[3].localA, cache[2].localA)));
    if (maxPenetrationIndex != 2) res[2] = length2(cross(sub(point.localA, cache[0].localA), sub(cache[3].localA, cache[1].localA)));
    if (maxPenetrationIndex != 3) res[3] = length2(cross(sub(point.localA, cache[0].localA), sub(cache[2].localA, cache[1].localA)));
    int maxIndex = -1;
    float maxValue = -kBulletLargeFloat;
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(res[i]) > maxValue) {
            maxIndex = i;
            maxValue = std::fabs(res[i]);
        }
    }
    return maxIndex;
}

struct SolverBody {
    VehicleRigidBody* body = nullptr;
    Vector3 deltaLinear{}, deltaAngular{};
    float inverseMass() const { return body ? body->inverseMass : 0.0f; }
    Vector3 velocity() const { return body ? body->velocity : Vector3{0, 0, 0}; }
    Vector3 angularVelocity() const { return body ? body->angularVelocity : Vector3{0, 0, 0}; }
    Vector3 inverseInertia(const Vector3& v) const { return body ? worldInverseInertia(*body, v) : Vector3{0, 0, 0}; }
    Vector3 centre(const Vector3& otherwise) const { return body ? body->position : otherwise; }
};

void applySolverImpulse(SolverBody& body, const Vector3& linear, const Vector3& angular, float magnitude, bool flag) {
    if (body.inverseMass() == 0.0f) return;
    if (flag) {
        body.deltaLinear[0] += linear[0] * magnitude;
        body.deltaLinear[1] += 0.0f * magnitude;
        body.deltaLinear[2] += linear[2] * magnitude;
        body.deltaAngular[0] += 0.0f * (1.0f * magnitude);
        body.deltaAngular[1] += angular[1] * (1.0f * magnitude);
        body.deltaAngular[2] += 0.0f * (1.0f * magnitude);
        return;
    }
    for (std::size_t a = 0; a < 3; ++a) {
        body.deltaLinear[a] += linear[a] * magnitude;
        body.deltaAngular[a] += angular[a] * (1.0f * magnitude);
    }
}

struct SolverConstraint {
    Vector3 relpos1CrossNormal{}, contactNormal{}, relpos2CrossNormal{}, angularComponentA{}, angularComponentB{};
    float appliedImpulse = 0, friction = 0, jacDiagABInv = 0, rhs = 0, cfm = 0, lowerLimit = 0, upperLimit = kUpperLimit;
    int frictionIndex = 0;
    int bodyA = 0, bodyB = 1;
    ManifoldPoint* point = nullptr;
    bool flag = false;
};

void resolveRow(std::vector<SolverBody>& bodies, SolverConstraint& c, bool upper) {
    SolverBody& a = bodies[std::size_t(c.bodyA)];
    SolverBody& b = bodies[std::size_t(c.bodyB)];
    float deltaImpulse = c.rhs - c.appliedImpulse * c.cfm;
    const float deltaVel1Dotn = dot(c.contactNormal, a.deltaLinear) + dot(c.relpos1CrossNormal, a.deltaAngular);
    const float deltaVel2Dotn = dot(c.relpos2CrossNormal, b.deltaAngular) - dot(c.contactNormal, b.deltaLinear);
    deltaImpulse -= deltaVel1Dotn * c.jacDiagABInv;
    deltaImpulse -= deltaVel2Dotn * c.jacDiagABInv;
    const float sum = c.appliedImpulse + deltaImpulse;
    if (c.lowerLimit > sum) {
        deltaImpulse = c.lowerLimit - c.appliedImpulse;
        c.appliedImpulse = c.lowerLimit;
    } else if (upper && !(c.upperLimit >= sum)) {
        deltaImpulse = c.upperLimit - c.appliedImpulse;
        c.appliedImpulse = c.upperLimit;
    } else {
        c.appliedImpulse = sum;
    }
    applySolverImpulse(a, scale(c.contactNormal, a.inverseMass()), c.angularComponentA, deltaImpulse, c.flag);
    applySolverImpulse(b, scale(neg(c.contactNormal), b.inverseMass()), c.angularComponentB, deltaImpulse, c.flag);
}

SolverConstraint frictionConstraint(const std::vector<SolverBody>& bodies, int bodyA, int bodyB, const Vector3& normal, ManifoldPoint& point,
                                    const Vector3& relPos1, const Vector3& relPos2) {
    const SolverBody& a = bodies[std::size_t(bodyA)];
    const SolverBody& b = bodies[std::size_t(bodyB)];
    SolverConstraint c;
    c.bodyA = bodyA;
    c.bodyB = bodyB;
    c.flag = (point.flags & 4) != 0;
    c.contactNormal = normal;
    c.friction = point.friction;
    c.relpos1CrossNormal = cross(relPos1, normal);
    c.angularComponentA = a.inverseInertia(c.relpos1CrossNormal);
    c.relpos2CrossNormal = cross(relPos2, neg(normal));
    c.angularComponentB = b.inverseInertia(c.relpos2CrossNormal);
    const float denom0 = a.body ? dot(normal, cross(c.angularComponentA, relPos1)) + a.inverseMass() : 0.0f;
    const float denom1 = b.body ? dot(normal, cross(neg(c.angularComponentB), relPos2)) + b.inverseMass() : 0.0f;
    c.jacDiagABInv = 1.0f / (denom0 + denom1);
    const float vel1Dotn = dot(normal, a.velocity()) + dot(c.relpos1CrossNormal, a.angularVelocity());
    const float vel2Dotn = dot(c.relpos2CrossNormal, b.angularVelocity()) - dot(normal, b.velocity());
    c.rhs = c.jacDiagABInv * (0.0f - (vel1Dotn + vel2Dotn));
    c.cfm = 0;
    c.lowerLimit = 0;
    c.upperLimit = kUpperLimit;
    return c;
}

void planeSpace(const Vector3& n, Vector3& p, Vector3& q) {
    if (std::fabs(n[2]) > 0.70710678f) {
        const float a = n[1] * n[1] + n[2] * n[2];
        const float k = 1.0f / std::sqrt(a);
        p = {0, -n[2] * k, n[1] * k};
        q = {a * k, -n[0] * p[2], n[0] * p[1]};
    } else {
        const float a = n[0] * n[0] + n[1] * n[1];
        const float k = 1.0f / std::sqrt(a);
        p = {-n[1] * k, n[0] * k, 0};
        q = {-n[2] * p[1], n[2] * p[0], a * k};
    }
}

}

Vector3 bulletApply(const BulletTransform& t, const Vector3& local) {
    return {dot(local, t.basis[0]) + t.origin[0], dot(local, t.basis[1]) + t.origin[1], dot(local, t.basis[2]) + t.origin[2]};
}

Vector3 bulletInverseApply(const BulletTransform& t, const Vector3& world) {
    const Vector3 v = sub(world, t.origin);
    return {t.basis[0][0] * v[0] + t.basis[1][0] * v[1] + t.basis[2][0] * v[2], t.basis[0][1] * v[0] + t.basis[1][1] * v[1] + t.basis[2][1] * v[2],
            t.basis[0][2] * v[0] + t.basis[1][2] * v[1] + t.basis[2][2] * v[2]};
}

BulletTransform bulletBodyTransform(const VehicleRigidBody& body) {
    const std::array<Vector3, 3> axes = bodyAxes(body);
    BulletTransform t;
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t k = 0; k < 3; ++k) t.basis[i][k] = axes[k][i];
    t.origin = body.position;
    return t;
}

BulletTransform bulletTransformFromMatrix(const float matrix[16]) {
    BulletTransform t;
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t k = 0; k < 3; ++k) t.basis[i][k] = matrix[k * 4 + i];
    t.origin = {matrix[12], matrix[13], matrix[14]};
    return t;
}

BulletBox bulletBox(const BulletTransform& transform, const Vector3& halfExtents) {
    BulletBox box;
    box.transform = transform;
    box.implicitHalf = {halfExtents[0] - box.margin, halfExtents[1] - box.margin, halfExtents[2] - box.margin};
    return box;
}

Vector3 bulletBoxHalfExtents(const BulletBox& box) {
    return {box.implicitHalf[0] + box.margin, box.implicitHalf[1] + box.margin, box.implicitHalf[2] + box.margin};
}

void bulletBoxAabb(const BulletBox& box, const BulletTransform& transform, Vector3& low, Vector3& high) {
    const Vector3 half = bulletBoxHalfExtents(box);
    for (std::size_t i = 0; i < 3; ++i) {
        const Vector3 row = {std::fabs(transform.basis[i][0]), std::fabs(transform.basis[i][1]), std::fabs(transform.basis[i][2])};
        const float extent = dot(half, row);
        low[i] = transform.origin[i] - extent;
        high[i] = transform.origin[i] + extent;
    }
}

float bulletBoxBreakingThreshold(const BulletBox& box) {
    BulletTransform identity;
    identity.basis = {Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1}};
    Vector3 low, high;
    bulletBoxAabb(box, identity, low, high);
    const float radius = std::sqrt(length2(sub(high, low))) * 0.5f;
    const Vector3 centre = scale(add(low, high), 0.5f);
    return (radius + std::sqrt(length2(centre))) * kBulletContactBreakingThreshold;
}

std::vector<BulletBox> levelVehicleBoxes(const assets::Model& model) {
    static const char kPrefix[] = "collision_vehicle";
    const auto matches = [](const std::string& name) {
        for (std::size_t i = 0; i < sizeof(kPrefix) - 1; ++i) {
            const char c = i < name.size() ? name[i] : '\0';
            if (std::tolower(static_cast<unsigned char>(c)) != kPrefix[i]) return false;
        }
        return true;
    };
    std::vector<BulletBox> boxes;
    for (const assets::CollisionBox& record : model.collisionBoxes) {
        if (!matches(record.name)) continue;
        boxes.push_back(bulletBox(bulletTransformFromMatrix(record.matrix), {record.half[0], record.half[1], record.half[2]}));
    }
    return boxes;
}

std::vector<BoxBoxContact> boxBoxContacts(const BulletTransform& a, const Vector3& halfA, const BulletTransform& b, const Vector3& halfB) {
    float R1[12], R2[12];
    toRows(a, R1);
    toRows(b, R2);
    const float side1[3] = {halfA[0] * 2.0f, halfA[1] * 2.0f, halfA[2] * 2.0f};
    const float side2[3] = {halfB[0] * 2.0f, halfB[1] * 2.0f, halfB[2] * 2.0f};
    std::vector<BoxBoxContact> out;
    boxBox(a.origin, R1, side1, b.origin, R2, side2, out);
    return out;
}

void refreshManifold(ContactManifold& manifold, const BulletTransform& a, const BulletTransform& b) {
    for (int i = int(manifold.points.size()) - 1; i >= 0; i--) {
        ManifoldPoint& point = manifold.points[std::size_t(i)];
        point.worldA = bulletApply(a, point.localA);
        point.worldB = bulletApply(b, point.localB);
        point.distance = dot(sub(point.worldA, point.worldB), point.normalOnB);
        point.lifeTime++;
    }
    const float threshold = manifold.breakingThreshold;
    for (int i = int(manifold.points.size()) - 1; i >= 0; i--) {
        const ManifoldPoint& point = manifold.points[std::size_t(i)];
        if (!(point.distance <= threshold)) {
            removeManifoldPoint(manifold, std::size_t(i));
            continue;
        }
        const Vector3 projected = sub(point.worldA, scale(point.normalOnB, point.distance));
        const Vector3 difference = sub(point.worldB, projected);
        if (dot(difference, difference) > threshold * threshold) removeManifoldPoint(manifold, std::size_t(i));
    }
}

void addManifoldContact(ContactManifold& manifold, const BulletTransform& a, const BulletTransform& b, const BoxBoxContact& contact, float friction,
                        float restitution) {
    if (!(contact.depth <= manifold.breakingThreshold)) return;
    ManifoldPoint point;
    point.worldA = add(contact.pointOnB, scale(contact.normalOnB, contact.depth));
    point.worldB = contact.pointOnB;
    point.localA = bulletInverseApply(a, point.worldA);
    point.localB = bulletInverseApply(b, point.worldB);
    point.normalOnB = contact.normalOnB;
    point.distance = contact.depth;
    const int index = cacheEntry(manifold, point);
    point.friction = std::clamp(friction, -kBulletFrictionLimit, kBulletFrictionLimit);
    point.restitution = restitution;
    if (index >= 0) {
        ManifoldPoint& old = manifold.points[std::size_t(index)];
        const int lifeTime = old.lifeTime;
        const std::array<float, 3> rows = old.rowImpulse;
        old = point;
        old.appliedImpulse = rows[0];
        old.lateralImpulse1 = rows[1];
        old.lateralImpulse2 = rows[2];
        old.rowImpulse = rows;
        old.lifeTime = lifeTime;
        return;
    }
    if (int(manifold.points.size()) == kManifoldPoints) {
        int insert = sortCachedPoints(manifold, point);
        if (insert < 0) insert = 0;
        manifold.points[std::size_t(insert)] = point;
        return;
    }
    manifold.points.push_back(point);
}

std::vector<bool> solveContactGroup(const std::vector<VehicleRigidBody*>& bodyList, const std::vector<SolverManifold>& manifolds,
                                    const BulletSolverInfo& info) {
    std::vector<SolverBody> bodies(bodyList.size() + 1);
    for (std::size_t i = 0; i < bodyList.size(); ++i) bodies[i + 1].body = bodyList[i];
    std::vector<bool> activated(bodyList.size(), false);
    const auto dynamic = [&](int index) { return index >= 0 && bodyList[std::size_t(index)]->inverseMass != 0.0f; };
    std::vector<SolverConstraint> contacts, frictions;
    for (const SolverManifold& entry : manifolds) {
        if (!dynamic(entry.bodyA) && !dynamic(entry.bodyB)) continue;
        ContactManifold& manifold = *entry.manifold;
        const int indexA = entry.bodyA + 1;
        const int indexB = entry.bodyB + 1;
        for (ManifoldPoint& point : manifold.points) {
            if (!(point.distance <= manifold.processingThreshold)) continue;
            if (entry.bodyA >= 0) activated[std::size_t(entry.bodyA)] = true;
            if (entry.bodyB >= 0) activated[std::size_t(entry.bodyB)] = true;
            const SolverBody& a = bodies[std::size_t(indexA)];
            const SolverBody& b = bodies[std::size_t(indexB)];
            SolverConstraint c;
            c.bodyA = indexA;
            c.bodyB = indexB;
            c.point = &point;
            c.flag = (point.flags & 4) != 0;
            const Vector3 relPos1 = sub(point.worldA, a.centre(point.worldA));
            const Vector3 relPos2 = sub(point.worldB, b.centre(point.worldB));
            const Vector3 normal = point.normalOnB;
            c.angularComponentA = a.inverseInertia(cross(relPos1, normal));
            if (point.flags & 1) c.angularComponentA = scale(c.angularComponentA, kContactAngularScale);
            c.angularComponentB = b.inverseInertia(neg(cross(relPos2, normal)));
            if (point.flags & 2) c.angularComponentB = scale(c.angularComponentB, kContactAngularScale);
            const float denom0 = a.body ? dot(normal, cross(c.angularComponentA, relPos1)) + a.inverseMass() : 0.0f;
            const float denom1 = b.body ? dot(normal, cross(neg(c.angularComponentB), relPos2)) + b.inverseMass() : 0.0f;
            c.jacDiagABInv = 1.0f / (denom0 + denom1);
            c.contactNormal = normal;
            c.relpos1CrossNormal = cross(relPos1, normal);
            c.relpos2CrossNormal = cross(relPos2, neg(normal));
            const Vector3 vel1 = add(a.velocity(), cross(a.angularVelocity(), relPos1));
            const Vector3 vel2 = add(b.velocity(), cross(b.angularVelocity(), relPos2));
            const Vector3 vel = sub(vel1, vel2);
            const float relVel = dot(normal, vel);
            const float penetration = point.distance + info.linearSlop;
            c.friction = point.friction;
            float restitution = 0.0f;
            if (point.lifeTime <= info.restingRestitutionThreshold) {
                restitution = -(point.restitution * relVel);
                if (restitution < 0.0f) restitution = 0.0f;
            }
            if (info.mode & 4) {
                c.appliedImpulse = point.appliedImpulse * info.warmStarting;
                applySolverImpulse(bodies[std::size_t(indexA)], scale(normal, a.inverseMass()), c.angularComponentA, c.appliedImpulse, c.flag);
                applySolverImpulse(bodies[std::size_t(indexB)], scale(normal, b.inverseMass()), neg(c.angularComponentB), -c.appliedImpulse, c.flag);
            } else {
                c.appliedImpulse = 0.0f;
            }
            const float vel1Dotn = dot(normal, a.velocity()) + dot(c.relpos1CrossNormal, a.angularVelocity());
            const float vel2Dotn = dot(c.relpos2CrossNormal, b.angularVelocity()) - dot(normal, b.velocity());
            const float positionalImpulse = c.jacDiagABInv * (-(penetration * info.erp) / info.timeStep);
            const float velocityImpulse = c.jacDiagABInv * (restitution - (vel1Dotn + vel2Dotn));
            if (info.splitImpulse && !(penetration > info.splitImpulseThreshold)) c.rhs = velocityImpulse;
            else c.rhs = positionalImpulse + velocityImpulse;
            c.cfm = 0;
            c.lowerLimit = 0;
            c.upperLimit = kUpperLimit;
            c.frictionIndex = int(frictions.size());
            if (!(info.mode & 0x20) || !point.lateralInitialized) {
                point.lateralDir1 = sub(vel, scale(normal, relVel));
                const float lateral = length2(point.lateralDir1);
                if (!(info.mode & 0x40) && aboveEpsilon(lateral)) {
                    point.lateralDir1 = scale(point.lateralDir1, 1.0f / std::sqrt(lateral));
                    frictions.push_back(frictionConstraint(bodies, indexA, indexB, point.lateralDir1, point, relPos1, relPos2));
                } else {
                    planeSpace(normal, point.lateralDir1, point.lateralDir2);
                    frictions.push_back(frictionConstraint(bodies, indexA, indexB, point.lateralDir1, point, relPos1, relPos2));
                }
                point.lateralInitialized = true;
            } else {
                frictions.push_back(frictionConstraint(bodies, indexA, indexB, point.lateralDir1, point, relPos1, relPos2));
            }
            frictions.back().appliedImpulse = 0.0f;
            contacts.push_back(c);
        }
    }
    if (contacts.empty()) return activated;
    for (int iteration = 0; iteration < info.iterations; ++iteration) {
        for (SolverConstraint& c : contacts) resolveRow(bodies, c, false);
        for (SolverConstraint& f : frictions) {
            const float total = contacts[std::size_t(&f - frictions.data())].appliedImpulse;
            if (total > 0.0f) {
                f.lowerLimit = -(f.friction * total);
                f.upperLimit = f.friction * total;
                resolveRow(bodies, f, true);
            }
        }
    }
    for (SolverConstraint& c : contacts) c.point->appliedImpulse = c.appliedImpulse;
    for (std::size_t i = 0; i < bodyList.size(); ++i) {
        if (bodyList[i]->inverseMass == 0.0f) continue;
        for (std::size_t a = 0; a < 3; ++a) {
            bodyList[i]->velocity[a] += bodies[i + 1].deltaLinear[a];
            bodyList[i]->angularVelocity[a] += bodies[i + 1].deltaAngular[a];
        }
    }
    return activated;
}

void solveVehicleContacts(VehicleRigidBody& body, std::vector<ContactManifold>& manifolds, const BulletSolverInfo& info) {
    std::vector<SolverManifold> entries;
    for (ContactManifold& manifold : manifolds) entries.push_back({&manifold, 0, -1});
    solveContactGroup({&body}, entries, info);
}

VehicleBoxContacts vehicleBoxContacts(const std::string& className, const std::vector<BulletBox>* boxes) {
    Vector3 low, high;
    vehicleCollisionBoxStandIn(className, low, high);
    return vehicleBoxContacts(low, high, boxes);
}

VehicleBoxContacts vehicleBoxContacts(const Vector3& low, const Vector3& high, const std::vector<BulletBox>* boxes) {
    VehicleBoxContacts contacts;
    contacts.boxes = boxes;
    contacts.childOffset = scale(add(low, high), 0.5f);
    BulletTransform identity;
    identity.basis = {Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1}};
    contacts.child = bulletBox(identity, scale(sub(high, low), 0.5f));
    return contacts;
}

void collideVehicleWithBoxes(const VehicleRigidBody& body, VehicleBoxContacts& contacts) {
    if (!contacts.boxes) return;
    const BulletTransform root = bulletBodyTransform(body);
    BulletTransform child = root;
    child.origin = bulletApply(root, contacts.childOffset);
    Vector3 childLow, childHigh;
    bulletBoxAabb(contacts.child, child, childLow, childHigh);
    const Vector3 childHalf = bulletBoxHalfExtents(contacts.child);
    const std::vector<BulletBox>& boxes = *contacts.boxes;
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const BulletBox& box = boxes[i];
        Vector3 low, high;
        bulletBoxAabb(box, box.transform, low, high);
        const bool overlap = !(childLow[0] > high[0] || childHigh[0] < low[0]) && !(childLow[2] > high[2] || childHigh[2] < low[2]) &&
                             !(childLow[1] > high[1] || childHigh[1] < low[1]);
        auto found = std::find_if(contacts.manifolds.begin(), contacts.manifolds.end(), [&](const ContactManifold& m) { return m.box == int(i); });
        if (found != contacts.manifolds.end() && !found->points.empty()) refreshManifold(*found, root, box.transform);
        if (!overlap) {
            if (found != contacts.manifolds.end()) {
                const std::size_t index = std::size_t(found - contacts.manifolds.begin());
                std::swap(contacts.manifolds[index], contacts.manifolds.back());
                contacts.manifolds.pop_back();
            }
            continue;
        }
        if (found == contacts.manifolds.end()) {
            ContactManifold manifold;
            manifold.box = int(i);
            manifold.breakingThreshold = std::min(bulletBoxBreakingThreshold(contacts.child), bulletBoxBreakingThreshold(box));
            contacts.manifolds.push_back(manifold);
            found = contacts.manifolds.end() - 1;
        }
        const float friction = kBulletVehicleFriction * kBulletBoxFriction;
        for (const BoxBoxContact& contact : boxBoxContacts(child, childHalf, box.transform, bulletBoxHalfExtents(box)))
            addManifoldContact(*found, root, box.transform, contact, friction, 0.0f);
        if (!found->points.empty()) refreshManifold(*found, root, box.transform);
    }
}

namespace {

BulletTransform childTransform(const VehicleRigidBody& body, const VehicleBoxContacts& shape) {
    const BulletTransform root = bulletBodyTransform(body);
    BulletTransform child = root;
    child.origin = bulletApply(root, shape.childOffset);
    return child;
}

bool aabbOverlap(const Vector3& lowA, const Vector3& highA, const Vector3& lowB, const Vector3& highB) {
    return !(lowA[0] > highB[0] || highA[0] < lowB[0]) && !(lowA[2] > highB[2] || highA[2] < lowB[2]) && !(lowA[1] > highB[1] || highA[1] < lowB[1]);
}

}

void collideVehiclePair(const VehicleRigidBody& first, const VehicleBoxContacts& firstShape, const VehicleRigidBody& second,
                        const VehicleBoxContacts& secondShape, std::optional<ContactManifold>& manifold) {
    const BulletTransform rootFirst = bulletBodyTransform(first);
    const BulletTransform rootSecond = bulletBodyTransform(second);
    const BulletTransform childFirst = childTransform(first, firstShape);
    const BulletTransform childSecond = childTransform(second, secondShape);
    Vector3 lowFirst, highFirst, lowSecond, highSecond;
    bulletBoxAabb(firstShape.child, childFirst, lowFirst, highFirst);
    bulletBoxAabb(secondShape.child, childSecond, lowSecond, highSecond);
    const bool overlap = aabbOverlap(lowFirst, highFirst, lowSecond, highSecond);
    if (manifold && !manifold->points.empty()) refreshManifold(*manifold, rootSecond, rootFirst);
    if (!overlap) {
        manifold.reset();
        return;
    }
    if (manifold && !manifold->points.empty()) refreshManifold(*manifold, rootSecond, rootFirst);
    if (!manifold) {
        manifold = ContactManifold{};
        manifold->breakingThreshold = std::min(bulletBoxBreakingThreshold(secondShape.child), bulletBoxBreakingThreshold(firstShape.child));
    }
    const float friction = kBulletVehicleFriction * kBulletVehicleFriction;
    for (const BoxBoxContact& contact :
         boxBoxContacts(childSecond, bulletBoxHalfExtents(secondShape.child), childFirst, bulletBoxHalfExtents(firstShape.child)))
        addManifoldContact(*manifold, rootSecond, rootFirst, contact, friction, 0.0f);
    if (!manifold->points.empty()) refreshManifold(*manifold, rootSecond, rootFirst);
    for (ManifoldPoint& point : manifold->points) point.flags = kVehiclePairContactFlagsStandIn;
}

void stepVehicleBoxContacts(VehicleRigidBody& body, VehicleBoxContacts& contacts, float dt) {
    collideVehicleWithBoxes(body, contacts);
    BulletSolverInfo info;
    info.timeStep = dt;
    solveVehicleContacts(body, contacts.manifolds, info);
}

}
