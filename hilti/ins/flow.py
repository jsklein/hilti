# $Id$
#
# Control flow operations. 

from instruction import *
from type import *

@instruction("jump", op1=Label)
class Jump(Instruction):
    """
    Jumps unconditionally to label *op2*.
    """
    pass

@instruction("return.void")
class ReturnVoid(Instruction):
    """
    Returns from the current function without returning any value.
    The function's signature must not specifiy a return value. 
    """
    pass

@instruction("return.result", op1=Optional(Integer))
class ReturnResult(Instruction):
    """
    Returns from the current function with the given value.
    The type of the value must match the function's signature. 
    """
    pass

@instruction("if.else", op1=Bool, op2=TupleType((Label, Label)))
class IfElse(Instruction):
    """
    Jumps to label *op2[0]* if *op1* is true, otherwise to *op2[1]*.
    """
    pass

@instruction("call", op1=Function, op2=AnyTuple, target=Any)
class Call(Instruction):
    """
    Calls *function* using the tuple in *op2* as 
    arguments. The argument types as well as the target's type must 
    match the function's signature.
    """
    pass

@instruction("call.tail.void", op1=Function, op2=AnyTuple, op3=Label)
class CallTailVoid(Instruction):
    """
    For internal use only.
    
    Calls *function* using the tuple in *op2* as arguments. The
    argument types must match the function's signature, and the
    function must not return a value. Different than *Call()*,
    *CallTailVoid* must be the *last instruction* of a *Block*.
    After the called function returns, control is passed to 
    block *op3*.
    
    This instruction is for internal use only. During the code
    generation, all *Call* instructions are converted into either
    *CallTailVoid* or *CallTailResult* instructions. 
    """
    pass

@instruction("call.tail.result", op1=Function, op2=AnyTuple, op3=Label, target=Any)
class CallTailResult(Instruction):
    """
    
    Like *Call()*, calls *function* using the tuple in *op2* as 
    arguments. The argument types must match the function's
    signature. The function must return a result value and it's type
    must likewise match the function's signature. Different than
    *Call()*, *CallTailVoid* must be the *last instruction* of a
    *Block*.  After the called function returns, control is passed
    to block *op3*.

    
    This instruction is for internal use only. During the code
    generation, all *Call* instructions are converted into either
    *CallTailVoid* or *CallTailResult* instructions. 
    """
    pass
