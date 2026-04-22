X(wglChoosePixelFormatARB, BOOL, (HDC hdc,
                                  const int *piAttribIList,
                                  const FLOAT *pfAttribFList,
                                  UINT nMaxFormats,
                                  int *piFormats,
                                  UINT *nNumFormats))
X(wglCreateContextAttribsARB, HGLRC, (HDC hDC, HGLRC hShareContext,
                                      const int *attribList))