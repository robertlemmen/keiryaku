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
        1)
 3)
