# $Id$
#
# Grammar productions.

import sys
import copy

import scope
import id

import binpac.util as util

class Production(object):
    """Base class for all grammar productions. 
        
    name: string - A name for this production; can be the None for anonymous
    productions.
    
    type: ~~Type - A type for storing the result of parsing this ~~Production.
    Must be given iff *name* is given.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    _counter = 0
    _symbols = {}
    
    def __init__(self, name=None, type=None, symbol=None, location=None):
        assert not name or type
        
        self._name = name
        self._type = type
        self._pred = None
        self._pre = []
        self._post = []
        self._hooks = []
        self._location = location
        self._noid = False

        if not symbol:
            # Internal production name.
            Production._counter += 1
            symbol = "T%d" % Production._counter
            
        else:
            i = 1
            s = symbol
            while s in Production._symbols:
                i += 1
                s = "%s_%d" % (symbol, i)

            symbol = s

        self._symbols[symbol] = 1
        self._symbol = symbol

    def location(self):
        """Returns the location where the production was defined.
        
        Returns: ~~Location - The location.
        """
        return self._location

    def symbol(self):
        """Returns production's unique LHS symbol. 
        
        Returns: string - The symbol.
        """
        return self._symbol
    
    def setName(self, name):
        """Sets the production's name. 
        
        name: string - The name. Setting it to None turns the production into
        an anonymous one. 
        
        Note: When you set a name, you also must set a type.
        """
        self._name = name
        
    def name(self):
        """Returns the production's name.
        
        Returns: string - The name, or None for anonymous productions. 
        """
        return self._name

    def setNoID(self):
        """Sets a flags indicating the HILTI struct using this production
        should not contain an item for its name. 
        """
        self._noid = True
        
    def isNoID(self):
        """Returns whether the HILTI struct uisng this production should not
        contain an item for this field. The default is yes but that can be
        changed with ~~setNoID.
        
        Returns: bool - True if no item should be created.
        """
        return self._noid
    
    def setType(self, type):
        """Sets the production's type. 
        
        tyoe: string - The type. 
        """
        self._type = type
    
    def type(self):
        """Returns the type for storing the result of parsing the production. 

        Returns: ~~Type - The type, or None if not type has been set.
        """
        return self._type.parsedType()
    
    def parsedType(self):
        """Returns the type of the production that is used for parsing it.
        This will usually be the same as ~~type, but may be different like in
        the case of ~~Variable a ~~filter may cause ~~type to be different.
        
        Can be overridden by derived classes. The default implementation
        returns the same as ~~type.
        
        Returns: ~~Type - The type.
        """
        return self.type()
    
    def hooks(self):
        """Returns the hooks associated with the production.
        
        Returns: list of ~~UnitHook - The hooks.
        """
        return self._hooks
        
    def addHook(self, hook):
        """Adds a hook to the production. The hook will be executed when the
        production has been fully parsed.
        
        hook: ~~UnitHook - The hook.
        """
        self._hooks += [hook]
    
    def __str__(self):
        return "%s%s" % (self._fmtLong(), self._fmtName())
    
    def _fmtName(self):
        return " [=: %s]" % self._name if self._name else ""
    
    def _fmtShort(self):
        # To be overridden by derived classes.
        return "<production>"
    
    def _fmtLong(self):
        # To be overridden by derived classes.
        return "<production>"

    def _rhss(self):
        # Must be overriden by derived classes, and return a list of RHS
        # alternatives for this production. Each RHS is itself a list of
        # Production instances.
        util.internal_error("Production._rhss not overridden.")
    
class Epsilon(Production):
    """An empty production.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, location=None):
        super(Epsilon, self).__init__(symbol="epsilon", location=location)
        self._tag = "epsilon"
        
    def _fmtShort(self):
        return "<epsilon>"    
    
    def _fmtLong(self):
        return "<epsilon>"    

    def _rhss(self):
        return [[]]
    
class Terminal(Production):
    """A terminal.
    
    name: string - A name for this production; can be the None for anonymous
    productions.

    type: ~~Type - A type for storing the result of parsing this ~~Production.
    Must be given iff *name* is given.

    expr: ~~Expression or None - An optinoal expression that will be evaluated
    after the terminal has been parsed but before its value is assigned to its
    destination variable. If the expression is given, *its* value is actually
    assigned to the destination variable instead (and also determined the type
    of that). This can be used, e.g., into coerce the parsed value into a
    different type.
    
    location: ~~Location - A location object decscribing the point of definition.
    
    Todo: The functionality for *expr* is not yet implemented, and the
    parameter just stored but otherwise ignored.
    """
    def __init__(self, name, type, expr, location=None):
        super(Terminal, self).__init__(name, type, symbol="terminal", location=location)
        self._expr = expr

    def expr():
        """Returns the expression associated with the production.
        
        Returns: ~~Expression - The expression.
        """
        return self._expr
        
class Literal(Terminal):
    """A literal. A literal is anythig which can be directly scanned for as
    sequence of bytes. 
    
    name: string - A name for this production; can be the None for anonymous
    productions.

    literal: ~~Expression - The literal. Currently, it must be of type ~~Bytes.

    expr: ~~Expression or None - An optional expression to be associated with
    the production. See ~~Terminal for more information.
    
    location: ~~Location - A location object decscribing the point of definition.
    """

    _counter = 1
    _all = {}
    
    def __init__(self, name, literal, expr=None, location=None):
        super(Literal, self).__init__(name, literal.type(), expr)
        self._literal = literal
        
        idx = str(literal.value()) + str(literal.type())
        
        try:
            self._id = Literal._all[idx]
        except:
            self._id = Literal._counter
            Literal._all[idx] = self._id
            Literal._counter += 1

    def literal(self):
        """Returns the literal.
        
        Returns: ~~Constant - The literal.
        """
        return self._literal

    def id(self):
        """Returns a unique for this terminal. The ID is unique across all
        ~~Grammars.
        
        Returns: integer - The id, which will be > 0. 
        """
        return self._id
    
    def _fmtShort(self):
        return "%s" % self.symbol()
    
    def _fmtLong(self):
        return "%s" % self._literal.value()

    def _rhss(self):
        return [[self]]
    
class Variable(Terminal):
    """A variable. A variable is a terminal that will be parsed from the input
    stream according to its type, yet is not recognizable as such in advance
    by just looking at the available bytes. If we start parsing, we assume it
    will match (and if not, generate an error).
    
    name: string - A name for this production; can be the None for anonymous
    productions.

    type: ~~Type - The type of the variable.

    expr: ~~Expression or None - An optional expression to be associated with
    the production. See ~~Terminal for more information.

    filter: ~~Expression - A function called when a value has been parsed for
    this variable. The function's return value is then used instead of the
    parsed value subsequently. None for no filtering. 
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    
    def __init__(self, name, type, expr=None, filter=None, location=None):
        super(Variable, self).__init__(name, type, expr, location=location)
        self._type = type
        self._filter = filter

    def parsedType(self):
        return self._type.parsedType()

    def type(self):
        return self._type.fieldType()
        
    def filter(self):
        """Returns the filter function.
        
        Returns: ~~Function - The filter function, or None if none defined.
        """
        return self._filter
    
    def _rhss(self):
        return [[self]]
        
    def _fmtShort(self):
        return "type(%s)" % self._type

    def _fmtLong(self):
        return "type(%s)" % self._type

class NonTerminal(Production):
    """A non-terminal.
    
    name: string - A name for this production; can be None for anonymous
    productions.
    
    type: ~~Type - A type for storing the result of parsing this ~~Production.
    Must be given iff *name* is given.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, name, type, symbol=None, location=None):
        super(NonTerminal, self).__init__(name, type, symbol=symbol, location=location)

    def _simplify(self, grammar):
        # Can be overriden by derived classes.
        pass
        
    def _fmtShort(self):
        return self.symbol()

    def __str__(self):
        return "%s%s" % (self._fmtLong(), self._fmtName())

class ChildGrammar(NonTerminal):
    """A type described by another independent unit type.
    
    ty: ~~Unit - The unit type to parse. 
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, type, symbol=None, location=None):
        super(NonTerminal, self).__init__(type.name(), type, symbol=symbol, location=location)
        self._params = []
        
    def params(self):
        """Returns the parameters passed into the parser for the child grammar.
        
        Returns: list of ~~Expr - Parameters to pass to the child grammar;
        returns an empty list if none. 
        """
        return self._params
        
    def setParams(self, params):
        """Sets the parameters passed into the parser for the child grammar.
        
        params: list of ~~Expr - Parameters to pass to the child grammar.
        """
        self._params = params
    
    def _rhss(self):
        return [[self.type().grammar().startSymbol()]]

    def _fmtShort(self):
        #return self.type().grammar().name()
        return self.symbol()
    
    def _fmtLong(self):
        prods = self.type().grammar().productions().values()
        return " | ".join([p._fmtShort() for p in prods])
    
class Sequence(NonTerminal):
    """A sequence of other productions. 
    
    name: string - A name for this production; can be the None 
    for anonymous productions.

    type: ~~Type - A type for storing the result of parsing this ~~Production.
    Must be given iff *name* is given.

    seq: list of ~~Production - The sequence of productions. 
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, name=None, type=None, seq=None, symbol=None, location=None):
        super(Sequence, self).__init__(name, type, symbol=symbol, location=location)
        self._seq = seq if seq != None else []

    def sequence(self):
        """Returns the sequence of productions.
        
        Returns: list of ~~Production - The sequence.
        """
        return self._seq

    def addProduction(self, prod):
        """Appends one production to the sequence.
        
        prod: ~~Production - The production.
        """
        self._seq += [prod]
    
    def _rhss(self):
        return [self._seq]

    def _simplify(self, grammar):
        # For productions of the form "A->B; B->C;", let A points to C
        # directly.
        prods = self._seq
        if len(prods) == 1 and isinstance(prods[0], NonTerminal):
            self._seq = [prods[0]]
        
        # For productions of the form "A->xBy ; B -> terminal"',
        for i in range(len(prods)):
            p = prods[i]
            if isinstance(p, NonTerminal):
                succ = grammar._nterms[p.symbol()]
                if isinstance(succ, Sequence) and len(succ.sequence()) == 1:
                    q = succ.sequence()[0]
                    if not isinstance(q, NonTerminal):
                        prods[i] = q
                        
    def _fmtLong(self):
        return "; ".join([p._fmtShort() for p in self._seq])
    
class Alternative(NonTerminal):
    """Base class for productions offering alternative RHSs.
    
    alts: list of ~~Production - The alternatives.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, alts, symbol=None, location=None):
        super(Alternative, self).__init__(name=None, type=None, symbol=symbol, location=location)
        self._alts = alts
        self._prefix = "<prefix missing>"

    def alternatives(self):
        """Returns the alternative productions.
        
        Returns: list of ~~Production - The alternatives.
        """
        return self._alts

    def _setAlternatives(self, alts):
        self._alts = alts
    
    def _rhss(self):
        return [[a] for a in self._alts]
    
    def _setFmtPrefix(self, prefix):
        self._prefix = prefix

    def _fmtLong(self):
        return "( %s| " % self._prefix + " || ".join([a._fmtShort() for a in self._alts]) + " )"
    
class LookAhead(Alternative):
    """A pair of alternatives between which we can decide with one token of look-ahead.
    
    alt1: ~~Production: The first alternative. 
    
    alt2: ~~Production: The second alternative. 
    """
    def __init__(self, alt1, alt2, symbol=None, location=None):
        super(LookAhead, self).__init__([alt1, alt2], symbol=symbol, location=location)
        self._setFmtPrefix("ll1")

    def setAlternatives(self, alt1, alt2):
        """Sets the two alternatives.
        
        alts: tuple (~~Production, ~~Production) - The alternatives.
        """
        self._setAlternatives([alt1, alt2])
        self._alt1 = alt1
        self._alt2 = alt2
        self._laheads = None
        
    def lookAheads(self):
        """Returns the look-aheads for the two alternatives. This function
        must only be called after the instance has been added to a ~Grammar,
        as that's when the look-aheads are computed. 
        
        Returns: 2-tuple (set, set): Two sets representing the look-aheads for
        the first and second alternative, respectively. Each set contains the
        ~~Literals for which the corresponding alternative should be selected.
        """
        assert self._laheads != None
        return self._laheads

    def _setLookAheads(self, laheads):
        self._laheads = laheads
    
class Conditional(Alternative):
    """Base class for productions offering alternative RHSs between we decide
    based on semantic conditions.
    
    alts: list of ~~Production - The alternatives.
    
    location: ~~Location - A location object decscribing the point of definition.
    """
    def __init__(self, alts, symbol=None, location=None):
        super(Conditional, self).__init__(alts, symbol=symbol, location=location)

class Boolean(Conditional):
    """A pair of alternatives between which we decide based on a boolean
    expression.
    
    expr: ~~Expr : An expression of type ~~Bool.

    alt1: ~~Production: The first alternative for the *true* case.
    
    alt2: ~~Production: The second alternative for the *false* case.
    """
    def __init__(self, expr, alt1, alt2, symbol=None, location=None):
        super(Boolean, self).__init__([alt1, alt2], symbol=symbol, location=location)
        self._setFmtPrefix("Bool")
        self._expr = expr
        self._alt1 = alt1
        self._alt2 = alt2

    def expr(self):
        """Returns the conditional expression.
        
        Returns: ~~Expression - The expression of type ~~Bool.
        """
        return self._expr
        
    def branches(self):
        """Returns the two possible branches.
        
        Returns: 2-tuple of ~~Production - The *true* and *false* branches.
        """
        return (self._alt1, self._alt2)
        
class Switch(Conditional):
    """Alternatives between which we decide based on which value out of a set
    of options is matched; plus a default if none.
    
    expr: ~~Expr : An expression. 
    
    alts: list of tuple (expr, ~~Production) - The alternatives. The value of
    *expr* is matched against the evaluated values of the expressions given in
    the list (which must be of the same type); if a match is found the
    corresponding production is selected.
    
    default: ~~Production - Default production if none of *alts* matches. If
    None, a ~~ParseError will be generated if no other alternative matches.
    """
    def __init__(self, expr, alts, default, symbol=None, location=None):
        super(Switch, self).__init__([p for (e, p) in alts] + ([default] if default else []), symbol=symbol, location=location)
        self._setFmtPrefix("Switch")
        self._expr = expr
        self._default = default
        self._cases = alts
        
    def expr(self):
        """Returns the switch expression.
        
        Returns: ~~Expression - The expression.
        """
        return self._expr
    
    def cases(self):
        """Returns the possible cases.
        
        Returns:  list of tuple (~~Expression, ~~Production) - The alternatives.
        """
        return self._cases
    
    def default(self):
        """Returns the default case.
        
        Returns: ~~Production - The default case.
        """
        return self._default
        
class Grammar:
    def __init__(self, name, root, params=None, addl_ids=None, location=None):
        """Instantiates a grammar given its root production.
        
        name: string - A name which uniquely identifies this grammar.
        root: ~~Production - The grammar's root production.
        params: list of ~~ID - Additional parameters passed into the
        grammar's parser. 
        addl_ids: list of ~~ID - Additional IDs that should become part of the
        generated parser object. 
        """
        self._name = name
        self._start = root
        self._productions = {}
        self._scope = scope.Scope(None, None)
        self._nterms = {}
        self._nullable = {}
        self._first = {}
        self._follow = {}
        self._params = params if params else []
        
        self._addProduction(root)
        self._simplify()
        self._simplify()
        self._computeTables()
        self._location = location
        
        if addl_ids:
            for id in addl_ids:
                self._scope.addID(id)
        
    def name(self):
        """Returns the name of the grammar. The name uniquely identifies the
        grammar. 
        
        Returns: string - The name.
        """
        return self._name

    def location(self):
        """Returns the location associated with the grammar.
        
        Returns: ~~Location - The location. 
        """
        return self._location
    
    def params(self):
        """Returns any additionla paramaters passed into the grammar's parser.
        
        Returns: list of ~~ID - The parameters.
        return self._params
        """
        return self._params
    
    def check(self):
        """Checks the grammar for ambiguity. From an ambigious grammar, no
        parser can be generated.
        
        Returns: string or None - If the return value is None, the grammar is
        fine; otherwise the returned string contains a descriptions of the
        ambiguities encountered, suitable for inclusion in an error message.
        """
        
        def _loc(p):
            gloc = " (%s)" % self.location() if self.location() else ""
            gname = "grammar %s%s, " % (self._name, gloc) if self._name else ""
            ploc = " (%s, %s)" % (p.symbol(), p.location()) if p.location() else "(%s)" % p.symbol()
            pname = "production %s%s" % (p.name() if p.name() else "<anonymous>", ploc)
                
            return "%s%s" % (gname, pname)
        
        msg = ""
        
        for p in self._nterms.values():
            
            if not isinstance(p, LookAhead):
                continue
            
            laheads = p.lookAheads()
            
            # This seems ok.
            #
            #if len(laheads[0]) == 0 or len(laheads[1]) == 0:
            #    msg += "%s is not reachable because of empty look-ahead set\n" % _loc(p)

            lid0 = [l.id() for l in laheads[0]]
            lid1 = [l.id() for l in laheads[1]]
                
            isect = set(lid0) & set(lid1)
            if isect:
                # FIMXE: I'm sure we can do the mapping id->names nicer ...
                syms = []
                for l in (laheads[0] | laheads[1]):
                    if l.id() in isect:
                        syms += [str(l.literal())]
                isect = ", ".join(syms)
                msg += "%s is ambigious for look-ahead symbol(s) {%s}\n" % (_loc(p), isect)

            laheads = laheads[0] | laheads[1]
                
            for lahead in laheads:
                if isinstance(lahead, Variable):
                    msg += "%s: look-ahead cannot depend on variable\n" % _loc(p)
                    break
                
        return msg if msg != "" else None

    def productions(self):
        """Returns the grammar's productions. The method returns a dictionary
        that maps each production's ~~symbol to the ~~Production itself. Only
        ~~NonTerminals are included.
        
        Returns: dictionary - The dictionary as described above.
        """
        return self._nterms

    def scope(self):
        """Returns a scope for this grammar. The scope contains all
        identifiers defined by this grammar. 
        
        Returns: ~~Scope - The scope.
        """
        return self._scope
    
    def startSymbol(self):
        """Returns starting production. 
        
        Returns: Productions - The starting instruction.
        """
        return self._start
    
    def _addProduction(self, p):
        if p.symbol() in self._productions:
            # Already added.
            return

        if p.name() and not p.isNoID():
            assert p.type()
            i = id.Attribute(p.name(), p.type())
            self._scope.addID(i)
        
        self._productions[p.symbol()] = p
        
        if isinstance(p, NonTerminal):
            self._nterms[p.symbol()] = p
            
            for rhs in p._rhss():
                for p in rhs:
                    # Add productions recursive.y.
                    self._addProduction(p)
            
    def _simplify(self):
        # Remove unused productions.
        def _closure(root, used):
            
            idx = root.symbol()
            
            if idx in used:
                return
            
            used.add(idx)
            
            for rhs in self._productions[idx]._rhss():
                for p in rhs:
                    _closure(p, used)
                        
            return used
        
        used = _closure(self._start, set())

        for sym in set(self._productions.keys()) - used:
            del self._nterms[sym]
            del self._productions[sym]
            
        # Do some production-specific simplyfing.
        for p in self._nterms.values():
            p._simplify(self)

    def _computeTables(self):
        # Computes FIRST, FOLLOW, & NULLABLE. This follows roughly the Algorithm
        # 3.13 from Modern Compiler Implementation in C by Appel/Ginsburg. See
        # http://books.google.com/books?id=A3yqQuLW5RsC&pg=PA49
        
        def _add(tbl, dst, src, changed):
            idx = dst.symbol()
            if tbl[idx] | set(src) == tbl[idx]:
                return changed
            
            tbl[idx] |= set(src)
            return True
            
        def _nullable(rhs, i, j):
            for l in range(i, j):
                if isinstance(rhs[l], Epsilon):
                    continue
                
                if isinstance(rhs[l], Terminal) or not self._nullable[rhs[l].symbol()]:
                    return False
                
            return True
        
        def _first(prod):
            if isinstance(prod, Epsilon):
                return []
            
            if isinstance(prod, Terminal):
                return [prod]
            
            return self._first[prod.symbol()]

        def _firstOfRhs(rhs):
            first = set()
            
            for prod in rhs:
                if isinstance(prod, Epsilon):
                    continue
                
                if isinstance(prod, Terminal):
                    return [prod]
                
                first |= self._first[prod.symbol()]
                
                if not self._nullable[prod.symbol()]:
                    break
            
            return first

        # Initialize sets.
        for t in self._nterms:
            self._nullable[t] = False
            self._first[t] = set()
            self._follow[t] = set()

        # Iterate until no further change.
        while True:
            changed = False
            for (sym, p) in self._nterms.items():
                for rhs in p._rhss():
                    k = len(rhs)

                    if _nullable(rhs, 0, k) and not self._nullable[sym]:
                        self._nullable[sym] = True
                        changed = True
                        
                    for i in range(k):
                        if _nullable(rhs, 0, i):
                            changed = _add(self._first, p, _first(rhs[i]), changed)

                        if not isinstance(rhs[i], NonTerminal):
                            continue
                                
                        if _nullable(rhs, i+1, k):
                            changed = _add(self._follow, rhs[i], self._follow[sym], changed)

                        for j in range(i+1, k):
                            if _nullable(rhs, i+1, j):
                                changed = _add(self._follow, rhs[i], _first(rhs[j]), changed)
                                    
            if not changed:
                break

        # Build the look-ahead sets.
        for (sym, p) in self._nterms.items():
            if not isinstance(p, LookAhead):
                continue

            laheads = [set(), set()]
            rhss = p._rhss()
            
            assert(len(rhss) == 2)

            for i in (0, 1):
                rhs = rhss[i]
                
                for term in _firstOfRhs(rhs):
                    laheads[i] |= set([term]) 
                                    
                if _nullable(rhs, 0, len(rhs)):
                    for term in self._follow[sym]:
                        laheads[i] |= set([term]) 
                        
            p._setLookAheads(laheads)
        
    def printTables(self, verbose=False, file=sys.stdout):
        """Prints the grammar in a human readable form. In *verbose* mode, the
        grammar and all the internal nullable/first/follow tables are printed.
        
        verbose: bool - True for verbose output.
        file: file - The print destination.
        """
        
        print >>file, "=== Grammar '%s'\n" % self._name

        for (sym, prod) in self._productions.items():
            start = "(*)" if sym == self._start.symbol() else ""
            print >>file, "%3s %5s -> %s" % (start, sym, prod)
        
        if verbose:
                    
            print >>file, ""
            print >>file, "    -- Epsilon:\n"
            
            for (sym, eps) in self._nullable.items():
                print >>file, "      %s = %s" % (sym, "true" if eps else "false" )
            
            print >>file, ""
            print >>file, "    -- First_1:\n"
            
            for (sym, first) in self._first.items():
                print >>file, "      %s = %s" % (sym, [str(f) for f in first])
                
            print >>file, ""
            print >>file, "    -- Follow:\n"
            
            for (sym, follow) in self._follow.items():
                print >>file, "      %s = %s" % (sym, [str(f) for f in follow])
                
            print >>file, ""
            
            print >>file, "    -- Parse table:\n"
    
            for (sym, p) in self._nterms.items():
                start = "(*)" if sym == self._start.symbol() else ""
                print >>file, "  %5s %15s" % (start, sym),
    
                if isinstance(p, LookAhead):
                    print >>file, "   ->",
                    def printAlt(i):
                        lahead = p.lookAheads()[i]
                        alt = p.alternatives()[i]
                        print >>file, "[%d] %s (with {%s})" % (i, alt, ", ".join([str(l) for l in lahead]))
                        
                    printAlt(0)
                    print >>file, " " * 29,
                    printAlt(1)
                    
                else:
                    print >>file, "   -> %s" % p

        print >>file

if __name__ == "__main__":

    from binpac.core import type
    from binpac.support import constant

    verbose = False
    
    # Simple example grammar from
    #
    # http://www.cs.uky.edu/~lewis/essays/compilers/td-parse.html

    a = Literal(None, constant.Constant("a", type.Bytes()))
    b = Literal(None, constant.Constant("b", type.Bytes()))
    c = Literal(None, constant.Constant("c", type.Bytes()))
    d = Literal(None, constant.Constant("d", type.Bytes()))

#    A = Sequence([a])
#    B = LookAhead([A, Sequence([A])])
#    print Grammar(B)
    
    
#    sys.exit(1)
    
    cC = Sequence(None, [], symbol="cC")
    bD = Sequence(None, [], symbol="bD")
    AD = Sequence(None, [], symbol="AD")
    aA = Sequence(None, [], symbol="aA")
    
    A = LookAhead(None, Epsilon(), a, symbol="A")
    B = LookAhead(None, Epsilon(), bD, symbol="B")
    C = LookAhead(None, AD, b, symbol="C")
    D = LookAhead(None, aA, c, symbol="D")

    ABA = Sequence(None, [A,B,A], symbol="ABA")
    S = LookAhead(None, ABA, cC, symbol="S")

    cC.addProduction(c)
    cC.addProduction(C)
    
    bD.addProduction(b)
    bD.addProduction(D)

    AD.addProduction(A)
    AD.addProduction(D)
    
    aA.addProduction(a)
    aA.addProduction(A)

    g1 = Grammar("Grammar1", S)
    g1.printTables(verbose)
    
    error = g1.check()
    if error:
        print error

    # Simple example grammar from "Parsing Techniques", Fig. 8.9
        
    hs = Literal(None, constant.Constant("#", type.Bytes()))
    pl = Literal(None, constant.Constant("(", type.Bytes()))
    pr = Literal(None, constant.Constant(")", type.Bytes()))
    no = Literal(None, constant.Constant("!", type.Bytes()))
    qu = Literal(None, constant.Constant("?", type.Bytes()))
    st = Variable(None, type.Bytes())

    F = Sequence(None, [no, st], symbol="Fact") 
    Q = Sequence(None, [qu, st], symbol="Question")
    
    S = LookAhead(None, None, None, symbol="Session")
    SS = Sequence(None, [pl, S, pr, S], symbol="SS")
    Fs = LookAhead(None, None, None, symbol="Facts")
    FsQ = Sequence(None, [Fs, Q], symbol="FsQ")
    FFs = Sequence(None, [F,Fs], symbol="FFs")
    
    S.setAlternatives(FsQ, SS)
    Fs.setAlternatives(FFs, Epsilon())
    
    root = Sequence(None, [S,hs], symbol="Start")
    g2 = Grammar("Grammar2", root)

    g2.printTables(verbose)
    
    error = g2.check()
    if error:
        print error
    
    # Simple example grammar from "Parsing Techniques", Fig. 8.9
        
    hdrkey = Variable(None, type.Bytes())
    colon = Literal(None, constant.Constant(":", type.Bytes()))
    ws = Literal(None, constant.Constant("[ \\t]*", type.Bytes()))
    hdrval = Variable(None, type.Bytes())
    nl = Literal(None, constant.Constant("[\\r\\n]", type.Bytes()))

    header = Sequence(None, [hdrkey, ws, colon, ws, hdrval, nl], symbol="Header")

    list1 = Sequence(None, [header], symbol="List1")
    list2 = LookAhead(None, list1, Epsilon(), symbol="List2")
    list1.addProduction(list2)
    
    all = LookAhead(None, list2, nl, symbol="All")
    g3 = Grammar("Grammar3", all)
    
    g3.printTables(verbose)
    
    error = g3.check()
    if error:
        print error
