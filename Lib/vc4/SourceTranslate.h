#ifndef _V3DLIB_VC4_SOURCETRANSLATE_H_
#define _V3DLIB_VC4_SOURCETRANSLATE_H_
#include "../SourceTranslate.h"

namespace V3DLib {
namespace vc4 {

using List = V3DLib::Target::Instr::List;

class SourceTranslate : public ISourceTranslate {
  using Parent = ISourceTranslate;

public:
  List load_var(Var &dst, Expr &e) override;
  List store_var(Var dst_addr, Var src) override;
  void regAlloc(List &instrs) override;
  bool stmt(List &seq, Stmt::Ptr s) override; 
};

void add_init_block(List &code);

}  // namespace vc4
}  // namespace V3DLib


#endif  // _V3DLIB_VC4_SOURCETRANSLATE_H_
