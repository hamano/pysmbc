/* -*- Mode: C; c-file-style: "gnu" -*-
 * pysmbc - Python bindings for libsmbclient
 * Copyright (C) 2002, 2005, 2006, 2007, 2008  Tim Waugh <twaugh@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <Python.h>
#include "smbcmodule.h"
#include "context.h"
#include "dir.h"
#include "file.h"
#include "smbcdirent.h"

static PyMethodDef SmbcMethods[] = {
  { NULL, NULL, 0, NULL }
};

PyObject *PermissionError;

void
initsmbc (void)
{
  PyObject *m = Py_InitModule ("smbc", SmbcMethods);
  PyObject *d = PyModule_GetDict (m);

  // Context type
  smbc_ContextType.tp_new = PyType_GenericNew;
  if (PyType_Ready (&smbc_ContextType) < 0)
    return;

  PyModule_AddObject (m, "Context", (PyObject *) &smbc_ContextType);

  // Dir type
  smbc_DirType.tp_new = PyType_GenericNew;
  if (PyType_Ready (&smbc_DirType) < 0)
    return;

  PyModule_AddObject (m, "Dir", (PyObject *) &smbc_DirType);

  // File type
  smbc_FileType.tp_new = PyType_GenericNew;
  if (PyType_Ready (&smbc_FileType) < 0)
    return;

  PyModule_AddObject (m, "File", (PyObject *) &smbc_FileType);

  // Dirent type
  smbc_DirentType.tp_new = PyType_GenericNew;
  if (PyType_Ready (&smbc_DirentType) < 0)
    return;

  PyModule_AddObject (m, "Dirent", (PyObject *) &smbc_DirentType);

#define INT_CONSTANT(prefix, name)			\
  do							\
  {							\
    PyObject *val = PyInt_FromLong (prefix##name);	\
    PyDict_SetItemString (d, #name, val);		\
    Py_DECREF (val);					\
  } while (0);

  INT_CONSTANT (SMBC_, WORKGROUP);
  INT_CONSTANT (SMBC_, SERVER);
  INT_CONSTANT (SMBC_, FILE_SHARE);
  INT_CONSTANT (SMBC_, PRINTER_SHARE);
  INT_CONSTANT (SMBC_, COMMS_SHARE);
  INT_CONSTANT (SMBC_, IPC_SHARE);
  INT_CONSTANT (SMB_CTX_, FLAG_USE_KERBEROS);
  INT_CONSTANT (SMB_CTX_, FLAG_FALLBACK_AFTER_KERBEROS);
  INT_CONSTANT (SMBCCTX_, FLAG_NO_AUTO_ANONYMOUS_LOGON);

  PermissionError = PyErr_NewException("smbc.PermissionError", NULL, NULL);
  Py_INCREF(PermissionError);
  PyModule_AddObject(m, "PermissionError", PermissionError);
}

///////////////
// Debugging //
///////////////

#define ENVAR "PYSMBC_DEBUG"
static int debugging_enabled = -1;

void
debugprintf (const char *fmt, ...)
{
  if (!debugging_enabled)
    return;

  if (debugging_enabled == -1)
    {
      if (!getenv (ENVAR))
	{
	  debugging_enabled = 0;
	  return;
	}

      debugging_enabled = 1;
    }
  
  {
    va_list ap;
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
  }
}
