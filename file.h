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

#ifndef HAVE_FILE_H
#define HAVE_FILE_H

typedef struct
{
  PyObject_HEAD
  Context *context;
  SMBCFILE *file;
} File;

extern PyMethodDef File_methods[];
extern PyTypeObject smbc_FileType;

#endif /* HAVE_FILE_H */
