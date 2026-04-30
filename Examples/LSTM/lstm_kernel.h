#ifndef _LSTM_LSTM_KERNEL_H
#define _LSTM_LSTM_KERNEL_H

namespace V3DLib {
class BaseKernel;  // Forward declaration
}

V3DLib::BaseKernel &update_gate_kernel();
V3DLib::BaseKernel &gradient_previous_state_input_kernel();
V3DLib::BaseKernel &gradient_cell_state_kernel();
V3DLib::BaseKernel &gradient_input_gate_kernel();
V3DLib::BaseKernel &gradient_output_gate_kernel();
V3DLib::BaseKernel &gradient_candidate_kernel();
V3DLib::BaseKernel &gradient_forget_kernel();

#endif // _LSTM_LSTM_KERNEL_H
