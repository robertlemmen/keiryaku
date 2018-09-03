(define X (+ 4 5))
(+ 1 X)
(define a 1)
(define f
    (lambda (x)
        (begin
            (define a 2)
            a)))
(f 1)
a
(define (mul2 x)
    (* 2 x))
(mul2 4)
===
10
2
1
8
