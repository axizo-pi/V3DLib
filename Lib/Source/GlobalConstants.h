#ifndef _V3DLIB_SOURCE_GLOBALCONSTANTS_H_
#define _V3DLIB_SOURCE_GLOBALCONSTANTS_H_
#include "Stmt.h"
#include "Float.h"

namespace V3DLib {
namespace GlobalConstants {

void reset();
void init(Stmt::Array &src);

}  // namespace GlobalConstants

Var Var_64();
Var Var_NaN();
Var Var_Inf();
Var Var_MinInf();
Var Var_MinFloat();
Var Var_MaxFloat();

Int   _64();
Float NaN();
Float Inf();
Float MinInf();
Float MinFloat();
Float MaxFloat();

}  // namespace V3DLib

#endif // _V3DLIB_SOURCE_GLOBALCONSTANTS_H_
