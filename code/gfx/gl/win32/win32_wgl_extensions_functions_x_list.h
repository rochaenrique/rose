// NOTE(erb): X(NAME, RETURN, PARAMS, EXTENSION_STRING, IS_REQUIRED)
X(wglChoosePixelFormatARB, BOOL, (HDC dc, 
                                  const int *int_attribs, 
                                  const FLOAT *float_attribs,
                                  UINT max_formats, int *pixel_formats,
                                  UINT *out_num_formats), "WGL_ARB_pixel_format", true)
// NOTE(erb): this function also softly depends on the the 'WGL_ARB_create_context_profile' 
// extension string, however, it seems that 'WGL_ARB_create_context' is always required and is 
// enough to guarantee the extension to work. 
X(wglCreateContextAttribsARB, HGLRC, (HDC dc, HGLRC shared_context, const int *attribs), "WGL_ARB_create_context", true) 
X(wglGetExtensionsStringARB, const char *, (HDC hdc), "WGL_ARB_extensions_string", true)
X(wglSwapIntervalEXT, BOOL, (int interval), "WGL_EXT_swap_control", false)
