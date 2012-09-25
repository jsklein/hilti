// $Id$

#include "hilti.h"
#include "context.h"

// This function is generated by the code generator(!).
extern hlt_execution_context* __hlt_execution_context_new(hlt_vthread_id vid);

hlt_execution_context* hlt_execution_context_new(hlt_vthread_id vid, yield_func* func)
{
    hlt_execution_context* ctx = __hlt_execution_context_new(vid);
    ctx->yield->succ = func;
    ctx->yield->frame = 0;
    ctx->yield->eoss = 0;

    // Note: don't initialize further context fields here, but in the
    // generated function __hlt_execution_context_new().

    return ctx;
}