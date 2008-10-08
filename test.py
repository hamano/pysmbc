import errno
import smbc
import gtk, gobject
import pwd, os

class AuthContext:
    def __init__ (self, parent):
        self.passes = 0
        self.has_failed = False
        self.auth_called = False
        self.tried_guest = False
        self.use_user = ''
        self.use_password = ''
        self.use_workgroup = ''
        self.parent = parent

    def perform_authentication (self):
        self.passes += 1
        if self.passes == 1:
            return 1

        if not self.has_failed:
            return 0

        self.has_failed = False
        if self.auth_called and not self.tried_guest:
            self.use_user = 'guest'
            self.use_password = ''
            self.tried_guest = True
            return 1

        if not self.auth_called:
            print "Expected authentication callback"
            raise RuntimeError

        # After that, prompt
        d = gtk.Dialog ("Authentication", self.parent,
                        gtk.DIALOG_MODAL | gtk.DIALOG_NO_SEPARATOR,
                        (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                         gtk.STOCK_OK, gtk.RESPONSE_OK))
        d.set_default_response (gtk.RESPONSE_OK)
        d.set_border_width (6)
        d.set_resizable (False)
        hbox = gtk.HBox (False, 12)
        hbox.set_border_width (6)
        image = gtk.Image ()
        image.set_from_stock ('gtk-dialog-authentication',
                              gtk.ICON_SIZE_DIALOG)
        hbox.pack_start (image, False, False, 0)
        vbox = gtk.VBox (False, 12)
        label = gtk.Label ('<span weight="bold" size="larger">' +
                           "You must log in to access %s." % self.for_server +
                           '</span>')
        label.set_use_markup (True)
        label.set_alignment (0, 0)
        label.set_line_wrap (True)
        vbox.pack_start (label, False, False, 0)

        table = gtk.Table (3, 2)
        table.set_row_spacings (6)
        table.set_col_spacings (6)
        table.attach (gtk.Label ("Username:"), 0, 1, 0, 1, 0, 0)
        username_entry = gtk.Entry ()
        table.attach (username_entry, 1, 2, 0, 1, 0, 0)
        table.attach (gtk.Label ("Domain:"), 0, 1, 1, 2, 0, 0)
        domain_entry = gtk.Entry ()
        table.attach (domain_entry, 1, 2, 1, 2, 0, 0)
        table.attach (gtk.Label ("Password:"), 0, 1, 2, 3, 0, 0)
        password_entry = gtk.Entry ()
        password_entry.set_activates_default (True)
        password_entry.set_visibility (False)
        table.attach (password_entry, 1, 2, 2, 3, 0, 0)
        vbox.pack_start (table, False, False, 0)
        hbox.pack_start (vbox, False, False, 0)
        d.vbox.pack_start (hbox)
        d.show_all ()

        if self.use_user == 'guest':
            self.use_user = pwd.getpwuid (os.getuid ())[0]
        username_entry.set_text (self.use_user)
        domain_entry.set_text (self.use_workgroup)
        response = d.run ()
        d.hide ()

        if response == gtk.RESPONSE_CANCEL:
            return -1

        self.use_user = username_entry.get_text ()
        self.use_password = password_entry.get_text ()
        self.use_workgroup = domain_entry.get_text ()
        return 1

    def initial_authentication (self):
        pass

    def failed (self, exc=None):
        self.has_failed = True
        if exc and not self.auth_called:
            raise exc

    def callback (self, server, share, workgroup, user, password):
        self.auth_called = True
        self.for_server = server
        self.for_share = share
        if self.passes == 1:
            self.initial_authentication ()

        if self.use_user:
            if self.use_workgroup:
                workgroup = self.use_workgroup

            return (workgroup, self.use_user, self.use_password)

        user = ''
        password = ''
        return (workgroup, user, password)

class Browser:
    def __init__ (self):
        w = gtk.Window ()
        w.set_default_size (300, 150)
        sw = gtk.ScrolledWindow ()
        sw.set_shadow_type (gtk.SHADOW_IN)
        sw.set_policy (gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        model = gtk.TreeStore (gobject.TYPE_PYOBJECT)
        tv = gtk.TreeView (model=model)
        w.add (sw)
        sw.add (tv)
        col = gtk.TreeViewColumn ("Name")
        cell = gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.set_cell_data_func (cell, self.cell_name)
        tv.append_column (col)
        col = gtk.TreeViewColumn ("Comment")
        cell = gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.set_cell_data_func (cell, self.cell_comment)
        tv.append_column (col)
        w.connect ('destroy', gtk.main_quit)
        tv.connect ('row-expanded', self.row_expanded)
        self.main = w
        w.show_all ()

        ctx = smbc.Context (debug=1)
        ctx.functionAuthData = self.auth_callback
        ctx.optionNoAutoAnonymousLogin = True
        self.smbc = ctx
        self.auth = AuthContext (w)
        workgroups = None
        try:
            while self.auth.perform_authentication () > 0:
                try:
                    workgroups = self.smbc.opendir ("smb://").getdents ()
                except Exception, e:
                    self.auth.failed (e)
        except RuntimeError, (e, s):
            if e != errno.ENOENT:
                raise

        if workgroups:
            for workgroup in workgroups:
                iter = model.append (None, [workgroup])
                i = model.append (iter)

    def auth_callback (self, server, share,
                       workgroup, user, password):
        return self.auth.callback (server, share, workgroup, user, password)

    def cell_name (self, column, cell, model, iter):
        entry = model.get_value (iter, 0)
        name = ''
        if entry != None:
            name = entry.name
        cell.set_property ('text', name)

    def cell_comment (self, column, cell, model, iter):
        entry = model.get_value (iter, 0)
        comment = ''
        if entry != None:
            comment = entry.comment
        cell.set_property ('text', comment)

    def row_expanded (self, treeview, iter, path):
        model = treeview.get_model ()
        entry = model.get_value (iter, 0)
        if entry == None:
            return

        if len (path) == 1:
            # Workgroup
            try:
                if self.expanding_row:
                    return
            except:
                self.expanding_row = 1

            while model.iter_has_child (iter):
                i = model.iter_nth_child (iter, 0)
                model.remove (i)

            uri = "smb://%s" % entry.name
            self.auth = AuthContext (self.main)
            servers = None
            try:
                while self.auth.perform_authentication () > 0:
                    try:
                        servers = self.smbc.opendir (uri).getdents ()
                    except Exception, e:
                        self.auth.failed (e)
            except RuntimeError, (e, s):
                if e != errno.ENOENT:
                    raise

            if servers:
                for server in servers:
                    i = model.append (iter, [server])
                    n = model.append (i)

            treeview.expand_row (path, 0)
            del self.expanding_row

        elif len (path) == 2:
            # Server
            try:
                if self.expanding_row:
                    return
            except:
                self.expanding_row = 1

            while model.iter_has_child (iter):
                i = model.iter_nth_child (iter, 0)
                model.remove (i)
            uri = "smb://%s" % entry.name

            self.auth = AuthContext (self.main)
            shares = None
            try:
                while self.auth.perform_authentication () > 0:
                    try:
                        shares = self.smbc.opendir (uri).getdents ()
                    except Exception, e:
                        self.auth.failed (e)
            except RuntimeError, (e, s):
                if e != errno.EACCES and e != errno.EPERM:
                    del self.expanding_row
                    raise

            if shares:
                for share in shares:
                    i = model.append (iter, [share])

            treeview.expand_row (path, 0)
            del self.expanding_row

b = Browser ()
gtk.main ()
