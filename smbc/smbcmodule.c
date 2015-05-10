/* -*- Mode: C; c-file-style: "gnu" -*-
 * pysmbc - Python bindings for libsmbclient
 * Copyright (C) 2002, 2005, 2006, 2007, 2008, 2010  Red Hat, Inc
 * Copyright (C) 2010  Open Source Solution Technology Corporation
 * Copyright (C) 2010  Patrick Geltinger <patlkli@patlkli.org>
 * Authors:
 *  Tim Waugh <twaugh@redhat.com>
 *  Tsukasa Hamano <hamano@osstech.co.jp>
 *  Patrick Geltinger <patlkli@patlkli.org>
 *  Fabio Isgrò <fisgro@babel.it>
 *  Roberto Polli <rpolli@babel.it>
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

PyObject *NoEntryError;
PyObject *PermissionError;
PyObject *ExistsError;
PyObject *NotEmptyError;
PyObject *TimedOutError;
PyObject *NoSpaceError;
PyObject *NotDirectoryError;
PyObject *ConnectionRefusedError;

#if PY_MAJOR_VERSION >= 3
  #define PYSMBC_INIT_ERROR NULL
  static struct PyModuleDef smbc_module = {
    PyModuleDef_HEAD_INIT,
    "_smbc",
    NULL,
    -1,
    SmbcMethods
  };

	#define PYSMBC_PROTOTYPE_HEADER PyObject * PyInit__smbc (void)
	#define PYSMBC_MODULE_CREATOR PyModule_Create (&smbc_module)
#else
	#define PYSMBC_INIT_ERROR
	#define PYSMBC_PROTOTYPE_HEADER void init_smbc (void)
	#define PYSMBC_MODULE_CREATOR Py_InitModule ("_smbc", SmbcMethods)
#endif

PYSMBC_PROTOTYPE_HEADER
{
  PyObject *m = PYSMBC_MODULE_CREATOR;

  PyObject *d = PyModule_GetDict (m);

  // Context type
  if (PyType_Ready (&smbc_ContextType) < 0)
    return PYSMBC_INIT_ERROR;
  PyModule_AddObject (m, "Context", (PyObject *) &smbc_ContextType);

  // Dir type
  if (PyType_Ready (&smbc_DirType) < 0)
    return PYSMBC_INIT_ERROR;
  PyModule_AddObject (m, "Dir", (PyObject *) &smbc_DirType);

  // File type
  if (PyType_Ready (&smbc_FileType) < 0)
    return PYSMBC_INIT_ERROR;
  PyModule_AddObject (m, "File", (PyObject *) &smbc_FileType);

  // Dirent type
  if (PyType_Ready (&smbc_DirentType) < 0)
    return PYSMBC_INIT_ERROR;
  PyModule_AddObject (m, "Dirent", (PyObject *) &smbc_DirentType);

  // ACL string constants
  PyModule_AddStringConstant(m, "XATTR_ALL", SMBC_XATTR_ALL);
  PyModule_AddStringConstant(m, "XATTR_ALL_SID", SMBC_XATTR_ALL_SID);
  PyModule_AddStringConstant(m, "XATTR_GROUP", SMBC_XATTR_GROUP);
  PyModule_AddStringConstant(m, "XATTR_GROUP_SID", SMBC_XATTR_GROUP_SID);
  PyModule_AddStringConstant(m, "XATTR_OWNER", SMBC_XATTR_OWNER);
  PyModule_AddStringConstant(m, "XATTR_OWNER_SID", SMBC_XATTR_OWNER_SID);
  PyModule_AddStringConstant(m, "XATTR_ACL", SMBC_XATTR_ACL);
  PyModule_AddStringConstant(m, "XATTR_ACL_SID", SMBC_XATTR_ACL_SID);
  PyModule_AddStringConstant(m, "XATTR_REVISION", SMBC_XATTR_REVISION);

#define INT_CONSTANT(prefix, name)			\
  do							\
  {							\
    PyObject *val = PyLong_FromLong (prefix##name);	\
    PyDict_SetItemString (d, #name, val);		\
    Py_DECREF (val);					\
  } while (0);

  INT_CONSTANT (SMBC_, WORKGROUP);
  INT_CONSTANT (SMBC_, SERVER);
  INT_CONSTANT (SMBC_, FILE_SHARE);
  INT_CONSTANT (SMBC_, PRINTER_SHARE);
  INT_CONSTANT (SMBC_, COMMS_SHARE);
  INT_CONSTANT (SMBC_, IPC_SHARE);
  INT_CONSTANT (SMBC_, DIR);
  INT_CONSTANT (SMBC_, FILE);
  INT_CONSTANT (SMBC_, LINK);
  INT_CONSTANT (SMB_CTX_, FLAG_USE_KERBEROS);
  INT_CONSTANT (SMB_CTX_, FLAG_FALLBACK_AFTER_KERBEROS);
  INT_CONSTANT (SMBCCTX_, FLAG_NO_AUTO_ANONYMOUS_LOGON);

  // define constants for ACL
  INT_CONSTANT (SMBC_, XATTR_FLAG_CREATE);
  INT_CONSTANT (SMBC_, XATTR_FLAG_REPLACE);

  // define exception objects
  NoEntryError = PyErr_NewException("smbc.NoEntryError", NULL, NULL);
  Py_INCREF(NoEntryError);
  PyModule_AddObject(m, "NoEntryError", NoEntryError);

  PermissionError = PyErr_NewException("smbc.PermissionError", NULL, NULL);
  Py_INCREF(PermissionError);
  PyModule_AddObject(m, "PermissionError", PermissionError);

  ExistsError = PyErr_NewException("smbc.ExistsError", NULL, NULL);
  Py_INCREF(ExistsError);
  PyModule_AddObject(m, "ExistsError", ExistsError);

  NotEmptyError = PyErr_NewException("smbc.NotEmptyError", NULL, NULL);
  Py_INCREF(NotEmptyError);
  PyModule_AddObject(m, "NotEmptyError", NotEmptyError);

  TimedOutError = PyErr_NewException("smbc.TimedOutError", NULL, NULL);
  Py_INCREF(TimedOutError);
  PyModule_AddObject(m, "TimedOutError", TimedOutError);

  NoSpaceError = PyErr_NewException("smbc.NoSpaceError", NULL, NULL);
  Py_INCREF(NoSpaceError);
  PyModule_AddObject(m, "NoSpaceError", NoSpaceError);

  NotDirectoryError = PyErr_NewException("smbc.NotDirectoryError", NULL, NULL);
  Py_INCREF(NotDirectoryError);
  PyModule_AddObject(m, "NotDirectoryError", NotDirectoryError);

  ConnectionRefusedError = PyErr_NewException("smbc.ConnectionRefusedError", NULL, NULL);
  Py_INCREF(ConnectionRefusedError);
  PyModule_AddObject(m, "ConnectionRefusedError", ConnectionRefusedError);

#if PY_MAJOR_VERSION >= 3
  return m;
#endif
}

void pysmbc_SetFromErrno()
{
  switch(errno){
  case EPERM:
	PyErr_SetFromErrno(PermissionError);
	break;
  case EEXIST:
	PyErr_SetFromErrno(ExistsError);
	break;
  case ENOTEMPTY:
	PyErr_SetFromErrno(NotEmptyError);
	break;
  case EACCES:
	PyErr_SetFromErrno(PermissionError);
	break;
  case ENOENT:
	PyErr_SetFromErrno(NoEntryError);
	break;
  case ETIMEDOUT:
	PyErr_SetFromErrno(TimedOutError);
	break;
  case ENOMEM:
	PyErr_SetFromErrno(PyExc_MemoryError);
	break;
  case ENOSPC:
	PyErr_SetFromErrno(NoSpaceError);
	break;
  case EINVAL:
	PyErr_SetFromErrno(PyExc_ValueError);
	break;
  case ENOTDIR:
    PyErr_SetFromErrno(NotDirectoryError);
    break;
  case ECONNREFUSED:
    PyErr_SetFromErrno(ConnectionRefusedError);
    break;
  default:
	PyErr_SetFromErrno(PyExc_RuntimeError);
  }
  return;
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
