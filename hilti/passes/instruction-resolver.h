
#ifndef HILTI_INSTRUCTION_RESOLVER_H
#define HILTI_INSTRUCTION_RESOLVER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Resolves instruction references. Initially, all instruction nodes are
/// creates an instances of statement::instruction::Unresolved. This pass
/// turns them into instances of classes derived from 
/// statement::instruction::Resolved, of which we have one class per
/// instruction type.
class InstructionResolver : public Pass<>
{
public:
   InstructionResolver() : Pass<>("hilti::InstructionResolver", true) {}
   virtual ~InstructionResolver();

   /// Resolves instruction references.
   ///
   /// module: The AST to resolve.
   ///
   /// Returns: True if no errors were encountered.
   bool run(shared_ptr<Node> module, bool report_errors) {
       _report_errors = report_errors;
       return processAllPreOrder(module);
   }

protected:
   void visit(statement::instruction::Unresolved* i) override;
   void visit(statement::instruction::Resolved* i) override;
   void visit(statement::Block *s) override;

private:
   bool run(shared_ptr<Node> module) override { assert(false); return false; }
   void processInstruction(shared_ptr<statement::Instruction> instr, shared_ptr<ID> id);

   bool _report_errors = true;
};

}
}

#endif
