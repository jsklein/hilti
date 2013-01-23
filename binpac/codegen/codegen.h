
#ifndef BINPAC_CODEGEN_CODEGEN_H
#define BINPAC_CODEGEN_CODEGEN_H

#include "../common.h"

namespace hilti {
    class Expression;
    class Module;
    class Type;
    class ID;

    namespace builder {
        class ModuleBuilder;
        class BlockBuilder;
    }

    namespace declaration {
        class Function;
    }
}

namespace binpac {

namespace codegen {
    class CodeBuilder;
    class TypeBuilder;
    class ParserBuilder;
}

class CodeGen : public ast::Logger
{
public:
    /// Constructor.
    CodeGen();
    virtual ~CodeGen();

    /// Compiles a BinPAC++ module into a HILTI module.
    ///
    /// module: The module to compile.
    ///
    /// debug: Debug level for the generated code.
    ///
    /// verify: True if the generated HILTI code should be run HILTI's verifier (usually a good idea...)
    ///
    /// Returns: The HILTI module, or null if an error occured.
    shared_ptr<hilti::Module> compile(shared_ptr<Module> module, int debug, bool verify);

    /// Returns the currently compiled module, or null if none.
    shared_ptr<Module> module() const;

    /// Returns the current HILTI module builder.
    shared_ptr<hilti::builder::ModuleBuilder> moduleBuilder() const;

    /// Returns the current HILTI block builder.
    shared_ptr<hilti::builder::BlockBuilder> builder() const;

    /// Returns the debug level.
    int debugLevel() const;

    /// Returns the type of the currently parsed unit. The method must only
    /// be called when parsing is in progress.
    shared_ptr<type::Unit> unit() const;

    /// Returns the HILTI epxression resulting from evaluating a BinPAC
    /// expression. The method adds the code to the current HILTI block.
    ///
    /// expr: The expression to evaluate.
    ///
    /// coerce_to: If given, the expr is first coerced into this type before
    /// it's evaluated. It's assumed that the coercion is legal and supported.
    ///
    /// Returns: The computed HILTI expression.
    shared_ptr<hilti::Expression> hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to = nullptr);

    /// Compiles a BinPAC statement into HILTI. The method adds the code to
    /// current HILTI block.
    ///
    /// stmt: The statement to compile
    void hiltiStatement(shared_ptr<Statement> stmt);

    /// Converts a BinPAC type into its corresponding HILTI type.
    ///
    /// type: The type to convert.
    ///
    /// Returns: The HILTI type.
    shared_ptr<hilti::Type> hiltiType(shared_ptr<Type> type);

    /// Coerces a HILTI expression of one BinPAC type into another. The
    /// method assumes that the coercion is legel.
    ///
    /// expr: The HILTI expression to coerce.
    ///
    /// src: The BinPAC type that \a expr currently has.
    ///
    /// dst: The BinPAC type that \a expr is to be coerced into.
    ///
    /// Returns: The coerced expression.
    shared_ptr<hilti::Expression> hiltiCoerce(shared_ptr<hilti::Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst);

    /// Returns the default value for instances of a BinPAC type that aren't
    /// further intiailized.
    ///
    /// type: The type to convert.
    ///
    /// null_on_default: If true, returns null if the type uses the HILTI
    /// default as its default.
    ///
    /// can_be_unset: If true, the method can return null to indicate that a
    /// value should be left unset by default. If false, there will always a
    /// value returned (unless \a null_on_default is set).
    ///
    /// Returns: The HILTI value, or null for HILTI's default.
    shared_ptr<hilti::Expression> hiltiDefault(shared_ptr<Type> type, bool null_on_default, bool can_be_unset);

    /// Turns a BinPAC ID into a HILTI ID.
    ///
    /// id: The ID to convert.
    ///
    /// qualify: If true, the ID will be scoped acoording to the current
    /// module. That means that if it's parent module is a different one, a
    /// corresponding namespace will be inserted.
    ///
    /// Returns: The converted ID.
    shared_ptr<hilti::ID> hiltiID(shared_ptr<ID> id, bool qualify = false);

    /// Return the parse function for a unit type. If it doesn't exist yet,
    /// it will be created.
    ///
    /// u: The unit to generate the parser for.
    ///
    /// Returns: The generated HILTI function with the parsing code.
    shared_ptr<hilti::Expression> hiltiParseFunction(shared_ptr<type::Unit> u);

    /// Generates the externally visible functions for parsing a unit type.
    ///
    /// u: The unit type to export via functions.
    void hiltiExportParser(shared_ptr<type::Unit> unit);

    /// Generates the implementation of unit-embedded hooks.
    ///
    /// u: The unit type to generate the hooks for.
    void hiltiUnitHooks(shared_ptr<type::Unit> unit);

    /// Generates code to execute the hooks associated with an unit item.
    /// This must only be called while a unit is being parsed.
    ///
    /// f: The field.
    void hiltiRunFieldHooks(shared_ptr<type::unit::Item> item);

    // Returns the HILTI struct type for a unit's parse object.
    //
    /// u: The unit to return the type for.
    ///
    /// Returns: The type.
    shared_ptr<hilti::Type> hiltiTypeParseObject(shared_ptr<type::Unit> unit);

    /// Returns the HILTI reference type for a unit's parse object.
    ///
    /// u: The unit to return the type for.
    ///
    /// Returns: The type.
    shared_ptr<hilti::Type> hiltiTypeParseObjectRef(shared_ptr<type::Unit> unit);

    /// Adds an external implementation of a unit hook.
    ///
    /// id: The hook's ID (full path).
    ///
    /// hook: The hook itself.
    void hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook);

    /// Adds a function to the current module.
    ///
    /// function: The function.
    shared_ptr<hilti::declaration::Function> hiltiDefineFunction(shared_ptr<Function> func);

    /// Calls a function.
    ///
    /// func: The function to call.
    ///
    /// args: The arguments.
    ///
    /// cookie: If we are currently parsing, the user's cookie; otherwise null.
    ///
    /// Returns: The function's result, or null for void functions.
    shared_ptr<hilti::Expression> hiltiCall(shared_ptr<expression::Function> func, const expression_list& args, shared_ptr<hilti::Expression> cookie);

    /// Returns a HILTI expression referencing the current parser object
    /// (assuming parsing is in process; if not aborts());
    ///
    shared_ptr<hilti::Expression> hiltiSelf();
    /// Returns a HILTI expression referencing the current user cookie
    /// (assuming parsing is in process; if not aborts());
    shared_ptr<hilti::Expression> hiltiCookie();

    /// Returns the HILTI type for the cookie argument.
    shared_ptr<hilti::Type> hiltiTypeCookie();

    /// Returns the HILTI-level name for a function.
    shared_ptr<hilti::ID> hiltiFunctionName(shared_ptr<binpac::Function> func);

    /// Returns the HILTI-level name for a function.
    shared_ptr<hilti::ID> hiltiFunctionName(shared_ptr<expression::Function> func);

    /// Binds the $$ identifier to a given value.
    ///
    /// val: The value.
    void hiltiBindDollarDollar(shared_ptr<hilti::Expression> val);

    /// Unbinds the $$ identifier.
    void hiltiUnbindDollarDollar();

    /// Returns the new() function that instantiates a new parser object. In
    /// addition to the type parameters, the returned function receives two
    /// parameters: a ``hlt_sink *`` with the sink the parser is connected to
    /// (NULL if none); and a ``bytes`` object with the MIME type associated
    /// with the input (empty if None). The function also runs the %init hook.
    ///
    /// unit: The unit type to return the function for.
    shared_ptr<hilti::Expression> hiltiFunctionNew(shared_ptr<type::Unit> unit);

    /// Converts an enum value from one type into an enum value of another by
    /// casting the integer value over.
    shared_ptr<hilti::Expression> hiltiCastEnum(shared_ptr<hilti::Expression> val, shared_ptr<hilti::Type> dst);

    /// Writes a new chunk of data into a field's sinks.
    ///
    /// field: The field.
    ///
    /// data: The data to write into the sink.
    void hiltiWriteToSinks(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> data);

    /// Writes a new chunk of data into a sink.
    ///
    /// sink: The sink to write to.
    ///
    /// data: The data to write into the sink.
    void hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> data);

    /// Writes a new chunk of data into a sink.
    ///
    /// begin: Start iterator of data to write.
    ///
    /// end: End iterator of data to write.
    ///
    /// data: The data to write into the sink.
    void hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> begin, shared_ptr<hilti::Expression> end);

    /// Applies transformation attributes (such as \a convert) to a value, if
    /// present.
    ///
    /// val: The value.
    ///
    /// attrs: The set of attributes to apply. The method filters for the
    /// ones supported and ignores all other ones.
    ///
    /// Returns: The transformed value, which may be just \c val if no
    /// transformation applied.
    shared_ptr<hilti::Expression> hiltiApplyAttributesToValue(shared_ptr<hilti::Expression> val, shared_ptr<AttributeSet> attrs);

    /// XXX
    void hiltiImportType(shared_ptr<ID> id, shared_ptr<Type> t);

private:
    bool _compiling = false;
    shared_ptr<Module> _module = nullptr;
    std::map<string, shared_ptr<Type>> _imported_types;

    int _debug;
    bool _verify;

    shared_ptr<hilti::builder::ModuleBuilder> _mbuilder;

    unique_ptr<codegen::CodeBuilder>     _code_builder;
    unique_ptr<codegen::ParserBuilder>   _parser_builder;
    unique_ptr<codegen::TypeBuilder>     _type_builder;
    unique_ptr<Coercer>     _coercer;
};

}

#endif