print("\nHello Python from module")

try:
    import utime
    print(utime)
except Exception as e:
    print("utime failed",e)
    utime = None

print("utime",utime)

try:
    import pystone
    print(pystone)

except Exception as e:
    print("pystone failed",e)
    pystone = None

if pystone:
    print("starting pystone")
    try:
        pystone.main()
    except Exception as e:
        print("pystone test failed",utime.time_ns())

