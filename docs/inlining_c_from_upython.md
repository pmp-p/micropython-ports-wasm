1  connaitre les globals() requises par C
2  connaitre les locals() définies par la fonction à booster

3  auto générer la récup de toutes ces variable depuis un bip buffer



4  valeur de retours ???


var1 = 1
var2 = 2


@inline_c
def make_me_do_c(self, ... ):
    global var1

    lv2 = 2

    with self as c:
        c.var1 = var1
        c.gv2 = var2   # C.attr name can be != python variable name

        #do locals() automatically ?

        c.code = """

    return var1 + gv2 + lv2 ;

        """
        #call compilation if not yet done or sum code() changed
        #and run wasm

    #retrieve something
    result = c()


    # jit ? replace make_me_do_c by call to cached wasm
    make_me_do_c( p_lv2 )

