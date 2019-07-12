import xpy.builtins
import utime as Time
import asyncio
loop = asyncio.get_event_loop()

print("-"*78)

with uses('xpy.femtoui') as crt:
    set_text = crt.set_text

    async def clock(t=1):
        global Clock, window, ulink
        way=1
        while True:
            clk = '{0:02d}:{1:02d}:{2:02d}  '.format( *Time.localtime()[3:6] )
            set_text( Clock, clk )
            await asyncio.sleep_ms(1000)

    async def scroll_banner(t=1):
        global window, ulink
        way=1
        while True:
            clk = '{0:02d}:{1:02d}:{2:02d}  '.format( *Time.localtime()[3:6] )
            t+=way
            banner = 'µpython did set my title at {0} !'.format(clk)
            ulink.DBG=0
            window.document.title = '|' + '-'*t + banner + '-'* (len(banner)-t) + '|'
            if t==len(banner):
                way=-1
            elif not t:
                way=1
            set_text( Clock, clk )
            await asyncio.sleep(3)

    async def scroll_test(t=1):
        global window, ulink
        way=1
        while True:
            clk = '{0:02d}:{1:02d}:{2:02d}  '.format( *Time.localtime()[3:6] )
            t+=way
            banner = '''my id is "window.test", and µpython wrote my .textContent at {0} !'''.format(clk)
            ulink.DBG=0
            window.test.textContent = '|' + '-'*t + banner + '-'* (len(banner)-t) + '|'
            if t==len(banner):
                way=-1
            elif not t:
                way=1
            set_text( Clock, clk )
            await asyncio.sleep(.2)

    Clock = crt.NodePath(crt.render, crt.Node() , name='Clock' )

    LX = 60
    LY = 20
    crt.set_x(Clock,LX)
    crt.set_z(Clock,LY)


    loop.create_task( clock() )


    loop.create_task( crt.taskMgr() )
del crt


# import ulink is compatible with wasm port only.
import imp
ulink = __import__('ulink')
window = ulink.window



async def __main__():
    await asyncio.sleep(1)

    if 'dev' in sys.argv:
        ulink.DBG=0
        doc = window.document
        print('window.document.title = "{0}"'.format( await window.document.title ) )
        print('window.test.textContent = "{0}"'.format( await window.test.textContent ) )
        return

    if 'exp' in sys.argv:
        # draw clock border
        ulink.DBG=1

        if 0:
            print('----------- SYNC-------------')
            with window.canvas.getContext("2d") as ctx:
                ctx.beginPath()
        else:
            print('-----------ASYNC-------------')
            ctx = await window.canvas.getContext("2d")
            ctx.beginPath()

        print()

        ulink.DBG=0

        return

#ctx.lineWidth = 10
#ctx.arc(width / 2, height / 2, ray, 0, 2 * math.pi)
#ctx.stroke()
#
#for i in range(60):
#    ctx.lineWidth = 1
#    if i % 5 == 0:
#        ctx.lineWidth = 3
#    angle = i * 2 * math.pi / 60 - math.pi / 3
#    x1 = width / 2 + ray * cos(angle)
#    y1 = height / 2 + ray * sin(angle)
#    x2 = width / 2 + ray * cos(angle) * 0.9
#    y2 = height / 2 + ray * sin(angle) * 0.9
#    ctx.beginPath()
#    ctx.moveTo(x1, y1)
#    ctx.lineTo(x2, y2)
#    ctx.stroke()
#
#def show_hours():
#    ctx.beginPath()
#    ctx.arc(width / 2, height / 2, ray * 0.05, 0, 2 * math.pi)
#    ctx.fillStyle = "#000"
#    ctx.fill()
#    for i in range(1, 13):
#        angle = i * math.pi / 6 - math.pi / 2
#        x3 = width / 2 + ray * cos(angle) * 0.75
#        y3 = height / 2 + ray * sin(angle) * 0.75
#        ctx.font = "20px Arial"
#        ctx.textAlign = "center"
#        ctx.textBaseline = "middle"
#        ctx.fillText(i, x3, y3)
#    # cell for day
#    ctx.fillStyle = "#000"
#    ctx.fillRect(width * 0.65, height * 0.47, width * 0.1, height * 0.06)
#
#show_hours()
#set_clock()


    loop.create_task( scroll_banner() )
    loop.create_task( scroll_test() )

loop.create_task(__main__())
asyncio.auto =1
