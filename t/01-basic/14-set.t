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
(define x (list 'a 'b 'c))
(define y x)
y
(list? y)
(set-cdr! x 4)
x
(eqv? x y)
y
(list? y)
(set-car! y 5)
x
(eqv? x y)
y
===
3
4
3
4
(a b c)
#t
(a . 4)
#t
(a . 4)
#f
(5 . 4)
#t
(5 . 4)
