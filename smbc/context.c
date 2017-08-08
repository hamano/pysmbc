/* -*- Mode: C; c-file-style: "gnu" -*-
 * pysmbc - Python bindings for libsmbclient
 * Copyright (C) 2002, 2005, 2006, 2007, 2008, 2010, 2011, 2012, 2013  Red Hat, Inc
 * Copyright (C) 2010  Open Source Solution Technology Corporation
 * Copyright (C) 2010  Patrick Geltinger <patlkli@patlkli.org>
 * Authors:
 *  Tim Waugh <twaugh@redhat.com>
 *  Tsukasa Hamano <hamano@osstech.co.jp>
 *  Patrick Geltinger <patlkli@patlkli.org>
 *  Roberto Polli <rpolli@babel.it>
 *  Fabio Isgro' <fisgro@babel.it>
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
      Py_DECREF (result);
      debugprintf ("<- auth_fn(), incorrect callback result\n");
      return;
    }

  strncpy (workgroup, use_workgroup, wgmaxlen - 1);
  workgroup[wgmaxlen - 1] = '\0';
  strncpy (username, use_username, unmaxlen - 1);
  username[unmaxlen - 1] = '\0';
  strncpy (password, use_password, pwmaxlen - 1);
  password[pwmaxlen - 1] = '\0';
  Py_DECREF (result);
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
				    &auth, &debug))
    {
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
	
  debugprintf ("-> Setting  client max protocol to SMB3()\n");
  lp_set_cmdline("client max protocol", "SMB3");

  debugprintf ("-> Context_init ()\n");

  errno = 0;
  ctx = smbc_new_context ();
  if (ctx == NULL)
    {
      PyErr_SetFromErrno (PyExc_RuntimeError);
      debugprintf ("<- Context_init() EXCEPTION\n");
      return -1;
    }

  smbc_setDebug (ctx, debug);

  self->context = ctx;
  smbc_setOptionUserData (ctx, self);
  if (auth)
    smbc_setFunctionAuthDataWithContext (ctx, auth_fn);

  if (smbc_init_context (ctx) == NULL)
    {
      PyErr_SetFromErrno (PyExc_RuntimeError);
      smbc_free_context (ctx, 0);
      debugprintf ("<- Context_init() EXCEPTION\n");
      return -1;
    }

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

  Py_TYPE(self)->tp_free ((PyObject *) self);
}

static PyObject *
Context_set_credentials_with_fallback (Context *self, PyObject *args)
{
  char *workgroup = NULL;
  char *user = NULL;
  char *password = NULL;
  debugprintf ("%p -> Context_set_credentials_with_fallback()\n",
	       self->context);
  if (!PyArg_ParseTuple (args, "sss", &workgroup, &user, &password))
    {
      debugprintf ("%p <- Context_open() EXCEPTION\n", self->context);
      return NULL;
    }

  smbc_set_credentials_with_fallback (self->context,
				      workgroup,
				      user,
				      password);
  debugprintf ("%p <- Context_set_credentials_with_fallback()\n",
	       self->context);
  Py_RETURN_NONE;
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
  if (!PyArg_ParseTuple (args, "s|ii", &uri, &flags, &mode))
    {
      debugprintf ("%p <- Context_open() EXCEPTION\n", self->context);
      return NULL;
    }

  largs = Py_BuildValue ("()");
  lkwlist = PyDict_New ();
  PyDict_SetItemString (lkwlist, "context", (PyObject *) self);
  file = (File *)smbc_FileType.tp_new (&smbc_FileType, largs, lkwlist);
  if (!file)
    {
      return PyErr_NoMemory ();
    }

  if (smbc_FileType.tp_init ((PyObject *)file, largs, lkwlist) < 0)
    {
      smbc_FileType.tp_dealloc ((PyObject *)file);
      debugprintf ("%p <- Context_open() EXCEPTION\n", self->context);
      // already set error
      return NULL;
    }

  fn = smbc_getFunctionOpen (self->context);
  errno = 0;
  file->file = (*fn) (self->context, uri, (int)flags, (mode_t)mode);
  if (!file->file)
    {
      pysmbc_SetFromErrno ();
      smbc_FileType.tp_dealloc ((PyObject *)file);
      file = NULL;
    }

  Py_DECREF (largs);
  Py_DECREF (lkwlist);
  debugprintf ("%p <- Context_open() = File\n", self->context);
  return (PyObject *)file;
}

static PyObject *
Context_creat (Context *self, PyObject *args)
{
  PyObject *largs, *lkwlist;
  char *uri;
  int mode = 0;
  File *file;
  smbc_creat_fn fn;

  if (!PyArg_ParseTuple (args, "s|i", &uri, &mode))
    {
      return NULL;
    }

  largs = Py_BuildValue ("()");
  lkwlist = PyDict_New ();
  PyDict_SetItemString (lkwlist, "context", (PyObject *) self);
  file = (File *)smbc_FileType.tp_new (&smbc_FileType, largs, lkwlist);
  if (!file)
    {
      return PyErr_NoMemory();
    }

  if (smbc_FileType.tp_init ((PyObject *)file, largs, lkwlist) < 0)
    {
      smbc_FileType.tp_dealloc ((PyObject *)file);
      return NULL;
    }

  fn = smbc_getFunctionCreat (self->context);
  errno = 0;
  file->file = (*fn) (self->context, uri, mode);
  if (!file->file)
    {
      pysmbc_SetFromErrno ();
      smbc_FileType.tp_dealloc ((PyObject *)file);
      file = NULL;
    }

  Py_DECREF (largs);
  Py_DECREF (lkwlist);
  return (PyObject *)file;
}

static PyObject *
Context_unlink (Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  smbc_unlink_fn fn;

  if(!PyArg_ParseTuple (args, "s", &uri))
    {
      return NULL;
    }

  fn = smbc_getFunctionUnlink (self->context);
  errno = 0;
  ret = (*fn) (self->context, uri);
  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return PyLong_FromLong (ret);
}

static PyObject *
Context_rename (Context *self, PyObject *args)
{
  int ret;
  char *ouri = NULL;
  char *nuri = NULL;
  Context *nctx = NULL;
  smbc_rename_fn fn;

  if (!PyArg_ParseTuple (args, "ss|O", &ouri, &nuri, &nctx))
    {
      return NULL;
    }

  fn = smbc_getFunctionRename(self->context);
  errno = 0;
  if (nctx && nctx->context)
    {
      ret = (*fn) (self->context, ouri, nctx->context, nuri);
    }
  else
    {
      ret = (*fn) (self->context, ouri, self->context, nuri);
    }

  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return PyLong_FromLong (ret);
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
  dir = smbc_DirType.tp_new (&smbc_DirType, largs, lkwlist);
  if (smbc_DirType.tp_init (dir, largs, lkwlist) < 0)
    {
      smbc_DirType.tp_dealloc (dir);
      debugprintf ("%p <- Context_opendir() EXCEPTION\n", self->context);
      dir = NULL;
    }
	else
	  {
			debugprintf ("%p <- Context_opendir() = Dir\n", self->context);
	  }

  Py_DECREF (largs);
  Py_DECREF (lkwlist);
  return dir;
}

static PyObject *
Context_mkdir (Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  unsigned int mode = 0;
  smbc_mkdir_fn fn;

  if (!PyArg_ParseTuple (args, "s|I", &uri, &mode))
    {
      return NULL;
    }

  fn = smbc_getFunctionMkdir (self->context);
  errno = 0;
  ret = (*fn) (self->context, uri, mode);
  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return PyLong_FromLong (ret);
}

static PyObject *
Context_rmdir (Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  smbc_rmdir_fn fn;

  if (!PyArg_ParseTuple (args, "s", &uri))
    {
      return NULL;
    }

  fn = smbc_getFunctionRmdir (self->context);
  errno = 0;
  ret = (*fn) (self->context, uri);
  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return PyLong_FromLong (ret);
}

static PyObject *
Context_stat (Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  smbc_stat_fn fn;
  struct stat st;

  if (!PyArg_ParseTuple (args, "s", &uri))
    {
      return NULL;
    }

  fn = smbc_getFunctionStat (self->context);
  errno = 0;
  ret = (*fn) (self->context, uri, &st);
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
Context_chmod (Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  mode_t mode = 0;
  smbc_chmod_fn fn;

  if (!PyArg_ParseTuple (args, "si", &uri, &mode))
    {
      return NULL;
    }

  errno = 0;
  fn = smbc_getFunctionChmod (self->context);
  ret = (*fn) (self->context, uri, mode);
  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return PyLong_FromLong (ret);
}


/**
 * Wrapper for the smbc_getxattr() smbclient function. From libsmbclient.h
 * @author fisgro@babel.it, rpolli@babel.it
 *
 * @param uri			The smb url of the file or directory to get extended
 *                  attributes for.
 *
 * @param name      The name of an attribute to be retrieved.  Names are of
 *                  one of the following forms:
 *
 *                     system.nt_sec_desc.<attribute name>
 *                     system.nt_sec_desc.*
 *                     system.nt_sec_desc.*+
 */
static PyObject *
Context_getxattr (Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  char *name = NULL;
  char *buffer = NULL;
  static smbc_getxattr_fn fn;

  // smbc_getxattr takes two string parameters
  if (!PyArg_ParseTuple (args, "ss", &uri, &name))
    {
      return NULL;
    }

  /* The security descriptor string returned by this call will vary depending on the requested attribute
   * A call with system.nt_sec_desc.* will return the longest string which would be in the following format:
   *
   * REVISION:<revision number>,OWNER:<sid>,GROUP:<sid>,ACL:<sid>:<type>/<flags>/<mask>
   *
   * There could be multiple ACL entries up to a reasonable maximum of 1820.
   *
   * <revision number> : 3 chars
   * <sid> :  184 chars
   * <type>:  1 char
   * <flags>: 3 chars
   * <mask>:  10 chars
   *
   * The maximum size of the security descriptor string returned can be
   * derived as follows (includes space for terminating null):
   * Sec Desc = 13 + 2 x (7 + <sid>) + 1820 * (5 + <acl>) = 375315
   *
   * References: https://msdn.microsoft.com/en-us/library/cc246018.aspx
   *             https://technet.microsoft.com/en-us/library/cc961995.aspx
   *             https://technet.microsoft.com/en-us/library/cc961986.aspx
   */

  size_t size = 375315;
  buffer = (char *)malloc (size);
  if(!buffer)
    return PyErr_NoMemory ();

  bzero(buffer, size);

  errno = 0;
  fn = smbc_getFunctionGetxattr(self->context);
  ret = (*fn)(self->context, uri, name, buffer, size);

  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      free(buffer);
      return NULL;
    }

  PyObject *value = PyUnicode_FromString(buffer);
  free(buffer);

  return value;
}


/**
 * Wrapper for the smbc_setxattr() smbclient function. From libsmbclient.h
 * @author fisgro@babel.it, rpolli@babel.it
 *
 * @param uri			The smb url of the file or directory to set extended
 *                  attributes for.
 *
 * @param name      The name of an attribute to be retrieved.  Names are of
 *                  one of the following forms:
 *
 *                     system.nt_sec_desc.<attribute name>
 * 		       system.nt_sec_desc.<attribute name>+
 * 		       system.nt_sec_desc.revision DANGEROUS!!!
 * 		       system.nt_sec_desc.owner
 * 		       system.nt_sec_desc.owner+
 * 		       system.nt_sec_desc.group
 * 		       system.nt_sec_desc.group+
 *                     system.nt_sec_desc.*
 *                     system.nt_sec_desc.*+
 * 		       system.nt_sec_desc.ACL:<type>/<flags>/<mask>
 *
 * @param value     The value to be assigned to the specified attribute name.
 *                  This buffer should contain only the attribute value if the
 *                  name was of the "system.nt_sec_desc.<attribute_name>"
 *                  form.  If the name was of the "system.nt_sec_desc.*" form
 *                  then a complete security descriptor, with name:value pairs
 *                  separated by tabs, commas, or newlines (not spaces!),
 *                  should be provided in this value buffer.  A complete
 *                  security descriptor will contain one or more entries
 *                  selected from the following:
 *
 *                    REVISION:<revision number>
 *                    OWNER:<sid or name>
 *                    GROUP:<sid or name>
 *                    ACL:<sid or name>:<type>/<flags>/<mask>
 *
 *                  The  revision of the ACL specifies the internal Windows NT
 *                  ACL revision for the security descriptor. If not specified
 *                  it defaults to  1.  Using values other than 1 may cause
 *                  strange behaviour.
 *
 *                  The owner and group specify the owner and group sids for
 *                  the object. If the attribute name (either '*+' with a
 *                  complete security descriptor, or individual 'owner+' or
 *                  'group+' attribute names) ended with a plus sign, the
 *                  specified name is resolved to a SID value, using the
 *                  server on which the file or directory resides.  Otherwise,
 *                  the value should be provided in SID-printable format as
 *                  S-1-x-y-z, and is used directly.  The <sid or name>
 *                  associated with the ACL: attribute should be provided
 *                  similarly.
 * @return          0 on success, < 0 on error with errno set:
 *                  - EINVAL  The client library is not properly initialized
 *                            or one of the parameters is not of a correct
 *                            form
 *                  - ENOMEM No memory was available for internal needs
 *                  - EEXIST  If the attribute already exists and the flag
 *                            SMBC_XATTR_FLAG_CREAT was specified
 *                  - ENOATTR If the attribute does not exist and the flag
 *                            SMBC_XATTR_FLAG_REPLACE was specified
 *                  - EPERM   Permission was denied.
 *                  - ENOTSUP The referenced file system does not support
 *                            extended attributes
 *
  */

static PyObject*
Context_setxattr (Context *self, PyObject *args)
{
  int ret;
  char *uri = NULL;
  char *name = NULL;
  char *value = NULL;
  unsigned int flags;
  static smbc_setxattr_fn fn;


  if (!PyArg_ParseTuple (args, "sssi", &uri, &name, &value, &flags))
    {
      return NULL;
    }

  if (!value)
    {
      return NULL;
    }

  errno = 0;
  fn = smbc_getFunctionSetxattr (self->context);

  ret = (*fn)(self->context, uri, name, value, strlen (value), flags);

  if (ret < 0)
    {
      pysmbc_SetFromErrno ();
      return NULL;
    }

  return PyLong_FromLong (ret);
}




static PyObject *
Context_getDebug (Context *self, void *closure)
{
  int d = smbc_getDebug (self->context);
  return PyLong_FromLong (d);
}

static int
Context_setDebug (Context *self, PyObject *value, void *closure)
{
  int d;

#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (value))
    value = PyLong_FromLong (PyInt_AsLong (value));
#endif

  if (!PyLong_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be int");
      return -1;
    }

  d = PyLong_AsLong (value);
  smbc_setDebug (self->context, d);
  return 0;
}

static PyObject *
Context_getNetbiosName (Context *self, void *closure)
{
  const char *netbios_name = smbc_getNetbiosName (self->context);
  return PyUnicode_FromString (netbios_name);
}

static int
Context_setNetbiosName (Context *self, PyObject *value, void *closure)
{
  wchar_t *w_name;
  size_t chars;
  char *name;
  size_t bytes;
  ssize_t written;

#if PY_MAJOR_VERSION < 3
  if (PyString_Check (value))
    value = PyUnicode_FromString (PyString_AsString (value));
#endif

  if (!PyUnicode_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be string");
      return -1;
    }

  chars = PyUnicode_GetSize (value); /* not including NUL */
  w_name = malloc ((chars + 1) * sizeof (wchar_t));
  if (!w_name)
    {
      PyErr_NoMemory ();
      return -1;
    }

#if PY_MAJOR_VERSION < 3
  if (PyUnicode_AsWideChar ((PyUnicodeObject *) value, w_name, chars) == -1)
#else
  if (PyUnicode_AsWideChar (value, w_name, chars) == -1)
#endif
    {
      free (w_name);
      return -1;
    }

  w_name[chars] = L'\0';
  bytes = MB_CUR_MAX * chars + 1; /* extra byte for NUL */
  name = malloc (bytes);
  if (!name)
    {
      free (w_name);
      PyErr_NoMemory ();
      return -1;
    }

  written = wcstombs (name, w_name, bytes);
  free (w_name);

  if (written == -1)
    name[0] = '\0';
  else
    /* NUL-terminate it (this is why we allocated an extra byte) */
    name[written] = '\0';

  smbc_setNetbiosName (self->context, name);
  // Don't free name: the API function just takes a reference(!)
  return 0;
}

static PyObject *
Context_getWorkgroup (Context *self, void *closure)
{
  const char *workgroup = smbc_getWorkgroup (self->context);
  return PyUnicode_FromString (workgroup);
}

static int
Context_setWorkgroup (Context *self, PyObject *value, void *closure)
{
  wchar_t *w_workgroup;
  size_t chars;
  char *workgroup;
  size_t bytes;
  ssize_t written;

#if PY_MAJOR_VERSION < 3
  if (PyString_Check (value))
    value = PyUnicode_FromString (PyString_AsString (value));
#endif

  if (!PyUnicode_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be string");
      return -1;
    }

  chars = PyUnicode_GetSize (value); /* not including NUL */
  w_workgroup = malloc ((chars + 1) * sizeof (wchar_t));
  if (!w_workgroup)
    {
      PyErr_NoMemory ();
      return -1;
    }

#if PY_MAJOR_VERSION < 3
  if (PyUnicode_AsWideChar ((PyUnicodeObject *) value,
			    w_workgroup, chars) == -1)
#else
  if (PyUnicode_AsWideChar (value, w_workgroup, chars) == -1)
#endif
    {
      free (w_workgroup);
      return -1;
    }

  w_workgroup[chars] = L'\0';
  bytes = MB_CUR_MAX * chars + 1; /* extra byte for NUL */
  workgroup = malloc (bytes);
  if (!workgroup)
    {
      free (w_workgroup);
      PyErr_NoMemory ();
      return -1;
    }

  written = wcstombs (workgroup, w_workgroup, bytes);
  free (w_workgroup);

  if (written == -1)
    workgroup[0] = '\0';
  else
    /* NUL-terminate it (this is why we allocated the extra byte) */
    workgroup[written] = '\0';

  smbc_setWorkgroup (self->context, workgroup);
  // Don't free workgroup: the API function just takes a reference(!)
  return 0;
}

static PyObject *
Context_getTimeout (Context *self, void *closure)
{
  int timeout = smbc_getTimeout (self->context);
  return PyLong_FromLong (timeout);
}

static int
Context_setTimeout (Context *self, PyObject *value, void *closure)
{
#if PY_MAJOR_VERSION < 3
  if (!PyInt_Check (value))
#else
  if (!PyLong_Check (value))
#endif
    {
      PyErr_SetString (PyExc_TypeError, "must be long");
      return -1;
    }

#if PY_MAJOR_VERSION < 3
  smbc_setTimeout (self->context, PyInt_AsLong (value));
#else
  smbc_setTimeout (self->context, PyLong_AsLong (value));
#endif
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
Context_getOptionFullTimeNames (Context *self, void *closure)
{
  smbc_bool b;
  b = smbc_getOptionFullTimeNames (self->context);
  return PyBool_FromLong ((long) b);
}

static int
Context_setOptionFullTimeNames (Context *self, PyObject *value,
				void *closure)
{
  if (!PyBool_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be Boolean");
      return -1;
    }

  smbc_setOptionFullTimeNames (self->context, value == Py_True);
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

static PyObject *
Context_getOptionUseKerberos (Context *self, void *closure)
{
  smbc_bool b;
  b = smbc_getOptionUseKerberos (self->context);
  return PyBool_FromLong ((long) b);
}

static int
Context_setOptionUseKerberos (Context *self, PyObject *value,
			      void *closure)
{
  if (!PyBool_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be Boolean");
      return -1;
    }

  smbc_setOptionUseKerberos (self->context, value == Py_True);
  return 0;
}

static PyObject *
Context_getOptionFallbackAfterKerberos (Context *self, void *closure)
{
  smbc_bool b;
  b = smbc_getOptionFallbackAfterKerberos (self->context);
  return PyBool_FromLong ((long) b);
}

static int
Context_setOptionFallbackAfterKerberos (Context *self, PyObject *value,
					void *closure)
{
  if (!PyBool_Check (value))
    {
      PyErr_SetString (PyExc_TypeError, "must be Boolean");
      return -1;
    }

  smbc_setOptionFallbackAfterKerberos (self->context, value == Py_True);
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

    { "timeout",
      (getter) Context_getTimeout,
      (setter) Context_setTimeout,
      "Get the timeout used for waiting on connections and response data(in milliseconds)",
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

		{ "optionFullTimeNames",
      (getter) Context_getOptionFullTimeNames,
      (setter) Context_setOptionFullTimeNames,
      "Use full time names (Create Time)",
      NULL },

    { "optionNoAutoAnonymousLogin",
      (getter) Context_getOptionNoAutoAnonymousLogin,
      (setter) Context_setOptionNoAutoAnonymousLogin,
      "Whether to automatically select anonymous login.",
      NULL },

    { "optionUseKerberos",
      (getter) Context_getOptionUseKerberos,
      (setter) Context_setOptionUseKerberos,
      "Whether to enable use of Kerberos.",
      NULL },

    { "optionFallbackAfterKerberos",
      (getter) Context_getOptionFallbackAfterKerberos,
      (setter) Context_setOptionFallbackAfterKerberos,
      "Whether to fallback after Kerberos.",
      NULL },

    { NULL }
  };

PyMethodDef Context_methods[] =
  {
    { "set_credentials_with_fallback",
      (PyCFunction) Context_set_credentials_with_fallback, METH_VARARGS,
      "set_credentials_with_fallback(workgroup, user, password)\n\n"
      "@type workgroup: string\n"
      "@param workgroup: Workgroup of user\n"
      "@type user: string\n"
      "@param user: Username of user\n"
      "@type password: string\n"
      "@param password: Password of user\n" },

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

	{ "getxattr",
      (PyCFunction) Context_getxattr, METH_VARARGS,
      "getxattr(uri, the_acl) -> int\n\n"
      "@type uri: string\n"
      "@param uri: URI to scan\n"
      "@type name: string\n"
      "@param name: the acl to get with the following syntax\n"
      "\n"
      "                      system.nt_sec_desc.<attribute name>\n"
"                     system.nt_sec_desc.*\n"
"                     system.nt_sec_desc.*+\n"
"                     \n"
"                  where <attribute name> is one of:\n"
"                  \n"
"                     revision\n"
"                     owner\n"
"                     owner+\n"
"                     group\n"
"                     group+\n"
"                     acl:<name or sid>\n"
"                     acl+:<name or sid>\n"
"                     \n"
"                  In the forms \"system.nt_sec_desc.*\" and\n"
"                  \"system.nt_sec_desc.*+\", the asterisk and plus signs are\n"
"                  literal, i.e. the string is provided exactly as shown, and\n"
"                  the value parameter will return a complete security\n"
"                  descriptor with name:value pairs separated by tabs,\n"
"                  commas, or newlines (not spaces!).\n"
"\n"
"                  The plus sign ('+') indicates that SIDs should be mapped\n"
"                  to names.  Without the plus sign, SIDs are not mapped;\n"
"                  rather they are simply converted to a string format.\n"
      "@return: a string representing the actual extended attributes of the uri" },
      
       { "setxattr",
      (PyCFunction) Context_setxattr, METH_VARARGS,
      "setxattr(uri, the_acl) -> int\n\n"
      "@type uri: string\n"
      "@param uri: URI to modify\n"
      "@type name: string\n"
      "@param name: the acl to set with the following syntax\n"
      "\n"
      "                      system.nt_sec_desc.<attribute name>\n"
"                     system.nt_sec_desc.*\n"
"                     system.nt_sec_desc.*+\n"
"                     \n"
"                  where <attribute name> is one of:\n"
"                  \n"
"                     revision\n"
"                     owner\n"
"                     owner+\n"
"                     group\n"
"                     group+\n"
"                     acl:<name or sid>\n"
"                     acl+:<name or sid>\n"
"                     \n"
"                  In the forms \"system.nt_sec_desc.*\" and\n"
"                  \"system.nt_sec_desc.*+\", the asterisk and plus signs are\n"
"                  literal, i.e. the string is provided exactly as shown, and\n"
"                  the value parameter will return a complete security\n"
"                  descriptor with name:value pairs separated by tabs,\n"
"                  commas, or newlines (not spaces!).\n"
"\n"
"                  The plus sign ('+') indicates that SIDs should be mapped\n"
"                  to names.  Without the plus sign, SIDs are not mapped;\n"
"                  rather they are simply converted to a string format.\n"
      "@type	string\n"
      "@param value - a string representing the acl\n"
      "@type	int\n"
      "@param flags - XATTR_FLAG_CREATE or XATTR_FLAG_REPLACE\n"
      "@return: 0 on success" },
    { NULL } /* Sentinel */
  };
#if PY_MAJOR_VERSION >= 3
  PyTypeObject smbc_ContextType =
    {
      PyVarObject_HEAD_INIT(NULL, 0)
      "smbc.Context",            /*tp_name*/
      sizeof(Context),           /*tp_basicsize*/
      0,                         /*tp_itemsize*/
      (destructor)Context_dealloc, /*tp_dealloc*/
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
      "SMBC context\n"
      "============\n\n"

      "  A context for libsmbclient calls.\n\n"
      "Optional parameters are:\n\n"
      "auth_fn: a function for collecting authentication details from\n"
      "the user. This is called whenever authentication details are needed.\n"
      "The parameters it will be given are all strings: server, share,\n"
      "workgroup, username, and password (these last two can be ignored).\n"
      "The function should return a tuple of strings: workgroup, username,\n"
      "and password.\n\n"
      "debug: an integer representing the debug level to use.\n"
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
#else
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

      "  A context for libsmbclient calls.\n\n"
      "Optional parameters are:\n\n"
      "auth_fn: a function for collecting authentication details from\n"
      "the user. This is called whenever authentication details are needed.\n"
      "The parameters it will be given are all strings: server, share,\n"
      "workgroup, username, and password (these last two can be ignored).\n"
      "The function should return a tuple of strings: workgroup, username,\n"
      "and password.\n\n"
      "debug: an integer representing the debug level to use.\n"
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
#endif
