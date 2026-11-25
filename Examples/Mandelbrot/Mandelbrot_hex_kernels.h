//
// This file is generated! Editing it directly is a bad idea.
//
// Hex representation of kernels for all VideoCores.
//
// Generation date: 20261125 03:45:42
//
#include "BaseKernel.h"  // struct generated_kernels

V3DLib::generated_kernels kernels = {
{
  // kernel hex output for vc4
  0x1002402715020fc0,  // mov ra0, uni                       ; nop
  0x1002406715020fc0,  // mov ra1, uni                       ; nop
  0x1002502715020fc0,  // mov rb0, uni                       ; nop
  0x100240a715020fc0,  // mov ra2, uni                       ; nop
  0x1002506715020fc0,  // mov rb1, uni                       ; nop
  0x100240e715020fc0,  // mov ra3, uni                       ; nop
  0x1002412715020fc0,  // mov ra4, uni                       ; nop
  0x100250a715020fc0,  // mov rb2, uni                       ; nop
  0x100250e715020fc0,  // mov rb3, uni                       ; nop
  0x1002416715020fc0,  // mov ra5, uni                       ; nop
  0x100241a715020fc0,  // mov ra6, uni                       ; nop
  0x1002512715020fc0,  // mov rb4, uni                       ; nop
  0x1002482715980d80,  // mov r0, elem                       ; nop
  0xd0024827110021c0,  // sig_small_imm shl r0, r0, 2        ; nop
  0x100241670c140c00,  // add ra5, ra5, r0                   ; nop
  0x100241e70d042f80,  // sub ra7, rb2, ra1                  ; nop
  0xe00250a700000000,  // load_imm rb2, nop, 0x00000000 (0.000000)
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d182f80,  // sub.sf nop, rb2, ra6               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf00809e7000003f8,  // branch.all_zs 1016
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0x1002512715000d80,  // mov rb4, ra0                       ; nop
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d1c4f80,  // sub.sf nop, rb4, ra7               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf00809e700000360,  // branch.all_zs 864
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0x100259e14010403e,  // nop nop, r0, r0                    ; mul24 r1, rb4, ra4
  0xd0024827110023c0,  // sig_small_imm shl r0, r1, 2        ; nop
  0x100242270c140c00,  // add ra8, ra5, r0                   ; nop
  0xe002516700000000,  // load_imm rb5, nop, 0x00000000 (0.000000)
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d105f80,  // sub.sf nop, rb5, ra4               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf00809e7000002b8,  // branch.all_zs 696
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0x100242670c985f80,  // add ra9, rb5, elem                 ; nop
  0x1002486715004fc0,  // mov r1, rb4                        ; nop
  0xd0024867080003c0,  // sig_small_imm itof r1, r1, 0       ; nop
  0x100259e0200c0031,  // nop nop, r0, r0                    ; fmul r0, ra3, r1
  0x100251a702080c00,  // fsub rb6, ra2, r0                  ; nop
  0xd002486708240dc0,  // sig_small_imm itof r1, ra9, 0      ; nop
  0x100259e020001039,  // nop nop, r0, r0                    ; fmul r0, rb1, r1
  0x100242a701000e00,  // fadd ra10, rb0, r0                 ; nop
  0x100251a715006fc0,  // mov rb6, rb6                       ; nop
  0xd002426715000fc0,  // sig_small_imm mov ra9, 0           ; nop
  0x100251e715280d80,  // mov rb7, ra10                      ; nop
  0x100242e715006fc0,  // mov ra11, rb6                      ; nop
  0x100259e12000703f,  // nop nop, r0, r0                    ; fmul r1, rb7, rb7
  0x100259e0202c0036,  // nop nop, r0, r0                    ; fmul r0, ra11, ra11
  0x1002432701000200,  // fadd ra12, r1, r0                  ; nop
  0xe00249e700000000,  // nop
  0xd00248a702322f80,  // sig_small_imm fsub r2, 4.0, ra12   ; nop
  0x100248670d243f80,  // sub r1, rb3, ra9                   ; nop
  0xd0024827080003c0,  // sig_small_imm itof r0, r1, 0       ; nop
  0x100259cd20000010,  // nop nop, r0, r0                    ; fmul ra13, r2, r0
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0xd00269e702340f80,  // sig_small_imm fsub.sf nop, 0, ra13 ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf00809e700000108,  // branch.all_zs 264
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0xd00269e702340f80,  // sig_small_imm fsub.sf nop, 0, ra13 ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0x100259e1202c703e,  // nop nop, r0, r0                    ; fmul r1, rb7, ra11
  0x100259e0202c7037,  // nop nop, r0, r0                    ; fmul r0, ra11, rb7
  0x1006522701000200,  // fadd.zc rb8, r1, r0                ; nop
  0x100259e12000703f,  // nop nop, r0, r0                    ; fmul r1, rb7, rb7
  0x100259e0202c0036,  // nop nop, r0, r0                    ; fmul r0, ra11, ra11
  0x100648a702000200,  // fsub.zc r2, r1, r0                 ; nop
  0x1006482715008fc0,  // mov.zc r0, rb8                     ; nop
  0x10064867010061c0,  // fadd.zc r1, r0, rb6                ; nop
  0x1006482701280580,  // fadd.zc r0, r2, ra10               ; nop
  0x100648a715000240,  // mov.zc r2, r1                      ; nop
  0x100651e715000000,  // mov.zc rb7, r0                     ; nop
  0x100642e715000480,  // mov.zc ra11, r2                    ; nop
  0x100259e22000703f,  // nop nop, r0, r0                    ; fmul r2, rb7, rb7
  0x100259e1202c0036,  // nop nop, r0, r0                    ; fmul r1, ra11, ra11
  0x1006482701000440,  // fadd.zc r0, r2, r1                 ; nop
  0x1006432715000000,  // mov.zc ra12, r0                    ; nop
  0xd00642670c241dc0,  // sig_small_imm add.zc ra9, ra9, 1   ; nop
  0xd00248a702322f80,  // sig_small_imm fsub r2, 4.0, ra12   ; nop
  0x100248670d243f80,  // sub r1, rb3, ra9                   ; nop
  0xd0024827080003c0,  // sig_small_imm itof r0, r1, 0       ; nop
  0x1002d9cd20000010,  // nop nop, r0, r0                    ; fmul.zc ra13, r2, r0
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0xd00269e702340f80,  // sig_small_imm fsub.sf nop, 0, ra13 ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf03809e7fffffef8,  // branch.any_zc -264
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe002486700000010,  // load_imm r1, nop, 0x00000010 (0.000000)
  0x100248670c0263c0,  // add r1, r1, qpu                    ; nop
  0xe002482700001200,  // load_imm r0, nop, 0x00001200 (0.000000)
  0x10025c6715000200,  // or vw_setup, r1, r0                ; nop
  0xe00242a700000100,  // load_imm ra10, nop, 0x00000100 (0.000000)
  0xe00249e700000000,  // nop
  0x100242a70c2a6dc0,  // add ra10, ra10, qpu                ; nop
  0x100049e715032fc0,  // mov.never nop, vpm_st_wait         ; nop
  0xe0025c67c0000000,  // load_imm vw_setup, nop, 0xc0000000 (-2.000000)
  0xe002486788014000,  // load_imm r1, nop, 0x88014000 (-0.000000)
  0xd002482711283dc0,  // sig_small_imm shl r0, ra10, 3      ; nop
  0x10025c6715000200,  // or vw_setup, r1, r0                ; nop
  0xd0024c2711240dc0,  // sig_small_imm shl vpm, ra9, 0      ; nop
  0x10025ca715200d80,  // mov vw_addr, ra8                   ; nop
  0xe002482700000040,  // load_imm r0, nop, 0x00000040 (0.000000)
  0x100242270c200c00,  // add ra8, ra8, r0                   ; nop
  0xe002482700000010,  // load_imm r0, nop, 0x00000010 (0.000000)
  0x100251670c005e00,  // add rb5, rb5, r0                   ; nop
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d105f80,  // sub.sf nop, rb5, ra4               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf03809e7fffffd48,  // branch.any_zc -696
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0x100251270c044f80,  // add rb4, rb4, ra1                  ; nop
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d1c4f80,  // sub.sf nop, rb4, ra7               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf03809e7fffffca0,  // branch.any_zc -864
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0x100248a715002fc0,  // mov r2, rb2                        ; nop
  0xd00250a70c0015c0,  // sig_small_imm add rb2, r2, 1       ; nop
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d182f80,  // sub.sf nop, rb2, ra6               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf03809e7fffffc08,  // branch.any_zc -1016
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0x100049e715c80d80,  // mov.never nop, vpm_ld_wait         ; nop
  0x100049e715032fc0,  // mov.never nop, vpm_st_wait         ; nop
  0x1002482715000d80,  // mov r0, ra0                        ; nop
  0xe002486700000000,  // load_imm r1, nop, 0x00000000 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xe004486700000001,  // load_imm r1.zs, nop, 0x00000001 (0.000000)
  0xd00269e70d0003c0,  // sig_small_imm sub.sf nop, r1, 0    ; nop
  0xf00809e7000000d0,  // branch.all_zs 208
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xd00240270d041dc0,  // sig_small_imm sub ra0, ra1, 1      ; nop
  0xe002502700000000,  // load_imm rb0, nop, 0x00000000 (0.000000)
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d000f80,  // sub.sf nop, rb0, ra0               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf00809e700000058,  // branch.all_zs 88
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe80249e70000001f,  // nop
  0x1002486715000fc0,  // mov r1, rb0                        ; nop
  0xd00250270c0013c0,  // sig_small_imm add rb0, r1, 1       ; nop
  0xe002482700000000,  // load_imm r0, nop, 0x00000000 (0.000000)
  0x100269e70d000f80,  // sub.sf nop, rb0, ra0               ; nop
  0xe008482700000001,  // load_imm r0.ns, nop, 0x00000001 (0.000000)
  0xd00269e70d0001c0,  // sig_small_imm sub.sf nop, r0, 0    ; nop
  0xf03809e7ffffffa8,  // branch.any_zc -88
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249a700000001,  // load_imm host_int, nop, 0x00000001 (0.000000)
  0xf0f809e700000008,  // branch 8
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe80249e70000000f,  // nop
  0x300249e7009e7000,  // sig_end nop nop, r0, r0            ; nop
  0xe00249e700000000,  // nop
  0xe00249e700000000,  // nop
  0xe00249e700000000   // nop
}, {
  // kernel hex output for vc6
  0x3d803186bb800000,  // nop                           ; nop                         ; ldunifrf.rf0
  0x3d807186bb800000,  // nop                           ; nop                         ; ldunifrf.rf1
  0x3d803186bb800000,  // nop                           ; nop                         ; ldunifrf.rf0
  0x3d80b186bb800000,  // nop                           ; nop                         ; ldunifrf.rf2
  0x3d80f186bb800000,  // nop                           ; nop                         ; ldunifrf.rf3
  0x3d813186bb800000,  // nop                           ; nop                         ; ldunifrf.rf4
  0x3d817186bb800000,  // nop                           ; nop                         ; ldunifrf.rf5
  0x3d81b186bb800000,  // nop                           ; nop                         ; ldunifrf.rf6
  0x3d81f186bb800000,  // nop                           ; nop                         ; ldunifrf.rf7
  0x3d823186bb800000,  // nop                           ; nop                         ; ldunifrf.rf8
  0x3d827186bb800000,  // nop                           ; nop                         ; ldunifrf.rf9
  0x3d82b186bb800000,  // nop                           ; nop                         ; ldunifrf.rf10
  0x3de02180b683f000,  // or rf0, 0, 0                  ; nop
  0x3de071803c83e048,  // sub.pushz r0, rf1, 8          ; nop
  0x0200001f00001000,  // b.allna  24
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003180bb801000,  // tidx r0                       ; nop
  0x3de031807d838002,  // shr r0, r0, 2                 ; nop
  0x3de02180b583800f,  // and rf0, r0, 15               ; nop
  0x3c003180bb802000,  // eidx r0                       ; nop
  0x3de031807c838002,  // shl r0, r0, 2                 ; nop
  0x3c00218938806240,  // add rf9, rf9, r0              ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3de031823883a004,  // add r2, r2, 4                 ; nop
  0x3de031827c83a004,  // shl r2, r2, 4                 ; nop
  0x3c003180b6812000,  // or r0, r2, r2                 ; nop
  0x3c002196b6800000,  // or rf22, r0, r0               ; nop
  0x3c203186bb800000,  // nop                           ; nop                         ; thrsw
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c0021873c83e1c1,  // sub rf7, rf7, rf1             ; nop
  0x3de0218bb683f000,  // or rf11, 0, 0                 ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3c00b1863c83e2ca,  // sub.pushn -, rf11, rf10       ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x0200038400001000,  // b.alla  896
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c00218cb6836000,  // or rf12, rf0, rf0             ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3c00b1863c83e307,  // sub.pushn -, rf12, rf7        ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x020002f400001000,  // b.alla  752
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x24003046bbf80306,  // nop                           ; smul24 r1, rf12, rf6
  0x3de031807c839002,  // shl r0, r1, 2                 ; nop
  0x3c00218d38806240,  // add rf13, rf9, r0             ; nop
  0x3de0218eb683f000,  // or rf14, 0, 0                 ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3c00b1863c83e386,  // sub.pushn -, rf14, rf6        ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x0200024c00001000,  // b.alla  584
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003180bb802000,  // eidx r0                       ; nop
  0x3c00318238806380,  // add r2, rf14, r0              ; nop
  0x3de03181fc806300,  // itof r1, rf12                 ; nop
  0x54003006bb380140,  // nop                           ; fmul r0, rf5, r1
  0x3c003183458060c0,  // fsub r3, rf3, r0              ; nop
  0x3de03181fc802000,  // itof r1, r2                   ; nop
  0x54003006bb380100,  // nop                           ; fmul r0, rf4, r1
  0x3c00045005ef0080,  // fadd rf16, rf2, r0            ; mov rf17, r3
  0x3de0048fb6fbf400,  // or rf15, 0, 0                 ; mov rf18, rf16
  0x54002053b6ff6452,  // or rf19, rf17, rf17           ; fmul r1, rf18, rf18
  0x54003006bbd804c0,  // nop                           ; fmul r0, rf19, rf19
  0x3c00219405808000,  // fadd rf20, r1, r0             ; nop
  0x3de031824583752a,  // fsub r2, 0x40800000, rf20     ; nop
  0x3c0031813c83e20f,  // sub r1, rf8, rf15             ; nop
  0x3de03180fc801000,  // itof r0, r1                   ; nop
  0x54001546bb080000,  // nop                           ; fmul rf21, r2, r0
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3de0b18645837540,  // fsub.pushn -, 0, rf21         ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x0200010c00001000,  // b.alla  264
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3de0b18645837540,  // fsub.pushn -, 0, rf21         ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x54003046bbf80493,  // nop                           ; fmul r1, rf18, rf19
  0x54003006bbf804d2,  // nop                           ; fmul r0, rf19, rf18
  0x3c0a318305808000,  // fadd.ifna r3, r1, r0          ; nop
  0x54003046bbd80480,  // nop                           ; fmul r1, rf18, rf18
  0x54003006bbd804c0,  // nop                           ; fmul r0, rf19, rf19
  0x3c0a318245801000,  // fsub.ifna r2, r1, r0          ; nop
  0x3c0a3180b681b000,  // or.ifna r0, r3, r3            ; nop
  0x3c0a318105830440,  // fadd.ifna r1, r0, rf17        ; nop
  0x3c0a318005832400,  // fadd.ifna r0, r2, rf16        ; nop
  0x3c0a3182b6809000,  // or.ifna r2, r1, r1            ; nop
  0x3c0a2192b6800000,  // or.ifna rf18, r0, r0          ; nop
  0x3c0a2193b6812000,  // or.ifna rf19, r2, r2          ; nop
  0x54003086bbd80480,  // nop                           ; fmul r2, rf18, rf18
  0x54003046bbd804c0,  // nop                           ; fmul r1, rf19, rf19
  0x3c0a318005811000,  // fadd.ifna r0, r2, r1          ; nop
  0x3c0a2194b6800000,  // or.ifna rf20, r0, r0          ; nop
  0x3dea218f3883e3c1,  // add.ifna rf15, rf15, 1        ; nop
  0x3de031824583752a,  // fsub r2, 0x40800000, rf20     ; nop
  0x3c0031813c83e20f,  // sub r1, rf8, rf15             ; nop
  0x3de03180fc801000,  // itof r0, r1                   ; nop
  0x540e1546bb080000,  // nop                           ; fmul.ifna rf21, r2, r0
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3de0b18645837540,  // fsub.pushn -, 0, rf21         ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x02fffefdff001000,  // b.anyna  -264
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c00318bb68363c0,  // or tmud, rf15, rf15           ; nop
  0x3c00318cb6836340,  // or tmua, rf13, rf13           ; nop
  0x04001346bbf95356,  // tmuwt -                       ; add rf13, rf13, rf22
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3de031823883a001,  // add r2, r2, 1                 ; nop
  0x3de031827c83a004,  // shl r2, r2, 4                 ; nop
  0x3c003180b6812000,  // or r0, r2, r2                 ; nop
  0x3c00218e38806380,  // add rf14, rf14, r0            ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3c00b1863c83e386,  // sub.pushn -, rf14, rf6        ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x02fffdbdff001000,  // b.anyna  -584
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c00218c3883e301,  // add rf12, rf12, rf1           ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3c00b1863c83e307,  // sub.pushn -, rf12, rf7        ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x02fffd15ff001000,  // b.anyna  -752
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3de0218b3883e2c1,  // add rf11, rf11, 1             ; nop
  0x3de03182b683f000,  // or r2, 0, 0                   ; nop
  0x3c00b1863c83e2ca,  // sub.pushn -, rf11, rf10       ; nop
  0x3de83182b683f001,  // or.ifa r2, 1, 1               ; nop
  0x3de071863c83a000,  // sub.pushz -, r2, 0            ; nop
  0x02fffc85ff001000,  // b.anyna  -896
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c203192bb814000,  // barrierid syncb               ; nop                         ; thrsw
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c203186bb800000,  // nop                           ; nop                         ; thrsw
  0x3c203186bb800000,  // nop                           ; nop                         ; thrsw
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c203186bb800000,  // nop                           ; nop                         ; thrsw
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000,  // nop                           ; nop
  0x3c003186bb800000   // nop                           ; nop
}, {
  // kernel hex output for vc7
  0x39800000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf0
  0x39804000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf1
  0x39800000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf0
  0x39808000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf2
  0x3980c000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf3
  0x39810000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf4
  0x39814000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf5
  0x39818000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf6
  0x3981c000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf7
  0x39820000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf8
  0x39824000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf9
  0x39828000bb03f000,  // nop                           ; nop                         ; ldunifrf.rf10
  0x38000000bb03f001,  // tidx rf0                      ; nop
  0x39e000007d03f002,  // shr rf0, rf0, 2               ; nop
  0x39e00000b503f00f,  // and rf0, rf0, 15              ; nop
  0x3800000bbb03f002,  // eidx rf11                     ; nop
  0x39e0000b7c03f2c2,  // shl rf11, rf11, 2             ; nop
  0x380000093803f24b,  // add rf9, rf9, rf11            ; nop
  0x39c0000bf903f003,  // mov rf11, 0                   ; nop
  0x39e0000b3803f2c4,  // add rf11, rf11, 4             ; nop
  0x39e0000b7c03f2c4,  // shl rf11, rf11, 4             ; nop
  0x38000018f903f2c3,  // mov rf24, rf11                ; nop
  0x38200000bb03f000,  // nop                           ; nop                         ; thrsw
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x380000073c03f1c1,  // sub rf7, rf7, rf1             ; nop
  0x39c0000bf903f003,  // mov rf11, 0                   ; nop
  0x39c0000cf903f003,  // mov rf12, 0                   ; nop
  0x380090063c03f2ca,  // sub.pushn -, rf11, rf10       ; nop
  0x39c8000cf903f043,  // mov.ifa rf12, 1               ; nop
  0x39e050063c03f300,  // sub.pushz -, rf12, 0          ; nop
  0x0200039400001000,  // b.alla  912
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x3800000cf903f003,  // mov rf12, rf0                 ; nop
  0x39c0000df903f003,  // mov rf13, 0                   ; nop
  0x380090063c03f307,  // sub.pushn -, rf12, rf7        ; nop
  0x39c8000df903f043,  // mov.ifa rf13, 1               ; nop
  0x39e050063c03f340,  // sub.pushz -, rf13, 0          ; nop
  0x0200030400001000,  // b.alla  768
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x24000340bb306000,  // nop                           ; smul24 rf13, rf12, rf6
  0x39e0000d7c03f342,  // shl rf13, rf13, 2             ; nop
  0x3800000d3803f24d,  // add rf13, rf9, rf13           ; nop
  0x39c0000ef903f003,  // mov rf14, 0                   ; nop
  0x39c0000ff903f003,  // mov rf15, 0                   ; nop
  0x380090063c03f386,  // sub.pushn -, rf14, rf6        ; nop
  0x39c8000ff903f043,  // mov.ifa rf15, 1               ; nop
  0x39e050063c03f3c0,  // sub.pushz -, rf15, 0          ; nop
  0x0200025c00001000,  // b.alla  600
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x3800000fbb03f002,  // eidx rf15                     ; nop
  0x3800000f3803f38f,  // add rf15, rf14, rf15          ; nop
  0x39e00010f603f320,  // itof rf16, rf12               ; nop
  0x54000400bb150000,  // nop                           ; fmul rf16, rf5, rf16
  0x380000104503f0d0,  // fsub rf16, rf3, rf16          ; nop
  0x39e0000ff603f3e0,  // itof rf15, rf15               ; nop
  0x540003c0bb10f000,  // nop                           ; fmul rf15, rf4, rf15
  0x380000110503f08f,  // fadd rf17, rf2, rf15          ; nop
  0x39c0000ff903f003,  // mov rf15, 0                   ; nop
  0x38000012f903f443,  // mov rf18, rf17                ; nop
  0x38000013f903f403,  // mov rf19, rf16                ; nop
  0x54000500bb492000,  // nop                           ; fmul rf20, rf18, rf18
  0x54000540bb4d3000,  // nop                           ; fmul rf21, rf19, rf19
  0x380000140503f515,  // fadd rf20, rf20, rf21         ; nop
  0x39c000154503fa94,  // fsub rf21, 0x40800000, rf20   ; nop
  0x380000163c03f20f,  // sub rf22, rf8, rf15           ; nop
  0x39e00016f603f5a0,  // itof rf22, rf22               ; nop
  0x54000540bb556000,  // nop                           ; fmul rf21, rf21, rf22
  0x39c00016f903f003,  // mov rf22, 0                   ; nop
  0x39c090064503f015,  // fsub.pushn -, 0, rf21         ; nop
  0x39c80016f903f043,  // mov.ifa rf22, 1               ; nop
  0x39e050063c03f580,  // sub.pushz -, rf22, 0          ; nop
  0x0200010c00001000,  // b.alla  264
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x39c00016f903f003,  // mov rf22, 0                   ; nop
  0x39c090064503f015,  // fsub.pushn -, 0, rf21         ; nop
  0x39c80016f903f043,  // mov.ifa rf22, 1               ; nop
  0x39e050063c03f580,  // sub.pushz -, rf22, 0          ; nop
  0x54000580bb493000,  // nop                           ; fmul rf22, rf18, rf19
  0x540005c0bb4d2000,  // nop                           ; fmul rf23, rf19, rf18
  0x380a00160503f597,  // fadd.ifna rf22, rf22, rf23    ; nop
  0x540005c0bb492000,  // nop                           ; fmul rf23, rf18, rf18
  0x54000640bb4d3000,  // nop                           ; fmul rf25, rf19, rf19
  0x380a00174503f5d9,  // fsub.ifna rf23, rf23, rf25    ; nop
  0x380a0016f903f583,  // mov.ifna rf22, rf22           ; nop
  0x380a00160503f416,  // fadd.ifna rf22, rf22, rf16    ; nop
  0x380a00170503f457,  // fadd.ifna rf23, rf23, rf17    ; nop
  0x380a0016f903f583,  // mov.ifna rf22, rf22           ; nop
  0x380a0012f903f5c3,  // mov.ifna rf18, rf23           ; nop
  0x380a0013f903f583,  // mov.ifna rf19, rf22           ; nop
  0x54000580bb492000,  // nop                           ; fmul rf22, rf18, rf18
  0x540005c0bb4d3000,  // nop                           ; fmul rf23, rf19, rf19
  0x380a00160503f597,  // fadd.ifna rf22, rf22, rf23    ; nop
  0x380a0014f903f583,  // mov.ifna rf20, rf22           ; nop
  0x39ea000f3803f3c1,  // add.ifna rf15, rf15, 1        ; nop
  0x39c000164503fa94,  // fsub rf22, 0x40800000, rf20   ; nop
  0x380000173c03f20f,  // sub rf23, rf8, rf15           ; nop
  0x39e00017f603f5e0,  // itof rf23, rf23               ; nop
  0x540e0540bb597000,  // nop                           ; fmul.ifna rf21, rf22, rf23
  0x39c00016f903f003,  // mov rf22, 0                   ; nop
  0x39c090064503f015,  // fsub.pushn -, 0, rf21         ; nop
  0x39c80016f903f043,  // mov.ifa rf22, 1               ; nop
  0x39e050063c03f580,  // sub.pushz -, rf22, 0          ; nop
  0x02fffefdff001000,  // b.anyna  -264
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x3800100bf903f3c3,  // mov tmud, rf15                ; nop
  0x3800100cf903f343,  // mov tmua, rf13                ; nop
  0x38001006bb03f00f,  // tmuwt -                       ; nop
  0x3800000d3803f358,  // add rf13, rf13, rf24          ; nop
  0x39c0000ff903f003,  // mov rf15, 0                   ; nop
  0x39e0000f3803f3c1,  // add rf15, rf15, 1             ; nop
  0x39e0000f7c03f3c4,  // shl rf15, rf15, 4             ; nop
  0x3800000e3803f38f,  // add rf14, rf14, rf15          ; nop
  0x39c0000ff903f003,  // mov rf15, 0                   ; nop
  0x380090063c03f386,  // sub.pushn -, rf14, rf6        ; nop
  0x39c8000ff903f043,  // mov.ifa rf15, 1               ; nop
  0x39e050063c03f3c0,  // sub.pushz -, rf15, 0          ; nop
  0x02fffdadff001000,  // b.anyna  -600
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x3800000c3803f301,  // add rf12, rf12, rf1           ; nop
  0x39c0000df903f003,  // mov rf13, 0                   ; nop
  0x380090063c03f307,  // sub.pushn -, rf12, rf7        ; nop
  0x39c8000df903f043,  // mov.ifa rf13, 1               ; nop
  0x39e050063c03f340,  // sub.pushz -, rf13, 0          ; nop
  0x02fffd05ff001000,  // b.anyna  -768
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x39e0000b3803f2c1,  // add rf11, rf11, 1             ; nop
  0x39c0000cf903f003,  // mov rf12, 0                   ; nop
  0x380090063c03f2ca,  // sub.pushn -, rf11, rf10       ; nop
  0x39c8000cf903f043,  // mov.ifa rf12, 1               ; nop
  0x39e050063c03f300,  // sub.pushz -, rf12, 0          ; nop
  0x02fffc75ff001000,  // b.anyna  -912
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38201012bb03f00e,  // barrierid syncb               ; nop                         ; thrsw
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38200000bb03f000,  // nop                           ; nop                         ; thrsw
  0x38200000bb03f000,  // nop                           ; nop                         ; thrsw
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38200000bb03f000,  // nop                           ; nop                         ; thrsw
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000,  // nop                           ; nop
  0x38000000bb03f000   // nop                           ; nop
}, {
  // Generated typelist
  8910758452931891121ul, // Float
  8910758452931891121ul, // Float
  8910758452931891121ul, // Float
  8910758452931891121ul, // Float
  9938785517697622658ul, // Int
  9938785517697622658ul, // Int
  9938785517697622658ul, // Int
  2262053869275221363ul, // Int::Ptr
  9938785517697622658ul  // Int
}
};
