#!/usr/bin/env python3

def if_offset_ana(f_if, f_xo):
    return((f_if/f_xo)*3*(2**12)-64)


def f_if(if_offset_ana, f_xo):
    return((if_offset_ana+64)*f_xo/(3*2**12))


def if_offset_dig(f_if, f_clk):
    return((f_if/f_clk)*3.*(2.**12)-64.)


def f_c(f_base, f_offset, f_xo, CHSPACE, CHNUM):
    return(f_base+f_offset+((f_xo/2**15)*CHSPACE)*CHNUM)


def f_base(f_xo, SYNT, B, D):
    return((f_xo/((B*D)/2.))*(SYNT/(2**18)))


def f_offset(f_xo, FC_OFFSET):
    return((f_xo/(2**18))*FC_OFFSET)


def main():
    print("test")
    print("f_if: ", f_if(0x36, 50000000))
    tmp = if_offset_dig(480000., 25000000.)
    print(f'if_offset_dig: 0x{round(tmp):2x} {tmp}')
    # print(f'Fc= {f_c((872000000.-860200000)/2., }')
    print("Fbase: ", f_base(50000000., 0x6828F*2**5+0b10011, 6., 1.))


if __name__ == "__main__":
    main()
