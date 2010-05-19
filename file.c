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
#include "smbcmodule.h"
#include "context.h"
#include "file.h"

//////////
// File //
//////////

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
  if(uri){
	fn = smbc_getFunctionOpen (ctx->context);
	file = (*fn) (ctx->context, uri, (int) flags, (mode_t) mode);
	if (file == NULL)
	  {
		PyErr_SetFromErrno (PyExc_RuntimeError);
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
    {
      Py_DECREF ((PyObject *) self->context);
    }

  self->ob_type->tp_free ((PyObject *) self);
}

static PyObject *
File_fstat(File *self, PyObject *args)
{
  Context *ctx = self->context;
  smbc_fstat_fn fn;
  struct stat st;
  int ret;

  fn = smbc_getFunctionFstat(ctx->context);
  ret = (*fn)(ctx->context, self->file, &st);
  if(ret < 0){
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
File_read(File *self, PyObject *args)
{
  Context *ctx = self->context;
  size_t size = 0;
  smbc_read_fn fn;
  char *buf;
  ssize_t len;
  PyObject *ret;
  smbc_fstat_fn fn_fstat;
  struct stat st;

  if(!PyArg_ParseTuple(args, "|k", &size)){
	return NULL;
  }
  fn = smbc_getFunctionRead(ctx->context);

  if(size == 0){
	fn_fstat = smbc_getFunctionFstat(ctx->context);
	(*fn_fstat)(ctx->context, self->file, &st);
	size = st.st_size;
  }

  buf = (char *)malloc(size);
  if(!buf){
	return PyErr_NoMemory();
  }
  len = (*fn)(ctx->context, self->file, buf, size);
  ret = PyString_FromStringAndSize(buf, len);
  free(buf);

  return ret;
}

static PyObject *
File_write(File *self, PyObject *args)
{
  Context *ctx = self->context;
  int size = 0;
  smbc_write_fn fn;
  char *buf;
  ssize_t len;

  if(!PyArg_ParseTuple(args, "s#", &buf, &size)){
	return NULL;
  }
  fn = smbc_getFunctionWrite(ctx->context);
  len = (*fn)(ctx->context, self->file, buf, size);

  return PyInt_FromLong(len);
}

static PyObject *
File_close(File *self, PyObject *args)
{
  Context *ctx = self->context;
  smbc_close_fn fn;
  int ret = 0;

  fn = smbc_getFunctionClose(ctx->context);
  if(self->file){
	ret = (*fn)(ctx->context, self->file);
	self->file = NULL;
  }
  return PyInt_FromLong(ret);
}

PyMethodDef File_methods[] =
  {
	{"fstat", (PyCFunction)File_fstat, METH_NOARGS, NULL},
	{"read", (PyCFunction)File_read, METH_VARARGS, NULL},
	{"write", (PyCFunction)File_write, METH_VARARGS, NULL},
	{"close", (PyCFunction)File_close, METH_NOARGS, NULL},
    { NULL } /* Sentinel */
  };

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

    "  A directory object."
    "",                        /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
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
