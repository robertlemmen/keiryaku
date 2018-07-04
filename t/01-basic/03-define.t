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
===
10
2
1
