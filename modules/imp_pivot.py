__name__ = imp.pivot_name
__file__ = imp.pivot_file
print('pivot',globals())

exec( compile( imp.pivot_code, __file__, 'exec') , globals(), globals() )
