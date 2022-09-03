/* Python interpreter main program */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_initconfig.h"    // _PyArgv
#include "pycore_interp.h"        // _PyInterpreterState.sysdict
#include "pycore_pathconfig.h"    // _PyPathConfig_ComputeSysPath0()
#include "pycore_pylifecycle.h"   // _Py_PreInitializeFromPyArgv()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()

/* Includes for exit_sigint() */
#include <stdio.h>                // perror()
#ifdef HAVE_SIGNAL_H
#  include <signal.h>             // SIGINT
#endif
#if defined(HAVE_GETPID) && defined(HAVE_UNISTD_H)
#  include <unistd.h>             // getpid()
#endif
#ifdef MS_WINDOWS
#  include <windows.h>            // STATUS_CONTROL_C_EXIT
#endif
/* End of includes for exit_sigint() */


//
const wchar_t* SYMBOLS_IN_FN = L"symbols_in.dat";
const wchar_t* SYMBOLS_OUT_FN = L"symbols_out.dat";
void* circ_buffer_in;
int num_of_circuit_symbols=0;
char* pos_strings;
const char* CURVES = "curves";
const char* CURVES_PREFS = "curves_prefs";
const wchar_t* CURVES_OUT_FN = L"curves_out.dat";

#define I_REAL 0
#define I_CPLX 1

typedef struct _draw_pref_t {
    int typ;
    double l_limit, r_limit;
	int i_subdiv;
	wchar_t *u_par, *u_res, *n_par, *n_res;
	int curvecolor, curvewidth;
	wchar_t *pagename;
	int flags;
} draw_pref_t;
//


int FileSize(const wchar_t* filename)
{
   struct _stat buf;
   int result;
   char timebuf[26];
   errno_t err;

   result = _wstat( filename, &buf );

   // Check if statistics are valid:
   if( result != 0 )
   {
      perror( "Problem getting information" );
      switch (errno)
      {
         case ENOENT:
           printf("File %s not found.\n", filename);
           break;
         case EINVAL:
           printf("Invalid parameter to _stat.\n");
           break;
         default:
           /* Should never be reached. */
           printf("Unexpected error in _stat.\n");
      }
   }
   else
   {
      return buf.st_size;
   }
}

char* search_circuit_id(char* start, int id)
{
	int i;
	char* p;
	
    p = start; i = 0;
	while (i < id) {
	 while (*p) p++; 
	 p++; // the zero byte
	 i++;
	}
	return p;
}

int PyMod_AddCircuitVariables()
{
	wchar_t fn[512];
	FILE *symbol_file;
	int file_size, numread, nr, stream_pos;
	char *p, *p0, *p1, *p_sym;
	int *p_int, datapos, c, iID;
	double rRe, rIm;
    Py_complex cval;
	unsigned char NumType;

	wcscpy(fn, session_folder);
	wcscat(fn, SYMBOLS_IN_FN);
	
    PyObject * module = PyImport_AddModule("__main__"); // borrowed reference
	if (!module)
		goto error; 

	PyObject * dictionary = PyModule_GetDict(module);   // borrowed reference
	if (!dictionary) 
		goto error;  

    // Load circuit data
    symbol_file = _wfopen(fn, L"rb");
    if (symbol_file) {
        file_size = FileSize(fn);
        circ_buffer_in = calloc(file_size, 1);
        numread = fread(circ_buffer_in, 1, file_size, symbol_file);		
        fclose(symbol_file);
    }
	else
		return 0;
    //
	
	//
	p = circ_buffer_in; 
	p += sizeof(int); //version
	p_int = (int*)p; datapos = *p_int; p += sizeof(int);
	pos_strings = p;
	
	p = p0 = circ_buffer_in; stream_pos = datapos;
	p = p0+stream_pos;

	c = *((int*)p); stream_pos += sizeof(int); p = p0+stream_pos;
	num_of_circuit_symbols = c;
	
	for (int i=0; i<c; i++) {
 	 NumType = *((unsigned char*)p); stream_pos += sizeof(unsigned char); p = p0+stream_pos;
 	 iID = *((int*)p); stream_pos += sizeof(int); p = p0+stream_pos;
     p_sym = search_circuit_id(pos_strings, iID);
	 
	 if (NumType == I_REAL) {
 	  rRe = *((double*)p); stream_pos += sizeof(double); p = p0+stream_pos;
      PyDict_SetItemString(dictionary, p_sym, Py_BuildValue("d", rRe));
	 }
	 else if (NumType == I_CPLX) {
 	  rRe = *((double*)p); stream_pos += sizeof(double); p = p0+stream_pos;
 	  rIm = *((double*)p); stream_pos += sizeof(double); p = p0+stream_pos;
	  cval.real = rRe;
	  cval.imag = rIm;
      PyDict_SetItemString(dictionary, p_sym, PyComplex_FromCComplex(cval));
	 }
	 else {
		 /* set an error message */
	 }
	}
	
	//
	
	return 1;
	
error:
	return 0;
}	

int PyMod_SaveCurves(PyObject * dictionary)
{
	wchar_t fn[512];
	FILE *symbol_file;
	PyObject *py_obj, *pValue, *pList, * pResult;
	char *p, *p_sym;
	double rRe, rIm, x, y, step;
	int file_size, numwrite, i, N, j, M, nw, alloc_size, len;
	void *circ_buffer_out, *curve_buffer_out;
    Py_complex cval;
	unsigned char NumType;
	draw_pref_t draw_pref;
	
	//
	alloc_size = 163840;
    curve_buffer_out = calloc(alloc_size, 1);
	p = curve_buffer_out;
	
    PyObject* CurvesPrefs = PyDict_GetItem(dictionary, PyUnicode_FromString(CURVES_PREFS));
	if (CurvesPrefs && PyList_Check(CurvesPrefs)) {
		N = PyList_Size(CurvesPrefs);
        //*((int*)p) = N; p += sizeof(int);
		for (i=0; i<N; i++) {
            PyObject *CurvePref = PyList_GetItem(CurvesPrefs, i);
            if (CurvePref && PyDict_Check(CurvePref)) {
				
                PyObject* l_limit = PyDict_GetItem(CurvePref, PyUnicode_FromString("l_limit"));
				if (l_limit && PyFloat_Check(l_limit)) {
                     draw_pref.l_limit = PyFloat_AsDouble(l_limit);
				}
				else {
            		 /* set an error message */
				}
				
                PyObject* r_limit = PyDict_GetItem(CurvePref, PyUnicode_FromString("r_limit"));
				if (r_limit && PyFloat_Check(r_limit)) {
                     draw_pref.r_limit = PyFloat_AsDouble(r_limit);
				}
				else {
            		 /* set an error message */
				}
				
                PyObject* i_subdiv = PyDict_GetItem(CurvePref, PyUnicode_FromString("i_subdiv"));
				if (i_subdiv && PyLong_Check(i_subdiv)) {
                     draw_pref.i_subdiv = PyLong_AsLong(i_subdiv);
				}
				else {
            		 /* set an error message */
				}
				
                PyObject* u_par = PyDict_GetItem(CurvePref, PyUnicode_FromString("u_par"));
				if (u_par && PyUnicode_Check(u_par)) {
                    draw_pref.u_par = PyUnicode_AsWideCharString(u_par, &len);
				}
				else {
            		 /* set an error message */
				}
				
				step = (draw_pref.r_limit-draw_pref.l_limit)/draw_pref.i_subdiv;
            		 /* set an error message : zero*/
				
			}
		}
	}
	
    PyObject* Curves = PyDict_GetItem(dictionary, PyUnicode_FromString(CURVES));
	if (Curves && PyList_Check(Curves)) {
		N = PyList_Size(Curves);
        *((int*)p) = N; p += sizeof(int);
		for (i=0; i<N; i++) {
            PyObject *Curve = PyList_GetItem(Curves, i);
            if (Curve && PyList_Check(Curve)) {
                M = PyList_Size(Curve);
                *((int*)p) = M; p += sizeof(int);
				x = draw_pref.l_limit;
                for (j=0; j<M; j++) {
                    PyObject *Value = PyList_GetItem(Curve, j);
                    if (Value && PyFloat_Check(Value)) {
                        y = PyFloat_AsDouble(Value);
 	                    *((double*)p) = x; p += sizeof(double);
 	                    *((double*)p) = y; p += sizeof(double);
						x += step;
                    }
                }
            }
		}	
	}
	else {
        *((int*)p) = N; p += sizeof(int);
	}
	nw = p-curve_buffer_out;
	
	wcscpy(fn, session_folder);
	wcscat(fn, CURVES_OUT_FN);
    symbol_file = _wfopen(fn, L"wb");
    if (symbol_file) {
        numwrite = fwrite(curve_buffer_out, 1, nw, symbol_file);
        fclose(symbol_file);
    }
	else
		return 0;
	free(curve_buffer_out);
	//
	
}

int PyMod_SaveCircuitVariables()
{
	wchar_t fn[512];
	FILE *symbol_file;
	PyObject *py_obj, *pValue, *pList, * pResult;
	char *p, *p_sym;
	double rRe, rIm, x, y, step;
	int file_size, numwrite, i, N, j, M, nw, alloc_size, len;
	void *circ_buffer_out, *curve_buffer_out;
    Py_complex cval;
	unsigned char NumType;
	draw_pref_t draw_pref;
	

    PyObject * module = PyImport_AddModule("__main__"); // borrowed reference
	if (!module)
		goto error; 

	PyObject * dictionary = PyModule_GetDict(module);   // borrowed reference
	if (!dictionary) 
		goto error;  

    file_size = 2*sizeof(int);
	file_size += num_of_circuit_symbols*(2*sizeof(double)+sizeof(unsigned char));
    circ_buffer_out = calloc(file_size, 1);

    p = circ_buffer_out;
	*((int*)p) = error_state; p += sizeof(int);
	*((int*)p) = num_of_circuit_symbols; p += sizeof(int);
	
	for (int i=0; i<num_of_circuit_symbols; i++) {
     p_sym = search_circuit_id(pos_strings, i);
     py_obj = PyDict_GetItem(dictionary, PyUnicode_FromString(p_sym));
	 if (PyFloat_Check(py_obj)) {
 	  NumType = I_REAL; *((unsigned char*)p) = NumType; p += sizeof(unsigned char);
      rRe = PyFloat_AsDouble(py_obj);
 	  *((double*)p) = rRe; p += sizeof(double);
 	  rIm = 0; *((double*)p) = rIm; p += sizeof(double);
	 }
	 else if (PyComplex_Check(py_obj)) {
 	  NumType = I_CPLX; *((unsigned char*)p) = NumType; p += sizeof(unsigned char);
      cval = PyComplex_AsCComplex(py_obj);
 	  *((double*)p) = cval.real; p += sizeof(double);
 	  *((double*)p) = cval.imag; p += sizeof(double);
	 }
	 else {
		 /* set an error message */
	 }
	}
	
	wcscpy(fn, session_folder);
	wcscat(fn, SYMBOLS_OUT_FN);
    symbol_file = _wfopen(fn, L"wb");
    if (symbol_file) {
        numwrite = fwrite(circ_buffer_out, 1, file_size, symbol_file);
        fclose(symbol_file);
    }
	else
		return 0;
	free(circ_buffer_out);
	
    return PyMod_SaveCurves(dictionary);
	
	return 1;
	
error:
	return 0;
}


