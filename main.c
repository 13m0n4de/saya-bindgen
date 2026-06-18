#include "chibicc/chibicc.h"

StringArray include_paths;
bool opt_fcommon = true;
bool opt_fpic = false;

char *base_file = NULL;

static StringArray std_include_paths;
static HashMap type_names;

static const char *saya_keywords[] = {
    "pub",      "use",   "as",     "fn",   "extern", "return", "struct",
    "type",     "let",   "if",     "else", "while",  "loop",   "break",
    "continue", "const", "static", "true", "false",  NULL,
};

static char *sanitize_ident(char *name) {
  for (int i = 0; saya_keywords[i]; i++) {
    if (strcmp(name, saya_keywords[i]) == 0)
      return format("%s_", name);
  }
  return name;
}

typedef struct {
  char *key;
  int val;
} EnumConst;

int cmp_enum_const(const void *a, const void *b) {
  return ((EnumConst *)a)->val - ((EnumConst *)b)->val;
}

// Returns true if a given file exists.
bool file_exists(char *path) {
  struct stat st;
  return !stat(path, &st);
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
int align_to(int n, int align) { return (n + align - 1) / align * align; }

static void add_default_include_paths(char *argv0) {
  // We expect that chibicc-specific include files are installed
  // to ./include relative to argv[0].
  strarray_push(&include_paths, format("%s/include", dirname(strdup(argv0))));

  // Add standard include paths.
  strarray_push(&include_paths, "/usr/local/include");
  strarray_push(&include_paths, "/usr/include/x86_64-linux-gnu");
  strarray_push(&include_paths, "/usr/include");

  // Keep a copy of the standard include paths for -MMD option.
  for (int i = 0; i < include_paths.len; i++)
    strarray_push(&std_include_paths, include_paths.data[i]);
}

static void build_type_names() {
  for (int i = 0; i < scope->vars.capacity; i++) {
    HashEntry *entry = &scope->vars.buckets[i];
    if (!entry->key) {
      continue;
    }
    VarScope *var_scope = entry->val;
    if (!var_scope->type_def) {
      continue;
    }
    hashmap_put2(&type_names, (char *)&var_scope->type_def, sizeof(Type *),
                 entry->key);
  }
}

char *type_to_saya(Type *ty) {
  switch (ty->kind) {
  case TY_VOID:
    return "()";
  case TY_BOOL:
    return "bool";
  case TY_CHAR:
    return ty->is_unsigned ? "u8" : "i8";
  case TY_SHORT:
    return ty->is_unsigned ? "u16" : "i16";
  case TY_INT:
    return ty->is_unsigned ? "u32" : "i32";
  case TY_LONG:
    return ty->is_unsigned ? "u64" : "i64";
  case TY_FLOAT:
    return "f32";
  case TY_DOUBLE:
  case TY_LDOUBLE:
    return "f64";
  case TY_PTR: {
    char *name = hashmap_get2(&type_names, (char *)&ty, sizeof(Type *));
    if (!name && ty->origin) {
      name = hashmap_get2(&type_names, (char *)&ty->origin, sizeof(Type *));
    }
    if (name) {
      return name;
    }
    if (ty->base->kind == TY_VOID) {
      return "*opaque";
    }
    return format("*%s", type_to_saya(ty->base));
  }
  case TY_ARRAY:
    return format("[%s; %d]", type_to_saya(ty->base), ty->array_len);
  case TY_FUNC: {
    char *params = "";
    for (Type *p = ty->params; p; p = p->next) {
      if (p == ty->params) {
        params = type_to_saya(p);
      } else {
        params = format("%s, %s", params, type_to_saya(p));
      }
    }
    return format("fn(%s) -> %s", params, type_to_saya(ty->return_ty));
  }
  case TY_ENUM:
  case TY_STRUCT:
  case TY_UNION: {
    char *name = hashmap_get2(&type_names, (char *)&ty, sizeof(Type *));
    if (!name && ty->origin) {
      name = hashmap_get2(&type_names, (char *)&ty->origin, sizeof(Type *));
    }
    if (name) {
      return name;
    }
    const char *kind = ty->kind == TY_ENUM     ? "enum"
                       : ty->kind == TY_STRUCT ? "struct"
                                               : "union";
    fprintf(stderr,
            "%s:%d: warning: %s type has no name, using '(unknown)' as type\n",
            ty->name->filename, ty->name->line_no, kind);
    return "(unknown)";
  }
  case TY_VLA:
    fprintf(stderr,
            "%s:%d: warning: VLA type is not supported, using '(unknown)' as "
            "type\n",
            ty->name->filename, ty->name->line_no);
    return "(unknown)";
  }

  unreachable();
}

static void emit_typedef() {
  for (int i = 0; i < scope->vars.capacity; i++) {
    HashEntry *entry = &scope->vars.buckets[i];
    if (!entry->key) {
      continue;
    }
    VarScope *var_scope = entry->val;
    if (!var_scope->type_def) {
      continue;
    }

    char *name = entry->key;
    Type *ty = var_scope->type_def;

    switch (ty->kind) {
    case TY_STRUCT:
      printf("pub struct %s {\n", sanitize_ident(name));
      for (Member *member = ty->members; member; member = member->next) {
        if (!member->name) {
          fprintf(stderr,
                  "%s:%d: warning: anonymous member in struct %s, skiping\n",
                  ty->name->filename, ty->name->line_no, name);
          continue;
        }
        char *member_name = sanitize_ident(
            format("%.*s", member->name->len, member->name->loc));
        printf("    %s: %s,\n", member_name, type_to_saya(member->ty));
      }
      printf("}\n");
      break;
    case TY_ENUM: {
      char *base;
      switch (ty->size) {
      case 1:
        base = ty->is_unsigned ? "u8" : "i8";
        break;
      case 2:
        base = ty->is_unsigned ? "u16" : "i16";
        break;
      case 4:
        base = ty->is_unsigned ? "u32" : "i32";
        break;
      case 8:
        base = ty->is_unsigned ? "u64" : "i64";
        break;
      default:
        base = "i32";
        break;
      }
      printf("pub type %s = %s;\n", sanitize_ident(name), base);

      EnumConst consts[scope->vars.capacity];
      int nconsts = 0;
      for (int j = 0; j < scope->vars.capacity; j++) {
        HashEntry *const_entry = &scope->vars.buckets[j];
        if (!const_entry->key) {
          continue;
        }
        VarScope *const_vs = const_entry->val;
        if (!const_vs->enum_ty || const_vs->enum_ty != ty) {
          continue;
        }
        consts[nconsts] = (EnumConst){const_entry->key, const_vs->enum_val};
        nconsts += 1;
      }

      qsort(consts, nconsts, sizeof(EnumConst), cmp_enum_const);

      for (int j = 0; j < nconsts; j++) {
        printf("pub const %s_%s: %s = %d;\n", sanitize_ident(name),
               sanitize_ident(consts[j].key), sanitize_ident(name),
               consts[j].val);
      }
      break;
    }
    case TY_UNION:
      fprintf(stderr, "%s:%d: warning: union %s is not supported, skipping\n",
              ty->name->filename, ty->name->line_no, name);
      break;
    default:
      // For pointer typedefs, expand via ty->base to avoid looking up ty itself
      // in type_names, which would produce "pub type Foo = Foo".
      if (ty->kind == TY_PTR) {
        printf("pub type %s = *%s;\n", sanitize_ident(name),
               type_to_saya(ty->base));
      } else {
        printf("pub type %s = %s;\n", sanitize_ident(name), type_to_saya(ty));
      }
      break;
    }
    printf("\n");
  }
}

static void emit_function(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (fn->ty->kind != TY_FUNC || fn->is_definition) {
      continue;
    }

    printf("@symbol(\"%s\") pub extern fn %s(", fn->name,
           sanitize_ident(fn->name));
    Type *param = fn->ty->params;
    while (param) {
      if (!param->name) {
        fprintf(
            stderr,
            "%s:%d: warning: anonymous parameter in %s, using '_' as name\n",
            fn->ty->name->filename, fn->ty->name->line_no, fn->name);
        printf("_: %s", type_to_saya(param));
      } else {
        char *pname =
            sanitize_ident(format("%.*s", param->name->len, param->name->loc));
        printf("%s: %s", pname, type_to_saya(param));
      }
      if (param->next) {
        printf(", ");
      }
      param = param->next;
    }
    printf(")");

    Type *return_ty = fn->ty->return_ty;
    if (return_ty->kind != TY_VOID) {
      printf(" -> %s", type_to_saya(return_ty));
    }

    printf(";\n");
  }
}

int main(int argc, char **argv) {
  (void)argc;

  init_macros();
  add_default_include_paths(argv[0]);

  Token *tok = tokenize_file("-"); // stdin
  tok = preprocess(tok);

  Obj *prog = parse(tok);

  build_type_names();

  emit_typedef();
  emit_function(prog);

  return 0;
}
