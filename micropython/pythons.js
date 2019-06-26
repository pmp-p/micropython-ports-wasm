"use strict";


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

String.prototype.endswith = String.prototype.endsWith
String.prototype.startswith = String.prototype.startsWith


function setdefault(n,v,o){
    if (o == null)
        o = window;

    if (undef(n,o)){
        o[n]=v;
        console.log('  |-- ['+n+'] set to ['+ o[n]+']' );
        return true;
    }
    return false;
}

setdefault('JSDIR','');

function include(filename, filetype){
    if (filetype===null ||typeof filetype === 'undefined')
        filetype = 'js';
        if (filename.endswith('css'))
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
    if (typeof fileref!="undefined")
        console.log("#included "+filename+' as ' +filetype);

        document.getElementsByTagName("head")[0].appendChild(fileref)
        fileref.async = false;
        fileref.defer = false;
        //fileref.src = window.URL.createObjectURL( window.EMScript );
        //document.body.appendChild(fileref);
}
window.include = include;

function _until(fn_solver){
    return async function fwrapper(){
        var argv = Array.from(arguments)
        function solve_me(){return  fn_solver.apply(window, argv ) }
        while (!await delay(0, solve_me ) )
            {};
    }
}



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
    console.log("preRun: End")
}


function write_file(dirname, filename, arraybuffer) {
    FS.createPath('/',dirname,true,true);
    FS.createDataFile(dirname,filename, arraybuffer, true, true);
}


//async
function postRun(){
    console.log("postRun: Begin")
    setTimeout(init_loop,500)
    console.log("postRun: End")
}

function PyRun_VerySimpleFile(text){
    var cs = allocate(intArrayFromString(text), 'i8', ALLOC_STACK);
    //console.log(script.text)
    Module._PyRun_VerySimpleFile(cs)
    Module._free(cs)
}

function PyRun_SimpleString(text){
    var cs = allocate(intArrayFromString(text), 'i8', ALLOC_STACK);
    //console.log(script.text)
    Module._PyRun_SimpleString(cs)
    try {
        Module._free(cs)
    } catch (x) {
            console.log('_free(): do not forget to turn off debugging')
    }
}



function init_loop(){

    console.log("init_loop:Begin (" + Module.arguments.length+")")
    var scripts = document.getElementsByTagName('script')

    var doscripts = true
    if (Module.arguments.length>1) {

        var argv0 = ""+Module.arguments[1]
        if (1)
            if (argv0.startswith('http')) {
                argv0 = CORS_BROKER+argv0
            }
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

    if (doscripts) {

        for(var i = 0; i < scripts.length; i++){
            var script = scripts[i]
            if(script.type == "text/µpython"){
                PyRun_SimpleString(script.text)
            }
        }
    }
    setTimeout(Module._repl_init, 1);
    console.log("init_loop:End")


}

// ============================== FILE I/O (sync => bad) =================================


function awfull_get(url) {
    function updateProgress (oEvent) {
      if (oEvent.lengthComputable) {
        var percentComplete = oEvent.loaded / oEvent.total;
      } else {
            // Unable to compute progress information since the total size is unknown
      }
    }

    function transferFailed(evt) {
      console.log("callfs: An error occurred while transferring the file '"+window.currentTransfer+"'");
    }

    function transferCanceled(evt) {
      console.log("callfs: transfer '"+window.currentTransfer+"' has been canceled by the user.");
    }

    var oReq = new XMLHttpRequest();

    function transferComplete(evt) {
        if (oReq.status==404){
            console.log("callfs: File not found : "+ url );
            window.currentTransferSize = -1 ;

        } else {
            window.currentTransferSize = oReq.response.length;
            console.log("callfs: Transfer is complete saving : "+window.currentTransferSize);
        }
    }

    oReq.overrideMimeType("text/plain; charset=x-user-defined");
    oReq.addEventListener("progress", updateProgress);
    oReq.addEventListener("load", transferComplete);
    oReq.addEventListener("error", transferFailed);
    oReq.addEventListener("abort", transferCanceled);
    oReq.open("GET",url ,false);
    oReq.send();
    return oReq.response
}

function hack_open(url, cachefile){
    try {
        if (url[0]==":")
            url = url.substr(1)
        else {
            if (url.includes('/wyz.fr/'))
                url = CORS_BROKER + url
        }

        var ab = awfull_get(url)
        var ret = ab.length

        window.urls.index += 1
        if (!cachefile){
            cachefile = "cache_"+window.urls.index
            ret = window.urls.index
        }
        FS.createDataFile("/", cachefile, ab, true, true);
        return ret
    } catch (x) {
        console.log("hack_open :"+x)
        return 0
    }
}


function file_exists(urlToFile, need_dot) {
    if (need_dot) {
        need_dot = urlToFile.split('.').pop()
        if (need_dot==urlToFile) {
            //console.log("file_exists not-a-file :"+urlToFile)
            return 0
        }
        //console.log("file_exists ? :"+urlToFile)
    }

    var xhr = new XMLHttpRequest()
    xhr.open('HEAD', urlToFile, false)
    xhr.send()
    var ret=0
    if (xhr.status == 200 )
        ret=1
    //console.log(ret)
    return ret
}

// ================== uplink ===============================================

// separate file clink for cpython , plink for micropython , entry point is embed_call(struct_from_json)

// ================= STDIN =================================================
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

if (0) { // SLOW
    function stdin_tx(key){
        window.stdin += key

        if (!window.stdin_raw) {
            console.log("key:"+key);
            return ;
        }
        var utf8 = unescape(encodeURIComponent(key));
        for(var i = 0; i < utf8.length; i++) {
            window.stdin_array.push( utf8.charCodeAt(i) );
        }
    }
    window.stdin_tx =stdin_tx

} else {
    function stdin_tx(key){
        window.stdin += key
    }

    function stdin_poll(){
        //pending draw ?
        if (stdout_blit)
            flush_stdout();

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
    setInterval( stdin_poll , 16)
    window.stdin_tx =stdin_tx
}





function stdin_tx_chr(chr){
    console.log("stdin:control charkey:"+chr);
    window.stdin_array.push( chr );
}

// ================ STDOUT =================================================

// TODO: add a dupterm for stderr, and display error in color in xterm if not in stdin_raw mode


window.stdout_blit = false
window.stdout_array = []


if (1) {
    function flush_stdout_utf8(){
        var uint8array = new Uint8Array(window.stdout_array)
        var string = new TextDecoder().decode( uint8array )
        term_impl(string)
        window.stdout_array=[]
        stdout_blit = false
    }

    function stdout_process_utf8(cc) {
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

    window.stdout_process = stdout_process_utf8
    window.flush_stdout = flush_stdout_utf8

}
/*
 else {

    function stdout_process_ascii(cc) {
        window.stdout_array.push( String.fromCharCode(cc) )
        stdout_blit = true
    }

    function flush_stdout_ascii(){
        term_impl(window.stdout_array.join("") )
        window.stdout_array=[]
        stdout_blit = false
    }

    window.stdout_process = stdout_process_ascii
    window.flush_stdout = flush_stdout_ascii
}
*/

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
            if (key=="id") {
                embed_call(jsdata)
                continue
            }
            console.log("muliplexer noise : "+ key+ " = " + jsdata[key] )
        }

    } catch (x) {
        // found a raw C string via libc
        console.log("C-OUT ["+text+"]")
        flush_stdout()
        term_impl(text+"\r\n")
    }
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
    /*
    for ?0=xxxxx&1=xxxx argv style
    var argv = []
    for (var i=0;i<10;i++) {
        var arg = new URL(window.location.href).searchParams.get(i);
        if (arg) {
            console.log("argv["+i+"]=",arg)
            argv.push(arg)
        }
        else break
    }
    if (argv.length>0) {
        console.log('running with sys.argv',argv)
        //Module.arguments = argv
        include("micropython.js")
        return;
    }
    */
    for(var i = 0; i < scripts.length; i++){
        var script = scripts[i]
        if(script.type == "text/µpython"){
            console.log("starting upython")
            include("micropython.js")
            break
        }
    }
}


if ( undef("CORS_BROKER") ){
    window.CORS_BROKER = "https://cors-anywhere.herokuapp.com/"
    console.log("using default brooker CORS_BROKER="+CORS_BROKER)
}



// ========================== C =============================
window.lib = {"name":"lib"};
window.urls = {"name":"http","index":-1}

async function _get(url,trigger){
    fetch(url).then( function(r) { return r.arrayBuffer(); } ).then( function(udata) { window[trigger] = udata } );
    await _until(defined)(trigger,window.urls)
    return window.urls[trigger]
}

async function dlopen_lzma(lib,size_hint) {
    if ( file_exists("lib/lib"+lib +".js") ){
        console.log(" =========== CAN RAW EVAL ========== ")
    }
    var lzma_file = "lib"+lib+".js.lzma"
    var blob = await get_lzma( window.lib, lib, lzma_file, size_hint, false, false)
    write_file("lib","lib"+trigger+".so",blob)
}




console.log('pythons included')


