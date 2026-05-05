# Neural Networks

## RNN - Recurrent Neural Network

[Reference project](Adapted from: https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/)


## LSTM - Long Short-Term Memory

Reference project: [lstmcpp](https://github.com/anudeepadi/lstmcpp)
Documentation: [https://medium.com/analytics-vidhya/lstms-explained-a-complete-technically-accurate-conceptual-guide-with-keras-2a650327e8f2](https://medium.com/analytics-vidhya/lstms-explained-a-complete-technically-accurate-conceptual-guide-with-keras-2a650327e8f2)
Possibly useful project: [kaldi-lstm](https://github.com/dophist/kaldi-lstmr)


## GRU - Gated Recurrent Unit

Wiki: [Gated recurrent unit](https://en.wikipedia.org/wiki/Gated_recurrent_unit)
Reference Platform: [kirit93/GRU](https://github.com/kirit93/GRU/tree/b173d0f4fd739b7961914ba4543fd57a7c3dded6)

Took an earlier commit, because the latest doesn't work (missing input file)

The convergense is dismal:

With default settings:

| input_dim     | = | 64;      |
| hidden_dim    | = | 128;     |
| output_dim    | = | 64;      |
| learning_rate | = | 0.0005f; |
| nepochs       | = | 1000;    |
| time_steps    | = | 20;      |
| decay         | = | 0.000f;  |

**276 epochs in 12 hours 33 minutes.**
