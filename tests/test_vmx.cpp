// VMX helper semantics against a plain big-endian byte model.
#include <cstring>

#include "opentit/recomp.h"
#include "test.h"

using namespace opentit;

namespace {
v128 from_be(const u8 (&b)[16]) { return vmx::load(b); }
void to_be(const v128& v, u8 (&b)[16]) { vmx::store(b, v); }
}  // namespace

TEST(vmx_load_store_roundtrip_and_element_order) {
    u8 in[16], out[16];
    for (int i = 0; i < 16; ++i) in[i] = static_cast<u8>(i * 17);
    const v128 v = from_be(in);
    CHECK_EQ(v.ub(0), in[0]);
    CHECK_EQ(v.uw(0), (u32(in[0]) << 24) | (u32(in[1]) << 16) | (u32(in[2]) << 8) | in[3]);
    to_be(v, out);
    CHECK(std::memcmp(in, out, 16) == 0);
}

TEST(vmx_merge_pack_splat) {
    v128 a, b;
    for (int i = 0; i < 4; ++i) {
        a.uw(i) = 0xA0 + i;
        b.uw(i) = 0xB0 + i;
    }
    const v128 h = vmx::vmrghw(a, b);
    CHECK_EQ(h.uw(0), 0xA0u);
    CHECK_EQ(h.uw(1), 0xB0u);
    CHECK_EQ(h.uw(2), 0xA1u);
    CHECK_EQ(h.uw(3), 0xB1u);
    const v128 l = vmx::vmrglw(a, b);
    CHECK_EQ(l.uw(0), 0xA2u);
    CHECK_EQ(l.uw(3), 0xB3u);
    const v128 p = vmx::vpkuwum(a, b);
    CHECK_EQ(p.uh(0), 0xA0u);
    CHECK_EQ(p.uh(4), 0xB0u);
    const v128 s = vmx::vspltw(a, 2);
    for (int i = 0; i < 4; ++i) CHECK_EQ(s.uw(i), 0xA2u);
    const v128 si = vmx::vspltish(-3);
    CHECK_EQ(si.sh(5), -3);
}

TEST(vmx_saturation_and_compare) {
    v128 a, b;
    for (int i = 0; i < 8; ++i) {
        a.sh(i) = 30000;
        b.sh(i) = static_cast<s16>(i * 1000);
    }
    const v128 r = vmx::vaddshs(a, b);
    CHECK_EQ(r.sh(0), 30000);
    CHECK_EQ(r.sh(7), 32767);
    const v128 m = vmx::vcmpequh(a, a);
    u8 cr[32] = {};
    vmx::cr6(cr, m);
    CHECK_EQ(cr[24], 1);
    CHECK_EQ(cr[26], 0);
}

TEST(vmx_shift_by_octet_and_lvsr) {
    u8 in[16];
    for (int i = 0; i < 16; ++i) in[i] = static_cast<u8>(i);
    const v128 v = from_be(in);
    v128 sh = v128::zero();
    sh.ub(15) = 4 << 3;  // 4 octets
    const v128 l = vmx::vslo(v, sh);
    CHECK_EQ(l.ub(0), 4);
    CHECK_EQ(l.ub(12), 0);
    const v128 r = vmx::vsro(v, sh);
    CHECK_EQ(r.ub(4), 0);
    CHECK_EQ(r.ub(3), 0);
    CHECK_EQ(r.ub(15), 11);
    const v128 ctl = vmx::lvsr(5);
    CHECK_EQ(ctl.ub(0), 11);
}

TEST(vmx_float_convert) {
    v128 a;
    for (int i = 0; i < 4; ++i) a.fw(i) = 1.5f * static_cast<f32>(i) - 2.0f;
    const v128 c = vmx::vctsxs(a, 1);  // * 2 then truncate
    CHECK_EQ(c.sw(0), -4);
    CHECK_EQ(c.sw(3), 5);
    const v128 back = vmx::vcfsx(c, 1);
    CHECK_EQ(back.fw(3), 2.5f);
}
