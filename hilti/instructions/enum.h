/// \type Enums
///
/// The ``enum`` data type represents a selection of unique values,
/// identified by labels. Enums must be defined in global space:
/// 
///     enum Foo { Red, Green, Blue }
/// 
/// The individual labels can then be used as global identifiers. In addition
/// to the given labels, each ``enum`` type also implicitly defines one
/// additional label called ''Undef''.
/// 
/// If not explictly initialized, enums are set to ``Undef`` initially.
/// 
/// TODO: Extend descriptions with the new semantics regarding storing
/// undefined values.
/// 
/// \default Undef
///
/// \cproto hlt_enum

iBegin(enum_, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::enum_, true);
    iOp2(optype::enum_, true);

    iValidate {
        auto ty_op1 = as<type::Enum>(op1->type());
        auto ty_op2 = as<type::Enum>(op2->type());

        if ( ty_op1 != ty_op2 )
            error(op2, "operands must be of the same enum type");
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")
iEnd

iBegin(enum_, FromInt, "enum.from_int")
    iTarget(optype::enum_)
    iOp1(optype::integer, true)

    iValidate {
    }

    iDoc(R"(    
        Converts the integer *op2* into an enum of the target's *type*. If the
        integer corresponds to a valid label (as set by explicit
        initialization), the results corresponds to that label. If integer
        does not correspond to any lable, the result will be of value
        ``Undef`` (yet internally, the integer value is retained.)
    )")

iEnd

