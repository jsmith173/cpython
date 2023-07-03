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

//#define DEBUGMSG

#ifdef DEBUGMSG
#define DBG(fmt, ...) \
 do { \
	 log_file = _wfopen(log_fn, L"at"); \
	 fprintf(log_file, fmt, ##__VA_ARGS__); \
	 fclose(log_file); \
 } while(0)
#else
#define DBG
#endif

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

#define bSym_CircuitID 7
#define bSym_PMBUS_Var 11

#define PMBUS_BUF_MAX 16384

#define DP_SAVE 1
#define DP_CHECKSIZE 0

#define ERROR_BUFFER_SIZE 512
#define EPS (1e-15)

typedef struct _draw_pref_t {
    int typ;
    double l_limit, r_limit;
	int i_subdiv;
	wchar_t *u_par, *u_res, *n_par, *n_res, *fname;
	int curvecolor, curvewidth;
	wchar_t *pagename;
	int flags;
} draw_pref_t;

typedef struct _circ_item_t {
    int SimType;
} circ_item_t;
//

circ_item_t* circ_items=NULL;

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
	int *p_int, datapos, c, iID, nValues;
	double rRe, rIm;
    Py_complex cval;
	unsigned char NumType, SimType, *pmbus_buffer=NULL, tmp;

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

    circ_items = (circ_item_t*)calloc(num_of_circuit_symbols*sizeof(circ_item_t), 1);

	for (int i=0; i<c; i++) {
 	 iID = *((int*)p); stream_pos += sizeof(int); p = p0+stream_pos;
 	 SimType = *((unsigned char*)p); stream_pos += sizeof(unsigned char); p = p0+stream_pos;
     p_sym = search_circuit_id(pos_strings, iID);

     circ_items[i].SimType = SimType;
	 if (SimType == bSym_PMBUS_Var) {
 	  nValues = *((int*)p); stream_pos += sizeof(int); p = p0+stream_pos;
      PyObject* pyList = PyList_New(nValues);
      PyDict_SetItemString(dictionary, p_sym, pyList);
      for (int j=0; j<nValues; j++) {
       tmp = *((unsigned char*)p); stream_pos += sizeof(unsigned char); p = p0+stream_pos;
       PyList_SetItem(pyList, j, Py_BuildValue("i", tmp));
      }
	 }
	 else {
      NumType = *((unsigned char*)p); stream_pos += sizeof(unsigned char); p = p0+stream_pos;

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
	}
	
	//
	
	return 1;
	
error:
	return 0;
}	

char* write_string(wchar_t* s, char* p)
{
	int copied, len = wcslen(s);
    char* p1;

    *((int*)p) = len; p += sizeof(int); p1 = p;
    wcscpy(p, s); p += len*sizeof(s[0]);
    copied = (int)(p - p1);
    return p;
}

char* write_ansistring(char* s, char* p)
{
	int copied, len = strlen(s);
	char* p1;
	
    *((int*)p) = len; p += sizeof(int); p1 = p;
    strcpy(p, s); p += len;
	copied = (int)(p-p1);
    return p;
}

char* set_curve_status(int st, char* p, char* err)
{
 *((int*)p) = st; p += sizeof(st); 
 return write_ansistring(err, p);
}

enum TDrawPrefField {
	Dp_typ=0, Dp_l_limit, Dp_r_limit, Dp_i_subdiv,
	Dp_u_par, Dp_u_res, Dp_n_par, Dp_n_res,
	Dp_curvecolor, Dp_curvewidth, Dp_pagename, Dp_fname, Dp_flags	
}; 

char* process_draw_pref(PyObject *CurvePref, draw_pref_t* draw_pref, char* p, int* len, char* err, int dp_save, int* iRet)
{
	int item_size, tmp, size=0, item_code, N=0, *i_item_ptr;
	char field[128], *p_item_num;
	PyObject* PyField;
	wchar_t* s_item_ptr;
	double* r_item_ptr;
	
	*iRet = 1;
	p_item_num = p; p += sizeof(N); size += sizeof(N); 
	
	for (int i=0; i<2; i++) {
		if (i == 0) { strcpy(field, "l_limit"); item_code = Dp_l_limit; r_item_ptr = &draw_pref->l_limit; } 
		if (i == 1) { strcpy(field, "r_limit"); item_code = Dp_r_limit; r_item_ptr = &draw_pref->r_limit; } 
		
		PyField = PyDict_GetItem(CurvePref, PyUnicode_FromString(field));
		if (PyField && PyFloat_Check(PyField)) {
			 *r_item_ptr = PyFloat_AsDouble(PyField);
			 item_size = sizeof(double); size += item_size; size += sizeof(item_code); N++;
			 if (dp_save) { 
			  *((int*)p) = item_code; p += sizeof(item_code);
			  memcpy(p, r_item_ptr, item_size); p += item_size; 
			 }
		}
		else {
			sprintf(err, "draw_pref: field %s not found", field);
			goto error;
		}
	}

	for (int i=0; i<5; i++) {
		if (i == 0) { strcpy(field, "typ"); item_code = Dp_typ; i_item_ptr = &draw_pref->typ; } 
		if (i == 1) { strcpy(field, "i_subdiv"); item_code = Dp_i_subdiv; i_item_ptr = &draw_pref->i_subdiv; } 
		if (i == 2) { strcpy(field, "curvecolor"); item_code = Dp_curvecolor; i_item_ptr = &draw_pref->curvecolor; } 
		if (i == 3) { strcpy(field, "curvewidth"); item_code = Dp_curvewidth; i_item_ptr = &draw_pref->curvewidth; } 
		if (i == 4) { strcpy(field, "flags"); item_code = Dp_flags; i_item_ptr = &draw_pref->flags; } 
		
		PyField = PyDict_GetItem(CurvePref, PyUnicode_FromString(field));
		if (PyField && PyLong_Check(PyField)) {
			 *i_item_ptr = PyLong_AsLong(PyField);
			 item_size = sizeof(int); size += item_size; size += sizeof(item_code); N++;
			 if (dp_save) { 
			  *((int*)p) = item_code; p += sizeof(item_code);
			  memcpy(p, i_item_ptr, item_size); p += item_size; 
			 }
		}
		else if (item_code == Dp_typ) {
			sprintf(err, "draw_pref: field %s not found", field);
			goto error;
		}
	}

	for (int i=0; i<6; i++) {
		if (i == 0) { strcpy(field, "u_par"); item_code = Dp_u_par; } 
		if (i == 1) { strcpy(field, "u_res"); item_code = Dp_u_res; } 
		if (i == 2) { strcpy(field, "n_par"); item_code = Dp_n_par; } 
		if (i == 3) { strcpy(field, "n_res"); item_code = Dp_n_res; } 
		if (i == 4) { strcpy(field, "pagename"); item_code = Dp_pagename; } 
		if (i == 5) { strcpy(field, "fname"); item_code = Dp_fname; } 
		
		PyField = PyDict_GetItem(CurvePref, PyUnicode_FromString(field));
		if (PyField && PyUnicode_Check(PyField)) {
			 s_item_ptr = PyUnicode_AsWideCharString(PyField, &tmp);
			 item_size = wcslen(s_item_ptr)*sizeof(wchar_t)+sizeof(int); size += item_size; size += sizeof(item_code); N++;
			 if (dp_save) { 
			  *((int*)p) = item_code; p += sizeof(item_code);
			  p = write_string(s_item_ptr, p); 
			 }
		}
		else if (item_code == Dp_fname) {
			sprintf(err, "draw_pref: field %s not found", field);
			goto error;
		}
	}
	
	
    if (dp_save) *((int*)p_item_num) = N;
	*len = size;
	
done:	
	return p;
	
error:
	*iRet = 0;
	return NULL;
	
}

int Curves_CalcSize(PyObject* dictionary, char* err, int* len)
{
	wchar_t fn[512];
	FILE *symbol_file;
	PyObject *py_obj, *pValue, *pList, * pResult;
	char *p=NULL, *p_sym;
	double rRe, rIm, x, y, step;
	int file_size, numwrite, i, N, j, M, nw, alloc_size, item_len, size, iRet;
	void *circ_buffer_out, *curve_buffer_out;
    Py_complex cval;
	unsigned char NumType;
	draw_pref_t draw_pref;
	
	size = 0; *len = size;
    PyObject* CurvesPrefs = PyDict_GetItem(dictionary, PyUnicode_FromString(CURVES_PREFS));
	if (CurvesPrefs && PyList_Check(CurvesPrefs)) {
		N = PyList_Size(CurvesPrefs);
		for (i=0; i<N; i++) {
            PyObject *CurvePref = PyList_GetItem(CurvesPrefs, i);
            if (CurvePref && PyDict_Check(CurvePref)) {
                p = process_draw_pref(CurvePref, &draw_pref, p, &item_len, err, DP_CHECKSIZE, &iRet);
				if (!iRet) goto error;
				size += item_len;
			}
		}
	}
	
	item_len = 0;
	size += sizeof(N);
    PyObject* Curves = PyDict_GetItem(dictionary, PyUnicode_FromString(CURVES));
	if (Curves && PyList_Check(Curves)) {
		N = PyList_Size(Curves);
		for (i=0; i<N; i++) {
            PyObject *Curve = PyList_GetItem(Curves, i);
            if (Curve && PyList_Check(Curve)) {
                M = PyList_Size(Curve);
				item_len += sizeof(M);
				item_len += M*(sizeof(int)+2*(sizeof(unsigned char)+sizeof(cval))); // max size (real type data is smaller)
            }
		}	
	}
	size += item_len;
	
done:	
	*len = size;
	return 1;
	
error:
	return 0;
	
}

int PyMod_SaveCurves(PyObject* dictionary)
{
	wchar_t fn[512];
	char err[ERROR_BUFFER_SIZE];
	FILE *symbol_file;
	PyObject *py_obj, *pValue, *pList, * pResult;
	char *p, *p_sym, *p_status;
	double rRe, rIm, x, y, step, r;
	int file_size, numwrite, i, j, k, K, N=0, N1=0, M, nw, alloc_size, len, iRet, status_size;
	void *curve_buffer_out=NULL;
    Py_complex cval;
	unsigned char NumType;
	draw_pref_t draw_pref;
	
	//
	memset(err, 0, sizeof(err));
	status_size = sizeof(iRet)+sizeof(err);
    iRet = Curves_CalcSize(dictionary, err, &alloc_size);
	alloc_size += status_size;
    curve_buffer_out = calloc(alloc_size, 1); p = p_status = curve_buffer_out;
	p = set_curve_status(1, p_status, "OK"); p = p_status+status_size;
	
	if (!iRet) {
		set_curve_status(iRet, p_status, err);
		goto error;
	}
	
    PyObject* CurvesPrefs = PyDict_GetItem(dictionary, PyUnicode_FromString(CURVES_PREFS));
	if (CurvesPrefs && PyList_Check(CurvesPrefs)) {
 	 N1 = PyList_Size(CurvesPrefs);
	}
	
    PyObject* Curves = PyDict_GetItem(dictionary, PyUnicode_FromString(CURVES));
	if (Curves && PyList_Check(Curves)) {
		N = PyList_Size(Curves);
        *((int*)p) = N; p += sizeof(int);
		for (i=0; i<N; i++) {
            PyObject *Curve = PyList_GetItem(Curves, i);
            if (Curve && PyList_Check(Curve)) {
                M = PyList_Size(Curve);
				
                PyObject *CurvePref = PyList_GetItem(CurvesPrefs, i);
                if (CurvePref && PyDict_Check(CurvePref)) {
                    p = process_draw_pref(CurvePref, &draw_pref, p, &len, err, DP_SAVE, &iRet);
                    if (!iRet) {
		                set_curve_status(iRet, p_status, err);
                        goto error;
                    }
                    step = (draw_pref.r_limit-draw_pref.l_limit)/draw_pref.i_subdiv;
					if (step < 0 || fabs(step) < EPS) {
			            sprintf(err, "draw_pref: step is invalid");
		                set_curve_status(iRet, p_status, err);
			            goto error;						
					}
				
                }			
				
                *((int*)p) = M; p += sizeof(int);
				x = draw_pref.l_limit;
                for (j=0; j<M; j++) {
                    PyObject *List = PyList_GetItem(Curve, j);
					if (List && PyList_Check(List)) {
                        K = PyList_Size(List);
                        if (K != 2) {
			             sprintf(err, "SaveCurves: expected number of items in curve point is 2");
		                 set_curve_status(0, p_status, err);
                         goto error;
                        }
                        *((int*)p) = K; p += sizeof(int);
                        for (k=0; k<K; k++) {
                            PyObject *Value = PyList_GetItem(List, k);
							if (Value && PyFloat_Check(Value)) {
 	                            NumType = I_REAL; *((unsigned char*)p) = NumType; p += sizeof(unsigned char);
								r = PyFloat_AsDouble(Value);
								*((double*)p) = r; p += sizeof(double);
							}
                       	    else if (Value && PyComplex_Check(Value)) {
 	                            NumType = I_CPLX; *((unsigned char*)p) = NumType; p += sizeof(unsigned char);
                                cval = PyComplex_AsCComplex(Value);
       	                        *((double*)p) = cval.real; p += sizeof(double);
 	                            *((double*)p) = cval.imag; p += sizeof(double);
							}
						}
					}

                }
            }
		}	
	}
	else {
        *((int*)p) = N; p += sizeof(int);
	}
	nw = p-curve_buffer_out;
	
	if (N) {
		wcscpy(fn, session_folder);
		wcscat(fn, CURVES_OUT_FN);
		symbol_file = _wfopen(fn, L"wb");
		if (symbol_file) {
			numwrite = fwrite(curve_buffer_out, 1, nw, symbol_file);
			fclose(symbol_file);
		}
		else
			return 0;
	}
	
done:	
	free(curve_buffer_out);
    return 1;
	
error:
	free(curve_buffer_out);
	return 0;

	//

}

int PyMod_SaveCircuitVariables()
{
	wchar_t fn[512];
	FILE *symbol_file;
	PyObject *py_obj, *pValue, *pList, * pResult;
	char *p, *p_sym;
	double rRe, rIm, x, y, step;
	int file_size, numwrite, i, N, j, M, nw, alloc_size, len, nValues;
	void *circ_buffer_out, *curve_buffer_out;
    Py_complex cval;
	unsigned char NumType, SimType, byte1;
	draw_pref_t draw_pref;


    PyObject * module = PyImport_AddModule("__main__"); // borrowed reference
	if (!module)
		goto error;

	PyObject * dictionary = PyModule_GetDict(module);   // borrowed reference
	if (!dictionary)
		goto error;

    file_size = 2*sizeof(int);
	file_size += num_of_circuit_symbols*(2*sizeof(double)+sizeof(unsigned char))+PMBUS_BUF_MAX;
    circ_buffer_out = calloc(file_size, 1);

    p = circ_buffer_out;
	*((int*)p) = error_state; p += sizeof(int);
	*((int*)p) = num_of_circuit_symbols; p += sizeof(int);

	for (int i=0; i<num_of_circuit_symbols; i++) {
     p_sym = search_circuit_id(pos_strings, i);
     py_obj = PyDict_GetItem(dictionary, PyUnicode_FromString(p_sym));

     SimType = circ_items[i].SimType;
     *((unsigned char*)p) = SimType; p += sizeof(unsigned char);

	 if (PyFloat_Check(py_obj)) {
 	  NumType = I_REAL; *((unsigned char*)p) = NumType; p += sizeof(unsigned char);
      rRe = PyFloat_AsDouble(py_obj);
 	  *((double*)p) = rRe; p += sizeof(double);
 	  rIm = 0; *((double*)p) = rIm; p += sizeof(double);
	 }
	 else if (PyLong_Check(py_obj)) {
 	  NumType = I_REAL; *((unsigned char*)p) = NumType; p += sizeof(unsigned char);
      rRe = PyLong_AsLong(py_obj);
 	  *((double*)p) = rRe; p += sizeof(double);
 	  rIm = 0; *((double*)p) = rIm; p += sizeof(double);
	 }
	 else if (PyComplex_Check(py_obj)) {
 	  NumType = I_CPLX; *((unsigned char*)p) = NumType; p += sizeof(unsigned char);
      cval = PyComplex_AsCComplex(py_obj);
 	  *((double*)p) = cval.real; p += sizeof(double);
 	  *((double*)p) = cval.imag; p += sizeof(double);
	 }
	 else if (PyList_Check(py_obj)) {
      nValues = PyList_Size(py_obj);
      *((int*)p) = nValues; p += sizeof(int);
      for (int j=0; j<nValues; j++) {
       pValue = PyList_GetItem(py_obj, j);
       byte1 = PyLong_AsLong(pValue);
  	   *((unsigned char*)p) = byte1; p += sizeof(unsigned char);
      }
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

#define FILE_NAME_LENGTH_MAX 512

int PyMod_GetSignalValue(const char* func_name, double r_par, double* r_result, int* err_code, char** msg)
{
	wchar_t fn[512];
	FILE *symbol_file;
	int file_size, numread, nr, stream_pos;
	char *p, *p0, *p1, *p_sym;
	int *p_int, datapos, c, iID;
	double rTmp;
    Py_complex cval;
	unsigned char NumType;
	char* err;
	PyObject *ptype, *pvalue, *ptraceback, *str;
	PyObject* pArgs, *pValue;
	int i, N=1;

    *err_code = 0; *msg = NULL;
    PyObject* module = PyImport_AddModule("__main__"); // borrowed reference
	if (!module) {
        *err_code = PYSIGERR_OTHER;
		goto error; 
	}

	PyObject* dictionary = PyModule_GetDict(module);   // borrowed reference
	if (!dictionary) { 
        *err_code = PYSIGERR_OTHER;
		goto error; 
	}

    PyObject* evFunc = PyDict_GetItemString(dictionary, func_name);
	if (!evFunc) {
        *err_code = PYSIGERR_FUNC_NOT_FOUND;
		goto error;  
	}
	
	if (PyCallable_Check(evFunc)) {
		pArgs = PyTuple_New(N);
		for (i = 0; i < N; ++i) {
			pValue = PyFloat_FromDouble(r_par);
			if (!pValue) {
				Py_DECREF(pArgs);
				*err_code = PYSIGERR_OTHER;
		        goto error;  
			}
			/* pValue reference stolen here: */
			PyTuple_SetItem(pArgs, i, pValue);
		}
		pValue = PyObject_CallObject(evFunc, pArgs);
		Py_DECREF(pArgs);
			
		if (PyErr_Occurred()) {
			PyErr_Fetch(&ptype, &pvalue, &ptraceback);
			PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
			
			str = PyObject_Str(pvalue);
			if (str) {
				PyObject* bytes = PyUnicode_AsUTF8String(str);
				err = PyBytes_AsString(bytes);
				*msg = calloc(strlen(err)+1, 1); strcpy(*msg, err);
				Py_XDECREF(bytes);
				*err_code = PYSIGERR_RUNTIME_ERROR;
				*r_result = 0;
				goto error;  
			}	
			else {
				*err_code = PYSIGERR_OTHER;
				*r_result = 0;
				goto error;  
			}
		}
		else if (pValue && PyFloat_Check(pValue)) {
			rTmp = PyFloat_AsDouble(pValue);
			*r_result = rTmp;
		}
		else {
			*err_code = PYSIGERR_OTHER;
			*r_result = 0;
			goto error;  
		}
		
		
	}
	
	return 0;
	
error:
	return 1;
}	

int PyMod_CompileFile(wchar_t* filename, int* err_code, char** msg)
{
	char fn_ansi[FILE_NAME_LENGTH_MAX];

    *msg = NULL;
	if (wcslen(filename) >= FILE_NAME_LENGTH_MAX) {
        *err_code = PYSIGERR_OTHER;
		goto error; 
	}
	
	wcstombs(fn_ansi, filename, FILE_NAME_LENGTH_MAX); 
	
    *err_code = 0;
    PyObject* module = PyImport_AddModule("__main__"); // borrowed reference
	if (!module) {
        *err_code = PYSIGERR_OTHER;
		goto error; 
	}

	PyObject* dictionary = PyModule_GetDict(module);   // borrowed reference
	if (!dictionary) { 
        *err_code = PYSIGERR_OTHER;
		goto error; 
	}

	char* err;
	PyObject *ptype, *pvalue, *ptraceback, *str;
    FILE* fp = fopen(fn_ansi, "r");
    PyObject* fobj = PyRun_File(fp, fn_ansi, Py_file_input, dictionary, dictionary);
	if (PyErr_Occurred()) {
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
		
        str = PyObject_Str(pvalue);
		if (str) {
            PyObject* bytes = PyUnicode_AsUTF8String(str);
            err = PyBytes_AsString(bytes);
			*msg = calloc(strlen(err)+1, 1); strcpy(*msg, err);
			Py_XDECREF(bytes);
			*err_code = PYSIGERR_COMPILE_ERR;
			fclose(fp);
			return 1;
		}	
		else {
			*err_code = PYSIGERR_OTHER;
			fclose(fp);
			return 1;
		}
	}
	
done:	
	fclose(fp);
	return 0;
	
error:
	return 1;
}	

