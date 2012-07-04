/* -*- Mode: C; c-file-style: "gnu" -*-
 * pysmbc - Python bindings for libsmbclient
 * Copyright (C) 2002, 2005, 2006, 2007, 2008, 2011  Tim Waugh <twaugh@redhat.com>
 * Copyright (C) 2010  Patrick Geltinger <patlkli@patlkli.org>
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
#include "smbcmodule.h"
#include "context.h"
#include "dir.h"
#include "smbcdirent.h"

typedef struct
{
  PyObject_HEAD
  Context *context;
  SMBCFILE *dir;
} Dir;

/////////
// Dir //
/////////

static PyObject *
Dir_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Dir *self;
  self = (Dir *) type->tp_alloc (type, 0);
  if (self != NULL)
    self->dir = NULL;

  return (PyObject *) self;
}

static int
Dir_init (Dir *self, PyObject *args, PyObject *kwds)
{
  PyObject *ctxobj;
  Context *ctx;
  const char *uri;
  smbc_opendir_fn fn;
  SMBCFILE *dir;
  static char *kwlist[] = 
    {
      "context",
      "uri",
      NULL
    };

  if (!PyArg_ParseTupleAndKeywords (args, kwds, "Os", kwlist, &ctxobj, &uri))
    return -1;

  debugprintf ("-> Dir_init (%p, \"%s\")\n", ctxobj, uri);
  if (!PyObject_TypeCheck (ctxobj, &smbc_ContextType))
    {
      PyErr_SetString (PyExc_TypeError, "Expected smbc.Context");
      debugprintf ("<- Dir_init() EXCEPTION\n");
      return -1;
    }

  Py_INCREF (ctxobj);
  ctx = (Context *) ctxobj;
  self->context = ctx;
  fn = smbc_getFunctionOpendir (ctx->context);
  errno = 0;
  dir = (*fn) (ctx->context, uri);
  if (dir == NULL) {
	pysmbc_SetFromErrno();
	return -1;
  }
  self->dir = dir;
  debugprintf ("%p <- Dir_init() = 0\n", self->dir);
  return 0;
}

static void
Dir_dealloc (Dir *self)
{
  Context *ctx = self->context;
  smbc_closedir_fn fn;
  if (self->dir)
    {
      debugprintf ("%p closedir()\n", self->dir);
      fn = smbc_getFunctionClosedir (ctx->context);
      (*fn) (ctx->context, self->dir);
    }

  if (self->context)
    {
      Py_DECREF ((PyObject *) self->context);
    }

  Py_TYPE(self)->tp_free ((PyObject *) self);
}

static PyObject *
Dir_getdents (Dir *self)
{
  PyObject *listobj;
  SMBCCTX *ctx;
  char dirbuf[1024];
  smbc_getdents_fn fn;
  int dirlen;

  debugprintf ("-> Dir_getdents()\n");
  ctx = self->context->context;
  listobj = PyList_New (0);
  fn = smbc_getFunctionGetdents (ctx);
  errno = 0;
  while ((dirlen = (*fn) (ctx, self->dir,
			  (struct smbc_dirent *) dirbuf,
			  sizeof (dirbuf))) != 0)
    {
      struct smbc_dirent *dirp;

      debugprintf ("dirlen = %d\n", dirlen);
      if (dirlen < 0)
	{
	  pysmbc_SetFromErrno();
	  Py_DECREF (listobj);
	  debugprintf ("<- Dir_getdents() EXCEPTION\n");
	  return NULL;
	}

      dirp = (struct smbc_dirent *) dirbuf;
      while (dirlen > 0)
	{
	  PyObject *dent;
	  PyObject *largs = Py_BuildValue ("()");
	  PyObject *lkwlist;
	  int len = dirp->dirlen;

	  PyObject *name = PyBytes_FromStringAndSize (dirp->name,
						      strlen (dirp->name));
	  PyObject *comment = PyBytes_FromStringAndSize (dirp->comment,
							 strlen(dirp->comment));
	  PyObject *type = PyLong_FromLong (dirp->smbc_type);
	  lkwlist = PyDict_New ();
	  PyDict_SetItemString (lkwlist, "name", name);
	  PyDict_SetItemString (lkwlist, "comment", comment);
	  PyDict_SetItemString (lkwlist, "smbc_type", type);
	  Py_DECREF (name);
	  Py_DECREF (comment);
	  Py_DECREF (type);
	  dent = smbc_DirentType.tp_new (&smbc_DirentType, largs, lkwlist);
	  smbc_DirentType.tp_init (dent, largs, lkwlist);
	  debugprintf ("%p\n", dent);
	  Py_DECREF (largs);
	  Py_DECREF (lkwlist);

	  PyList_Append (listobj, dent);
	  Py_DECREF (dent);

	  dirp = (struct smbc_dirent *) (((char *) dirp) + len);
	  dirlen -= len;
	}
    }

  debugprintf ("<- Dir_getdents() = list\n");
  return listobj;
}

PyMethodDef Dir_methods[] =
  {
    { "getdents",
      (PyCFunction) Dir_getdents, METH_NOARGS,
      "getdents() -> list\n\n"
      "@return: a list of L{smbc.Dirent} objects" },

    { NULL } /* Sentinel */
  };

#if PY_MAJOR_VERSION >= 3
  PyTypeObject smbc_DirType =
    {
      PyVarObject_HEAD_INIT(NULL, 0)
      "smbc.Dir",                /*tp_name*/
      sizeof(Dir),               /*tp_basicsize*/
      0,                         /*tp_itemsize*/
      (destructor)Dir_dealloc,  /*tp_dealloc*/
      0,                         /*tp_print*/
      0,                         /*tp_getattr*/
      0,                         /*tp_setattr*/
      0,                         /*tp_reserved*/
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
      "SMBC Dir\n"
      "========\n\n"
  
      "  A directory object."
      "",                        /* tp_doc */
      0,                         /* tp_traverse */
      0,                         /* tp_clear */
      0,                         /* tp_richcompare */
      0,                         /* tp_weaklistoffset */
      0,                         /* tp_iter */
      0,                         /* tp_iternext */
      Dir_methods,               /* tp_methods */
      0,                         /* tp_members */
      0,                         /* tp_getset */
      0,                         /* tp_base */
      0,                         /* tp_dict */
      0,                         /* tp_descr_get */
      0,                         /* tp_descr_set */
      0,                         /* tp_dictoffset */
      (initproc)Dir_init,        /* tp_init */
      0,                         /* tp_alloc */
      Dir_new,                   /* tp_new */
    };
#else
  PyTypeObject smbc_DirType =
    {
      PyObject_HEAD_INIT(NULL)
      0,                         /*ob_size*/
      "smbc.Dir",                /*tp_name*/
      sizeof(Dir),               /*tp_basicsize*/
      0,                         /*tp_itemsize*/
      (destructor)Dir_dealloc,  /*tp_dealloc*/
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
      "SMBC Dir\n"
      "========\n\n"
  
      "  A directory object."
      "",                        /* tp_doc */
      0,                         /* tp_traverse */
      0,                         /* tp_clear */
      0,                         /* tp_richcompare */
      0,                         /* tp_weaklistoffset */
      0,                         /* tp_iter */
      0,                         /* tp_iternext */
      Dir_methods,               /* tp_methods */
      0,                         /* tp_members */
      0,                         /* tp_getset */
      0,                         /* tp_base */
      0,                         /* tp_dict */
      0,                         /* tp_descr_get */
      0,                         /* tp_descr_set */
      0,                         /* tp_dictoffset */
      (initproc)Dir_init,        /* tp_init */
      0,                         /* tp_alloc */
      Dir_new,                   /* tp_new */
    };
#endif

