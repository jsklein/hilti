// $Id$
//
// Globals variables.
//
// These should be limited to a mininum, and must be ensured that they are
// used in a thread-safe fashion.

#include "hilti.h"

/// Returns the global execution context used by the main tread.
///
/// Returns: The context.
extern hlt_execution_context* hlt_global_execution_context();

/// The execution context used by the main thread. This will be initialized
/// by code generated by the code generator.
extern hlt_execution_context* __hlt_global_execution_context;

/// The thread manager if threading has been started, or 0 otherwise. This
/// will be initialized by ~~hlt_threading_start.
extern hlt_thread_mgr* __hlt_global_thread_mgr;

/// The current hook state. This must only modified by
/// ~~__hlt_hook_group_enable, which protects accesses via the lock.
extern hlt_hook_state* __hlt_global_hook_state;
extern pthread_mutex_t __hlt_global_hook_state_lock;



