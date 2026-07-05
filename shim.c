#include "chibicc/chibicc.h"

HashEntry *hashmap_bucket_at(HashMap *map, int i) { return &map->buckets[i]; }
VarScope *hashentry_var_scope(HashEntry *entry) { return entry->val; }
_Noreturn void chibicc_unreachable(void) { unreachable(); }
