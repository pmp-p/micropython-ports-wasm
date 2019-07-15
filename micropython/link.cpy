if __UPY__:
    import embed
    from ujson import dumps, loads
    import ubinascii as binascii
    DBG = 1
else:
    DBG = 0
    import binascii
    from json import dumps, loads

ADBG = 1


class Proxy:
    @classmethod
    def ni(cls, *argv, **kw):
        print(cls, 'N/I', argv, kw)

    get = ni
    act = ni
    set = ni
    tmout = 300
    cfg = {"get": {}, "set": {}, "act": {}}

class obj:
    def __init__(self):
        self.myattr = 666

    def __delattr__(self, name):
        print('delattr ', name)


# the method used is slow : we create new proxies along the path
# for performance reason cases where an attribute of same name is repeated along the rpc path
# should not be handled
#  eg   window.document.something_not_window.document would NOT be addressable.
# window and window.document.something_not_window could be the same CallPath object
# faster but messy and safeguards required.

# TODO: benchmark both.


class CallPath(dict):

    proxy = Proxy
    cache = {}

    def __setup__(self, host, fqn, csp, default=None, tip='[object %s]'):
        self.__fqn = fqn
        self.__name = fqn.rsplit(".", 1)[-1]
        self.__host = host
        self.__solved = None

        # that shoul be an empty call stack pipeline for new object paths.
        self.__csp = csp
        self.__tmout = 500
        self.__tip = tip
        if host:
            self.__host._cp_setup(self.__name, default)
        return self

    if not __UPY__:

        def __init__(self, host, fqn, default=None, tip='[object %s]'):
            self.__setup__(host, fqn, default, tip)
            dict.__init__(self)

    @classmethod
    def set_proxy(cls, proxy):
        cls.proxy = proxy
        proxy.cp = cls
        cls.get = proxy.get
        cls.set = proxy.set
        cls.act = proxy.act

    def __delattr__(self, name):
        print('delattr ', name)

    def __getattr__(self, name):
        if name == 'finalize':
            csp = list(self.__csp)
            self.__csp.clear()
            while len(csp):
                flush = csp.pop(0)
                self.proxy.act(*flush)  # does discard
            asyncio.io.finalize()
            return None

        if name in self:
            return self[name]
        fqn = "%s.%s" % (self.__fqn, name)
        if DBG:
            if len(self.__csp):
                print(f"17x: cp-ast {fqn} with PARENT call(s) pending", *self.__csp)
            else:
                print(f"17x: cp-ast {fqn}")

        if __UPY__:
            newattr = self.__class__().__setup__(self, fqn, self.__csp)
        else:
            newattr = self.__class__(self, fqn, self.__csp)

        self._cp_setup(name, newattr)
        return newattr


    def __setattr__(self, name, value):
        if name.startswith("_"):
            self[name] = value
            return

        # print('setattr',name,value)
        fqn = "%s.%s" % (self.__fqn, name)
        #if DBG: print(f"102: cp->pe {fqn} := {value}",*self.__csp)
        self.finalize

#        if len(self.__csp):
#            self.proxy.set(fqn, value, self.__csp)
#            self.__csp.clear()
#            # can't assign path-end to a dynamic callpath
#            return

        # FIXME: caching type/value is flaky
        # setattr(self.__class__, name, PathEnd(self, fqn, value))
        self.proxy.set(fqn, value)

    # FIXME: caching type/value is flaky
    #    def __setitem__(self, name, v):
    #        fqn = "%s.%s" % (self.__fqn, name)
    #        setattr(self.__class__, key, PathEnd(host, fqn, default=v))

    def _cp_setup(self, key, v):
        if v is None:
            if DBG:
                print(f"126: cp-set {self.__fqn}.{key}, NOREF => left unset")
            return


        elif DBG:
            if len(self.__csp):
                print(f"122: cp-set {self.__fqn}.{key} <= {v} with call(s) PARENT pending", *self.__csp)
            else:
                print(f"122: cp-set {self.__fqn}.{key} <= {v}", *self.__csp)

        if not isinstance(v, self.__class__):
            print(f"130: cp-set {key} := {v} ")
            if self.__class__.set:
                if DBG:
                    print("133:cp-set", key, v)
                self.proxy.set("%s.%s" % (self.__fqn, key), v)

        dict.__setitem__(self, key, v)

    async def __solver(self):
        cs = []
        p = self
        while p.__host:
            if p.__csp:
                cs.extend(p.__csp)
            p = p.__host
        cs.reverse()

        unsolved = self.__fqn
        cid = '?'
        # FIXME: (maybe) for now just limit to only one level of call()
        # horrors like "await window.document.getElementById('test').textContent.length" are long enough already.
        if len(cs):
            solvepath, argv, kw = cs.pop(0)
            cid = self.proxy.act(solvepath, argv, kw)
            if DBG:
                print(self.__fqn, '__solver about to wait_answer(%s)' % cid, solvepath, argv, kw)
            self.proxy.q_req.append(cid)
            self.__solved, unsolved = await self.proxy.wait_answer(cid, unsolved, solvepath)

            # FIXME:         #timeout: raise ? or disconnect event ?
            # FIXME: strip solved part on fqn and continue in callstack
            if not len(unsolved):
                return self.__solved
        else:
            if DBG:
                print('__solver about to get(%s)' % unsolved)

            if __UPY__:
                self.__solved = await self.proxy.get(unsolved, None)
                return self.__solved
            else:
                return await self.proxy.get(unsolved, None)

        return 'future-async-unsolved[%s->%s]' % (cid, unsolved)

    if __UPY__:

        def __iter__(self):
            if ADBG:print("195:cp-(async)iter", self.__fqn,*self.__csp)
            yield from self.__solver()
            try:
                return self.__solved
            finally:
                self.__solved = None

    else:
        pass
    def __await__(self):
        if ADBG:print("205:cp-(async)await", self.__fqn,*self.__csp)
        return self.__solver().__await__()


    def __call__(self, *argv, **kw):
#        if DBG:
#            print("273:cp-call (a/s)?", self.__fqn, argv, kw)
        # stack up the (a)sync call list
        self.__csp.append([self.__fqn, argv, kw])
        return self


    def __enter__(self):
        if DBG:print("280:cp-enter")
        return self

    def __aenter__(self):
        if ADBG:print("216:cp-(async)enter", self.__fqn,*self.__csp)
        return self


    def __exit__(self, type, value, traceback):
        # this is a non-awaitable batch !
        #  need async with !!!!!!

        self.finalize
        # do io claims cleanup
        asyncio.io.finalize()

        print("#FIMXE: __del__ on proxy to release js obj")

    def __aexit__(self, type, value, traceback):
        if ADBG:print("231:cp-(async)exit", self.__fqn,*self.__csp)
        self.__exit(type, value, traceback)


    # maybe yield from https://stackoverflow.com/questions/33409888/how-can-i-await-inside-future-like-objects-await
    #        async def __call__(self, *argv, **kw):
    #
    #            self.__class__.act(self.__fqn, argv, kw)
    #            cid  = str( self.__class__.caller_id )
    #            rv = ":async-cp-call:%s" % cid
    #            while True:
    #                if cid in self.q_return:
    #                    oid = self.q_return.pop(cid)
    #                    if oid in self.cache:
    #                        return self.cache[oid]
    #
    #                    rv = self.__class__(None,oid)
    #                    self.cache[oid]=rv
    #                    break
    #                await asleep_ms(1)
    #            return rv

    def __repr__(self):
        # if self.__fqn=='window' or self.__fqn.count('|'):
        # print("FIXME: give tip about remote object proxy")
        if self.__tip.count('%s'):
            return self.__tip % self.__fqn
        return self.__tip
        # raise Exception("Giving cached value from previous await %s N/I" % self.__fqn)

        # return ":async-pe-get:%s" % self.__fqn


# ======================================================================================


import asyncio
import embed
import ubinascii


class SyncProxy(Proxy):
    def __init__(self):
        self.caller_id = 0
        self.tmout = 800 # ms
        self.cache = {}
        asyncio.io.DBG=1
        self.q_return = asyncio.io.q  # {}
        self.q_req = asyncio.io.req # []

        self.q_reply = []
        self.q_sync = []
        self.q_async = []

    def new_call(self):
        self.caller_id += 1
        return str(self.caller_id)

    def io(self, cid,raw=None, m=None, jscmd=None):
        if jscmd:
            #print("\n  >> io=",cid, jscmd)
            jscmd = binascii.hexlify( jscmd ).decode()
            embed.os_write( '{"dom-%s":{"id":"%s","m":"//%c:%s"}}\n' % (cid, cid,m, jscmd) )
        elif raw:
            #print("\n  >> io=",cid, raw)
            embed.os_write( raw )
        #print()
        return cid

    def unref(self, cid):
        if cid in self.q_req:
            self.q_req.remove(cid)
        else:
            print('308: %s was never awaited'%cid)
        oid = self.q_return.pop(cid)
        tip = "%s@%s" % (oid, cid)
        if isinstance(oid, str) and oid.startswith('js|'):
            if oid.find('/') > 0:
                oid, tip = oid.split('/', 1)
        if not oid in self.cache:
            #create a new pipeline for the ref object
            self.cache[oid] = CallPath().__setup__(None, oid, [], tip=tip)
        return oid, self.cache[oid]




    def set(self, cp, argv, cs=None):
        cid = self.new_call()
        if cs is not None:
            if len(cs) > 1:
                raise Exception('please simplify assign via (await %s(...)).%s = %r' % (cs[0][0], cp, argv))
            solvepath, targv, kw = cs.pop(0)
            unsolved = cp[len(solvepath) + 1 :]

            jsdata = f"JSON.parse(`{dumps(targv)}`)"
            target = solvepath.rsplit('.', 1)[0]
            assign = f'JSON.parse(`{dumps(argv)}`)'
            doit = f"{solvepath}.apply({target},{jsdata}).{unsolved} = {assign}\n"
            #if DBG:
            #    print("74:", doit)

            # TODO: get js exceptions ?
            self.io( cid, raw=doit)

            return

        if cp.count('|'):
            cp = cp.split('.')
            cp[0] = f'window["{cp[0]}"]'
            cp = '.'.join(cp)

        if DBG:
            print('306: set', cp, argv)

        if isinstance(argv,str):
            argv = argv.replace('"', '\\\"')

        # TODO: get js exceptions ?
        jscmd = f'{cp} = JSON.parse(`{dumps(argv)}`)'
        self.io( cid , m='S', jscmd=jscmd)
        #


    async def get(self, cp, argv, **kw):
        cid = self.new_call()
        self.q_req.append(cid)

        # IO
        self.io(cid,raw='{"dom-%s":{"id":"%s","m":"%s"}}\n' % (cid, cid, cp))
        # test

        while True:
            if self.q_return:
                if cid in self.q_return:
                    self.q_req.remove(cid)
                    return self.q_return.pop(cid)
            await asyncio.sleep_ms(1)

    # act will discard results, if results are needed you MUST use await
    def act(self, cp, c_argv, c_kw, **kw):
        # self.q_async.append( {"m": cp, "a": c_argv, "k": c_kw, "id": cid} )
        cid = self.new_call()
        # TODO: get js exceptions ?
        c_argv = dumps(c_argv)
        c_kw = dumps(c_kw)
        raw = '{"dom-%s":{"id":"%s","m":"%s", "a": %s, "k": %s }}\n' % (cid, cid, cp, c_argv, c_kw)
        return self.io( cid , raw=raw) # return cid


    async def wait_answer(self, cid, fqn, solvepath):
        if DBG:
            print('\tas id %s' % cid)
        unsolved = fqn[len(solvepath) + 1 :]
        tmout = self.tmout
        if unsolved:
            if DBG:
                print('\twill remain', unsolved)

        solved = None

        while tmout > 0:
            if cid in self.q_return:
                oid, solved = self.unref(cid)
                if DBG:print("OID=",oid,"solved=",solved)
                if len(unsolved):
                    solved = await self.get("%s.%s" % (oid, unsolved), None)
                    unsolved = ''
                    break
                break
            await asyncio.sleep_ms(1)
            tmout -= 1
        else:
            print(f"TIMEOUT({self.tmout} ms): wait_answer({cid}) for {fqn}")
        return solved, unsolved
#
# https://raw.githubusercontent.com/micropython/micropython-lib/master/socket/socket.py
#
#from usocket import *
#import usocket as _socket
#
#
#_GLOBAL_DEFAULT_TIMEOUT = 30
#IPPROTO_IP = 0
#IP_ADD_MEMBERSHIP = 35
#IP_DROP_MEMBERSHIP = 36
#INADDR_ANY = 0
#
#error = OSError
#
#def _resolve_addr(addr):
#    if isinstance(addr, (bytes, bytearray)):
#        return addr
#    family = _socket.AF_INET
#    if len(addr) != 2:
#        family = _socket.AF_INET6
#    if addr[0] == "":
#        a = "0.0.0.0" if family == _socket.AF_INET else "::"
#    else:
#        a = addr[0]
#    a = getaddrinfo(a, addr[1], family)
#    return a[0][4]
#
#def inet_aton(addr):
#    return inet_pton(AF_INET, addr)
#
#def create_connection(addr, timeout=None, source_address=None):
#    s = socket()
#    #print("Address:", addr)
#    ais = getaddrinfo(addr[0], addr[1])
#    #print("Address infos:", ais)
#    for ai in ais:
#        try:
#            s.connect(ai[4])
#            return s
#        except:
#            pass
#
#
#class socket(_socket.socket):
#
#    def accept(self):
#        s, addr = super().accept()
#        addr = _socket.sockaddr(addr)
#        return (s, (_socket.inet_ntop(addr[0], addr[1]), addr[2]))
#
#    def bind(self, addr):
#        return super().bind(_resolve_addr(addr))
#
#    def connect(self, addr):
#        return super().connect(_resolve_addr(addr))
#
#    def sendall(self, *args):
#        return self.send(*args)
#
#    def sendto(self, data, addr):
#        return super().sendto(data, _resolve_addr(addr))

CallPath.set_proxy(SyncProxy())

window = CallPath().__setup__(None, 'window', [], tip="[ object Window]")
