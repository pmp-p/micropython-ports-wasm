"use strict";

// will hold embed I/O , stdin and stdout buffers.

window.plink = {}


function undef(e,o){
    if (typeof o === 'undefined' || o === null)
        o = window;
    //else console.log('domain '+o)

    try {
        e = o[e];
    } catch (x) { return true }

    if (typeof e === 'undefined' || e === null)
        return true;
    return false;
}

function defined(e,o){return !undef(e,o)}

const delay = (ms, fn_solver) => new Promise(resolve => setTimeout(() => resolve(fn_solver()), ms*1000));

String.prototype.rsplit = function(sep, maxsplit) {
    var split = this.split(sep);
    return maxsplit ? [ split.slice(0, -maxsplit).join(sep) ].concat(split.slice(-maxsplit)) : split;
}

/*
if(typeof(String.prototype.trim) === "undefined")
{
    String.prototype.endswith = String.prototype.endsWith
    String.prototype.startswith = String.prototype.startsWith
    String.prototype.trim = function()
    {
        return String(this).replace(/^\s+|\s+$/g, '');
    };
}
*/

function trim(str) {
    var str = str.replace(/^\s\s*/, ''),
        ws = /\s/,
        i = str.length;
    while (ws.test(str.charAt(--i)));
    return str.slice(0, i + 1);
}


function register(fn,fn_dn){
    if ( undef(fn_dn) )
        fn_dn = fn.name;
    //console.log('  |-- added ' + fn_dn );
    window[fn_dn]=fn;
}

//cyclic dep
window.register = register
register(undef)
//register(module_load)

function setdefault(n,v,o){
    if (o == null)
        o = window;

    if (undef(n,o)){
        o[n]=v;
        console.log('  |-- ['+n+'] set to ['+ o[n]+']' );
        return true;
    }
    return false
}

setdefault('JSDIR','')


function include(filename, filetype){
    if (filetype===null ||typeof filetype === 'undefined')
        filetype = 'js';
        if (filename.endsWith('css'))
            filetype = 'css';

    if ( (filename.indexOf('.') === 0) || (filename.indexOf('/') === 0 ) ){
        //absolute !
    } else {
        //corrected
        filename = window.JSDIR + filename;
    }

    if (filetype=="js"){ //if filename is a external JavaScript file
        var fileref=document.createElement('script')
        fileref.setAttribute("type","text/javascript")
        fileref.setAttribute("src", filename)
        fileref.setAttribute('async',false);
    }
    else if (filetype=="css"){ //if filename is an external CSS file
        var fileref=document.createElement("link")
        fileref.setAttribute("rel", "stylesheet")
        fileref.setAttribute("type", "text/css")
        fileref.setAttribute("href", filename)
    }   else {
        console.log("#error can't include "+filename+' as ' +filetype);
        return false;
    }
    // .py includes ??

    if (typeof fileref!="undefined")
        console.log("#included "+filename+' as ' +filetype);
        document.getElementsByTagName("head")[0].appendChild(fileref)
        fileref.async = false;
        fileref.defer = false;
}
register(include)



function _until(fn_solver){
    return async function fwrapper(){
        var argv = Array.from(arguments)
        function solve_me(){return  fn_solver.apply(window, argv ) }
        while (!await delay(0, solve_me ) )
            {};
    }
}
register(_until)



// ============================== FILE I/O (sync => bad) =================================
include("asmjs_file_api.js")


// ================== uplink ===============================================

// separated file link.cpy => clink for cpython , ulink for micropython
// entry point is embed_call(struct_from_json)


// ================= web->sockets ===============================

include("asmjs_socket_api.js")

// =================  EMSCRIPTEN ================================



function preRun(){
    console.log("preRun: Begin")
    //FS.init()
    var argv = window.location.href.split('?',2)
    var e;
    while (e=argv.shift())
        Module.arguments.push(e)
    argv = Module.arguments.pop().split('&')
    while (e=argv.shift())
        Module.arguments.push(e)

    if ( defined('PYTHONSTARTUP') )
        PYTHONSTARTUP()
    else
        console.log("preRun: function PYTHONSTARTUP() ? " )

    console.log("preRun: End")
}


function write_file(dirname, filename, arraybuffer) {
    FS.createPath('/',dirname,true,true);
    FS.createDataFile(dirname,filename, arraybuffer, true, true);
}

function postRun() {
    console.log("postRun: Begin")
    setTimeout(init_repl_begin, 1600)
    console.log("postRun: -> init_repl_begin")
    console.log("postRun: End")
}

async function init_repl_begin(){

    console.log("init_repl: Begin (" + Module.arguments.length+")")
    var scripts = document.getElementsByTagName('script')

    window.pyscripts = new Array()

    window.plink.shm =  Module._shm_ptr()


    // get repl max buffer size but don't start it yet
    window.PyRun_SimpleString_MAXSIZE = Module._repl_run(1)

    console.log("init_repl: shm "+window.plink.shm+"["+PyRun_SimpleString_MAXSIZE +"]")

    if (Module.arguments.length>1) {

        var argv0 = ""+Module.arguments[1]
        if (argv0.startsWith('http'))
            if (window.urls.cors)
                argv0 = window.urls.cors(argv0)
        else console.log("CORS PATCH OFF")

        console.log('running with sys.argv', argv0)

        window.currentTransferSize = 0
        window.currentTransfer = argv0

        var ab = awfull_get(argv0)
        if (window.currentTransferSize>=0) {
            console.log(ab.length)
            FS.createDataFile("/",'main.py', ab, true, true);
            PyRun_VerySimpleFile('main.py')
            doscripts = false
        } else {
            console.log("error getting main.py from '"+argv0+"'")
            term_impl("Javascript : error getting main.py from '"+argv0+"'\r\n")
            //TODO: global control var to skip page scripts
        }

    }

    for(var i = 0; i < scripts.length; i++){
        var script = scripts[i]

        if(script.type == "text/µpython"){
            pyscripts.push(script.text)

        }
    }

    // run scripts or start repl asap
    PyRun_SimpleString()
}


function init_repl_end() {

    console.log("shared memory ptr : " + window.plink.shm )

    // go for banner and prompt
    if (Module._repl_run(0)) {
        // feed repl, roughly 1000/60 ( usual screen sync )
        setInterval( stdin_poll , 16)
    }

    console.log("init_repl: End")
    window.init_repl_end = null

}

// =========================== REPL shm interface ===============================

window.BAD_CORE = 1

// TODO: ring buffer.

function prepro(text) {
    if (BAD_CORE) {
    // hack until import => __import__ is fixed for micropython core
    // beh text =text.replace(/import (.+?\w+);/,'imp.ort("$1");')
    // beh import mod1,mod2,mod
    // beh import mod as ule etc ..........
    // just fix that import please
        text =text.replace(/import (.+?\w+)\n/,'imp.ort("$1",host=__import__(__name__))\n')
    }
    return text
}


// FIXME: ordering of scripts blocks => use a queue and do it async
function PyRun_SimpleString(text){
    if ( getValue( plink.shm, 'i8') ) {
        console.log("shm still locked, retrying in 16 ms")
        setTimeout(PyRun_SimpleString, 16, text )
        return //
    }

    if (!text)
        text = pyscripts.shift()

    if ( text.startsWith("#!") ) {
        text = trim(text.replace("\n","").substr(2))
        if ( text.endsWith('.py') ) {
            console.log("Getting shebang py=["+text+"]")
            text = awfull_get(text)
        }
    }

    if ( text ) {
        text= prepro(text)
        if (text.length >= PyRun_SimpleString_MAXSIZE)
            console.log("ERROR: python code ring buffer overrun")

        text = prepro(text)
        stringToUTF8( text, plink.shm, PyRun_SimpleString_MAXSIZE )
        console.log("wrote "+text.length+"B to shm")
    } else
        console.log("invalid text block")

    if (pyscripts.length)
        return setTimeout(PyRun_SimpleString, 16 )

    if (init_repl_end)
        init_repl_end()

}

// not sure about ALLOC normal or stack
function PyRun_VerySimpleFile(text){
    var cs = allocate(intArrayFromString(text), 'i8', ALLOC_STACK);
    Module._PyRun_VerySimpleFile(cs)
    Module._free(cs)
}




// ================= STDIN =================================================
var pts = {}
   pts.i = {}
   pts.i.line = ""
   pts.i.raw = true

   pts.o = {}


plink.pts = pts
window.stdin_array = []
window.stdin = ""
window.stdin_raw = true

function window_prompt(){
    if (window.stdin.length>0) {
        var string = window.stdin
        window.stdin = ""
        console.log("sent ["+string+"]")
        return string
    }
    return null
}

function stdin_tx(key){
    window.stdin += key
}

function stdin_poll(){
    //pending draw ?
    if (stdout_blit)
        flush_stdout();

    //pending io/rpc ?
    if (plink.io)
        plink.io()

    if (!window.stdin_raw)
        return

    if (!window.stdin.length)
        return

    var utf8 = unescape(encodeURIComponent(window.stdin));
    for(var i = 0; i < utf8.length; i++) {
        window.stdin_array.push( utf8.charCodeAt(i) );
    }
    window.stdin = ""
}

window.stdin_tx =stdin_tx


// ================ STDOUT =================================================

// TODO: add a dupterm for stderr, and display error in color in xterm if not in stdin_raw mode


window.stdout_blit = false
window.stdout_array = []


function flush_stdout_utf8(){
    var uint8array = new Uint8Array(window.stdout_array)
    var string = new TextDecoder().decode( uint8array )
    term_impl(string)
    window.stdout_array=[]
    stdout_blit = false
}


function stdout_process1_utf8(cc) {
    window.stdout_array.push(cc)

    if (window.stdin_raw) {
        if (cc<128)
            stdout_blit = true
        return
    }

    if (cc==10) {
        stdout_blit = true
        return
    }
    //no blit on non raw mode until crlf
}

// TODO: find a javascript guru to optim array processing
function stdout_process_utf8(cc) {
    if (  Array.isArray(cc) ) {
        while (cc.length>0)
            stdout_process1_utf8(cc.shift())
    } else
        stdout_process1_utf8(cc)
}

window.stdout_process = stdout_process_utf8
window.flush_stdout = flush_stdout_utf8



// this is a demultiplexer for stdout and os (DOM/js ...) control
function pts_decode(text){

    try {
        var jsdata = JSON.parse(text);
        for (key in jsdata) {
            // TODO: multiple fds for pty.js
            if (key=="1") {
                stdout_process( jsdata[key] )
                continue
            }
            embed_call(jsdata[key])
        }


    } catch (x) {
        // found a raw C string via libc
        console.log("C-OUT ["+text+"]")
        flush_stdout()
        try {
            posix.syslog(text)
        } catch (y) {
            term_impl(text+"\r\n")
        }

    }
}

// Ctrl+L is mandatory ! need xterm.js 3.14+
function xterm_helper(term, key) {
    function ESC(data) {
        return String.fromCharCode(27)+data
    }
    if ( key.charCodeAt(0)==12 ) {
        var cy = 0+term.buffer.cursorY
        if ( cy > 0) {
            if (cy <= term.rows) {
                term.write( ESC("[B") )
                term.write( ESC("[J") )
                term.write( ESC("[A") )
            }

            term.write( ESC("[A") )
            term.write( ESC("[K") )
            term.write( ESC("[1J"))

            for (var i=1;i<cy;i++) {
                term.write( ESC("[A") )
                term.write( ESC("[M") )
            }
            term.write( ESC("[M") )
        }
        return false
    }
    return true
}
// ========================== startup hooks ======================================

window.Module = {
    preRun : [preRun],
    postRun: [postRun],
    print : pts_decode,
    printErr : console.log,
}


async function pythons(argc, argv){
    var scripts = document.getElementsByTagName('script')

    for(var i = 0; i < scripts.length; i++){
        var script = scripts[i]
/*
        console.log("l="+ script.language +" src="+script.src+ " len="+ script.text.length+ " async="+script.async)
        if (script.text.length)
            console.log(script.text)
*/
        if(script.type == "text/µpython"){

            var emterpretURL = "micropython.binary"
            var emterpretXHR = new XMLHttpRequest;
                emterpretXHR.open("GET", emterpretURL, !0),
                emterpretXHR.responseType = "arraybuffer",
                emterpretXHR.onload = function() {
                    if (200 === emterpretXHR.status || 0 === emterpretXHR.status) {
                        Module.emterpreterFile = emterpretXHR.response
                        console.log("Using µpython VM via emterpreter (async)")
                    } else {
                        console.log("Using µpython VM synchronously because no micropython.binary => " + emterpretXHR.status )
                    }
                    include("micropython.js")
                    //include("loader.js")
                }
                emterpretXHR.send(null)
            break
        }
    }
}


if ( undef("CORS_BROKER") ){
    window.CORS_BROKER = "https://cors-anywhere.herokuapp.com/"
    console.log("using default brooker CORS_BROKER="+CORS_BROKER)
}




console.log('pythons included')


