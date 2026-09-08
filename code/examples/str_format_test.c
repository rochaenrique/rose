#include "base/base.h"

#include "base/base.c"

void
test_varg(const char *format, va_list src_args)
{
  Scratch scratch = scratch_begin(0, 0);
  
  String8 clib = {0};
  {
    va_list args;
    va_copy(args, src_args);
    clib = push_str8(scratch.arena, 256);
    clib.size = vsnprintf((char *)clib.value, clib.size, format, args);
    va_end(args);
  }
  
  String8 base = {0};
  {
    va_list args;
    va_copy(args, src_args);
    base = push_str8f_varg(scratch.arena, format, args);
    va_end(args);
  }
  
  B32 is_equal = str8_equals(clib, base);
  Assert(is_equal);
  scratch_end(&scratch);
}

void
test(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  {
    test_varg(format, args);
  }
  va_end(args);
}

void
test_u64_specifier(const char *format, char specifier, U64 value)
{
  Scratch scratch = scratch_begin(0, 0);
  String8 f = S8CStr((char *)format);
  
  String8_Writer sw = push_str8_writer(scratch.arena, f.size+2);
  str8_writer_str(&sw, f);
  str8_writer_char(&sw, specifier, 1);
  str8_writer_char(&sw, 0, 1);
  
  test((const char *)sw.str.value, value);
  
  scratch_end(&scratch);
}

void
test_u64_all(const char *format, U64 value)
{
  test_u64_specifier(format, 'u', value);
  test_u64_specifier(format, 'o', value);
  test_u64_specifier(format, 'x', value);
  test_u64_specifier(format, 'X', value);
}

void
test_f64_specifier(const char *format, char specifier, F64 value)
{
  Scratch scratch = scratch_begin(0, 0);
  String8 f = S8CStr((char *)format);
  
  String8_Writer sw = push_str8_writer(scratch.arena, f.size+2);
  str8_writer_str(&sw, f);
  str8_writer_char(&sw, specifier, 1);
  str8_writer_char(&sw, 0, 1);
  
  test((const char *)sw.str.value, value);
  
  scratch_end(&scratch);
}

void
test_f64_all(const char *format, F64 value)
{
  //test_f64_specifier(format, 'a', value);
  //test_f64_specifier(format, 'A', value);
  test_f64_specifier(format, 'f', value);
  test_f64_specifier(format, 'F', value);
}

OS_ENTRY_POINT()
{
  //- os init
  B32 os_initialized = os_init();
  if (!os_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  Thread_Context tctx = {0};
  thread_context_init(&tctx);
  os_thread_context_set(&tctx);
  
  {
    U64 x = 10;
    test_u64_all("%#5", x);
    test_u64_all("%#.5", x);
    test_u64_all("%#05", x);
    test_u64_all("%#0.5", x);
    test_u64_all("%#-5", x);
    test_u64_all("%#-.5", x);
    test_u64_all("%#-05", x);
    test_u64_all("%#-0.5", x);
    test_u64_all("%5", x);
    test_u64_all("%.5", x);
    test_u64_all("%05", x);
    test_u64_all("%0.5", x);
    test_u64_all("%-5", x);
    test_u64_all("%-.5", x);
    test_u64_all("%-05", x);
    test_u64_all("%-0.5", x);
  }
  
  test("%d %d %d", -69, 100, 4);
  
  {
    I32 values[] = { -69, 69, 0, 1000, 234324 };
    for(EachElement(I32, x, values))
    {
      test("%5d", *x);
      test("%.5d", *x);
      test("%05d", *x);
      test("%0.5d", *x);
      test("%-5d", *x);
      test("%-.5d", *x);
      test("%-05d", *x);
      test("%-0.5d", *x);
    }
  }
  
  {
    F64 values[] = { -1, 3.1416, -69.420f };
    for(EachElement(F64, x, values))
    {
      test_f64_all("%"   , *x);
      test_f64_all("%4.2", *x);
      test_f64_all("%.8" , *x);
      test_f64_all("%8"  , *x);
      test_f64_all("%0.8", *x);
      test_f64_all("%08" , *x);
      test_f64_all("%#"  , *x);
    }
  }
  
  {
    test("Name [%10.10s]\n", "John");
    
    char *names[] = 
    {
      "John",
      "Jean-Francois",
      "Yoko",
      "Enrique Rocha Benatti",
    };
    
    for(EachIdx(idx, names))
    {
      test("Name %d [%10.10s]\n", (I32)idx+1, names[idx]);
    }
    
    String8 str = S8("big fat str");
    test("%.3f -> %.*s %d %u", 1.0f, S8Expand(str), -123, 12);
  }
  
  {
    String8 str = S8("big fat str");
    Scratch scr = scratch_begin(0, 0);
    
    String8 clib = {0};
    {
      clib = push_str8(scr.arena, 256);
      clib.size = snprintf((char *)clib.value, clib.size, "%.*s", S8Expand(str));
    }
    
    String8 base = push_str8f(scr.arena, "%S", str);
    
    B32 is_equal = str8_equals(clib, base);
    Assert(is_equal);
    scratch_end(&scr);
  }
  
  {
    String8 str = S8("big fat str");
    Scratch scr = scratch_begin(0, 0);
    
    String8 clib = {0};
    {
      clib = push_str8(scr.arena, 256);
      clib.size = snprintf((char *)clib.value, clib.size, "%20.*s", S8Expand(str));
    }
    
    String8 base = push_str8f(scr.arena, "%20S", str);
    
    B32 is_equal = str8_equals(clib, base);
    Assert(is_equal);
    scratch_end(&scr);
  }
  
}