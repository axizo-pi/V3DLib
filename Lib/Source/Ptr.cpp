#include "Ptr.h"
#include "Common/SharedArray.h"
#include "Lang.h"  // comment()

namespace V3DLib {

///////////////////////////////////////////////////////////////////////////////
// Class PointerExpr
///////////////////////////////////////////////////////////////////////////////

PointerExpr::PointerExpr(Expr::Ptr e) : BaseExpr(e, "PointerExpr") {}
PointerExpr::PointerExpr(BaseExpr const &e) : BaseExpr(e.expr(), "PointerExpr") {}

PointerExpr &PointerExpr::self() { return *(const_cast<PointerExpr *>(this)); }

PointerExpr PointerExpr::add(IntExpr b) {
  Expr::Ptr e = mkApply(expr(), Op(ADD, INT32), (b << 2).expr());
  return PointerExpr(e);
}


///////////////////////////////////////////////////////////////////////////////
// Class Pointer
///////////////////////////////////////////////////////////////////////////////

namespace {

PointerExpr bare_addself(Pointer &self, IntExpr b) {
	return mkApply(self.expr(), Op(ADD, INT32), b.expr());
}

}  // anon namespace


Pointer::Pointer() : BaseExpr("Ptr") {}


Pointer::Pointer(PointerExpr rhs) : Pointer() {
  assign(expr(), rhs.expr());
}


PointerExpr Pointer::operator=(PointerExpr rhs) { assign(expr(), rhs.expr()); return rhs; }

PointerExpr Pointer::operator+(int b)       { return add(b); }
PointerExpr Pointer::operator+=(IntExpr b)  { return addself(b); }
PointerExpr Pointer::operator+=(int b)      { return addself(b); }
PointerExpr Pointer::operator+(IntExpr b)   { return add(b); }
PointerExpr Pointer::operator-(IntExpr b)   { return sub(b); }

PointerExpr Pointer::add(int b)     const { return mkApply(expr(), Op(ADD, INT32), mkIntLit(4*b)); }
PointerExpr Pointer::add(IntExpr b) const { return mkApply(expr(), Op(ADD, INT32), (b << 2).expr()); }
PointerExpr Pointer::sub(IntExpr b) const { return mkApply(expr(), Op(SUB, INT32), (b << 2).expr()); }
PointerExpr Pointer::addself(int b)       { return (self() = self() + b); }
PointerExpr Pointer::addself(IntExpr b)   { return (self() = self() + b); }
PointerExpr Pointer::subself(IntExpr b)   { return (self() = self() - b); }



Pointer &Pointer::self() {
  return *(const_cast<Pointer *>(this));
}


void Pointer::reset_increment() {
  // Does nothing for now
  // TODO cleanup
  // increment.reset(nullptr);
}


void Pointer::inc() {
/*	
  std::unique_ptr<Int> e;
  int const INC = 16*4;  // for getting next block for a sequential pointer
  e.reset(new Int(INC));  comment("pointer increment");
*/
  Expr::Ptr e = std::make_shared<Expr>(Var_64());

  self() = bare_addself(*this, e);  comment("increment pointer");
}


/**
 * Convenience function to get rid of the pesky '<<2' needed for ptr's.
 * The factor 4 must now be in the offset.
 *
 * This, however does 'add tmp, ...; mov dst, tmp'. I would rather see it
 * add directly.
 */
PointerExpr Pointer::offset(IntExpr b) {
	return mkApply(expr(), Op(ADD, INT32), b.expr());
}


Expr::Ptr Pointer::getUniformPtr() {
  Expr::Ptr e = std::make_shared<Expr>(Var(UNIFORM, true));
  return e;
}


bool Pointer::passParam(IntList &uniforms, BaseSharedArray const *p) {
  uniforms.append(p->getAddress());
  return true;
}


PointerExpr devnull() {
  assertq(!Platform::compiling_for_vc4(), "devnull() is for v3d only", true);
  Expr::Ptr e = std::make_shared<Expr>(Var(STANDARD, RSV_DEVNULL));
  return PointerExpr(e);
}

}  // namespace V3DLib
