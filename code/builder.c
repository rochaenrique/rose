#include "base/base.h"

#include "base/base.c"

typedef enum Column_Kind
{
  ColumnKind_OS,
  ColumnKind_Compiler,
  ColumnKind_EntryPath,
  ColumnKind_OutputPath,
  ColumnKind_DebugFlag,
  
  ColumnKind_COUNT,
} Column_Kind;
U32 column_max_lengths[ColumnKind_COUNT];

typedef struct Table Table;
struct Table
{
  char *format_str_nt;
  Column_Kind *columns;
  U32 column_count;
};

global Table cmd_table;

Table
table_make(Arena *arena, char *format, Column_Kind *columns, U32 column_count)
{
  Assert(arena);
  Assert(format);
  Assert((!columns && column_count == 0) || (columns && column_count > 0));
  
  Table table = {0};
  table.format_str_nt = format;
  table.column_count = column_count;
  table.columns = PushArray(arena, column_count, Column_Kind);
  ArrayCopy(columns, table.columns, column_count);
  return(table);
}

String8
table_row_varg(Arena *arena, Table *table, va_list source_args)
{
  Assert(arena);
  Assert(table);
  
  String8_Builder sb = {0};
  va_list args;
  va_copy(args, source_args);
  Scratch scratch = scratch_begin(&arena, 1);
  
  U32 column_idx = 0;
  
  for(char *it = table->format_str_nt;*it;++it)
  {
    if(*it == '%')
    {
      Arena *conflicts[] = {arena, scratch.arena};
      
      if((*(it+1)) == 'T')
      {
        Assert(column_idx < table->column_count);
        Column_Kind column = table->columns[column_idx++];
        String8 label = va_arg(args, String8);
        I32 space_left = (I32)(column_max_lengths[column] - label.size);
        
        Scratch arg_scratch = scratch_begin(conflicts, ArraySize(conflicts));
        {
          String8 resolved = push_str8f(arg_scratch.arena, "%.*s%*.s", S8Expand(label), space_left, "");
          str8_builder_str(scratch.arena, &sb, resolved);
        }
        scratch_end(&arg_scratch);
        
        it++;
      }
      else if(*(it+1) == 'S')
      {
        String8 label = va_arg(args, String8);
        Scratch arg_scratch = scratch_begin(conflicts, ArraySize(conflicts));
        {
          String8 resolved = push_str8f(arg_scratch.arena, "%.*s", S8Expand(label));
          str8_builder_str(scratch.arena, &sb, resolved);
        }
        scratch_end(&arg_scratch);
        
        it++;
      }
      else
      {
        char *letter = it;
        for(;*letter;letter++)
        {
          if(char_is_alphabetic(*letter))
          {
            break;
          }
        }
        Assert((letter - it) >= 1);
        String8 slice = S8Struct(it, (letter - it)+1);
        
        Scratch arg_scratch = scratch_begin(conflicts, ArraySize(conflicts));
        {
          char *fmt = push_cstr(arg_scratch.arena, slice);
          String8 resolved = push_str8f_varg(arg_scratch.arena, (const char *)fmt, args);
          str8_builder_str(scratch.arena, &sb, resolved);
        }
        scratch_end(&arg_scratch);
        
        it = letter;
      }
    }
    else
    {
      str8_builder_char(scratch.arena, &sb, *it);
    }
  }
  
  String8 result = push_str8_copy(arena, sb.buffer);
  va_end(args);
  scratch_end(&scratch);
  return(result);
}

String8
table_row_str8(Arena *arena, Table *table, ...)
{
  va_list args;
  va_start(args, table);
  String8 str = table_row_varg(arena, table, args);
  va_end(args);
  return(str);
}

void
table_row_log(Table *table, ...)
{
  Scratch scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, table);
  String8 str = table_row_varg(scratch.arena, table, args);
  va_end(args);
  os_log(str);
  scratch_end(&scratch);
}

typedef enum Entry_Point_Kind
{
  EntryPointKind_CMain,
  EntryPointKind_WinMain,
  EntryPointKind_COUNT,
} Entry_Point_Kind;

typedef enum Language_Kind
{
  LanguageKind_CPP,
  LanguageKind_C,
  LanguageKind_ObjectiveC,
  LanguageKind_COUNT,
} Language_Kind;

typedef struct Entry_Point  Entry_Point;
struct Entry_Point
{
  Entry_Point *next;
  String8 path;
  Entry_Point_Kind kind;
  Language_Kind language;
};
typedef struct Entry_Point_List Entry_Point_List;
struct Entry_Point_List
{
  Entry_Point *first;
  Entry_Point *last;
};
raddbg_type_view(Entry_Point_List, list($.first, $.next));

typedef enum Compiler_Kind
{
  CompilerKind_MSVC,
  CompilerKind_Clang,
  CompilerKind_GCC,
  CompilerKind_COUNT,
} Compiler_Kind;

typedef B8 Compiler_Array_Flag[CompilerKind_COUNT];
raddbg_type_view(Compiler_Array_Flag, array($, CompilerKind_COUNT));
typedef B8 OS_Array_Flag[OSKind_COUNT];
raddbg_type_view(OS_Array_Flag, array($, OSKind_COUNT));

typedef enum Compiler_Flag
{
  CompilerFlag_FullWarn,
  CompilerFlag_Debug,
  CompilerFlag_NoWarnNamelessMemberStructs,
  CompilerFlag_NoWarnInitalizedNotUsed,
  CompilerFlag_NoWarnParameterNotReferenced,
  CompilerFlag_COUNT,
} Compiler_Flag;

typedef B8 Compiler_Flag_Array_Flag[CompilerFlag_COUNT];
raddbg_type_view(Compiler_Flag_Array_Flag, array($, CompilerFlag_COUNT));

//////////////////////////////////////////////
// NOTE(erb): tables
// NOTE(erb): cross compile table would be specific to everyones computer 
// because of VMs and all. You would need to read some configuration file or
// something that gets the cross compilation shells, for example: win32,linux: wsl
String8 entry_point_kind_labels[] = 
{
  [EntryPointKind_CMain  ] = S8Lit("main"),
  [EntryPointKind_WinMain] = S8Lit("winmain"),
  [EntryPointKind_WinMain] = S8Lit("os_entry_point"),
};

typedef struct Language_Info Language_Info;
struct Language_Info
{
  String8_Array extensions;
  Compiler_Array_Flag compilers;
};

#define L(_language_prefix, _msvc, _clang, _gcc, ...) \
[LanguageKind_##_language_prefix] = { \
.compilers = { [CompilerKind_MSVC] = _msvc, [CompilerKind_Clang] = _clang, [CompilerKind_GCC] = _gcc, }, \
.extensions = S8ArrayLit(__VA_ARGS__), }
Language_Info languages_info[] =
{
  //            msvc   clang  gcc
  L(CPP,         true, true,  true, S8Lit("cpp"), S8Lit("cxx"), S8Lit("cc"), S8Lit("c++"), S8Lit("cp")),
  L(C,           true, true,  true, S8Lit("c")),
  L(ObjectiveC, false, true, false, S8Lit("mm"), S8Lit("m")),
};
#undef L

typedef struct Compiler_Info Compiler_Info;
struct Compiler_Info
{
  String8 lib_delim;
  String8_Array labels;
  String8 flag_strings[CompilerFlag_COUNT];
};

#define C(compiler_prefix, _lib_delim, _labels, ...) \
[CompilerKind_##compiler_prefix] = { \
.lib_delim = _lib_delim, \
.labels = _labels, \
.flag_strings = { __VA_ARGS__ } }

#define F(prefix, str) [CompilerFlag_##prefix] = str

Compiler_Info compilers_infos[] = 
{
  C(MSVC,  
    S8Lit(" "),
    S8ArrayLit(S8Lit("msvc"), S8Lit("cl")),
    F(FullWarn,                     S8Lit("/WX /W4")),
    F(Debug,                        S8Lit("/Z7 /FS")),
    F(NoWarnNamelessMemberStructs,  S8Lit("/wd4201")),
    F(NoWarnInitalizedNotUsed,      S8Lit("/wd4189")),
    F(NoWarnParameterNotReferenced, S8Lit("/wd4100"))
    ),
  
  C(GCC,   
    S8Lit(" -l"), 
    S8ArrayLit(S8Lit("gcc"), S8Lit("gcc++")),
    F(FullWarn,                     S8Lit("-Wall -Wextra")),
    F(Debug,                        S8Lit("-g -ggdb")),
    F(NoWarnNamelessMemberStructs,  S8Lit("-Wno-gnu-anonymous-struct")),
    F(NoWarnInitalizedNotUsed,      S8Lit("")),
    F(NoWarnParameterNotReferenced, S8Lit("-Wno-unused-variable"))
    ),
  
  C(Clang, 
    S8Lit(" -l"), 
    S8ArrayLit(S8Lit("clang"), S8Lit("g++")),
    F(FullWarn,                     S8Lit("-Wall -Wextra")),
    F(Debug,                        S8Lit("-g -ggdb -Wno-varargs")),
    F(NoWarnNamelessMemberStructs,  S8Lit("-Wno-gnu-anonymous-struct")),
    F(NoWarnInitalizedNotUsed,      S8Lit("")),
    F(NoWarnParameterNotReferenced, S8Lit("-Wno-unused-variable"))
    ),
};
#undef F
#undef C

typedef struct OS_Info OS_Info;
struct OS_Info
{
  char path_delim;
  String8_Array labels;
  String8_Array libraries;
  Compiler_Kind prefered_compiler;
};

#define OS(prefix, _path_delim, _labels, _libraries, compiler) \
[OSKind_##prefix] = { .path_delim = _path_delim, .labels = _labels, .libraries = _libraries, .prefered_compiler = compiler }
OS_Info oss_infos[] = 
{
  OS(Linux, 
     '/', 
     S8ArrayLit(S8Lit("linux"), S8Lit("lnx")), 
     S8ArrayLit(//- wayland stuff
                S8Lit("GLESv2"), S8Lit("EGL"), S8Lit("m"), 
                S8Lit("wayland-client"), S8Lit("wayland-server"), 
                S8Lit("wayland-cursor"), S8Lit("wayland-egl"),
                //- x11 stuff
                S8Lit("X11"), S8Lit("GL")),
     CompilerKind_Clang),
  OS(Win32,
     '\\',
     S8ArrayLit(S8Lit("win32"), S8Lit("w32"), S8Lit("windows"), S8Lit("win")),
     S8ArrayLit(S8Lit("..\\external\\freetype\\bin\\freetype.lib")),
     CompilerKind_MSVC),
  
  OS(OSX,
     '/',
     S8ArrayLit(S8Lit("osx"), S8Lit("macos"), S8Lit("mac"), S8Lit("apple"), S8Lit("darwin")), 
     S8ArrayLit(S8Lit("this is not a real library name just a weird stub")),
     CompilerKind_Clang),
};
#undef OS
String8 os_cross_compile_command_table[][OSKind_COUNT] = 
{
  [OSKind_Win32][OSKind_Linux] = S8Lit("wsl"),
};
typedef struct Compiler_Flag_Info Compiler_Flag_Info;
struct Compiler_Flag_Info
{
  B32 value_if_specified;
  String8 env_str;
  String8 label;
};
#define F(prefix, _label, _env_str, _value) \
[CompilerFlag_##prefix] = { .label = S8Lit(_label), .env_str = S8Lit(_env_str), .value_if_specified = _value }
Compiler_Flag_Info compiler_flags_info[] = 
{
  F(Debug,                        "Debug",                        "DEBUG",                         true),
  F(FullWarn,                     "FullWarn",                     "NO_FULL_WARN",                  false),
  F(NoWarnNamelessMemberStructs,  "NoWarnNamelessMemberStructs",  "WARN_NAMELESS_MEMBER_STRUCTS",  false),
  F(NoWarnInitalizedNotUsed,      "NoWarnInitalizedNotUsed",      "WARN_INITIALIZED_NOT_USED",     false),
  F(NoWarnParameterNotReferenced, "NoWarnParameterNotReferenced", "WARN_PARAMETER_NOT_REFERENCED", false),
};
#undef F


typedef struct Build_Command Build_Command;
struct Build_Command
{
  OS_Kind os;
  Compiler_Kind compiler;
  String8 os_path;
  String8 built_str;
  String8 unix_output_file_name;
  Build_Command *next;
};

typedef struct Build_Command_List Build_Command_List;
struct Build_Command_List
{
  Build_Command *first;
  Build_Command *last;
  U32 count;
};

B32
is_os_compiler_valid(OS_Kind os, Compiler_Kind compiler)
{
  B32 result = true;
  if (os != OSKind_Win32 && compiler == CompilerKind_MSVC)
  {
    result = false;
  }
  return(result);
}

String8
path_extension(String8 path)
{
  String8 result = str8_index_postfix(path, '.', IndexFlags_Backward|IndexFlags_ValuePlusOne);
  return(result);
}

B32
file_contains_entry_point_function(String8 path, Entry_Point_Kind *entry_kind_result)
{
  B32 result = false;
  for (U32Each(entry_kind, EntryPointKind_COUNT))
  {
    String8 func_name = entry_point_kind_labels[entry_kind];
    if (os_file_contains_str_case_insens(path, func_name))
    {
      result = true;
      *entry_kind_result = (Entry_Point_Kind)entry_kind;
      break;
    }
  }
  return(result);
}

B32
is_language_extension(String8 ext, Language_Kind *language_result)
{
  B32 result = false;
  for (U32Each(language_kind, LanguageKind_COUNT))
  {
    String8_Array extensions = languages_info[language_kind].extensions;
    if (str8_array_contains(extensions, ext))
    {
      *language_result = (Language_Kind)language_kind;
      result = true;
    }
  }
  return(result);
}

Entry_Point_List 
get_entry_points(Arena *arena, String8_List files)
{ 
  Entry_Point_List result = {0};
  
  for (EachNode(String8_Node, file, files.first))
  {
    String8 path = file->str;
    String8 extension = path_extension(path);
    
    Language_Kind language = 0;
    if (is_language_extension(extension, &language))
    {
      Entry_Point_Kind entry_kind = 0;
      if (file_contains_entry_point_function(path, &entry_kind))
      {
        Entry_Point *entry = PushStruct(arena, Entry_Point);
        entry->path = path;
        entry->kind = entry_kind;
        entry->language = language;
        SLLQueueAppend(entry, result.first, result.last);
      }
    }
  }
  return(result);
}

I32
run_command(String8 cmd)
{
  Scratch scratch = scratch_begin(0, 0);
  char *cstr = push_cstr(scratch.arena, cmd);
  I32 result = system(cstr);
  scratch_end(&scratch);
  return(result);
}

String8
str8_from_compiler_flags(Arena *arena, Compiler_Kind compiler, Compiler_Flag_Array_Flag flags)
{
  String8 *flags_strings = compilers_infos[compiler].flag_strings;
  
  Scratch scratch = scratch_begin(&arena, 1);
  String8_Builder sb = {0};
  for (U32Each(flag_it, CompilerFlag_COUNT))
  {
    if (flags[flag_it])
    {
      str8_builder_str(arena, &sb, flags_strings[flag_it]);
      if (flag_it < CompilerFlag_COUNT-1)
      {
        str8_builder_char(scratch.arena, &sb, ' ');
      }
    }
  }
  String8 result = push_str8_copy(arena, sb.buffer);
  scratch_end(&scratch);
  return(result);
}

typedef struct Build_Options Build_Options;
struct Build_Options
{
  String8 target_path;
  Compiler_Array_Flag compilers; 
  OS_Array_Flag oss; 
  B32 only_print_cmd;
  B32 infer_best_compiler_for_os;
};

typedef struct Compiler_Kind_Maybe  Compiler_Kind_Maybe ;
struct Compiler_Kind_Maybe
{
  Compiler_Kind value;
  B32 exists;
};

Compiler_Kind_Maybe
str8_to_compiler_kind(String8 str)
{
  Compiler_Kind_Maybe result = {0};
  for (U32Each(compiler, CompilerKind_COUNT))
  {
    String8_Array compiler_labels = compilers_infos[compiler].labels;
    if (str8_array_contains(compiler_labels, str))
    {
      result.value = compiler;
      result.exists = true;
      break;
    }
  }
  return(result);
}

typedef struct OS_Kind_Maybe  OS_Kind_Maybe ;
struct OS_Kind_Maybe
{
  OS_Kind value;
  B32 exists;
};

OS_Kind_Maybe
str8_to_os_kind(String8 str)
{
  OS_Kind_Maybe result = {0};
  for (U32Each(os, OSKind_COUNT))
  {
    String8_Array os_labels = oss_infos[os].labels;
    if (str8_array_contains(os_labels, str))
    {
      result.value = os;
      result.exists = true;
      break;
    }
  }
  return(result);
}

Build_Options
parse_options(String8_Array args)
{
  Build_Options options = {0};
  
  B32 has_compiler = false;
  B32 has_os = false;
  
  for (U32Each(arg_idx, args.count))
  {
    String8 arg = args.items[arg_idx];
    
    Compiler_Kind_Maybe compiler_result = str8_to_compiler_kind(arg);
    if (compiler_result.exists)
    {
      has_compiler = options.compilers[compiler_result.value] = true;
    }
    
    OS_Kind_Maybe os_result = str8_to_os_kind(arg);
    if (os_result.exists)
    {
      has_os = options.oss[os_result.value] = true;
    }
    
    if (str8_equals(arg, S8("fake")))
    {
      options.only_print_cmd = true;
    }
    
    if (str8_equals(arg, S8("infer")))
    {
      options.infer_best_compiler_for_os = true;
    }
    
    if (options.target_path.value == 0)
    {
      options.target_path = arg;
    } 
    else
    {
      // TODO(erb): error here: too many targets
    }
  }
  
  if(options.infer_best_compiler_for_os && has_compiler)
  {
    os_log(S8("'infer' can only be used without compilers - ignoring compilers"));
  }
  else if (!has_compiler)
  {
    ArraySetValues(options.compilers, CompilerKind_COUNT, true);
  }
  
  if (!has_os)
  {
    ArraySetValues(options.oss, OSKind_COUNT, true);
  }
  return(options);
}

typedef struct Command_Strings Command_Strings;
struct Command_Strings
{
  String8 prefix;
  String8 postfix;
};

typedef struct Command_Cache Command_Cache;
struct Command_Cache
{
  Arena arena;
  Command_Strings compiler_strings[CompilerKind_COUNT];
  String8 os_compiler_library_strings[OSKind_COUNT*CompilerKind_COUNT];
};

Command_Strings
generate_compiler_build_command(Arena *arena, 
                                Compiler_Kind compiler,
                                String8_List includes,
                                Compiler_Flag_Array_Flag compiler_flags)
{
  Command_Strings cmd = {0};
  
  Scratch scratch = scratch_begin(&arena, 1);
  
  String8 project_flags = str8_from_compiler_flags(scratch.arena, compiler, compiler_flags);
  switch (compiler)
  {
    case CompilerKind_GCC:
    case CompilerKind_Clang: 
    {
      
      // NOTE(erb): prefix
      {
        String8 includes_str = str8_list_flatten_prefix_nodes(scratch.arena, &includes, S8(" -I"));
        String8_List list = {0};
        str8_list_push(scratch.arena, &list, (compiler == CompilerKind_Clang)?S8("clang"):S8("gcc"));
        str8_list_push(scratch.arena, &list, S8("-fsanitize=address,undefined"));
        str8_list_push(scratch.arena, &list, project_flags);
        str8_list_push(scratch.arena, &list, includes_str);
        cmd.prefix = str8_list_flatten_delim(arena, list, S8(" "));
      }
    } break;
    
    case CompilerKind_MSVC:
    {
      // NOTE(erb): prefix
      {
        String8 includes_str = str8_list_flatten_prefix_nodes(scratch.arena, &includes, S8(" /I"));
        String8_List list = {0};
        str8_list_push(scratch.arena, &list, S8("cl /FC /nologo"));
        str8_list_push(scratch.arena, &list, project_flags);
        str8_list_push(scratch.arena, &list, includes_str);
        cmd.prefix = str8_list_flatten_delim(arena, list, S8(" "));
      }
      
      // NOTE(erb): postfix
      cmd.postfix = S8("/link /incremental:no");
    } break;
  }
  scratch_end(&scratch);
  
  return(cmd);
}

String8 
unix_concat_path(Arena *arena, String8 s1, String8 s2)
{
  return(str8_concat_3(arena, s1, S8("/"), s2));
}

Entry_Point_List
entry_points_from_path(Arena *arena, String8 target)
{
  File_Kind target_kind = os_get_file_kind(target);
  // TODO(erb): check error
  
  Entry_Point_List entry_points = {0};
  
  //- get all the files
  Scratch files_scratch = scratch_begin(&arena, 1);
  String8_List files = {0};
  switch(target_kind)
  {
    case FileKind_Text: 
    {
      str8_list_push(files_scratch.arena, &files, target);
    } break;
    
    case FileKind_Directory:
    {
      files = os_get_directory_files(files_scratch.arena, target);
      // NOTE(erb): here we add directory to targets since os_get_directory_files 
      // only returns the file part of the directory files.
      String8_List full_paths = {0};
      for (EachNode(String8_Node, file_path, files.first))
      {
        String8 full = unix_concat_path(files_scratch.arena, target, file_path->str);
        str8_list_push(files_scratch.arena, &full_paths, full);
      }
      files = full_paths;
    } break;
  }
  
  //- get files that are entry points
  entry_points = get_entry_points(arena, files);
  scratch_end(&files_scratch);
  return(entry_points);
}

OS_Kind_Maybe
os_contained_in_str(String8 path)
{
  OS_Kind_Maybe result = {0};
  for (U32Each(os_idx, OSKind_COUNT))
  {
    String8_Array *labels = &oss_infos[os_idx].labels;
    if (str8_contains_str_array_node(path, labels))
    {
      result.exists = true;
      result.value = (OS_Kind)os_idx;
      break;
    }
  }
  return(result);
}

String8
os_path_from_unix_path(Arena *arena, OS_Kind origin, String8 path)
{
  String8 result = push_str8_copy(arena, path);
  switch(origin)
  {
    case OSKind_Win32: result = str8_replace_char(result, '/', '\\'); break;
    case OSKind_OSX:
    case OSKind_Linux: break;
  }
  return(result);
}

Command_Strings *
get_commpiler_strings(Command_Cache *command_cache, Compiler_Kind compiler)
{
  Command_Strings *result = 0;
  Command_Strings *test = (command_cache->compiler_strings + compiler);
  if (test->prefix.size > 0 || test->postfix.size > 0)
  {
    result = test;
  }
  return(result);
}

String8
unix_path_dir_part(String8 path)
{
  String8 result = str8_index_prefix(path, '/', IndexFlags_ValuePlusOne|IndexFlags_Backward);
  return(result);
}

String8
unix_path_file_part(String8 path)
{
  String8 result = str8_index_postfix(path, '/', IndexFlags_ValuePlusOne|IndexFlags_Backward);
  return(result);
}

String8
unix_path_file_name(String8 path)
{
  String8 result = unix_path_file_part(path);
  result = str8_index_prefix(result, '.', IndexFlags_FallbackValueLength); // NOTE(erb): strip extension
  return(result);
}

String8
get_compiler_output_flag(Arena *arena, Compiler_Kind compiler, String8 unix_path)
{
  String8 result = {0};
  switch(compiler)
  {
    case CompilerKind_MSVC: 
    {
      result = push_str8f(arena, "/Fe:\"%.*s.exe\"", S8Expand(unix_path_file_name(unix_path)));
    } break;
    case CompilerKind_GCC:
    case CompilerKind_Clang:
    {
      result = push_str8f(arena, "-o %.*s", S8Expand(unix_path_file_name(unix_path)));
    } break;
  }
  return(result);
}

String8
get_os_libs_string(Command_Cache *command_cache, OS_Kind os, Compiler_Kind compiler)
{
  String8 result = {0};
  if (is_os_compiler_valid(os, compiler))
  {
    U32 idx = (os*CompilerKind_COUNT + compiler);
    String8 *test = (command_cache->os_compiler_library_strings + idx);
    if (test->size == 0)
    {
      String8_Array os_libs = oss_infos[os].libraries;
      String8 compiler_lib_delim = compilers_infos[compiler].lib_delim;
      *test = str8_array_flatten_prefix_nodes(&command_cache->arena, &os_libs, compiler_lib_delim);
    }
    result = *test;
  }
  return(result);
}

B32
can_compile_to_os(OS_Kind os)
{
  String8 cross_compile_cmd = os_cross_compile_command_table[os_kind_this][os];
  B32 result = (os_kind_this == (OS_Kind)os) || (cross_compile_cmd.value != 0);
  return(result);
}

String8 
build_command_str(Arena *arena, Command_Cache *command_cache, OS_Kind os, Compiler_Kind compiler, 
                  String8 path, String8 unix_output_path)
{
  String8 result = {0};
  Command_Strings *command = get_commpiler_strings(command_cache, compiler);
  String8 os_libs_str = get_os_libs_string(command_cache, os, compiler);
  String8 output_flag = get_compiler_output_flag(arena, compiler, unix_output_path);
  
  String8 cross_compile_cmd = os_cross_compile_command_table[os_kind_this][os];
  if (cross_compile_cmd.size == 0)
  {
    result = push_str8f(arena, "%.*s %.*s %.*s %.*s %.*s",
                        S8Expand(command->prefix),
                        S8Expand(output_flag),
                        S8Expand(path),
                        S8Expand(os_libs_str),
                        S8Expand(command->postfix));
  }
  else
  {
    result = push_str8f(arena, "%.*s %.*s %.*s %.*s %.*s %.*s",
                        S8Expand(cross_compile_cmd),
                        S8Expand(command->prefix),
                        S8Expand(output_flag),
                        S8Expand(path),
                        S8Expand(os_libs_str),
                        S8Expand(command->postfix));
  }
  return(result);
}

String8
os_get_main_label(OS_Kind os)
{
  String8 result = oss_infos[os].labels.items[0];
  return(result);
}

String8
compiler_get_main_label(Compiler_Kind comp)
{
  String8 result = compilers_infos[comp].labels.items[0];
  return(result);
}

void
build_entry_point(Arena *cmd_list_arena, Build_Command_List *cmd_list,
                  Build_Options *options, Entry_Point *entry, Command_Cache *command_cache)
{
  
  // NOTE(erb): os from the path of the file
  OS_Array_Flag oss = {0};
  OS_Kind_Maybe os_in_path = os_contained_in_str(entry->path);
  for (U32Each(os, OSKind_COUNT))
  {
    if (options->oss[os])
    {
      if (os_in_path.exists)
      {
        oss[os] = ((OS_Kind)os == os_in_path.value);
      }
      else
      {
        oss[os] = true;
      }
    }
  }
  
  // NOTE(erb): compilers from language and options
  Compiler_Array_Flag compilers = {0};
  
  for (U32Each(comp, CompilerKind_COUNT))
  {
    B32 language = languages_info[entry->language].compilers[comp];
    compilers[comp] = language && options->compilers[comp];
  }
  
  Scratch scratch = scratch_begin(0, 0);
  String8 unix_relative = unix_concat_path(scratch.arena, S8(".."), entry->path);
  
  // NOTE(erb): build the command for every configuration
  for (U32Each(os, OSKind_COUNT))
  {
    if (oss[os] && can_compile_to_os(os))
    {
      Scratch cmd_scratch = scratch_begin(&scratch.arena, 1);
      String8 os_relative_path = os_path_from_unix_path(scratch.arena, os, unix_relative);
      
      String8 unix_output_path = {0};
      if(os_in_path.exists && os_in_path.value == (OS_Kind)os)
      {
        unix_output_path = unix_relative;
      }
      else
      {
        String8 label = os_get_main_label(os);
        String8 dir_part = unix_path_dir_part(unix_relative);
        String8 file_part = unix_path_file_part(unix_relative);
        unix_output_path = push_str8f(cmd_scratch.arena, "%.*s%.*s_%.*s", 
                                      S8Expand(dir_part), S8Expand(label), S8Expand(file_part));
      }
      
      U32 compiler_count = 0;
      for (U32Each(comp, CompilerKind_COUNT))
      {
        if (compilers[comp] && is_os_compiler_valid(os, comp))
        {
          compiler_count++;
        }
      }
      
      for (U32Each(comp, CompilerKind_COUNT))
      {
        B32 is_prefered = false;
        if(options->infer_best_compiler_for_os)
        {
          is_prefered = (Compiler_Kind)comp == oss_infos[os].prefered_compiler;
        }
        else
        {
          is_prefered = compilers[comp];
        }
        
        if (is_prefered && is_os_compiler_valid(os, comp))
        {
          // TODO(erb): add an option to always specify the compiler in the output
          if (compiler_count > 1 && !options->infer_best_compiler_for_os)
          {
            String8 label = compiler_get_main_label(comp);
            String8 dir_part = unix_path_dir_part(unix_output_path);
            String8 file_part = unix_path_file_part(unix_output_path);
            unix_output_path = push_str8f(cmd_scratch.arena, "%.*s%.*s_%.*s", 
                                          S8Expand(dir_part), S8Expand(label), S8Expand(file_part));
          }
          
          
          Build_Command *cmd = PushStruct(cmd_list_arena, Build_Command);
          cmd->os = os;
          cmd->compiler = comp;
          cmd->os_path = push_str8_copy(cmd_list_arena, os_relative_path);
          cmd->unix_output_file_name = push_str8_copy(cmd_list_arena, unix_path_file_name(unix_output_path));
          cmd->built_str = build_command_str(cmd_list_arena, command_cache, os, 
                                             comp, os_relative_path, unix_output_path);
          
          SLLQueueAppend(cmd, cmd_list->first, cmd_list->last);
          cmd_list->count++;
          
          // TODO(erb): add some run command to file and the post-command script
        }
      }
      scratch_end(&cmd_scratch);
    }
  }
  scratch_end(&scratch);
}

#define rebuild_yourself(includes, compiler_flags) rebuild_file(S8(__FILE__), (includes), (compiler_flags));

B32
rebuild_file(String8 file_path, String8_List includes, Compiler_Flag_Array_Flag compiler_flags)
{
  SupressUnused(includes);
  SupressUnused(compiler_flags);
  // NOTE(erb): this gets the file kind through the extension but it could be 
  // infered through #defines
  Scratch scratch = scratch_begin(0, 0);
  Full_Path module_path = os_get_full_path(scratch.arena, file_path);
  
  B32 result = false;
  Language_Kind language = 0;
  if (is_language_extension(path_extension(module_path.file_part), &language))
  {
    //String8_Array os_libs = os_kind_libraries[os_kind_this];
    Compiler_Array_Flag compilers = {0};
    ArrayCopy(languages_info[language].compilers, compilers, CompilerKind_COUNT);
    
    for (U32Each(comp, CompilerKind_COUNT))
    {
      if (compilers[comp] && is_os_compiler_valid(os_kind_this, comp))
      {
        // TODO(erb): do something smarter here
        String8 cmd = {0};
        result = (run_command(cmd) == 0);
        break;
      }
    }
  }
  
  scratch_end(&scratch);
  return(result);
}

void
get_enviornment_compiler_flags(Compiler_Flag_Array_Flag flags)
{
  Scratch scratch = scratch_begin(0, 0);
  
  String8 cc_flags_var = os_get_enviornment_variable(scratch.arena, "BLD_CC_FLAGS");
  
  if (cc_flags_var.value != 0)
  {
    String8_List flags_list = str8_split_blank(scratch.arena, str8_remove_char(cc_flags_var, '"'));
    for (EachNode(String8_Node, node, flags_list.first))
    {
      for (U32Each(flag_it, CompilerFlag_COUNT))
      {
        Compiler_Flag_Info info = compiler_flags_info[flag_it];
        if (str8_equals(node->str, info.env_str))
        {
          flags[flag_it] = (info.value_if_specified);
          break;
        }
      }
    }
  }
  scratch_end(&scratch);
}

U32
run_command_thread_proc(void *parameter)
{
  Thread_Context tctx = {0};
  thread_context_init(&tctx);
  os_thread_context_set(&tctx);
  
  OS_Clock_Time time_begin = os_get_clock_time();
  
  Build_Command *cmd = (Build_Command *)parameter;
  if (run_command(cmd->built_str)) 
  {
    os_log(S8("-- failed"));
  }
  table_row_log(&cmd_table, cmd->unix_output_file_name, os_seconds_elapsed(time_begin, os_get_clock_time()));
  return(0);
}

void
column_max_str8(Column_Kind kind, String8 label)
{
  column_max_lengths[kind] = (U32)Max(column_max_lengths[kind], label.size);
}

void 
column_max_compute_meta()
{
  for(U32Each(os, OSKind_COUNT))
  {
    column_max_str8(ColumnKind_OS, os_get_main_label(os));
  }
  
  for(U32Each(comp, CompilerKind_COUNT))
  {
    column_max_str8(ColumnKind_Compiler, compiler_get_main_label(comp));
  }
  
  for(U32Each(idx, CompilerFlag_COUNT))
  {
    Compiler_Flag_Info *info = (compiler_flags_info + idx);
    column_max_str8(ColumnKind_DebugFlag, info->label);
  }
}

int
main(int argc, char **argv)
{
  //- init
  os_init();
  Thread_Context tctx = {0};
  thread_context_init(&tctx);
  os_thread_context_set(&tctx);
  
  Arena arena_store = {0};
  {
    U64 size = MB(1);
    arena_init(&arena_store, os_memory_commit(size), size);
  }
  Arena *arena = &arena_store;
  String8_Array args_list = str8_array_from_cstr_array(arena, argv+1, argc-1);
  
  // NOTE(erb): compute largest columns
  column_max_compute_meta();
  
  String8 target_dir = {0};
  {
    target_dir = os_get_enviornment_variable(arena, "BLD_TARGET_DIR");
    if (target_dir.value == 0)
    {
      target_dir = S8("build");
    }
  }
  
  String8_List project_includes = {0};
  {
    String8 include_var = os_get_enviornment_variable(arena, "BLD_INCLUDES");
    if (include_var.value != 0)
    {
      project_includes = str8_split_blank(arena, include_var);
    }
    else
    {
      str8_list_push(arena, &project_includes, S8(".."));
      str8_list_push(arena, &project_includes, S8("..\\external\\freetype"));
    }
  }
  
  Compiler_Flag_Array_Flag project_compilation_flags = {0};
  project_compilation_flags[CompilerFlag_FullWarn] = true;
  project_compilation_flags[CompilerFlag_NoWarnNamelessMemberStructs] = true;
  //project_compilation_flags[CompilerFlag_NoWarnInitalizedNotUsed] = true;
  project_compilation_flags[CompilerFlag_NoWarnParameterNotReferenced] = true;
  
  get_enviornment_compiler_flags(project_compilation_flags);
  
  os_logf("[target dir '%.*s']", S8Expand(target_dir));
  
  os_log(S8("[includes]"));
  for(EachNode(String8_Node, node, project_includes.first))
  {
    os_logf("  [%.*s]", S8Expand(node->str));
  }
  
  os_log(S8("[flags]"));
  {
    Column_Kind columns[] = {ColumnKind_DebugFlag};
    Table table = table_make(arena, "  [%T = %s]", columns, ArraySize(columns));
    for (U32Each(flag_it, CompilerFlag_COUNT))
    {
      table_row_log(&table, 
                    compiler_flags_info[flag_it].label, 
                    project_compilation_flags[flag_it]?"y":"n");
    }
  }
  
#if 0
  rebuild_yourself();
#endif
  
  //-code
  Build_Options options = parse_options(args_list);
  // TODO(erb):  check error
  
  if (options.target_path.value != 0)
  {
    //run_command(S8("raddbg --ipc kill_all"));
    
    // NOTE(erb): clear target directory
    {
      B32 deleted_dir = os_delete_directory_recur(target_dir);
      B32 created_dir = os_create_directory(target_dir);
      if (!deleted_dir && !created_dir)
      {
        os_logf("[failed clear dir '%.*s]'", S8Expand(target_dir));
      }
    }
    
    // NOTE(erb): get entry points
    Entry_Point_List entry_points = entry_points_from_path(arena, options.target_path);
    
    if (entry_points.first != 0)
    {
      OS_Clock_Time time_begin = os_get_clock_time();
      
      os_set_current_directory(S8("build"));
      os_log(S8("-> /build"));
      
      Command_Cache command_cache = {0};
      command_cache.arena = push_sub_arena(arena, KB(2));
      {
        // NOTE(erb): generate commands
        for (U32Each(comp, CompilerKind_COUNT))
        {
          if (options.compilers[comp])
          {
            Command_Strings *cmd = (command_cache.compiler_strings + comp);
            *cmd = generate_compiler_build_command(&command_cache.arena, comp, project_includes, 
                                                   project_compilation_flags);
          }
        }
      }
      
      Build_Command_List cmd_list = {0};
      // NOTE(erb): emit entry point commands
      for (EachNode(Entry_Point, entry, entry_points.first))
      {
        build_entry_point(arena, &cmd_list, &options, entry, &command_cache);
      }
      
      // NOTE(erb): calculate largest output path
      for (EachNode(Build_Command, cmd, cmd_list.first)) 
      {
        column_max_str8(ColumnKind_EntryPath, cmd->os_path);
        column_max_str8(ColumnKind_OutputPath, cmd->unix_output_file_name);
      }
      
      // NOTE(erb): print commands
      {
        Column_Kind columns[] = {
          ColumnKind_OS, 
          ColumnKind_Compiler, 
          ColumnKind_EntryPath, 
          ColumnKind_OutputPath
        };
        Table table = table_make(arena, "  [%T %T %T -> %T]", columns, ArraySize(columns));
        for (EachNode(Build_Command, cmd, cmd_list.first)) 
        {
          table_row_log(&table, 
                        os_get_main_label(cmd->os), 
                        compiler_get_main_label(cmd->compiler), 
                        cmd->os_path, cmd->unix_output_file_name);
        }
      }
      
      Column_Kind columns[] = {ColumnKind_OutputPath};
      cmd_table = table_make(arena, "[%T %.3fs]", columns, ArraySize(columns));
      
      
      U32 handle_count = 0;
      OS_Thread_Handle *handles = PushArray(arena, cmd_list.count, OS_Thread_Handle);
      for (EachNode(Build_Command, cmd, cmd_list.first)) 
      {
        if (options.only_print_cmd)
        {
          os_log(cmd->built_str);
        }
        else
        {
          OS_Thread_Handle *h = (handles + handle_count++);
          *h = os_thread_create_and_run(run_command_thread_proc, (void *)cmd);
        }
      }
      
      if(handle_count > 0)
      {
        B32 threads_joined = os_thread_join_threads(handles, handle_count);
        if(threads_joined)
        {
          os_log(S8("threads failed"));
        }
      }
      
      table_row_log(&cmd_table, S8("TOTAL"), os_seconds_elapsed(time_begin, os_get_clock_time()));
    }
    else
    {
      os_logf("no entry points in %.*s", S8Expand(options.target_path));
    }
  }
  else
  {
    os_log(S8("expected a target"));
  }
  
  return(0);
}