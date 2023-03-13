/* Minimal main program -- everything is loaded from the library */

#include "Python.h"

#ifdef MS_WINDOWS


int
wmain(int argc, wchar_t **argv)
{
    wchar_t* program = Py_DecodeLocale(argv[0], NULL);
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }
    Py_SetProgramName(program);  /* optional but recommended */
    Py_Initialize();
    PyRun_SimpleString("from time import time,ctime\n"
        "print('Today is', ctime(time()))\n");
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
    PyMem_RawFree(program);
    return 0;
}





#else
int
main(int argc, char **argv)
{
    return Py_BytesMain(argc, argv);
}
#endif
