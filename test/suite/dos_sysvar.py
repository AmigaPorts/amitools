import pytest


def dos_sysvar_test(vamos):
    """a System() child gets a copy of the parent's local variables"""
    prog_name = vamos.get_prog_bin_name("dos_sysvar")
    vamos.run_prog_checked("dos_sysvar", prog_name)
