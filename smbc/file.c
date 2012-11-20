/* -*- Mode: C; c-file-style: "gnu" -*-
 * pysmbc - Python bindings for libsmbclient
 * Copyright (C) 2002, 2005, 2006, 2007, 2008, 2010, 2011, 2012  Red Hat, Inc
 * Copyright (C) 2010  Open Source Solution Technology Corporation
 * Copyright (C) 2010  Patrick Geltinger <patlkli@patlkli.org>
 * Authors:
 *  Tim Waugh <twaugh@redhat.com>
 *  Tsukasa Hamano <hamano@osstech.co.jp>
 *  Patrick Geltinger <patlkli@patlkli.org>
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
#include "file.h"

//////////
// File //
//////////

/*
  The size of off_t is potentionally unknown, so try to use the
  biggest integer type possible when coverting between ptyhon
  values and off_t.

  In order to communicate offset values between python objects and C
  types use off_t_long as the C type and OFF_T_FORMAT as the format
  character for Py_BuildValue, PyArg_Parse, and similar functions.
*/
#ifdef HAVE_LONG_LONG
typedef PY_LONG_LONG off_t_long;
#define OFF_T_FORMAT "L"
#else
typedef PY_LONG off_t_long;
#define OFF_T_FORMAT "l"
#endif

static PyObject *
File_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  File *self;
  self = (File *) type->tp_alloc (type, 0);
  if (self != NULL)
    self->file = NULL;

  return (PyObject *) self;
}

static int
File_init (File *self, PyObject *args, PyObject *kwds)
{
  PyObject *ctxobj;
  Context *ctx;
  char *uri = NULL;
  int flags = 0;
  int mode = 0;
  smbc_open_fn fn;
  SMBCFILE *file;
  static char *kwlist[] = 
    {
      "context",
      "uri",
      "flags",
      "mode",
      NULL
    };

  if (!PyArg_ParseTupleAndKeywords (args, kwds, "O|sii", kwlist, &ctxobj,
				    &uri, &flags, &mode))
    return -1;

  debugprintf ("-> File_init (%p, \"%s\")\n", ctxobj, uri);
  if (!PyObject_TypeCheck (ctxobj, &smbc_ContextType))
    {
      PyErr_SetString (PyExc_TypeError, "Expected smbc.Context");
      debugprintf ("<- File_init() EXCEPTION\n");
      return -1;
    }

  Py_INCREF (ctxobj);
  ctx = (Context *) ctxobj;
  self->context = ctx;
  if (uri)
    {
      fn = smbc_getFunctionOpen (ctx->context);
      file = (*fn) (ctx->context, uri, (int) flags, (mode_t) mode);
      if (file == NULL)
	{
	  pysmbc_SetFromErrno();
          Py_DECREF (ctxobj);
	  return -1;
	}

      self->file = file;
    }

  debugprintf ("%p open()\n", self->file);
  debugprintf ("%p <- File_init() = 0\n", self->file);
  return 0;
}

static void
File_dealloc (File *self)
{
  Context *ctx = self->context;
  smbc_close_fn fn;
  if (self->file)
    {
      debugprintf ("%p close()\n", self->file);
      fn = smbc_getFunctionClose (ctx->context);
      (*fn) (ctx->context, self->file);
    }

  if (self->context)
    Py_DECREF ((PyObject *) self->context);

  Py_TYPE (self)->tp_free ((PyObject *) self);
}

static PyObject *
File_read (File *self, PyObject *args)
{
  Context *ctx = self->context;
  size_t size = 0;
  smbc_read_fn fn;
  char *buf;
  ssize_t len;
  PyObject *ret;
  smbc_fstat_fn fn_fstat;
  struct stat st;

  if (!PyArg_ParseTuple (args, "|k", &size))
	return NULL;

  fn = smbc_getFunctionRead (ctx->context);

  if (size == 0)
    {
      fn_fstat = smbc_getFunctionFstat (ctx->context);
      (*fn_fstat) (ctx->context, self->file, &st);
      size = st.st_size;
    }

  buf = (char *)malloc (size);
  if (!buf)
    return PyErr_NoMemory ();

  len = (*fn) (ctx->context, self->file, buf, size);
  if (len < 0)
    {
      pysmbc_SetFromErrno ();
      free (buf);
      return NULL;
    }

  ret = PyBytes_FromStringAndSize (buf, len);
  free (buf);
  return ret;
}

static PyObject *
File_write (File *self, PyObject *args)
{
  Context *ctx = self->context;
  int size = 0;
  smbc_write_fn fn;
  char *buf;
  ssize_t len;

  if (!PyArg_ParseTuple (args, "s#", &buf, &size))
    return NULL;

  fn = smbc_getFunctionWrite (ctx->context);
  len = (*fn) (ctx->context, self->file, buf, size);
  if (len < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return PyLong_FromLong (len);
}

static PyObject *
File_fstat (File *self, PyObject *args)
{
  Context *ctx = self->context;
  smbc_fstat_fn fn;
  struct stat st;
  int ret;

  fn = smbc_getFunctionFstat (ctx->context);
  errno = 0;
  ret = (*fn) (ctx->context, self->file, &st);
  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return Py_BuildValue ("(IKKKIIKIII)",
			st.st_mode,
			(unsigned long long)st.st_ino,
			(unsigned long long)st.st_dev,
			(unsigned long long)st.st_nlink,
			st.st_uid,
			st.st_gid,
			st.st_size,
			st.st_atime,
			st.st_mtime,
			st.st_ctime);
}

static PyObject *
File_close (File *self, PyObject *args)
{
  Context *ctx = self->context;
  smbc_close_fn fn;
  int ret = 0;

  fn = smbc_getFunctionClose (ctx->context);
  if (self->file)
    {
      ret = (*fn) (ctx->context, self->file);
      self->file = NULL;
    }

  return PyLong_FromLong (ret);
}

static PyObject *
File_iter (PyObject *self)
{
  Py_INCREF (self);
  return self;
}

static PyObject *
File_iternext (PyObject *self)
{
  File *file = (File *) self;
  Context *ctx = file->context;
  smbc_read_fn fn;
  char buf[2048];
  ssize_t len;
  fn = smbc_getFunctionRead (ctx->context);
  len = (*fn) (ctx->context, file->file, buf, 2048);
  if (len > 0)
    return PyBytes_FromStringAndSize (buf, len);
  else if (len == 0)
    PyErr_SetNone (PyExc_StopIteration);
  else
    pysmbc_SetFromErrno ();

  return NULL;
}

static PyObject *
File_lseek (File *self, PyObject *args)
{
  Context *ctx = self->context;
  smbc_lseek_fn fn;
  off_t_long py_offset;
  off_t offset;
  int whence=0;
  off_t ret;

  if (!PyArg_ParseTuple (args, (OFF_T_FORMAT "|i"), &py_offset, &whence))
    return NULL;

  offset = py_offset;

  /* check for data loss from cast */
  if ((off_t_long)offset != py_offset)
    PyErr_SetString (PyExc_OverflowError, "Data loss in casting off_t");

  fn = smbc_getFunctionLseek (ctx->context);
  ret = (*fn) (ctx->context, self->file, offset, whence);
  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return Py_BuildValue (OFF_T_FORMAT, ret);
}

PyMethodDef File_methods[] =
  {
	{"read", (PyCFunction)File_read, METH_VARARGS,
	 "read(size) -> string\n\n"
	 "@type size: int\n"
	 "@param size: size of reading\n"
	 "@return: read data"
	},
	{"write", (PyCFunction)File_write, METH_VARARGS,
	 "write(buf) -> int\n\n"
	 "@type buf: string\n"
	 "@param buf: write data\n"
	 "@return: size of written"
	 },
	{"fstat", (PyCFunction)File_fstat, METH_NOARGS,
	 "fstat() -> tuple\n\n"
	 "@return: fstat information"
	},
	{"close", (PyCFunction)File_close, METH_NOARGS,
	 "close() -> int\n\n"
	 "@return: on success, < 0 on error"
	},
	{"lseek", (PyCFunction)File_lseek, METH_VARARGS,
	 "lseek(offset, whence=0)\n\n"
	 "@return: on success, current offset location, othwerwise -1"
	},
	{"seek", (PyCFunction)File_lseek, METH_VARARGS,
	 "seek(offset, whence=0)\n\n"
	 "@return: on success, current offset location, othwerwise -1"
	},
    { NULL } /* Sentinel */
  };

#if PY_MAJOR_VERSION >= 3
  PyTypeObject smbc_FileType =
    {
      PyVarObject_HEAD_INIT(NULL, 0)
      "smbc.File",               /*tp_name*/
      sizeof(File),              /*tp_basicsize*/
      0,                         /*tp_itemsize*/
      (destructor)File_dealloc,  /*tp_dealloc*/
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
      "SMBC File\n"
      "=========\n\n"
  
      "  A file object."
      "",                        /* tp_doc */
      0,                         /* tp_traverse */
      0,                         /* tp_clear */
      0,                         /* tp_richcompare */
      0,                         /* tp_weaklistoffset */
      File_iter,                 /* tp_iter */
      File_iternext,             /* tp_iternext */
      File_methods,              /* tp_methods */
      0,                         /* tp_members */
      0,                         /* tp_getset */
      0,                         /* tp_base */
      0,                         /* tp_dict */
      0,                         /* tp_descr_get */
      0,                         /* tp_descr_set */
      0,                         /* tp_dictoffset */
      (initproc)File_init,       /* tp_init */
      0,                         /* tp_alloc */
      File_new,                  /* tp_new */
    };
#else
  PyTypeObject smbc_FileType =
    {
      PyObject_HEAD_INIT(NULL)
      0,                         /*ob_size*/
      "smbc.File",               /*tp_name*/
      sizeof(File),              /*tp_basicsize*/
      0,                         /*tp_itemsize*/
      (destructor)File_dealloc,  /*tp_dealloc*/
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
      "SMBC File\n"
      "=========\n\n"
  
      "  A file object."
      "",                        /* tp_doc */
      0,                         /* tp_traverse */
      0,                         /* tp_clear */
      0,                         /* tp_richcompare */
      0,                         /* tp_weaklistoffset */
      File_iter,                 /* tp_iter */
      File_iternext,             /* tp_iternext */
      File_methods,              /* tp_methods */
      0,                         /* tp_members */
      0,                         /* tp_getset */
      0,                         /* tp_base */
      0,                         /* tp_dict */
      0,                         /* tp_descr_get */
      0,                         /* tp_descr_set */
      0,                         /* tp_dictoffset */
      (initproc)File_init,       /* tp_init */
      0,                         /* tp_alloc */
      File_new,                  /* tp_new */
    };
#endif
