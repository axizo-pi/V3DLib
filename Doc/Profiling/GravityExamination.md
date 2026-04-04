<head>
	<link rel="stylesheet" type="text/css" href="../css/docs.css">

<style>

img {
  width: 30%;
}

</style>
</head>

# Gravity Examination

Done on **20260330** and later.

This is an examination of the working of the [barrier command](https://en.wikipedia.org/wiki/Memory_barrier){:target="_blank"}.

`barrier` is a hardware operation on `v3d`.  
For `vc4`, there is an implementation using `mutex` and VPM as shared memory over all QPU's.

**Conclusion: `barrier vc4` does not work properly.**

## Reference Output

The default output of `Gravity` is as follows.

- `BATCH_STEPS == 1`
- 32 gravitational entities
- Calculation step is **1 day**
- Simulation runs for **250 years**

![Gravity Reference](../images/gravity_reference.bmp)


Runs below done with BATCH\_STEPS = 1; this **might make a big difference** for `vc`.
 
Examination with `vc7` show that using `barrier` with `BATCH_STEPS = 400` makes a
significant difference in the output. Conclusion is that `barrier` is required in this case.

**TODO** examine this for `vc6`, `vc4`.

## Pi2, vc4

 | barrier | QPU's | TMU | L2 Cache | Output OK | time | mbox error | Comment |
 | ------- | ----- | --- | -------- | --------- | ---- | ---------- | ------- |
 | yes     |  1    | yes | disabled | yes       | 42s  | no,  5     | Lost power at 6            |
 | yes     |  2    | yes | disabled | yes       | 34s  | no,  3     | |
 | yes     |  6    | yes | disabled | ?         | -    | YES, 1     | mbox error consistent at 1 |
 | yes     | 12    | yes | disabled | ?         | -    | YES, 1     | mbox error consistent at 1 |
 | no      |  1    | yes | disabled | yes       | 42s  | no,  3     | |
 | no      |  2    | yes | disabled | yes       | 32s  | no,  3     | |
 | no      |  3    | yes | disabled | yes       | 27s  | no,  3     | faster than n=12!?         |
 | no      |  6    | yes | disabled | yes       | 26s  | no,  3     | idem |
 | no      |  9    | yes | disabled | yes       | 27s  | no,  4     | idem |
 | no      | 12    | yes | disabled | yes       | 29s  | no,  3     | |
 | no      |  1    | no  | disabled | yes       | 41s  | no,  2     | |
 | no      |  6    | no  | disabled | yes       | 26s  | no,  2     | faster than n=12!?         |
 | no      | 12    | no  | disabled | yes       | 28s  | no,  3     | |

### Conclusions

- The power socket of the pi2-1 appears to be sketchy; power lost during testing.
  The connection is not firm.
- barrier has no effect; it leads to mbox errors for n >= 6
- Less QPU's works better!

## Pi3, vc4

_Not 100% about following values, I may have made an error with disabling TMU._

 | barrier | QPU's | BATCH\_STEPS | TMU | L2 Cache | Output OK | time  | mbox error | Comment |
 | ------- | ----- | ------------ | --- | -------- | --------- | ----  | ---------- | ------- |
 | yes     |  1    |   1          | yes | enabled  | yes       | 56s   | no,  6     | |
 | "       | 12    |   1          | "   | "        | "         | 50s   | yes, 5     | |
 | "       |  1    |   1          | "   | disabled | "         | 36s   | no,  7     | |
 | "       |  1    |  10          | "   | disabled | **NO**    | 25s   | no,  1     | Inner entities diverge |
 | "       |  1    | 100          | "   | disabled | **NO**    | 24s   | no,  1     | Inner entities diverge |
 | "       |  3    |   1          | yes | disabled | yes       | 24s   | no,  3     | |
 | "       |  9    |   1          | yes | disabled | -         | -     | yes, 1     | mbox error |
 | "       | 12    |   1          | yes | disabled | -         | -     | yes, 1     | mbox error consistent at 1 |
 | "       |  1    |   1          | no  | enabled  | **NO**    | 82s   | no,  1     | |
 | "       | 12    |   1          | no  | enabled  | -         | -     | no,  1     | mbox error |
 | "       | 12    |  10          | yes | disabled | **NO**    | 19s   | no,  1     | Major divergences |
 | "       | 12    | 100          | yes | disabled | **NO**    | 17s   | no,  1     | Major divergences |
 | no      |  1    |   1          | yes | enabled  | yes       | 56s   | no,  6     | |
 | "       | 12    |   1          | yes | "        | "         | 34s   | no,  5     | |
 | no      |  1    |   1          | yes | disabled | yes       | 36s   | no,  8     | Best |
 | no      |  1    |  10          | yes | disabled | **NO**    | 25s   | no,  2     | Inner entities diverge |
 | no      | 12    |   1          | yes | disabled | yes       | 19s   | no,  8     | Best |
 | no      | 12    |  10          | yes | disabled | **NO**    |  8.8s | no,  2     | Significant divergences |
 | no      |  1    |   1          | no  | enabled  | NO        | 83s   | no,  3     | |
 | no      | 12    |   1          | no  | enabled  | NO        | 62s   | no,  2     | |
 | no      |  1    |   1          | no  | disabled | NO        | 65s   | no,  2     | |
 | no      | 12    |   1          | no  | disabled | NO        | 44s   | no,  2     | |


### Conclusions

- Only `BATCH_STEPS == 1` gives proper results
- DMA load does _not_ give good results, divergences in output
- Disabling L2 cache with TMU load actually _increases_ performance
- `barrier` detrimental to performance, also mbox errors for #QPU's > 1

## Pi4, vc6

 | barrier | QPU's | BATCH\_STEPS | TMU | L2 Cache | Output OK | time  | mbox error | Comment |
 | ------- | ----- | ------------ | --- | -------- | --------- | ----  | ---------- | ------- |
 | yes     |  1    |   1          | -   | -        | yes       | 20s   | no,  3     |         |
 | yes     |  1    |  10          | -   | -        | yes       |  9s   | no,  3     |         |
 | yes     |  1    | 100          | -   | -        | yes       |  7.8s | no,  3     |         |
 | yes     |  8    |   1          | -   | -        | yes       | 13s   | no,  3     |         |
 | yes     |  8    |  10          | -   | -        | yes       |  2.6s | no,  3     |         |
 | yes     |  8    | 100          | -   | -        | yes       |  1.4s | no,  5     |         |
 | no      |  1    |   1          | -   | -        | yes       | 20s   | no,  3     |         |
 | no      |  1    |  10          | -   | -        | yes       |  9s   | no,  3     |         |
 | no      |  1    | 100          | -   | -        | yes       |  8s   | no,  3     |         |
 | no      |  8    |   1          | -   | -        | yes       | 13s   | no,  3     |         |
 | no      |  8    |  10          | -   | -        | **NO**    |  2.6s | no,  5     | Inner entities diverge |
 | no      |  8    | 100          | -   | -        | **NO**    |  1.3s | no,  5     | Inner entities diverge, less than previous |

### Observations

- `barrier` has no effect for `BATCH_STEPS == 1` 

## Pi5, vc7

 | barrier | QPU's | BATCH\_STEPS | TMU | L2 Cache | Output OK | time | mbox error | Comment |
 | ------- | ----- | ------------ | --- | -------- | --------- | ---- | ---------- | ------- |
 | yes     |  1    |   1          | -   | -        | yes       | 9s   | no,  3     |         |
 | yes     |  1    |  10          | -   | -        | yes       | 5s   | no,  3     |         |
 | yes     |  1    | 100          | -   | -        | yes       | 4.5s | no,  3     | Tiny differences in output |
 | yes     | 16    |   1          | -   | -        | yes       | 4s   | no,  5     |         |
 | yes     | 16    |  10          | -   | -        | yes       | 0.8s | no,  5     |         |
 | yes     | 16    | 100          | -   | -        | yes       | 0.5s | no,  5     | Tiny differences in output |
 | no      |  1    |   1          | -   | -        | yes       | 9s   | no,  5     |         |
 | no      |  1    |  10          | -   | -        | yes       | 4.8s | no,  5     | |
 | no      |  1    | 100          | -   | -        | yes       | 4.8s | no,  5     | |
 | no      | 16    |   1          | -   | -        | yes       | 4s   | no,  5     |         |
 | no      | 16    |  10          | -   | -        | **NO**    | 0.8s | no,  5     | Inner entities diverge |
 | no      | 16    | 100          | -   | -        | **NO**    | 0.4s | no,  5     | Inner entities diverge |

### Observations

- The 'tiny differences can be explained by the increasing steps in the output.
  The calculation itself is likely identical.
- `barrier` has no effect for `BATCH_STEPS == 1` 
- For `BATCH_STEPS > 1`, `#QPU's == 1`, output also OK
