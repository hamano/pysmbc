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
#include "smbcdirent.h"

typedef struct
{
  PyObject_HEAD
  unsigned int smbc_type;
  char *comment;
  char *name;
} Dirent;

/////////
// Dir //
/////////

static PyObject *
Dirent_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Dirent *self;
  debugprintf ("-> Dirent_new ()\n");
  self = (Dirent *) type->tp_alloc (type, 0);
  if (self != NULL)
    {
      self->smbc_type = -1;
      self->comment = NULL;
      self->name = NULL;
    }

  debugprintf ("<- Dirent_new ()\n");
  return (PyObject *) self;
}

static int
Dirent_init (Dirent *self, PyObject *args, PyObject *kwds)
{
  const char *name;
  const char *comment;
  unsigned int smbc_type;
  static char *kwlist[] =
    {
      "name",
      "comment",
      "smbc_type",
      NULL
    };

  debugprintf ("%p -> Dirent_init ()\n", self);
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "ssi", kwlist,
				    &name, &comment, &smbc_type))
    {
      debugprintf ("<- Dirent_init() EXCEPTION\n");
      return -1;
    }

  self->name = strdup (name);
  self->comment = strdup (comment);
  self->smbc_type = smbc_type;
  debugprintf ("%p <- Dirent_init()\n", self);
  return 0;
}

static void
Dirent_dealloc (Dirent *self)
{
  free (self->comment);
  free (self->name);
  Py_TYPE(self)->tp_free ((PyObject *) self);
}

static PyObject *
Dirent_repr (PyObject *self)
{
  static const char *types[] =
    {
      "?",
      "Workgroup",
      "Server",
      "File share",
      "Printer share",
      "Comms share",
      "IPC share",
      "Dir",
      "File",
      "Link",
    };

  Dirent *dent = (Dirent *) self;
  char s[1024];
  snprintf (s, sizeof (s),
	    "<smbc.Dirent object \"%s\" (%s) at %p>", dent->name,
	    dent->smbc_type < (sizeof (types) / sizeof *(types)) ?
	    types[dent->smbc_type] : "?",
	    dent);
  return PyBytes_FromStringAndSize (s, strlen (s));
}

static PyObject *
Dirent_getName (Dirent *self, void *closure)
{
  return PyBytes_FromStringAndSize (self->name, strlen (self->name));
}

static PyObject *
Dirent_getComment (Dirent *self, void *closure)
{
  return PyBytes_FromStringAndSize (self->comment, strlen (self->comment));
}

static PyObject *
Dirent_getSmbcType (Dirent *self, void *closure)
{
  return PyLong_FromLong (self->smbc_type);
}

PyGetSetDef Dirent_getseters[] =
  {
    { "name",
      (getter) Dirent_getName, (setter) NULL,
      "name", NULL },

    { "comment",
      (getter) Dirent_getComment, (setter) NULL,
      "comment", NULL },

    { "smbc_type",
      (getter) Dirent_getSmbcType, (setter) NULL,
      "smbc_type", NULL },

    { NULL }
  };

#if PY_MAJOR_VERSION >= 3
  PyTypeObject smbc_DirentType =
    {
      PyVarObject_HEAD_INIT(NULL, 0)
      "smbc.Dirent",             /*tp_name*/
      sizeof(Dirent),            /*tp_basicsize*/
      0,                         /*tp_itemsize*/
      (destructor)Dirent_dealloc,  /*tp_dealloc*/
      0,                         /*tp_print*/
      0,                         /*tp_getattr*/
      0,                         /*tp_setattr*/
      0,                         /*tp_reserved*/
      Dirent_repr,               /*tp_repr*/
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
      "SMBC Dirent\n"
      "=========\n\n"
  
      "  A directory entry object."
      "",                        /* tp_doc */
      0,                         /* tp_traverse */
      0,                         /* tp_clear */
      0,                         /* tp_richcompare */
      0,                         /* tp_weaklistoffset */
      0,                         /* tp_iter */
      0,                         /* tp_iternext */
      0,                         /* tp_methods */
      0,                         /* tp_members */
      Dirent_getseters,          /* tp_getset */
      0,                         /* tp_base */
      0,                         /* tp_dict */
      0,                         /* tp_descr_get */
      0,                         /* tp_descr_set */
      0,                         /* tp_dictoffset */
      (initproc)Dirent_init,     /* tp_init */
      0,                         /* tp_alloc */
      Dirent_new,                /* tp_new */
    };
#else
  PyTypeObject smbc_DirentType =
    {
      PyObject_HEAD_INIT(NULL)
      0,                         /*ob_size*/
      "smbc.Dirent",             /*tp_name*/
      sizeof(Dirent),            /*tp_basicsize*/
      0,                         /*tp_itemsize*/
      (destructor)Dirent_dealloc,  /*tp_dealloc*/
      0,                         /*tp_print*/
      0,                         /*tp_getattr*/
      0,                         /*tp_setattr*/
      0,                         /*tp_compare*/
      Dirent_repr,               /*tp_repr*/
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
      "SMBC Dirent\n"
      "=========\n\n"
  
      "  A directory entry object."
      "",                        /* tp_doc */
      0,                         /* tp_traverse */
      0,                         /* tp_clear */
      0,                         /* tp_richcompare */
      0,                         /* tp_weaklistoffset */
      0,                         /* tp_iter */
      0,                         /* tp_iternext */
      0,                         /* tp_methods */
      0,                         /* tp_members */
      Dirent_getseters,          /* tp_getset */
      0,                         /* tp_base */
      0,                         /* tp_dict */
      0,                         /* tp_descr_get */
      0,                         /* tp_descr_set */
      0,                         /* tp_dictoffset */
      (initproc)Dirent_init,     /* tp_init */
      0,                         /* tp_alloc */
      Dirent_new,                /* tp_new */
    };
#endif

