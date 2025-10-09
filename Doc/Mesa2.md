* mesa - old mesa library
* mesa2 - new mesa library
* `vc6` is a synonym for define `V3D 4.x
* `vc7` is a synonym for define `V3D 7.x


Remember: vc6 has one regfile, not a and b

| raddr\_a | read address from regfile a |
| raddr\_b | read address from regfile b |
| waddr\_a | write address to regfile a |
| waddr\_b | write address to regfile b |

--------------------

mesa2, src/broadcom/qpu/qpu\_instr.h:
```
struct v3d_qpu_input {
        union {
                enum v3d_qpu_mux mux; /* V3D 4.x */
                uint8_t raddr; /* V3D 7.x */
        };
        enum v3d_qpu_input_unpack unpack;
};
```

`mux` maps to fields `add\_a/b` and `mul\_a/b'.

This implies that mux values are removed in vc7.
This doesn't make sense, it means that the registers are discarded.

* `vc6`, both mesa libs:
Struct `v3d\_qpu\_input` used in `v3d\_qpu\_alu\_instr`, part of  `v3d\_qpu\_instr`.
The latter contains `raddr\_a/b`

--------------------

mesa2, src/broadcom/qpu/qpu\_instr.h, struct `v3d\_qpu_sig`:
```
...
        bool small_imm_a:1; /* raddr_a (add a), since V3D 7.x */
        bool small_imm_b:1; /* raddr_b (add b) */
        bool small_imm_c:1; /* raddr_c (mul a), since V3D 7.x */
        bool small_imm_d:1; /* raddr_d (mul b), since V3D 7.x */
...
```

Previously, there was only one field `small\_imm`; this maps to `small\_imm\_b`.
So in `mesa`, only `raddr\_b` could be used for small imm.
In `mesa2`, for `V2D 7`, all raddr fields can be used for small\_imm.
