# Neural Networks

## RNN - Recurrent Neural Network

[Reference project](https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/)


## LSTM - Long Short-Term Memory

- Reference project: [lstmcpp](https://github.com/anudeepadi/lstmcpp)  
- [Documentation](https://medium.com/analytics-vidhya/lstms-explained-a-complete-technically-accurate-conceptual-guide-with-keras-2a650327e8f2)
- Possibly useful project: [kaldi-lstm](https://github.com/dophist/kaldi-lstmr)


## GRU - Gated Recurrent Unit

- Wiki: [Gated recurrent unit](https://en.wikipedia.org/wiki/Gated_recurrent_unit)  
- Reference Platform: [kirit93/GRU](https://github.com/kirit93/GRU/tree/b173d0f4fd739b7961914ba4543fd57a7c3dded6)

Took an earlier commit, because the latest doesn't work (missing input file).

Gradient calculation is now doing something.
The loss convergence is still dismal.

This might be due to not using the latest commit of this project.

Letting this project rest until I find a solution, or I find a better GRU project
to use as reference. I examine it from time to time.

### Timing:

The convergence is dismal.

Default settings:

    input_dim     = 64;
    hidden_dim    = 128;
    output_dim    = 64;
    learning_rate = 0.0005f;
    nepochs       = 1000;
    time_steps    = 20;
    decay         = 0.000f;

- Original, reference app:  **958 epochs in 45 hours 56 minutes.**. That's **2.88 minutes per epoch**.
- This app: **10 epochs in 133 minutes**. That's over **13 minutes** per epoch. Severely disappointed.....

-------------------------------------------

## Other candidates

Both projects are `C++` libraries which support GRU, RNN, LSTM.

- [netcpp](https://github.com/steckdenis/nnetcpp/tree/master)
- [SUDL](https://github.com/kymo/SUDL/tree/master)

Each project has its own design principles which basically bury the calculations.
Because the operations are not transparent, I am currently hesitant to use these libraries.
