import utime as Time
import site

def __getattr__(attr):
    if attr=='sleep':
        return site.sleep
    return getattr(Time,attr)
