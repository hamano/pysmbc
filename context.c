/* -*- Mode: C; c-file-style: "gnu" -*-
 * pysmbc - Python bindings for libsmbclient
 * Copyright (C) 2002, 2005, 2006, 2007, 2008, 2010  Red Hat, Inc
 * Copyright (C) 2010  Open Source Solution Technology Corporation
 * Authors:
 *  Tim Waugh <twaugh@redhat.com>
 *  Tsukasa Hamano <hamano@osstech.co.jp>
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

#include <Python.h>
#include <string.h>
#include "smbcmodule.h"
#include "context.h"
#include "dir.h"
#include "file.h"

static void
auth_fn (SMBCCTX *ctx,
	 const char *server, const char *share,
	 char *workgroup, int wgmaxlen,
	 char *username, int unmaxlen,
	 char *password, int pwmaxlen)
{
  PyObject *args;
  PyObject *kwds;
  PyObject *result;
  Context *self;
  const char *use_workgroup, *use_username, *use_password;

  debugprintf ("-> auth_fn (server=%s, share=%s)\n",
	       server ? server : "",
	       share ? share : "");

  self = smbc_getOptionUserData (ctx);
  if (self->auth_fn == NULL)
    {
      debugprintf ("<- auth_fn (), no callback\n");
      return;
    }

  if (!server || !*server)
    {
      debugprintf ("<- auth_fn(), no server\n");
      return;
    }

  args = Py_BuildValue ("(sssss)", server, share, workgroup,
			username, password);
  kwds = PyDict_New ();

  result = PyObject_Call (self->auth_fn, args, kwds);
  Py_DECREF (args);
  Py_DECREF (kwds);
  if (result == NULL)
    {
      debugprintf ("<- auth_fn(), failed callback\n");
      return;
    }

  if (!PyArg_ParseTuple (result, "sss",
			 &use_workgroup,
			 &use_username,
			 &use_password))
    {
      debugprintf ("<- auth_fn(), incorrect callback result\n");
      return;
    }

  strncpy (workgroup, use_workgroup, wgmaxlen);
  strncpy (username, use_username, unmaxlen);
  strncpy (password, use_password, pwmaxlen);
  debugprintf ("<- auth_fn(), got callback result\n");
}

/////////////
// Context //
/////////////

static PyObject *
Context_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Context *self;
  self = (Context *) type->tp_alloc (type, 0);
  if (self != NULL)
    self->context = NULL;

  return (PyObject *) self;
}

static int
Context_init (Context *self, PyObject *args, PyObject *kwds)
{
  PyObject *auth = NULL;
  int debug = 0;
  SMBCCTX *ctx;
  static char *kwlist[] = 
    {
      "auth_fn",
      "debug",
      NULL
    };

  if (!PyArg_ParseTupleAndKeywords (args, kwds, "|Oi", kwlist,
									&auth, &debug)){
    return -1;
  }

  if (auth)
    {
      if (!PyCallable_Check (auth))
	{
	  PyErr_SetString (PyExc_TypeError, "auth_fn must be callable");
	  return -1;
	}

      Py_XINCREF (auth);
      self->auth_fn = auth;
    }

  debugprintf ("-> Context_init ()\n");

  errno = 0;
  ctx = smbc_new_context ();
  if (ctx == NULL)
    {
      PyErr_SetFromErrno (PyExc_RuntimeError);
      debugprintf ("<- Context_init() EXCEPTION\n");
      return -1;
    }

  if (smbc_init_context (ctx) == NULL)
    {
      PyErr_SetFromErrno (PyExc_RuntimeError);
      smbc_free_context (ctx, 0);
      debugprintf ("<- Context_init() EXCEPTION\n");
      return -1;
    }

  smbc_setDebug (ctx, debug);

  self->context = ctx;
  smbc_setOptionUserData (ctx, self);
  if (auth)
    smbc_setFunctionAuthDataWithContext (ctx, auth_fn);

  debugprintf ("%p <- Context_init() = 0\n", self->context);
  return 0;
}

static void
Context_dealloc (Context *self)
{
  if (self->context)
    {
      debugprintf ("%p smbc_free_context()\n", self->context);
      smbc_free_context (self->context, 1);
    }

  self->ob_type->tp_free ((PyObject *) self);
}

static PyObject *
Context_open (Context *self, PyObject *args)
{
  PyObject *largs, *lkwlist;
  char *uri;
  File *file;
  int flags = 0;
  int mode = 0;
  smbc_open_fn fn;

  debugprintf ("%p -> Context_open()\n", self->context);
  if(!PyArg_ParseTuple (args, "s|ii", &uri, &flags, &mode)){
      debugprintf ("%p <- Context_open() EXCEPTION\n", self->context);
      return NULL;
  }

  largs = Py_BuildValue ("()");
  lkwlist = PyDict_New ();
  PyDict_SetItemString (lkwlist, "context", (PyObject *) self);
  file = (File *)smbc_FileType.tp_new(&smbc_FileType, largs, lkwlist);
  if(!file){
	return PyErr_NoMemory();
  }
  if (smbc_FileType.tp_init ((PyObject *)file, largs, lkwlist) < 0){
	smbc_FileType.tp_dealloc((PyObject *)file);
	debugprintf ("%p <- Context_open() EXCEPTION\n", self->context);
	return NULL;
  }
  fn = smbc_getFunctionOpen (self->context);
  file->file = (*fn)(self->context, uri, (int)flags, (mode_t)mode);
  if(!file->file){
	PyErr_SetFromErrno(PyExc_RuntimeError);
	return NULL;
  }
  Py_DECREF (largs);
  Py_DECREF (lkwlist);
  debugprintf ("%p <- Context_open() = File\n", self->context);
  return (PyObject *)file;
}

static PyObject *
Context_creat(Context *self, PyObject *args)
{
  PyObject *largs, *lkwlist;
  char *uri;
  int mode = 0;
  File *file;
  smbc_creat_fn fn;

  if(!PyArg_ParseTuple (args, "s|i", &uri, &mode)){
      return NULL;
  }

  largs = Py_BuildValue ("()");
  lkwlist = PyDict_New ();
  PyDict_SetItemString (lkwlist, "context", (PyObject *) self);
  file = (File *)smbc_FileType.tp_new(&smbc_FileType, largs, lkwlist);
  if(!file){
	return PyErr_NoMemory();
  }
  if (smbc_FileType.tp_init ((PyObject *)file, largs, lkwlist) < 0){
	smbc_FileType.tp_dealloc((PyObject *)file);
	return NULL;
  }
  fn = smbc_getFunctionCreat(self->context);
  file->file = (*fn)(self->context, uri, mode);
  if(!file->file){
	PyErr_SetFromErrno(PyExc_RuntimeError);
	return NULL;
  }
  Py_DECREF (largs);
  Py_DECREF (lkwlist);
  return (PyObject *)file;
}

static PyObject *
Context_unlink(Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  smbc_unlink_fn fn;

  if(!PyArg_ParseTuple (args, "s", &uri)) {
	return NULL;
  }

  fn = smbc_getFunctionUnlink(self->context);
  ret = (*fn)(self->context, uri);
  return PyInt_FromLong(ret);
}

static PyObject *
Context_rename(Context *self, PyObject *args)
{
  int ret;
  char *ouri = NULL;
  char *nuri = NULL;
  Context *nctx = NULL;
  smbc_rename_fn fn;

  if (!PyArg_ParseTuple(args, "ss|O", &ouri, &nuri, &nctx)) {
	return NULL;
  }

  fn = smbc_getFunctionRename(self->context);
  if(nctx && nctx->context){
	ret = (*fn)(self->context, ouri, nctx->context, nuri);
  }else{
	ret = (*fn)(self->context, ouri, self->context, nuri);
  }
  return PyInt_FromLong(ret);
}

static PyObject *
Context_opendir (Context *self, PyObject *args)
{
  PyObject *largs, *lkwlist;
  PyObject *uri;
  PyObject *dir;

  debugprintf ("%p -> Context_opendir()\n", self->context);
  if (!PyArg_ParseTuple (args, "O", &uri))
    {
      debugprintf ("%p <- Context_opendir() EXCEPTION\n", self->context);
      return NULL;
    }

  largs = Py_BuildValue ("()");
  lkwlist = PyDict_New ();
  PyDict_SetItemString (lkwlist, "context", (PyObject *) self);
  PyDict_SetItemString (lkwlist, "uri", uri);
  dir = PyType_GenericNew (&smbc_DirType, largs, lkwlist);
  if (smbc_DirType.tp_init (dir, largs, lkwlist) < 0)
    {
      smbc_DirType.tp_dealloc (dir);
      debugprintf ("%p <- Context_opendir() EXCEPTION\n", self->context);
      return NULL;
    }

  Py_DECREF (largs);
  Py_DECREF (lkwlist);
  debugprintf ("%p <- Context_opendir() = Dir\n", self->context);
  return dir;
}

static PyObject *
Context_mkdir(Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  unsigned int mode = 0;
  smbc_mkdir_fn fn;

  if(!PyArg_ParseTuple (args, "s|I", &uri, &mode)) {
	return NULL;
  }

  fn = smbc_getFunctionMkdir(self->context);
  ret = (*fn)(self->context, uri, mode);
  return PyInt_FromLong(ret);
}

static PyObject *
Context_rmdir(Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  smbc_rmdir_fn fn;

  if(!PyArg_ParseTuple (args, "s", &uri)) {
	return NULL;
  }

  fn = smbc_getFunctionRmdir(self->context);
  ret = (*fn)(self->context, uri);
  return PyInt_FromLong(ret);
}

static PyObject *
Context_stat(Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  smbc_stat_fn fn;
  struct stat st;

  if(!PyArg_ParseTuple (args, "s", &uri)) {
	return NULL;
  }

  fn = smbc_getFunctionStat(self->context);
  ret = (*fn)(self->context, uri, &st);
  if(ret < 0){
	PyErr_SetString(PyExc_RuntimeError, "No such file or directory");
	return NULL;
  }
  return Py_BuildValue("(IkkkIIkkkk)",
					   st.st_mode,
					   st.st_ino,
					   st.st_dev,
					   st.st_nlink,
					   st.st_uid,
					   st.st_gid,
					   st.st_size,
					   st.st_atime,
					   st.st_mtime,
					   st.st_ctime);
}

static PyObject *
Context_chmod(Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  mode_t mode = 0;
  smbc_chmod_fn fn;

  if(!PyArg_ParseTuple (args, "si", &uri, &mode)) {
	return NULL;
  }

  fn = smbc_getFunctionChmod(self->context);
  ret = (*fn)(self->context, uri, mode);
  return PyInt_FromLong(ret);
}

static PyObject *
Context_getDebug (Context *self, void *closure)
{
  int d = smbc_getDebug (self->context);
  return PyInt_FromLong (d);
}

static int
Context_setDebug (Context *self, PyObject *value, void *closure)
{
  int d;

  if (!PyInt_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be int");
      return -1;
    }

  d = PyInt_AsLong (value);
  smbc_setDebug (self->context, d);
  return 0;
}

static PyObject *
Context_getNetbiosName (Context *self, void *closure)
{
  const char *netbios_name = smbc_getNetbiosName (self->context);
  return PyString_FromString (netbios_name);
}

static int
Context_setNetbiosName (Context *self, PyObject *value, void *closure)
{
  char *name;

  if (!PyString_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be string");
      return -1;
    }

  name = strdup (PyString_AsString (value));
  if (!name)
    {
      return -1;
    }

  smbc_setNetbiosName (self->context, name);
  // Don't free name: the API function just takes a reference(!)
  return 0;
}

static PyObject *
Context_getWorkgroup (Context *self, void *closure)
{
  const char *workgroup = smbc_getWorkgroup (self->context);
  return PyString_FromString (workgroup);
}

static int
Context_setWorkgroup (Context *self, PyObject *value, void *closure)
{
  char *workgroup;

  if (!PyString_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be string");
      return -1;
    }

  workgroup = strdup (PyString_AsString (value));
  if (!workgroup)
    {
      return -1;
    }

  smbc_setWorkgroup (self->context, workgroup);
  // Don't free workgroup: the API function just takes a reference(!)
  return 0;
}

static int
Context_setFunctionAuthData (Context *self, PyObject *value, void *closure)
{
  if (!PyCallable_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be callable object");
      return -1;
    }

  Py_XINCREF (value);
  self->auth_fn = value;
  smbc_setFunctionAuthDataWithContext (self->context, auth_fn);
  return 0;
}

static PyObject *
Context_getOptionDebugToStderr (Context *self, void *closure)
{
  smbc_bool b;
  b = smbc_getOptionDebugToStderr (self->context);
  return PyBool_FromLong ((long) b);
}

static int
Context_setOptionDebugToStderr (Context *self, PyObject *value,
				void *closure)
{
  if (!PyBool_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be Boolean");
      return -1;
    }

  smbc_setOptionDebugToStderr (self->context, value == Py_True);
  return 0;
}

static PyObject *
Context_getOptionNoAutoAnonymousLogin (Context *self, void *closure)
{
  smbc_bool b;
  b = smbc_getOptionNoAutoAnonymousLogin (self->context);
  return PyBool_FromLong ((long) b);
}

static int
Context_setOptionNoAutoAnonymousLogin (Context *self, PyObject *value,
				       void *closure)
{
  if (!PyBool_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be Boolean");
      return -1;
    }

  smbc_setOptionNoAutoAnonymousLogin (self->context, value == Py_True);
  return 0;
}

PyGetSetDef Context_getseters[] =
  {
    { "debug",
      (getter) Context_getDebug,
      (setter) Context_setDebug,
      "Debug level.",
      NULL },

    { "netbiosName",
      (getter) Context_getNetbiosName,
      (setter) Context_setNetbiosName,
      "Netbios name used for making connections.",
      NULL },

    { "workgroup",
      (getter) Context_getWorkgroup,
      (setter) Context_setWorkgroup,
      "Workgroup used for making connections.",
      NULL },

    { "functionAuthData",
      (getter) NULL,
      (setter) Context_setFunctionAuthData,
      "Function for obtaining authentication data.",
      NULL },

    { "optionDebugToStderr",
      (getter) Context_getOptionDebugToStderr,
      (setter) Context_setOptionDebugToStderr,
      "Whether to log to standard error instead of standard output.",
      NULL },

    { "optionNoAutoAnonymousLogin",
      (getter) Context_getOptionNoAutoAnonymousLogin,
      (setter) Context_setOptionNoAutoAnonymousLogin,
      "Whether to automatically select anonymous login.",
      NULL },

    { NULL }
  };

PyMethodDef Context_methods[] =
  {
    { "opendir",
      (PyCFunction) Context_opendir, METH_VARARGS,
      "opendir(uri) -> Dir\n\n"
      "@type uri: string\n"
      "@param uri: URI to opendir\n"
      "@return: a L{smbc.Dir} object for the URI" },

    { "open",
      (PyCFunction) Context_open, METH_VARARGS,
      "open(uri) -> File\n\n"
      "@type uri: string\n"
      "@param uri: URI to open\n"
      "@return: a L{smbc.File} object for the URI" },

    { "creat",
      (PyCFunction) Context_creat, METH_VARARGS,
      "creat(uri) -> File\n\n"
      "@type uri: string\n"
      "@param uri: URI to creat\n"
      "@return: a L{smbc.File} object for the URI" },

    { "unlink",
      (PyCFunction) Context_unlink, METH_VARARGS,
      "unlink(uri) -> int\n\n"
      "@type uri: string\n"
      "@param uri: URI to unlink\n"
      "@return: 0 on success, < 0 on error" },

    { "rename",
      (PyCFunction) Context_rename, METH_VARARGS,
      "rename(ouri, nuri) -> int\n\n"
      "@type ouri: string\n"
      "@param ouri: The original smb uri\n"
      "@type nuri: string\n"
      "@param nuri: The new smb uri\n"
      "@return: 0 on success, < 0 on error" },

    { "mkdir",
      (PyCFunction) Context_mkdir, METH_VARARGS,
      "mkdir(uri, mode) -> int\n\n"
      "@type uri: string\n"
      "@param uri: URI to mkdir\n"
      "@param mode: Specifies the permissions to use.\n"
      "@return: 0 on success, < 0 on error" },

    { "rmdir",
      (PyCFunction) Context_rmdir, METH_VARARGS,
      "rmdir(uri) -> int\n\n"
      "@type uri: string\n"
      "@param uri: URI to rmdir\n"
      "@return: 0 on success, < 0 on error" },

    { "stat",
      (PyCFunction) Context_stat, METH_VARARGS,
      "stat(uri) -> tuple\n\n"
      "@type uri: string\n"
      "@param uri: URI to get stat information\n"
      "@return: stat information" },

    { "chmod",
      (PyCFunction) Context_chmod, METH_VARARGS,
      "chmod(uri, mode) -> int\n\n"
      "@type uri: string\n"
      "@param uri: URI to chmod\n"
      "@type mode: int\n"
      "@param mode: permissions to set\n"
      "@return: 0 on success, < 0 on error" },

    { NULL } /* Sentinel */
  };

PyTypeObject smbc_ContextType =
  {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "smbc.Context",            /*tp_name*/
    sizeof(Context),           /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Context_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "SMBC context\n"
    "============\n\n"

    "  A context for libsmbclient calls."
    "",                        /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Context_methods,           /* tp_methods */
    0,                         /* tp_members */
    Context_getseters,         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Context_init,    /* tp_init */
    0,                         /* tp_alloc */
    Context_new,               /* tp_new */
  };
