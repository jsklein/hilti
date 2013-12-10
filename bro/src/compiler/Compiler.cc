
#include <dlfcn.h>

#include <Traverse.h>
#include <Scope.h>
#include <Dict.h>
#include <Hash.h>
#include <Func.h>
#include <Obj.h>
#undef List

#include <util/util.h>

#include "Compiler.h"
#include "ModuleBuilder.h"
#include "../LocalReporter.h"

namespace BifConst { namespace Hilti {  extern int save_hilti;  }  }

using namespace bro::hilti::compiler;

// Walk the Bro AST to normalize nodes in preparation for compiling into
// HILTI.
class NormalizerCallBack : public ::TraversalCallback {
public:
	NormalizerCallBack(Compiler* compiler) : compiler(compiler) {};
	TraversalCode PreID(const ::ID* id) override;
	TraversalCode PreExpr(const ::Expr*) override;
	TraversalCode PreTypedef(const ::ID*) override;

	void Finalize();

private:
	void NormalizeFunction(const ::Func* func);

	Compiler* compiler;

	std::map<std::string, ::ID*> new_globals;
};

void NormalizerCallBack::Finalize()
	{
	for ( auto g : new_globals )
		global_scope()->Insert(g.first.c_str(), g.second);
	}

void NormalizerCallBack::NormalizeFunction(const ::Func* f)
	{
	if ( f->Name() != string("anonymous-function") )
		return;

	// We mess with Bro's symbol table here: We give anonymous functions
	// a unique (but deterministic) global name and then register them in
	// global scope so that the runtime can find them.
	ODesc d;
	f->Describe(&d);
	auto hash = ::util::hash(d.Description());
	auto nname = ::util::fmt("anonymous_function_%s", ::util::uitoa_n(hash, 64, 5));

	const_cast<::Func *>(f)->SetName(nname.c_str());

	//auto name = ::util::fmt("GLOBAL::%s", nname.c_str());
	auto id = new ::ID(nname.c_str(), ::SCOPE_GLOBAL, false);
	id->SetVal(new ::Val(const_cast<::Func *>(f)));
	id->SetConst();

	new_globals.insert(std::make_pair(nname.c_str(), id));
	}

TraversalCode NormalizerCallBack::PreID(const ::ID* id)
	{
	if ( (! id->ID_Val()) && id->Type()->Tag() == TYPE_FUNC && compiler->HaveHiltiBif(id->Name()) )
		{
		// A HILTI-only built-in function won't have a value. Fake one.
		auto non_const_id = const_cast<::ID *>(id);
		auto fval = new ::Val(new ::BuiltinFunc(0, id->Name(), true));
		non_const_id->SetVal(fval);
		}

	return TC_CONTINUE;
	}

TraversalCode NormalizerCallBack::PreExpr(const ::Expr* expr)
	{
	if ( expr->Tag() == EXPR_CONST )
		{
		auto val = dynamic_cast<const ::ConstExpr *>(expr)->Value();
		assert(val);

		if ( val->Type()->Tag() == TYPE_FUNC )
			NormalizeFunction(val->AsFunc());
		}

	return TC_CONTINUE;
	}

TraversalCode NormalizerCallBack::PreTypedef(const ::ID* id)
	{
	auto t = id->Type();

	if ( t->Tag() == TYPE_RECORD )
		{
		// Go through the fields an normalize any function constant
		// we have have as defaul.t
		auto rt = t->AsRecordType();

		for ( int i = 0; i < rt->NumFields(); i++ )
			{
			if ( rt->FieldType(i)->Tag() == TYPE_FUNC && rt->FieldDefault(i) )
				NormalizeFunction(rt->FieldDefault(i)->AsFunc());
			}
		}

	return TC_CONTINUE;
	}

// Walk to Bro AST to gather the interesting pieces for us to compile.
class bro::hilti::compiler::CollectorCallback : public TraversalCallback {
public:
	CollectorCallback();

	bool Traverse();

	std::list<const Func *> Functions(const string& ns);
	std::list<const ::ID *> Globals(const string& ns);

protected:
	TraversalCode PreFunction(const ::Func*) override;
	TraversalCode PostFunction(const ::Func*) override;
	TraversalCode PreStmt(const ::Stmt*) override;
	TraversalCode PostStmt(const ::Stmt*) override;
	TraversalCode PreExpr(const ::Expr*) override;
	TraversalCode PostExpr(const ::Expr*) override;
	TraversalCode PreID(const ::ID*) override;
	TraversalCode PostID(const ::ID*) override;
	TraversalCode PreTypedef(const ::ID*) override;
	TraversalCode PostTypedef(const ::ID*) override;
	TraversalCode PreDecl(const ::ID*) override;
	TraversalCode PostDecl(const ::ID*) override;

private:
	typedef std::set<const ::Func *> function_set;
	typedef std::list<const ::Func *> function_list;
	typedef std::list<const ::ID *> id_list;
	typedef std::map<string, std::shared_ptr<id_list>> ns_map;
	typedef std::set<string> name_set;
	ns_map namespaces;
	name_set used_ids;
	int level;
};

CollectorCallback::CollectorCallback()
	{
	level = 0;
	}

std::list<const ::Func *> CollectorCallback::Functions(const string& ns)
	{
	function_list functions;
	function_set sfunctions;

	auto n = namespaces.find(ns);

	if ( n == namespaces.end() )
		return functions;

	for ( auto id : *(*n).second )
		{
		if ( ! id->ID_Val() )
			continue;

		if ( id->ID_Val()->Type()->Tag() == TYPE_FUNC )
			{
			auto func = id->ID_Val()->AsFunc();

			if ( func->Flavor() == ::FUNC_FLAVOR_FUNCTION &&
			     used_ids.find(id->Name()) == used_ids.end() &&
			     ! ::util::startsWith(id->Name(), "anonymous_function") )
				continue;

			if ( ! func->HasBodies() )
				continue;

			assert(func);
			sfunctions.insert(func);
			}
		}

	for ( auto f : sfunctions )
		functions.push_back(f);

	functions.sort([](const Func* a, const Func* b) -> bool {
		return strcmp(a->Name(), b->Name()) < 0;
	});

	return functions;
	}

std::list<const ::ID *> CollectorCallback::Globals(const string& ns)
	{
	std::list<const ::ID *> globals;
	std::set<const ::ID *>  sglobals;

#if LIMIT_COMPILED_SYMBOLS
	if ( ns != "GLOBAL" && ns != "Log" )
		// FIXME: Remove once we can compile all globals.
		return globals;
#endif

	auto n = namespaces.find(ns);

	if ( n == namespaces.end() )
		return globals;

	for ( auto id : *(*n).second )
		{
		if ( ! id->IsGlobal() )
			continue;

		if ( id->IsConst() )
			continue;

		if ( id->HasVal() && id->ID_Val()->Type()->Tag() == TYPE_FUNC )
			continue;

		if ( id->AsType() )
			continue;

		if ( used_ids.find(id->Name()) == used_ids.end() )
			continue;

		sglobals.insert(id);
		}

	for ( auto id : sglobals )
		globals.push_back(id);

	globals.sort([](const ::ID* a, const ::ID* b) -> bool {
		return strcmp(a->Name(), b->Name()) < 0;
	});

	return globals;
	}

TraversalCode CollectorCallback::PreID(const ::ID* id)
	{
	if ( ! id->IsGlobal() )
		{
		++level;
		return TC_CONTINUE;
		}

	auto n = namespaces.find(id->ModuleName());

	if ( n == namespaces.end() )
		n = namespaces.insert(std::make_pair(id->ModuleName(), std::make_shared<id_list>())).first;

	n->second->push_back(id);

	if ( level > 0 )
		used_ids.insert(id->Name());

	++level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PostID(const ::ID*)
	{
	--level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PreFunction(const ::Func*)
	{
	++level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PostFunction(const ::Func*)
	{
	--level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PreStmt(const ::Stmt*)
	{
	++level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PostStmt(const ::Stmt*)
	{
	--level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PreExpr(const ::Expr*)
	{
	++level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PostExpr(const ::Expr*)
	{
	--level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PreTypedef(const ::ID*)
	{
	++level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PostTypedef(const ::ID*)
	{
	--level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PreDecl(const ::ID*)
	{
	++level;
	return TC_CONTINUE;
	}

TraversalCode CollectorCallback::PostDecl(const ::ID*)
	{
	--level;
	return TC_CONTINUE;
	}

Compiler::Compiler(shared_ptr<::hilti::CompilerContext> arg_hilti_context)
	{
	hilti_context = arg_hilti_context;

	auto glue_id = ::hilti::builder::id::node("BroGlue");
	glue_module_builder = std::make_shared<::hilti::builder::ModuleBuilder>(hilti_context, glue_id);
	}

Compiler::~Compiler()
	{
	}

std::list<const Func *> Compiler::Functions(const string& ns)
	{
	return collector_callback->Functions(ns);
	}

std::list<const ::ID *> Compiler::Globals(const string& ns)
	{
	return collector_callback->Globals(ns);
	}

std::shared_ptr<::hilti::builder::BlockBuilder> Compiler::Builder() const
	{
	return mbuilders.back()->builder();
	}

::hilti::builder::ModuleBuilder* Compiler::ModuleBuilder()
	{
	return mbuilders.back();
	}

void Compiler::pushModuleBuilder(::hilti::builder::ModuleBuilder* mbuilder)
	{
	mbuilders.push_back(mbuilder);
	}

void Compiler::popModuleBuilder()
	{
	assert(mbuilders.size() > 0);
	mbuilders.pop_back();
	}

extern ::Stmt* stmts;

Compiler::module_list Compiler::CompileAll()
	{
	collector_callback = std::make_shared<CollectorCallback>();

	if ( ::stmts )
		{
		NormalizerCallBack cb(this);
		traverse_all(&cb);
		cb.Finalize();

		traverse_all(collector_callback.get());
		}

	Compiler::module_list modules;

	for ( auto ns : GetNamespaces() )
		{
		auto mbuilder = new class ModuleBuilder(this, hilti_context, ns);

		pushModuleBuilder(mbuilder);

		try
			{
			auto module = mbuilder->Compile();

			if ( ! module )
				return Compiler::module_list();

			modules.push_back(module);
			}

		catch ( std::exception& e )
			{
			std::cerr << "Uncaught exception: " << e.what() << std::endl;
			abort();
			}

		popModuleBuilder();

		delete mbuilder;
		}

	return modules;
	}

std::shared_ptr<::hilti::Module> Compiler::FinalizeGlueBuilder()
	{
	if ( BifConst::Hilti::save_hilti )
		glue_module_builder->saveHiltiCode(::util::fmt("bro.%s.hlt", glue_module_builder->module()->id()->name()));

	glue_module_builder->importModule("Hilti");
	glue_module_builder->importModule("LibBro");

	return glue_module_builder->finalize();
	}

::hilti::builder::ModuleBuilder* Compiler::GlueModuleBuilder() const
	{
	return glue_module_builder.get();
	}

std::list<string> Compiler::GetNamespaces() const
	{
	auto ids = global_scope()->GetIDs();

	std::set<string> nsset;

	IterCookie* iter = ids->InitForIteration();

	while ( auto id = ids->NextEntry(iter) )
		nsset.insert(id->ModuleName());

	std::list<string> namespaces;

	for ( auto ns : nsset )
		namespaces.push_back(ns);

	namespaces.sort();
	return namespaces;
	}

string Compiler::HiltiSymbol(const ::Func* func, shared_ptr<::hilti::Module> module, bool include_module)
	{
	return normalizeSymbol(func->Name(), "", "", module ? module->id()->name() : "", true, include_module);
#if 0
	if ( func->GetKind() == Func::BUILTIN_FUNC )
		return func->Name();

	switch ( func->FType()->Flavor() ) {
	case FUNC_FLAVOR_FUNCTION:
		name = ::util::fmt("bro_function_%s", func->Name());
		break;

	case FUNC_FLAVOR_EVENT:
		name = ::util::fmt("bro_event_%s", func->Name());
		break;

	case FUNC_FLAVOR_HOOK:
		name = ::util::fmt("bro_hook_%s", func->Name());
		break;

	default:
		reporter::internal_error("unknown function flavor");
	}

	return ::util::strreplace(name, "::", "_");
#endif
	}

std::string Compiler::HiltiStubSymbol(const ::Func* func, shared_ptr<::hilti::Module> module, bool include_module)
	{
	return normalizeSymbol(func->Name(), "", "stub", module ? module->id()->name() : "", true, include_module);

#if 0
	auto name = func->Name();

	auto var = extract_var_name(name);
	auto mod = extract_module_name(name);

	if ( mod.empty() )
		mod = "GLOBAL";

	mod = ::util::strtolower(mod);

	if ( include_module )
		return ::util::fmt("%s_bro_event_%s_stub", mod, name);
	else
		return ::util::fmt("bro_event_%s_stub", var);
#endif
	}

string Compiler::HiltiSymbol(const ::ID* id, shared_ptr<::hilti::Module> module)
	{
	return normalizeSymbol(id->Name(), "", "", module ? module->id()->name() : "", id->IsGlobal());
	}

string Compiler::HiltiSymbol(const std::string& id, bool global, const std::string& module, bool include_module)
	{
	return normalizeSymbol(id, "", "", module, global, include_module);
	}

std::string Compiler::normalizeSymbol(const std::string sym, const std::string prefix, const std::string postfix, const std::string& module, bool global, bool include_module)
	{
	auto id = ::hilti::builder::id::node(sym);

	auto local = id->local();

	if ( prefix.size() )
		local = ::util::fmt("%s_%s", prefix, local);

	if ( postfix.size() )
		local = ::util::fmt("%s_%s", local, postfix);

	if ( module.size() && ::util::strtolower(id->scope()) == ::util::strtolower(module) )
		return local;

	if ( global && id->scope().size() && ! include_module )
		return ::util::fmt("%s::%s", id->scope(), local);

	if ( include_module )
		{
		auto scope = id->scope();

		if ( scope.empty() )
			scope = "GLOBAL";

		return ::util::fmt("%s_%s", ::util::strtolower(scope), local);
		}

	return local;
	}

std::string Compiler::HiltiODescSymbol(const ::BroObj* obj)
	{
	::ODesc d;
	obj->Describe(&d);

	std::string sym = d.Description();

	for ( auto p = sym.begin(); p != sym.end(); ++p )
		{
		if ( ! isalnum(*p) )
			*p = '_';
		}

	while ( sym.find("__") != std::string::npos )
		sym = ::util::strreplace(sym, "__", "_");

	return sym;
	}

bool Compiler::HaveHiltiBif(const std::string& name, std::string* hilti_name)
	{
	auto bif_symbol = ::util::fmt("%s_bif", HiltiSymbol(name, true, "", true));

	// See if we have a statically compiled bif function available.
	if ( ! dlsym(RTLD_DEFAULT, bif_symbol.c_str()) )
		return false;

	if ( hilti_name )
		*hilti_name = name + "_bif";

	return true;
	}
