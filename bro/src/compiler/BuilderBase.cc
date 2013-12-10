
#include "DebugLogger.h"
#include "Obj.h"
#include "Expr.h"
#undef List

#include <util/util.h>

#include "BuilderBase.h"
#include "ExpressionBuilder.h"
#include "ModuleBuilder.h"
#include "Compiler.h"
#include "TypeBuilder.h"
#include "ValueBuilder.h"
#include "ConversionBuilder.h"
#include "../LocalReporter.h"

using namespace bro::hilti::compiler;

BuilderBase::BuilderBase(class ModuleBuilder* arg_mbuilder)
	{
	mbuilder = arg_mbuilder;
	}

std::shared_ptr<::hilti::builder::BlockBuilder> BuilderBase::Builder() const
	{
	return mbuilder->Compiler()->Builder();
	}

::hilti::builder::ModuleBuilder* BuilderBase::ModuleBuilder()
	{
	return mbuilder->Compiler()->ModuleBuilder();
	}

::hilti::builder::ModuleBuilder* BuilderBase::GlueModuleBuilder() const
	{
	return mbuilder->Compiler()->GlueModuleBuilder();
	}

void BuilderBase::Error(const std::string& msg, const BroObj* obj) const
	{
	auto s = msg + ::util::fmt(" [%s]", Location(obj));
	DBG_LOG_COMPILER("builder error: %s", s.c_str());

#if 1
	fprintf(stderr, ">> %s\n", msg.c_str());
	abort();
#else
	throw BuilderException(s);
#endif
	}

void BuilderBase::Warning(const std::string& msg, const BroObj* obj) const
	{
	auto s = msg + ::util::fmt(" [%s]", Location(obj));
	DBG_LOG_COMPILER("builder warning: %s", s.c_str());
	reporter::warning(s);
	}

void BuilderBase::InternalError(const std::string& msg, const BroObj* obj) const
	{
	auto s = msg + ::util::fmt(" [%s]", Location(obj));
	DBG_LOG_COMPILER("internal builder error: %s", s.c_str());
	reporter::internal_error(s);
	}

class Compiler* BuilderBase::Compiler() const
	{
	return mbuilder->Compiler();
	}

class ExpressionBuilder* BuilderBase::ExpressionBuilder() const
	{
	return mbuilder->ExpressionBuilder();
	}

class StatementBuilder* BuilderBase::StatementBuilder() const
	{
	return mbuilder->StatementBuilder();
	}

class TypeBuilder* BuilderBase::TypeBuilder() const
	{
	return mbuilder->TypeBuilder();
	}

class ValueBuilder* BuilderBase::ValueBuilder() const
	{
	return mbuilder->ValueBuilder();
	}

class ::bro::hilti::ValueConverter* BuilderBase::ValueConverter() const
	{
	return mbuilder->ValueConverter();
	}

std::string BuilderBase::HiltiSymbol(const ::Func* func, bool include_module)
	{
	return mbuilder->Compiler()->HiltiSymbol(func, mbuilder->module(), include_module);
	}

std::string BuilderBase::HiltiStubSymbol(const ::Func* func, bool include_module)
	{
	return mbuilder->Compiler()->HiltiStubSymbol(func, mbuilder->module(), include_module);
	}

std::string BuilderBase::HiltiSymbol(const ::ID* id)
	{
	return mbuilder->Compiler()->HiltiSymbol(id, mbuilder->module());
	}

std::string BuilderBase::HiltiODescSymbol(const ::BroObj* obj)
	{
	return mbuilder->Compiler()->HiltiODescSymbol(obj);
	}

std::shared_ptr<::hilti::Type> BuilderBase::HiltiType(const ::BroType* type)
	{
	return mbuilder->TypeBuilder()->Compile(type);
	}

std::shared_ptr<::hilti::type::Function> BuilderBase::HiltiFunctionType(const ::FuncType* type)
	{
	return mbuilder->TypeBuilder()->FunctionType(type);
	}

std::shared_ptr<::hilti::Expression> BuilderBase::HiltiValue(const ::Val* val, ::BroType* target_type)
	{
	return mbuilder->ValueBuilder()->Compile(val, target_type);
	}

shared_ptr<::hilti::Expression> BuilderBase::HiltiInitValue(const ::BroType* type)
	{
	return mbuilder->ValueBuilder()->InitValue(type);
	}

shared_ptr<::hilti::Expression> BuilderBase::HiltiBroVal(const ::BroType* type)
	{
	return mbuilder->ValueBuilder()->TypeVal(type);
	}

shared_ptr<::hilti::Expression> BuilderBase::HiltiBroVal(const ::Func* func)
	{
	return mbuilder->ValueBuilder()->FunctionVal(func);
	}

shared_ptr<::hilti::Expression> BuilderBase::HiltiBroType(const ::BroType* type)
	{
	return mbuilder->ValueBuilder()->BroType(type);
	}

std::shared_ptr<::hilti::Expression> BuilderBase::HiltiExpression(const ::Expr* expr, ::BroType* target_type)
	{
	return mbuilder->ExpressionBuilder()->Compile(expr, target_type);
	}

std::shared_ptr<::hilti::Expression> BuilderBase::HiltiIndex(const ::Expr* idx)
	{
	if ( idx->Tag() != EXPR_LIST )
		return HiltiExpression(idx);

	auto lexpr = idx->AsListExpr();

	if ( lexpr->Exprs().length() == 1 )
		return HiltiExpression(lexpr->Exprs()[0]);
	else
		return HiltiExpression(lexpr);
	}

std::shared_ptr<::hilti::Expression> BuilderBase::HiltiStructField(const char* fname)
	{
	return ::hilti::builder::string::create(fname);
	}

shared_ptr<::hilti::Expression> BuilderBase::RuntimeHiltiToVal(shared_ptr<::hilti::Expression> val, const ::BroType* type)
	{
	return mbuilder->ConversionBuilder()->ConvertHiltiToBro(val, type);
	}

shared_ptr<::hilti::Expression> BuilderBase::RuntimeValToHilti(shared_ptr<::hilti::Expression> val, const ::BroType* type)
	{
	return mbuilder->ConversionBuilder()->ConvertBroToHilti(val, type);
	}

shared_ptr<::hilti::Expression> BuilderBase::DeclareFunction(const ::Func* func)
	{
	return mbuilder->DeclareFunction(func);
	}

std::shared_ptr<::hilti::Expression> BuilderBase::DeclareGlobal(const ::ID* id)
	{
	return mbuilder->DeclareGlobal(id);
	}

std::shared_ptr<::hilti::Expression> BuilderBase::DeclareLocal(const ::ID* id)
	{
	return mbuilder->DeclareLocal(id);
	}

const ::Func* BuilderBase::CurrentFunction() const
	{
	return mbuilder->CurrentFunction();
	}

shared_ptr<::hilti::Expression> BuilderBase::HiltiCallFunction(const ::Expr* func, ::FuncType* ftype, ListExpr* args)
	{
	return mbuilder->HiltiCallFunction(func, ftype, args);
	}

void BuilderBase::MapType(const ::BroType* from, const ::BroType* to)
	{
	return mbuilder->ConversionBuilder()->MapType(from, to);
	}

std::string BuilderBase::Location(const ::BroObj *obj) const
	{
	if ( ! obj || ! obj->GetLocationInfo()->filename )
		return "<no location>";

	auto file = obj->GetLocationInfo()->filename;
	auto start = obj->GetLocationInfo()->first_line;
	auto end = obj->GetLocationInfo()->last_line;

	return end && start != end ? ::util::fmt("%s:%d-%d", file, start, end)
		: ::util::fmt("%s:%d", file, start);
	}
