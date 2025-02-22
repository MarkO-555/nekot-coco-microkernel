
set -eux

(
    echo "#line 1 \"prelude.h\""
    cat prelude.h

    for f in [a-z]*.h
    do
        echo "#line 1 \"$f\""
        cat $f
    done

    echo "#line 1 \"SYNTHETIC\""
    for f in [a-z]*.h
    do
        b=$(basename $f .h)
        grep -wq "struct $b" $f && python3 -c "print('struct $b %s;' % '$b'.title())"
    done

    for f in [a-z]*.c
    do
        echo "#line 1 \"$f\""
        cat $f
    done
) > _nekot.c

(
set -x
../../bin/gcc6809 -S -Wall -std=gnu99 -f'no-builtin' -f'omit-frame-pointer' -f'whole-program' -Os _nekot.c
)

(
    echo "#line 1 \"prelude.h\""
    cat prelude.h

    for f in [a-z]*.h
    do
        echo "#line 1 \"$f\""
        cat $f
    done

    echo "#line 1 \"SYNTHETIC\""
    for f in [a-z]*.h
    do
        b=$(basename $f .h)
        grep -wq "struct $b" $f && python3 -c "print('struct $b %s;' % '$b'.title())"
    done

    echo "#line 1 \"../games/blue/blue.c\""
    cat ../games/blue/blue.c
) > _blue.c

(
    set -x
    ../../bin/gcc6809 -S -Wall -std=gnu99 -f'no-builtin' -f'omit-frame-pointer' -f'whole-program' -Os _blue.c
    python3  define_screens.py  < ../games/blue/blue.c >> _blue.s
)
