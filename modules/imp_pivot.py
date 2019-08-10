__name__ = imp.pivot_name
__file__ = imp.pivot_file
__dict__ = globals()
print('pivot',__name__,__file__, __dict__)

exec( compile( imp.pivot_code, __file__, 'exec') , __dict__, __dict__)
