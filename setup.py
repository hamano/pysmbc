from distutils.core import setup, Extension
setup (name="smbc", version="1.0",
       ext_modules=[Extension("smbc",
                              ["smbcmodule.c",
                               "context.c",
                               "dir.c",
                               "file.c",
                               "smbcdirent.c"],
                              libraries=["smbclient"])])
