(define x 2)
(+ x 1)
(set! x 3)
(+ x 1)
(define retx
    (lambda ()
        x))
x
(set! x 4)
x
===
3
4
3
4
