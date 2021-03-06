
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::address::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    // TODO: Not sure which version is faster.

#if 1
    auto v1 = cg()->llvmExtractValue(op1, 0);
    auto v2 = cg()->llvmExtractValue(op2, 0);
    auto cmp1 = cg()->builder()->CreateICmpEQ(v1, v2);

    v1 = cg()->llvmExtractValue(op1, 1);
    v2 = cg()->llvmExtractValue(op2, 1);
    auto cmp2 = cg()->builder()->CreateICmpEQ(v1, v2);

    auto result = cg()->builder()->CreateAnd(cmp1, cmp2);

#else
    auto p1 = cg()->llvmCreateAlloca(op1->getType());
    auto p2 = cg()->llvmCreateAlloca(op2->getType());

    cg()->llvmCreateStore(op1, p1);
    cg()->llvmCreateStore(op2, p2);

    auto size = cg()->llvmSizeOf(op1->getType());
    auto result = cg()->llvmMemEqual(p1, p2, size);
#endif

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::address::Family* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto c = cg()->llvmExtractValue(op1, 0);
    auto cmp1 = cg()->builder()->CreateICmpEQ(c, cg()->llvmConstInt(0, 64));

    auto v = cg()->llvmExtractValue(op1, 1);
    auto masked = cg()->builder()->CreateAnd(v, cg()->llvmConstInt(0xffffffff00000000, 64));
    auto cmp2 = cg()->builder()->CreateICmpEQ(masked, cg()->llvmConstInt(0, 64));

    auto v4 = cg()->llvmEnum("Hilti::AddrFamily::IPv4");
    auto v6 = cg()->llvmEnum("Hilti::AddrFamily::IPv6");

    auto and_ = cg()->builder()->CreateAnd(cmp1, cmp2);
    auto result = cg()->builder()->CreateSelect(and_, v4, v6);

    cg()->llvmStore(i, result);
}

