# Neural Networks

## RNN - Recurrent Neural Network

Works great.

[Reference project](https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/)


## LSTM - Long Short-Term Memory

Has issues.

- Reference project: [lstmcpp](https://github.com/anudeepadi/lstmcpp)  
- [Documentation](https://medium.com/analytics-vidhya/lstms-explained-a-complete-technically-accurate-conceptual-guide-with-keras-2a650327e8f2)
- Possibly useful project: [kaldi-lstm](https://github.com/dophist/kaldi-lstmr)


## GRU - Gated Recurrent Unit

The convergence is dismal and the running time hopeless; see test run below.

I'm looking for a better reference project; `nnetcpp` is the current candidate.

- Wiki: [Gated recurrent unit](https://en.wikipedia.org/wiki/Gated_recurrent_unit)  
- Reference Platform: [kirit93/GRU](https://github.com/kirit93/GRU/tree/b173d0f4fd739b7961914ba4543fd57a7c3dded6)


-------------------------------------------
## Other Candidates

Both projects are `C++` libraries which support GRU, RNN, LSTM.

- [nnetcpp](https://github.com/steckdenis/nnetcpp/tree/master)
- [SUDL](https://github.com/kymo/SUDL/tree/master)

Each project has its own design principles which basically bury the calculations.
Because the operations are not transparent, I am currently hesitant to use these libraries.


-------------------------------------------
## Test run for GRU 

- Run date: 20261105
- QPU no convergence
- QPU runs >7x longer

### GRU C++ run (original)
    
- CTRL-C'd after Epoch 3
    
    > time ./train
    Size of input file is : 332143
    Epoch: 0
    Loss: 2.52096, Epoch: 0
    Writing weights to file. 
    Epoch: 1
    Loss: 2.02756, Epoch: 1
    Writing weights to file. 
    Epoch: 2
    Loss: 1.76053, Epoch: 2
    Writing weights to file. 
    Epoch: 3
    Loss: 1.52805, Epoch: 3
    Writing weights to file. 
    Epoch: 4
    ^C
    
    real	7m40.890s
    user	7m39.000s
    sys	0m1.840s

### QPU

    Doing train
    [2026-11-06 05:08:22] WARNING: train loop epoch: 0, limit: 6642
    Loss: 4.15873, Epoch: 0
    Writing weights to file. 
    [2026-11-06 05:23:13] WARNING: train loop epoch: 1, limit: 6642
    Loss: 4.15868, Epoch: 1
    Writing weights to file. 
    [2026-11-06 05:38:04] WARNING: train loop epoch: 2, limit: 6642
    Loss: 4.15882, Epoch: 2
    Writing weights to file. 
    Adjusting learning rate to 0.0005
    Loss: 4.15886, Epoch: 3
    Writing weights to file. 
    Adjusting learning rate to 0.0005

    real	59m5.202s
    user	0m0.004s
    sys	0m0.004s

