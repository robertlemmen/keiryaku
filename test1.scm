2
(+ 1 2)
(if #t 4 5)
(if #f 4 5)
(if 2 6 7)
(begin [+ 1 2] (+ 3 2)  (+ 3 4))
(quote 8)

((lambda (n)
    (if (= n 0)
        0
        (+ n 1)))
 8)

(define X (+ 4 5))
(+ 1 X)

(define z 8)
(let [(x 1) (y 2)] (+ (+ x y) z))
