import sys,typing

def npe():
    print('Null Pointer Exception')
void = typing.TypeVar('ptr')

def cify(instance,namespace=None):
    if namespace is None:
        namespace = instance.__class__.__name__

    defcount = 0
    cmap = {}

    module_table = []


    table = ["\n\n/***************************** MODULE INTERFACE ***************************/"]
    for function_name in dir(instance):

        if function_name.startswith('_'):
            continue

        func = getattr(instance, function_name)

        argv = []
        item_pos = 0
        clines = ['']
        proto = len(clines)
        clines.append('<proto>')

        rti = None
        function_type = "void"

        def read_stack(T, IDX, name ):
            if T in ['int','float']:
                return f'    if (argc>{IDX}) {name} = mp_obj_get_{T}(argv[{IDX}]);'
            if T=='str':
                return f'    if (argc>{IDX}) {name} = mp_obj_new_str_via_qstr(argv[{IDX}],strlen(argv[{IDX}]));'

            return f'// if (argc>{IDX}) {name} = mp_obj_get_<<<{T}>>>(argv[{IDX}]);'


        def read_init(IDX, T, name, V):

            if T == void.__name__:
                if V in (None,npe,):
                    return f"""
    void (*{name})(void);
    if (argc>{IDX})
        {name} = (void*)argv[{IDX}] ;
    else {name} = &null_pointer_exception;

"""

            if T in ['int','float']:
                    return f"""
    int {name};
    if (argc>{IDX})
        {name} = mp_obj_get_{T}(argv[{IDX}]);
    else {name} = {V} ;

"""

            if T=='str':
                val = repr(V)
                if val[0]=="'":
                    val = '"%s"' % val[1:-1]
                return f"""
    mp_obj_t {name};
    if (argc>{IDX})
        {name} = (mp_obj_t*)argv[{IDX}];
    else {name} =  mp_obj_new_str_via_qstr({val},{len(V)});

"""
            if T=='const char*':
                val = repr(V)
                if val[0]=="'":
                    val = '"%s"' % val[1:-1]
                return f"""
    const char *{name};
    if (argc>{IDX})
        {name} = mp_obj_str_get_str(argv[{IDX}]);
    else
        {name} = mp_obj_new_str_via_qstr({val},{len(V)});
"""

            V = 'NULL';
            return f"""
    {T} {name};
    if (argc>{IDX})
        {name} = ({T})argv[{IDX}];
    else {name} = {V} ;

"""

        for item_pos,(k,v) in enumerate( func.__annotations__.items()):

            if k=='return':
                function_type = v.__name__
                rti = v.__name__
                continue

            clines.append( read_init(item_pos, v.__name__, k, func.__defaults__[item_pos]) )
            argv.append(v.__name__)

        clines[proto] = f"STATIC mp_obj_t //{function_type}\n"
        clines[proto] += f"{namespace}_{function_name}(size_t argc, const mp_obj_t *argv) {{"


        if rti and rti!='ptr':
            rti_stmt = f"    //return {rti}()"
        else:
            rti_stmt = "    return None;"
        try:
            clines = '\n'.join(clines)
        except:
            for cl in clines:
                print(cl)
            raise SystemExit
        cmap[function_name]=[ clines , rti_stmt ]

        table.append('')
        table.append(f"STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN({namespace}_{function_name}_obj,")
        table.append(f"    0, {len(argv)}, {namespace}_{function_name});")

        module_table.append(f"    {{MP_ROM_QSTR(MP_QSTR_{function_name}), (mp_obj_t)&{namespace}_{function_name}_obj }},")


    module_table='\n'.join( module_table )
    table.append(f"""
STATIC const mp_map_elem_t {namespace}_globals_table[] = {{
    {{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_{namespace}) }},
    {{ MP_ROM_QSTR(MP_QSTR___file__), MP_ROM_QSTR(MP_QSTR_flashrom) }},

{module_table}

}};

STATIC MP_DEFINE_CONST_DICT(mp_module_{namespace}_globals, {namespace}_globals_table);

const mp_obj_module_t mp_module_{namespace} = {{
    .base = {{ &mp_type_module }},
    .globals = (mp_obj_dict_t*)&mp_module_{namespace}_globals,
}};

""" )
    cmap[-1] = "\n".join(table)
    return cmap


if __name__=='__main__':

    #=================== what a .pym file would produce ===============

    class module:

        def os_read() -> bytes: pass

        def echoint(num : int=0) -> int: pass

        def callsome(fn : void=npe) -> void: pass

        def somecall(s:str='pouet') : pass

    #====================================================================

    instance = module()

    for k,v in cify( module()).items():
        print(v)
