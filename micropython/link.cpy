__UPY__ = True
# maybe related https://github.com/mavier/jsobject


if __UPY__:
    import embed
    from ujson import dumps,loads
    DBG=1
else:
    DBG=0


class Proxy:
    @classmethod
    def ni(cls,*argv,**kw):print(cls,'N/I',argv,kw)
    get = ni
    act = ni
    set = ni
    tmout = 300
    cfg = {"get": {}, "set": {}, "act": {}}



class JSProxy(Proxy):
    def __init__(self):
        self.caller_id = 0
        self.tmout = 300
        self.cache = {}
        self.q_return = {}
        self.q_reply = []
        self.q_sync = []
        self.q_async = []

    def new_call(self):
        self.caller_id += 1
        return str(self.caller_id)

    async def wait_answer(self,cid, fqn, solvepath):
        #cid = str(self.caller_id)
        if DBG:print('\tas id %s' % cid)
        unsolved = fqn[len(solvepath)+1:]
        tmout = self.tmout
        if DBG and unsolved:
            print('\twill remain', unsolved )
        solved = None

        while tmout>0:
            if cid in self.q_return:
                oid, solved = self.unref(cid)
                if len(unsolved):
                    solved = await self.get( "%s.%s" % ( oid, unsolved ) , None )
                    unsolved = ''
                    break
                break
            await aio.asleep_ms(1)
            tmout -= 1
        return solved, unsolved


    def unref(self, cid):
        oid = self.q_return.pop(cid)
        tip="%s@%s"%( oid,cid )
        if isinstance(oid,str) and oid.startswith('js|'):
            if oid.find('/')>0:
                oid,tip = oid.split('/',1)
        if not oid in self.cache:
           self.cache[oid] = CallPath(None,oid,tip=tip)
        return oid, self.cache[oid]

    async def get(self, cp,argv,**kw):
        print('async_js_get',cp,argv)
        cid = self.new_call()
        self.q_async.append( {"id": cid, 'm':cp } )

        while True:
            if cid in self.q_return:
                return self.q_return.pop(cid)
            await aio.asleep_ms(1)

    def set(self,cp, argv, cs=None):
        if cs is not None:
            if len(cs)>1:
                raise Exception('please simplify assign via (await %s(...)).%s = %r' % (cs[0][0], name,argv))
            value = argv
            solvepath, argv, kw = cs.pop(0)
            unsolved = cp[len(solvepath)+1:]

            jsdata = f"JSON.parse(`{json.dumps(argv)}`)"
            target = solvepath.rsplit('.',1)[0]
            assign = f'JSON.parse(`{json.dumps(value)}`)'
            doit = f"{solvepath}.apply({target},{jsdata}).{unsolved} = {assign}"
            print("74:",doit)
            return self.q_sync.append(doit)

        if cp.count('|'):
            cp  = cp.split('.')
            cp[0] = f'window["{cp[0]}"]'
            cp = '.'.join( cp )

        if DBG:
            print('82: set',cp,argv)
        self.q_sync.append( f'{cp} =  JSON.parse(`{json.dumps(argv)}`)' )

    def act(self, cp,c_argv,c_kw,**kw):
        cid = self.new_call()
        self.q_async.append( {"m": cp, "a": c_argv, "k": c_kw, "id": cid} )
        return cid


class obj:

    def __init__(self):
        self.myattr = 666

    def __delattr__(self, name):
        print('delattr ',name)


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



    def __setup__(self, host, fqn, default=None, tip='[object %s]'):
        self.__fqn = fqn
        self.__name = fqn.rsplit(".", 1)[-1]
        self.__host = host
        self.__solved = None

        #empty call stack
        self.__cs = []
        self.__tmout = 500
        self.__tip  = tip
        if host:
            self.__host._cp_setup(self.__name, default)
        return self

    if not __UPY__:
        def __init__(self, host, fqn, default=None, tip='[object %s]'):
            self.__setup__(host, fqn, default, tip)
            dict.__init__(self)


    @classmethod
    def set_proxy(cls,proxy):
        cls.proxy = proxy
        proxy.cp = cls
        cls.get = proxy.get
        cls.set = proxy.set
        cls.act = proxy.act

    def __delattr__(self, name):
        print('delattr ',name)

    def __getattr__(self, name):
        if name in self:
            return self[name]
        fqn = "%s.%s" % (self.__fqn, name)
        if DBG:
            print("163:cp-ast", fqn)
            if len(self.__cs) and self.__cs[-1]:
                print("163:cp-ast with call pending", fqn , *self.__cs[-1])

        if __UPY__:
            newattr = self.__class__().__setup__(self, fqn)
        else:
            newattr = self.__class__(self, fqn)

        self._cp_setup(name, newattr)
        return newattr

    def __setattr__(self, name, value):
        if name.startswith("_"):
            self[name]=value
            return

        #print('setattr',name,value)
        fqn = "%s.%s" % (self.__fqn, name)
        if DBG:
            print("105:cp->pe", fqn, ":=", value, self.__cs)

        if len(self.__cs):
            self.proxy.set(fqn,value,self.__cs)
            self.__cs.clear()
            #can't assign path-end to a dynamic callpath
            return

#FIXME: caching type/value is flaky
        #setattr(self.__class__, name, PathEnd(self, fqn, value))
        self.proxy.set(fqn, value)

#FIXME: caching type/value is flaky
#    def __setitem__(self, name, v):
#        fqn = "%s.%s" % (self.__fqn, name)
#        setattr(self.__class__, key, PathEnd(host, fqn, default=v))

    def _cp_setup(self, key, v):
        if len(self.__cs) and self.__cs[-1]:
            print("210:cp-set with call pending", self.__fqn , *self.__cs[-1])
        if v is None:
            if DBG:print("212:cp-set None ????", key, v)
            return

        if not isinstance(v, self.__class__):
            print("87:cp-set", key, v)
            if self.__class__.set:
                if DBG:
                    print("93:cp-set", key, v)
                self.proxy.set("%s.%s" % (self.__fqn, key), v)

        dict.__setitem__(self, key, v)


    async def __solver(self):
        cs = []
        p = self
        while p.__host:
            if p.__cs :
                cs.extend(p.__cs)
            p = p.__host
        cs.reverse()

        unsolved = self.__fqn
        cid='?'
        # FIXME: (maybe) for now just limit to only one level of call()
        # horrors like "await window.document.getElementById('test').textContent.length" are long enough already.
        if len(cs):
            solvepath, argv, kw = cs.pop(0)
            cid = self.proxy.act(solvepath, argv, kw )
            if DBG:print(self.__fqn,'__solver about to wait_answer(%s)'%cid, solvepath,argv,kw)
            solved, unsolved = await self.proxy.wait_answer(cid, unsolved, solvepath)
#FIXME:         #timeout: raise ? or disconnect event ?
#FIXME: strip solved part on fqn and continue in callstack
            if not len(unsolved):
                return solved
        else:
            if DBG:print('__solver about to get(%s)' % unsolved)

            if __UPY__:
                self.__solved = await self.proxy.get( unsolved , None )
                return self.__solved
            else:
                return await self.proxy.get( unsolved , None )

        return 'future-async-unsolved[%s->%s]' % (cid,unsolved)

    if __UPY__:
        def __iter__(self):
            yield from self.__solver()
            try:
                return self.__solved
            finally:
                self.__solved = None

    else:
        def __await__(self):
            return self.__solver().__await__()

    def __call__(self, *argv, **kw):
        if DBG:
            print("255:cp-call (a/s)?", self.__fqn, argv, kw)
        #stack up the (a)sync call list
        self.__cs.append( [self.__fqn, argv, kw ] )
        return self


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
        #if self.__fqn=='window' or self.__fqn.count('|'):
        #print("FIXME: give tip about remote object proxy")
        if self.__tip.count('%s'):
            return self.__tip % self.__fqn
        return self.__tip
        #raise Exception("Giving cached value from previous await %s N/I" % self.__fqn)

        #return ":async-pe-get:%s" % self.__fqn

#======================================================================================
"""
-----------SYNC-------------
163:cp-ast window.canvas.getContext.beginPath
163:cp-ast with call pending window.canvas.getContext.beginPath window.canvas.getContext ('2d',) {}
209:cp-set None ???? beginPath None
255:cp-call (a/s)? window.canvas.getContext.beginPath () {}

-----------ASYNC-------------
163:cp-ast window.canvas.getContext.beginPath
163:cp-ast with call pending window.canvas.getContext.beginPath window.canvas.getContext ('2d',) {}
209:cp-set None ???? beginPath None
255:cp-call (a/s)? window.canvas.getContext.beginPath () {}
window.canvas.getContext.beginPath __solver about to wait_answer(1) window.canvas.getContext ('2d',) {}
        as id 1
        will remain beginPath


"""

import asyncio
import embed
import ubinascii
class SyncProxy(Proxy):


    def __init__(self):
        self.caller_id = 0
        self.tmout = 300
        self.cache = {}
        self.q_return = asyncio.io.q # {}
        self.q_reply = []
        self.q_sync = []
        self.q_async = []

    def new_call(self):
        self.caller_id += 1
        return str(self.caller_id)

    def set(self, cp,argv,cs=None):
        if cs is not None:
            if len(cs)>1:
                raise Exception('please simplify assign via (await %s(...)).%s = %r' % (cs[0][0], name,argv))
            value = argv
            solvepath, argv, kw = cs.pop(0)
            unsolved = cp[len(solvepath)+1:]

            jsdata = f"JSON.parse(`{dumps(argv)}`)"
            target = solvepath.rsplit('.',1)[0]
            assign = f'JSON.parse(`{dumps(value)}`)'
            doit = f"{solvepath}.apply({target},{jsdata}).{unsolved} = {assign}"
            if DBG:print("74:",doit)
            return self.q_sync.append(doit)

        if cp.count('|'):
            cp  = cp.split('.')
            cp[0] = f'window["{cp[0]}"]'
            cp = '.'.join( cp )

        if DBG:
            print('82: set',cp,argv)
        argv = argv.replace('"','\\\"')
        self.q_sync.append( f'{cp} = JSON.parse(`{dumps(argv)}`)' )
        cid = self.new_call()

        #IO
        self.io(cid, 'S', CallPath.proxy.q_sync )

    def io(self, cid, m, q):
        while len(q):
            jscmd = ubinascii.hexlify( q.pop(0) ).decode()
            embed.os_write('{"dom-%s":{"id":"%s","m":"//%c:%s"}}' % (cid, cid, m, jscmd) )


    async def get(self, cp,argv,**kw):
        cid = self.new_call()
        print('async_js_get(%s)'%cid,cp,argv)

        #self.q_async.append( {"id": cid, 'm':cp } )

        #IO
        embed.os_write('{"dom-%s":{"id":"%s","m":"%s"}}' % (cid, cid, cp) )
        #test

        while True:
            if self.q_return:
                print("AGET",self.q_return)
                if cid in self.q_return:
                    return self.q_return.pop(cid)
            await asyncio.sleep_ms(1)

    def act(self, cp,c_argv,c_kw,**kw):
        cid = self.new_call()
        self.q_async.append( {"m": cp, "a": c_argv, "k": c_kw, "id": cid} )
        return cid


    async def wait_answer(self,cid, fqn, solvepath):
        #cid = str(self.caller_id)
        if DBG:print('\tas id %s' % cid)
        unsolved = fqn[len(solvepath)+1:]
        tmout = self.tmout
        if DBG and unsolved:
            print('\twill remain', unsolved )
        solved = None

        while tmout>0:
            if cid in self.q_return:
                oid, solved = self.unref(cid)
                if len(unsolved):
                    solved = await self.get( "%s.%s" % ( oid, unsolved ) , None )
                    unsolved = ''
                    break
                break
            await asyncio.sleep_ms(1)
            tmout -= 1
        return solved, unsolved



CallPath.set_proxy(SyncProxy())

window = CallPath().__setup__(None, 'window', tip="[ object Window]")





