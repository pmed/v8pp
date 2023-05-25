// raw types here, so just pass value, not reference or pointer
function js_param_int(i, i64, p64, u64) {
    dlog("js_param_int i:", i, " i64:", i64, " p64:", p64, " u64:", u64);
    return i + i64 + p64 + u64;
};
function js_param_string(psz, s, rs, crs) {
    dlog("js_param_string psz:", psz, " s:", s, " rs:", rs, " crs:", crs);
    return psz + s + rs + crs;
};
function js_param_utype(ut1, put, cput, rut, crut) {
    dlog("js_param_utype ut1:", ut1, " ut1[0]", ut1[0], " put:", put, " cput:", cput, " rut:", rut, " crut:", crut);
    // return 0;
    return [0, [1024, "ok"]];
};